#include "../../include/polling.hpp"
#include "../../include/Client.hpp"
#include "../../include/signals.hpp"
#include "../../include/Utils.hpp"
#include "../../include/CGI.hpp"
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <ctime>

#define BUFFER_SIZE 4096
#define MAX_SEND_RETRIES 7

// Forward declarations
void handle_file_upload(Client& client);
void handle_json_upload(Client& client);

void log_new_connection(int fd)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (getpeername(fd, (struct sockaddr*)&addr, &len) == 0)
    {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
        std::cout << "new connection at fd: " << fd
                  << " from " << ip << ":" << ntohs(addr.sin_port) << std::endl;
    }
    else
        std::cout << "new connection at fd: " << fd << "not found" << std::endl;
}

bool handle_connection_attempt(
    int fd,
    std::map<int, Server>& servers,
    std::map<int, Client>& clients,
    int epoll_fd
)
{
    // Make sure fd is actually one of our server sockets
    std::map<int, Server>::iterator server_it = servers.find(fd);
    if (server_it == servers.end())
        return false;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Accept as many pending connections as possible (non-blocking)
    while (true)
    {
        int client_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1)
        {
            // No more connections available
            break;
        }

        // make new client socket non-blocking
        int flags = fcntl(client_fd, F_GETFL, 0);
        fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

        // create Client object and store it
        Client new_client(client_fd, server_it->second);
        clients.insert(std::make_pair(client_fd, new_client));

        // register client socket with epoll
        struct epoll_event event;
        event.events  = EPOLLIN | EPOLLOUT;
        event.data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
        {
            perror("epoll_ctl: add client");
            close(client_fd);
            clients.erase(client_fd);
            continue;
        }

        // log this connection
        log_new_connection(client_fd);
    }

    return true;
}

bool handle_client_data(int client_fd, Client& client, int epoll_fd)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        // Client disconnected or error
        if (bytes_read == 0) {
            std::cout << "Client " << client_fd << " disconnected" << std::endl;
            // If request was in progress but client closed connection, log it
            if (!client.getRequest().isComplete() && !client.getRequest().getMethod().empty()) {
                client.getResponse().sendClientClosedRequest();
                std::cout << "Client closed connection during request processing" << std::endl;
            }
        } else {
            std::cout << "Error reading from client " << std::endl;
        }
        
        // Remove from epoll and close
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        close(client_fd);
        return true; // Signal that client should be removed from map
    }
    
    // Null-terminate the buffer
    buffer[bytes_read] = '\0';
    
    // Set max body size on request if not already set
    if (!client.getRequest().isComplete()) {
        // Get the appropriate max body size limit
        size_t max_body_size = client.getServer().get_client_max_size();
        if (!client.getRequest().getUri().empty()) {
            std::pair<bool, const Location*> location_pair = client.getServer().get_location(client.getRequest().getUri());
            if (location_pair.first) {
                max_body_size = location_pair.second->get_client_max_size();
            }
        }
        client.getRequest().setMaxBodySize(max_body_size);
    }
    
    // Append data to client
    client.appendData(std::string(buffer, bytes_read));
    
    // Check for request parsing errors after appending data
    if (client.getRequest().hasError()) {
        std::string error_type = client.getRequest().getErrorType();
        if (error_type == "ERROR_REQUEST_ENTITY_TOO_LARGE") {
            std::cout << "Client " << client_fd << " request too large (Content-Length exceeds limit)" << std::endl;
            client.getResponse().sendError(HTTP_REQUEST_ENTITY_TOO_LARGE, "Request entity too large");
            client.setResponseSent(false);
            // Don't return false here - let the response be sent
        }
    }
    
    // If request is complete, handle it
    if (client.isRequestReady() && !client.isResponseSent()) {
        handle_http_request(client);
        
        // Send response with non-blocking approach
        std::string response = client.getResponse().buildResponse(client.getRequest().getVersion());
        ssize_t bytes_sent = send(client_fd, response.c_str(), response.length(), MSG_NOSIGNAL | MSG_DONTWAIT);
        
        if (bytes_sent > 0) {
            // Reset retry counter on successful send
            client.resetSendRetries();
            if (bytes_sent == (ssize_t)response.length()) {
                // Complete response sent
                client.setResponseSent(true);
                std::cout << "Complete response sent to client " << client_fd << std::endl;
            } else {
                // Partial send - in a real implementation, we'd buffer the remaining data
                std::cout << "Partial response sent to client " << client_fd 
                         << " (" << bytes_sent << "/" << response.length() << " bytes)" << std::endl;
                client.setResponseSent(true); // For simplicity, mark as sent
            }
        } else if (bytes_sent == -1) {
            client.incrementSendRetries();
            if (client.getSendRetries() > MAX_SEND_RETRIES) {
                std::cout << "Max send retries exceeded for client " << client_fd << std::endl;
                // Error occurred - max retries exceeded
                client.setResponseSent(true); // Mark as sent to trigger cleanup
            } else {
                // Socket not ready for writing or temporary error, will try again on EPOLLOUT
                std::cout << "Socket " << client_fd << " send failed, retry " << client.getSendRetries() << "/" << MAX_SEND_RETRIES << std::endl;
                return false; // Don't close connection yet
            }
        } else {
            // This shouldn't happen (bytes_sent should be > 0 or == -1)
            std::cout << "Unexpected send result for client " << client_fd << std::endl;
            client.setResponseSent(true); // Mark as sent to trigger cleanup
        }
        
        // Close connection after sending response (HTTP/1.0 style)
        if (client.isResponseSent()) {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
            close(client_fd);
            return true; // Signal that client should be removed from map
        }
    }
    
    return false; // Client should remain in map
}

void handle_http_request(Client& client)
{
    Request& request = client.getRequest();
    Response& response = client.getResponse();
    const Server& server = client.getServer();
    
    std::cout << "Handling request: " << request.getMethod() << " " << request.getUri() << std::endl;
    
    // Check for request parsing errors first
    if (request.hasError()) {
        std::string error_type = request.getErrorType();
        if (error_type == "ERROR_HEADER_TOO_LARGE") {
            response.sendRequestHeaderTooLarge();
            return;
        } else if (error_type == "ERROR_REQUEST_ENTITY_TOO_LARGE") {
            ServerConfig config;
            config.error_pages = server.get_error_pages();
            response.sendError(HTTP_REQUEST_ENTITY_TOO_LARGE, config, server.get_root());
            return;
            // response.sendError(HTTP_REQUEST_ENTITY_TOO_LARGE, "Request entity too large");
            // return;
        }
    }
    
    // Basic request validation
    if (request.getMethod().empty() || request.getUri().empty()) {
        ServerConfig config;
        config.error_pages = server.get_error_pages();
        response.sendError(HTTP_BAD_REQUEST, config, ".");
        return;
    }
    
    // Check if method is allowed
    if (request.getMethod() != METHOD_GET && 
        request.getMethod() != METHOD_POST && 
        request.getMethod() != METHOD_DELETE) {
        ServerConfig config;
        config.error_pages = server.get_error_pages();
        response.sendError(HTTP_METHOD_NOT_ALLOWED, config, ".");
        return;
    }
    
    // Get location configuration early for POST requests
    std::pair<bool, const Location*> location_pair = server.get_location(request.getUri());
    const Location* location = location_pair.first ? location_pair.second : NULL;
    
    // Handle POST requests early for file uploads
    if (request.getMethod() == METHOD_POST) {
        // Check request body size against location or server configuration
        // size_t request_body_size = request.getBody().length();
        // size_t max_body_size = location ? location->get_client_max_size() : server.get_client_max_size();
        
        // if (request_body_size > max_body_size) {
        //     std::cout << "Request body too large: " << request_body_size 
        //               << " bytes (max: " << max_body_size << " bytes)" << std::endl;
        //     response.sendError(HTTP_REQUEST_ENTITY_TOO_LARGE);
        //     return;
        // }
        
        // Check authentication for upload endpoints
        if (location) {
            const Location& upload_location = *location;
            std::string auth_realm = upload_location.get_auth_basic_realm();
            std::string auth_user_file = upload_location.get_auth_basic_user_file();
            if (!auth_realm.empty() && !auth_user_file.empty())
            {
                std::string auth_header = request.getHeader("authorization");
                if (!Utils::validateBasicAuth(auth_header))
                {
                    response.setStatus(HTTP_UNAUTHORIZED);
                    response.setHeader("WWW-Authenticate", "Basic realm=\"" + auth_realm + "\"");
                    response.setHeader("Content-Type", "text/html");
                    response.setBody("<html><body><h1>401 Unauthorized</h1><p>Authentication required for upload.</p></body></html>");
                    std::cout << "Authentication failed for upload: " << request.getUri() << std::endl;
                    return;
                }
            }
        }
        
        std::string content_type = request.getHeader("content-type");
        if (content_type.find("multipart/form-data") != std::string::npos) {
            handle_file_upload(client);
            return;
        }
        // Handle JSON POST requests to upload endpoint
        if (request.getUri() == "/upload" && content_type.find("application/json") != std::string::npos) {
            handle_json_upload(client);
            return;
        }
    }
    
    // Get location configuration if not already found
    if (!location) {
        location_pair = server.get_location(request.getUri());
        location = location_pair.first ? location_pair.second : NULL;
    }
    
    if (!location) {
        ServerConfig config;
        config.error_pages = server.get_error_pages();
        response.sendError(HTTP_NOT_FOUND, config, ".");
        return;
    }
    
    const Location& location_ref = *location;
    
    // Handle HTTP redirection first (return directive)
    Config::ReturnData return_data = location_ref.get_return();
    if (return_data.code != -1) {
        response.setStatus(return_data.code);
        response.setHeader("Location", return_data.text);
        response.setHeader("Content-Type", "text/html");

        std::ostringstream body;
        body << "<html><body><h1>" << return_data.code << " ";
        if (return_data.code == 301) body << "Moved Permanently";
        else if (return_data.code == 302) body << "Found";
        else body << "Redirect";
        body << "</h1>";
        body << "<p>The document has moved <a href=\"" << return_data.text << "\">here</a>.</p>";
        body << "</body></html>";
        
        response.setBody(body.str());
        std::cout << "Redirecting to: " << return_data.text << " with status " << return_data.code << std::endl;
        return;
    }
    
    // Check if method is allowed for this location
    std::vector<std::string> allowed_methods = location_ref.get_methods();
    if (!allowed_methods.empty()) {
        bool method_allowed = false;
        for (std::vector<std::string>::const_iterator it = allowed_methods.begin();
             it != allowed_methods.end(); ++it) {
            if (*it == request.getMethod()) {
                method_allowed = true;
                break;
            }
        }
        if (!method_allowed) {
            ServerConfig config;
            config.error_pages = server.get_error_pages();
            response.sendError(HTTP_METHOD_NOT_ALLOWED, config, server.get_root());
            return;
        }
    }
    
    // Check authentication if required (only for POST requests)
    if (request.getMethod() == METHOD_POST) {
        std::string auth_realm = location_ref.get_auth_basic_realm();
        std::string auth_user_file = location_ref.get_auth_basic_user_file();
        if (!auth_realm.empty() && !auth_user_file.empty()) {
            std::string auth_header = request.getHeader("authorization");
            if (!Utils::validateBasicAuth(auth_header)) {
                response.setStatus(HTTP_UNAUTHORIZED);
                response.setHeader("WWW-Authenticate", "Basic realm=\"" + auth_realm + "\"");
                response.setHeader("Content-Type", "text/html");
                response.setBody("<html><body><h1>401 Unauthorized</h1><p>Authentication required.</p></body></html>");
                std::cout << "Authentication failed for " << request.getUri() << std::endl;
                return;
            }
            std::cout << "Authentication successful for " << request.getUri() << std::endl;
        }
    }
    
    // Build file path using nginx-style path resolution
    std::string file_path;
    std::string uri_path = request.getUri();
    std::string location_route = location_ref.getRoute();
    std::string location_root = location_ref.get_root();
    std::string location_alias = location_ref.getAlias();
    
    // Nginx-style path resolution
    if (!location_alias.empty()) {
        // ALIAS directive: replace location prefix with alias path
        // Example: location /images { alias /var/www/img; }
        // Request: /images/photo.jpg -> /var/www/img/photo.jpg
        if (uri_path.find(location_route) == 0) {
            std::string remaining_path = uri_path.substr(location_route.length());
            // Ensure no double slashes
            if (location_alias[location_alias.length() - 1] == '/' && !remaining_path.empty() && remaining_path[0] == '/')
            {
                file_path = location_alias + remaining_path.substr(1);
            } else if (location_alias[location_alias.length() - 1] != '/' && !remaining_path.empty() && remaining_path[0] != '/') {
                file_path = location_alias + "/" + remaining_path;
            } else {
                file_path = location_alias + remaining_path;
            }
        } else {
            file_path = location_alias;
        }
    } else {
        // ROOT directive: append full URI path to root
        // Example: root /var/www; location /images {}
        // Request: /images/photo.jpg -> /var/www/images/photo.jpg
        if (location_root[location_root.length() - 1] == '/' && uri_path[0] == '/') {
            file_path = location_root + uri_path.substr(1);
        } else if (location_root[location_root.length() - 1] != '/' && uri_path[0] != '/') {
            file_path = location_root + "/" + uri_path;
        } else {
            file_path = location_root + uri_path;
        }
    }
    
    // Handle index files for directory requests
    if (file_path[file_path.length() - 1] == '/' || Utils::isDirectory(file_path)) {
        std::vector<std::string> indexes = location_ref.get_indexes();
        if (!indexes.empty()) {
            if (file_path[file_path.length() - 1] != '/') {
                file_path += "/";
            }
            file_path += indexes[0];
        }
    }
    
    std::cout << "LOG: Route: '" << location_route << "', URI: '" << uri_path << "', File path: '" << file_path << "'" << std::endl;
    std::cout << "LOG: Location root: '" << location_root << "', Location alias: '" << location_alias << "'" << std::endl;
    
    // Check if file exists
    if (!Utils::fileExists(file_path)) {
        ServerConfig config;
        config.error_pages = server.get_error_pages();
        response.sendError(HTTP_NOT_FOUND, config, location_root);
        return;
    }
    
    // Handle directory listing
    if (Utils::isDirectory(file_path)) {
        if (location_ref.get_autoindex()) {
            response.sendDirectoryListing(file_path, request.getUri());
        } else {
            // Try to serve index file
            std::vector<std::string> indexes = location_ref.get_indexes();
            bool index_found = false;
            for (std::vector<std::string>::const_iterator it = indexes.begin(); it != indexes.end(); ++it) {
                std::string index_path = Utils::joinPath(file_path, *it);
                if (Utils::fileExists(index_path)) {
                    response.sendFile(index_path);
                    index_found = true;
                    break;
                }
            }
            if (!index_found) {
                ServerConfig config;
                config.error_pages = server.get_error_pages();
                response.sendError(HTTP_FORBIDDEN, config, location_root);
            }
        }
        return;
    }
    
    // Handle DELETE method
    if (request.getMethod() == METHOD_DELETE) {
        if (Utils::fileExists(file_path) && !Utils::isDirectory(file_path)) {
            if (remove(file_path.c_str()) == 0) {
                response.setStatus(HTTP_NO_CONTENT);
                response.setHeader("Content-Type", "text/plain");
                response.setBody("File deleted successfully");
                std::cout << "Deleted file: " << file_path << std::endl;
            } else {
                ServerConfig config;
                config.error_pages = server.get_error_pages();
                response.sendError(HTTP_FORBIDDEN, config, server.get_root());
                std::cout << "Failed to delete file: " << file_path << std::endl;
            }
        } else {
            ServerConfig config;
            config.error_pages = server.get_error_pages();
            response.sendError(HTTP_NOT_FOUND, config, server.get_root());
            std::cout << "File not found for deletion: " << file_path << std::endl;
        }
        return;
    }
    
    // Check if it's a CGI script
    std::map<std::string, std::string> cgi_extensions = location_ref.get_cgi_extensions();
    if (CGI::isCGIScript(file_path, cgi_extensions)) {
        handle_cgi_request(client, file_path, location_ref, server);
        return;
    }
    
    // Serve static file
    response.sendFile(file_path);
}

void handle_cgi_request(Client& client, const std::string& script_path, const Location& location, const Server& server)
{
    Request& request = client.getRequest();
    Response& response = client.getResponse();
    
    CGI cgi;
    cgi.setScriptPath(script_path);
    cgi.setRequest(request);
    cgi.setDocumentRoot(location.getAlias());
    cgi.setServerInfo("localhost", "8080");
    
    // Get interpreter for this script type
    std::string ext = Utils::getExtension(script_path);
    std::map<std::string, std::string> cgi_extensions = location.get_cgi_extensions();
    std::map<std::string, std::string>::const_iterator it = cgi_extensions.find(ext);
    if (it != cgi_extensions.end()) {
        cgi.setInterpreter(it->second);
    }
    
    // Execute CGI script
    std::string cgi_output = cgi.execute();
    
    if (cgi_output == "timeout"){
        ServerConfig config;
        config.error_pages = server.get_error_pages();
        response.sendError(HTTP_GATEWAY_TIMEOUT, config, server.get_root());
        return;
    }

    if (cgi_output.empty()) {
        ServerConfig config;
        config.error_pages = server.get_error_pages();
        response.sendError(HTTP_INTERNAL_SERVER_ERROR, config, server.get_root());
        return;
    }
    
    // Check for error responses (status codes >= 400)
    std::string headers = cgi.getHeaders();
    std::string body = cgi.getBody();
    
    // Parse status from headers if present
    int status_code = HTTP_OK;
    if (headers.find("Status:") != std::string::npos) {
        size_t status_pos = headers.find("Status:");
        if (status_pos != std::string::npos) {
            size_t code_start = status_pos + 7; // Length of "Status:"
            while (code_start < headers.length() && headers[code_start] == ' ') {
                code_start++;
            }
            if (code_start < headers.length()) {
                std::string status_str = headers.substr(code_start, 3);
                status_code = atoi(status_str.c_str());
                if (status_code < 100 || status_code > 599) {
                    status_code = HTTP_OK; // Default to 200 if invalid
                }
            }
        }
    }
    
    // Set response status
    response.setStatus(status_code);
    
    // Parse and set headers
    std::istringstream header_stream(headers);
    std::string line;
    bool content_type_set = false;
    
    while (std::getline(header_stream, line)) {
        // Remove carriage return if present
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        
        if (line.empty() || line.find("Status:") == 0) continue;
        
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string header_name = line.substr(0, colon_pos);
            std::string header_value = line.substr(colon_pos + 1);
            
            // Trim whitespace from header value
            while (!header_value.empty() && header_value[0] == ' ') {
                header_value.erase(0, 1);
            }
            while (!header_value.empty() && header_value[header_value.length() - 1] == ' ') {
                header_value.erase(header_value.length() - 1);
            }
            
            response.setHeader(header_name, header_value);
            
            if (header_name == "Content-Type") {
                content_type_set = true;
            }
        }
    }
    
    // Set default content type if not specified
    if (!content_type_set) {
        response.setHeader("Content-Type", "text/html");
    }
    
    // Set response body
    response.setBody(body);
}

void handle_file_upload(Client& client)
{
    Request& request = client.getRequest();
    Response& response = client.getResponse();
    
    std::string content_type = request.getHeader("content-type");
    std::string body = request.getBody();
    
    // Extract boundary from content-type
    size_t boundary_pos = content_type.find("boundary=");
    if (boundary_pos == std::string::npos) {
        response.sendError(HTTP_BAD_REQUEST, "Missing boundary in multipart data");
        return;
    }
    
    std::string boundary = "--" + content_type.substr(boundary_pos + 9);
    
    // Simple multipart parsing
    size_t start_pos = body.find(boundary);
    if (start_pos == std::string::npos) {
        response.sendError(HTTP_BAD_REQUEST, "Invalid multipart data");
        return;
    }
    
    // Find content-disposition header
    size_t headers_start = start_pos + boundary.length();
    size_t headers_end = body.find("\r\n\r\n", headers_start);
    if (headers_end == std::string::npos) {
        headers_end = body.find("\n\n", headers_start);
        if (headers_end == std::string::npos) {
            response.sendError(HTTP_BAD_REQUEST, "Invalid multipart headers");
            return;
        }
        headers_end += 2;
    } else {
        headers_end += 4;
    }
    
    std::string headers = body.substr(headers_start, headers_end - headers_start);
    
    // Extract filename
    std::string filename;
    size_t filename_pos = headers.find("filename=\"");
    if (filename_pos != std::string::npos) {
        filename_pos += 10; // length of "filename=\""
        size_t filename_end = headers.find("\"", filename_pos);
        if (filename_end != std::string::npos) {
            filename = headers.substr(filename_pos, filename_end - filename_pos);
        }
    }
    
    if (filename.empty()) {
        response.sendError(HTTP_BAD_REQUEST, "No filename provided");
        return;
    }
    
    // Security: validate filename
    if (filename.find("..") != std::string::npos || filename.find("/") != std::string::npos) {
        response.sendError(HTTP_BAD_REQUEST, "Invalid filename");
        return;
    }
    
    // Find file content
    size_t content_start = headers_end;
    size_t content_end = body.find(boundary, content_start);
    if (content_end == std::string::npos) {
        response.sendError(HTTP_BAD_REQUEST, "Invalid multipart data end");
        return;
    }
    
    // Remove trailing \r\n before boundary
    if (content_end >= 2 && body.substr(content_end - 2, 2) == "\r\n") {
        content_end -= 2;
    } else if (content_end >= 1 && body.substr(content_end - 1, 1) == "\n") {
        content_end -= 1;
    }
    
    std::string file_content = body.substr(content_start, content_end - content_start);
    
    // Get location-specific body size limit
    const Server& server = client.getServer();
    std::pair<bool, const Location*> upload_location_pair = server.get_location(client.getRequest().getUri());
    size_t max_file_size = upload_location_pair.first ? 
        upload_location_pair.second->get_client_max_size() : 
        server.get_client_max_size();
    
    // Check file size against limits
    size_t file_size = file_content.length();
    if (file_size > max_file_size) {
        std::cout << "File too large: " << file_size 
                  << " bytes (max: " << max_file_size << " bytes)" << std::endl;
        response.sendError(HTTP_REQUEST_ENTITY_TOO_LARGE, "File size exceeds configured limit");
        return;
    }
    
    // Save file to uploads directory
    std::string upload_path = "www/uploads/" + filename;
    std::ofstream outfile(upload_path.c_str(), std::ios::binary);
    if (!outfile.is_open()) {
        response.sendError(HTTP_INTERNAL_SERVER_ERROR, "Failed to create upload file");
        return;
    }
    
    outfile.write(file_content.c_str(), file_content.length());
    outfile.close();
    
    // Send success response
    response.setStatus(HTTP_CREATED);
    response.setHeader("Content-Type", "text/html");
    
    std::ostringstream success_body;
    success_body << "<!DOCTYPE html>\n";
    success_body << "<html><head><title>Upload Success</title></head>\n";
    success_body << "<body>\n";
    success_body << "<h1>File Upload Successful</h1>\n";
    success_body << "<p>File <strong>" << filename << "</strong> has been uploaded successfully.</p>\n";
    success_body << "<p>File size: " << file_content.length() << " bytes</p>\n";
    success_body << "<p><a href=\"/uploads\">View uploaded files</a></p>\n";
    success_body << "<p><a href=\"/upload\">Upload another file</a></p>\n";
    success_body << "</body></html>\n";
    
    response.setBody(success_body.str());
    
    std::cout << "File uploaded successfully: " << filename << " (" << file_content.length() << " bytes)" << std::endl;
}

void handle_json_upload(Client& client)
{
    Request& request = client.getRequest();
    Response& response = client.getResponse();
    
    std::string body = request.getBody();
    
    // Validate JSON content
    if (body.empty()) {
        response.setStatus(HTTP_BAD_REQUEST);
        response.setHeader("Content-Type", "application/json");
        response.setBody("{\"error\": \"Empty request body\"}");
        return;
    }
    
    // For demonstration, we'll echo back the received JSON with a success message
    std::ostringstream json_response;
    json_response << "{\n";
    json_response << "  \"status\": \"success\",\n";
    json_response << "  \"message\": \"JSON data received successfully\",\n";
    json_response << "  \"received_data\": " << body << ",\n";
    json_response << "  \"timestamp\": \"" << time(NULL) << "\"\n";
    json_response << "}";
    
    response.setStatus(HTTP_OK);
    response.setHeader("Content-Type", "application/json");
    response.setBody(json_response.str());
    
    std::cout << "JSON POST request processed successfully for /upload endpoint" << std::endl;
}

void close_clients(std::map<int, Client>& clients)
{
    std::map<int, Client>::iterator it;
    for (it = clients.begin(); it != clients.end(); ++it) {
        close(it->first);
    }
}

void check_client_timeouts(std::map<int, Client>& clients, int epoll_fd)
{
    std::vector<int> timed_out_clients;
    
    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        if (it->second.isTimedOut(CLIENT_TIMEOUT))
        {
            std::cout << "Client " << it->first << " timed out" << std::endl;
            
            // Send timeout response if request was in progress
            if (!it->second.getRequest().isComplete() && !it->second.getRequest().getMethod().empty())
            {
                it->second.getResponse().sendRequestTimeout();
            }
            
            timed_out_clients.push_back(it->first);
        }
    }
    
    // Clean up timed out clients
    for (std::vector<int>::iterator fd_it = timed_out_clients.begin(); 
         fd_it != timed_out_clients.end(); ++fd_it)
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, *fd_it, NULL);
        close(*fd_it);
        clients.erase(*fd_it);
    }
}

void process_epoll_events(int epoll_fd, struct epoll_event* events, int active_fds,
                                 std::map<int, Server>& servers,
                                 std::map<int, Client>& clients)
{
    for (int i = 0; i < active_fds; ++i)
    {
        int fd = events[i].data.fd;
        uint32_t revents = events[i].events;
        
        if (revents & EPOLLIN || revents & EPOLLHUP )
        {
            if (servers.find(fd) != servers.end()) {
                // Server socket - handle new connections
                handle_connection_attempt(fd, servers, clients, epoll_fd);
            } else {
                // Client socket - handle data
                std::map<int, Client>::iterator client_it = clients.find(fd);
                if (client_it != clients.end()) {
                    bool should_remove = handle_client_data(fd, client_it->second, epoll_fd);
                    if (should_remove || client_it->second.isResponseSent()) {
                        clients.erase(client_it);
                    }
                }
            }
        }
    }
} 
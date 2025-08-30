#include "../../include/Response.hpp"
#include "../../include/Utils.hpp"
#include <fstream>
#include <sstream>
#include <dirent.h>

Response::Response() : status_code(HTTP_OK), is_sent(false), http_version("HTTP/1.1") {
    setStatusMessage();
}

Response::~Response() {}

void Response::setStatus(int code) {
    status_code = code;
    setStatusMessage();
}

void Response::setHeader(const std::string& name, const std::string& value) {
    headers[name] = value;
}

void Response::setBody(const std::string& content) {
    body = content;
    setHeader("Content-Length", Utils::toString(body.length()));
}

void Response::setBodyFromFile(const std::string& filename) {
    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file.is_open()) {
        setStatus(HTTP_NOT_FOUND);
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    body = buffer.str();
    
    setHeader("Content-Length", Utils::toString(body.length()));
    setHeader("Content-Type", getContentType(filename));

    Utils::logInfo("Set body from file: " + filename + ", Body length: " + Utils::toString(body.length()) + " bytes.");
}

void Response::setStatusMessage() {
    switch (status_code) {
        case HTTP_OK:
            status_message = "OK";
            break;
        case HTTP_NOT_FOUND:
            status_message = "Not Found";
            break;
        case HTTP_FORBIDDEN:
            status_message = "Forbidden";
            break;
        default:
            status_message = Utils::getStatusMessage(status_code);
    }
}

std::string Response::getContentType(const std::string& filename) {
    return Utils::getMimeType(filename);
}

std::string Response::generateErrorPage(int code) {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"en\">\n";
    html << "<head>\n";
    html << "    <meta charset=\"UTF-8\">\n";
    html << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "    <title>" << code << " " << Utils::getStatusMessage(code) << " - WebServ</title>\n";
    html << "    <style>\n";
    html << "        * { margin: 0; padding: 0; box-sizing: border-box; }\n";
    html << "        body {\n";
    html << "            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n";
    html << "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n";
    html << "            min-height: 100vh;\n";
    html << "            display: flex;\n";
    html << "            align-items: center;\n";
    html << "            justify-content: center;\n";
    html << "            color: white;\n";
    html << "        }\n";
    html << "        .error-container {\n";
    html << "            background: rgba(255, 255, 255, 0.1);\n";
    html << "            backdrop-filter: blur(10px);\n";
    html << "            border-radius: 20px;\n";
    html << "            padding: 40px;\n";
    html << "            text-align: center;\n";
    html << "            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);\n";
    html << "            max-width: 500px;\n";
    html << "            width: 90%;\n";
    html << "        }\n";
    html << "        .error-code {\n";
    html << "            font-size: 4rem;\n";
    html << "            font-weight: bold;\n";
    html << "            margin-bottom: 20px;\n";
    html << "            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);\n";
    html << "        }\n";
    html << "        .error-message {\n";
    html << "            font-size: 1.5rem;\n";
    html << "            margin-bottom: 20px;\n";
    html << "            font-weight: 300;\n";
    html << "        }\n";
    html << "        .error-description {\n";
    html << "            font-size: 1rem;\n";
    html << "            line-height: 1.6;\n";
    html << "            opacity: 0.9;\n";
    html << "            margin-bottom: 30px;\n";
    html << "        }\n";
    html << "        .back-button {\n";
    html << "            display: inline-block;\n";
    html << "            padding: 12px 24px;\n";
    html << "            background: rgba(255, 255, 255, 0.2);\n";
    html << "            color: white;\n";
    html << "            text-decoration: none;\n";
    html << "            border-radius: 25px;\n";
    html << "            transition: all 0.3s ease;\n";
    html << "            border: 1px solid rgba(255, 255, 255, 0.3);\n";
    html << "        }\n";
    html << "        .back-button:hover {\n";
    html << "            background: rgba(255, 255, 255, 0.3);\n";
    html << "            transform: translateY(-2px);\n";
    html << "        }\n";
    html << "        .webserv-footer {\n";
    html << "            margin-top: 30px;\n";
    html << "            font-size: 0.8rem;\n";
    html << "            opacity: 0.7;\n";
    html << "        }\n";
    html << "    </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "    <div class=\"error-container\">\n";
    html << "        <div class=\"error-code\">" << code << "</div>\n";
    html << "        <div class=\"error-message\">" << Utils::getStatusMessage(code) << "</div>\n";
    html << "        <div class=\"error-description\">" << getErrorDescription(code) << "</div>\n";
    html << "        <a href=\"javascript:history.back()\" class=\"back-button\">Go Back</a>\n";
    html << "        <div class=\"webserv-footer\">WebServ - HTTP Server</div>\n";
    html << "    </div>\n";
    html << "</body>\n";
    html << "</html>\n";
    return html.str();
}

std::string Response::getErrorDescription(int code) {
    switch (code) {
        // 4xx Client Errors
        case 400: return "The server cannot process the request due to invalid syntax or malformed request.";
        case 401: return "Authentication is required to access this resource. Please provide valid credentials.";
        case 402: return "Payment is required to access this resource.";
        case 403: return "You don't have permission to access this resource on this server.";
        case 404: return "The requested resource could not be found on this server.";
        case 405: return "The request method is not allowed for the requested resource.";
        case 406: return "The server cannot produce a response matching the acceptable values defined in the request headers.";
        case 407: return "The client must first authenticate itself with the proxy.";
        case 408: return "The server timed out waiting for the request.";
        case 409: return "The request conflicts with the current state of the resource.";
        case 410: return "The requested resource is no longer available and will not be available again.";
        case 411: return "The request did not specify the length of its content, which is required by the requested resource.";
        case 412: return "The server does not meet one of the preconditions that the requester put on the request.";
        case 413: return "The request is larger than the server is willing or able to process.";
        case 414: return "The URI provided was too long for the server to process.";
        case 415: return "The request entity has a media type which the server or resource does not support.";
        case 416: return "The client has asked for a portion of the file that the server cannot supply.";
        case 417: return "The server cannot meet the requirements of the Expect request-header field.";
        case 418: return "The server refuses the attempt to brew coffee with a teapot.";
        case 421: return "The request was directed at a server that is not able to produce a response.";
        case 422: return "The request was well-formed but was unable to be followed due to semantic errors.";
        case 423: return "The resource that is being accessed is locked.";
        case 424: return "The request failed because it depended on another request and that request failed.";
        case 425: return "The server is unwilling to risk processing a request that might be replayed.";
        case 426: return "The server refuses to perform the request using the current protocol.";
        case 428: return "The origin server requires the request to be conditional.";
        case 429: return "Too many requests have been sent in a given amount of time. Please wait before trying again.";
        case 431: return "The server is unwilling to process the request because either individual header fields or all header fields collectively are too large.";
        case 451: return "The requested resource is unavailable for legal reasons.";
        
        // 5xx Server Errors
        case 500: return "The server encountered an unexpected condition that prevented it from fulfilling the request.";
        case 501: return "The server does not support the functionality required to fulfill the request.";
        case 502: return "The server received an invalid response from an upstream server while acting as a gateway or proxy.";
        case 503: return "The server is temporarily overloaded or under maintenance. Please try again later.";
        case 504: return "The server did not receive a timely response from an upstream server while acting as a gateway or proxy.";
        case 505: return "The server does not support the HTTP protocol version used in the request.";
        case 506: return "Transparent content negotiation for the request results in a circular reference.";
        case 507: return "The server is unable to store the representation needed to complete the request.";
        case 508: return "The server detected an infinite loop while processing the request.";
        case 510: return "Further extensions to the request are required for the server to fulfill it.";
        case 511: return "The client needs to authenticate to gain network access.";
        
        // Nginx-specific codes
        case 444: return "The server returned no information and closed the connection (nginx specific).";
        case 494: return "The client sent too large request headers (nginx specific).";
        case 499: return "The client closed the connection before the server could respond (nginx specific).";
        
        // 6xx Future Extension (not standard HTTP but prepared for future use)
        case 600: return "An extended error occurred while processing your request.";
        
        // 2xx Success (shouldn't normally generate error pages)
        case 200: return "The request has succeeded.";
        case 201: return "The request has been fulfilled and a new resource has been created.";
        case 202: return "The request has been accepted for processing, but processing has not been completed.";
        case 204: return "The server successfully processed the request, but is not returning any content.";
        
        // 3xx Redirection
        case 301: return "The resource has been permanently moved to a new location.";
        case 302: return "The resource has been temporarily moved to a different location.";
        case 304: return "The resource has not been modified since the last request.";
        
        default: 
            // Handle ranges for codes not explicitly defined
            if (code >= 400 && code < 500) {
                return "A client error occurred while processing your request.";
            } else if (code >= 500 && code < 600) {
                return "A server error occurred while processing your request.";
            } else if (code >= 600 && code < 700) {
                return "An extended error occurred while processing your request.";
            }
            return "An unexpected error occurred while processing your request.";
    }
}

std::string Response::generateDirectoryListing(const std::string& path, const std::string& uri) {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html>\n";
    html << "<head>\n";
    html << "    <title>Index of " << uri << "</title>\n";
    html << "    <style>\n";
    html << "        body {\n";
    html << "            font-family: Arial, sans-serif;\n";
    html << "            margin: 0;\n";
    html << "            padding: 0;\n";
    html << "            min-height: 100vh;\n";
    html << "            color: white;\n";
    html << "            position: relative;\n";
    html << "        }\n";
    html << "        .video-background {\n";
    html << "            position: fixed;\n";
    html << "            top: 0;\n";
    html << "            left: 0;\n";
    html << "            width: 100%;\n";
    html << "            height: 100%;\n";
    html << "            z-index: -1;\n";
    html << "            object-fit: cover;\n";
    html << "        }\n";
    html << "        .content-wrapper {\n";
    html << "            position: relative;\n";
    html << "            z-index: 1;\n";
    html << "            background-color: rgba(0, 0, 0, 0.7);\n";
    html << "            min-height: 100vh;\n";
    html << "            padding: 40px;\n";
    html << "        }\n";
    html << "        h1 {\n";
    html << "            color: white;\n";
    html << "            margin-bottom: 30px;\n";
    html << "            text-shadow: 2px 2px 4px rgba(0,0,0,0.5);\n";
    html << "        }\n";
    html << "        .directory-container {\n";
    html << "            background-color: rgba(255, 255, 255, 0.1);\n";
    html << "            border-radius: 10px;\n";
    html << "            backdrop-filter: blur(10px);\n";
    html << "            padding: 20px;\n";
    html << "            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);\n";
    html << "        }\n";
    html << "        table {\n";
    html << "            width: 100%;\n";
    html << "            border-collapse: collapse;\n";
    html << "            margin-top: 20px;\n";
    html << "        }\n";
    html << "        th, td {\n";
    html << "            padding: 12px 15px;\n";
    html << "            text-align: left;\n";
    html << "            border-bottom: 1px solid rgba(255, 255, 255, 0.1);\n";
    html << "        }\n";
    html << "        th {\n";
    html << "            background-color: rgba(255, 255, 255, 0.1);\n";
    html << "            font-weight: 600;\n";
    html << "            text-transform: uppercase;\n";
    html << "            font-size: 0.9em;\n";
    html << "            letter-spacing: 1px;\n";
    html << "        }\n";
    html << "        tr:hover {\n";
    html << "            background-color: rgba(255, 255, 255, 0.05);\n";
    html << "        }\n";
    html << "        a {\n";
    html << "            text-decoration: none;\n";
    html << "            color: #7fdbff;\n";
    html << "            transition: color 0.3s ease;\n";
    html << "        }\n";
    html << "        a:hover {\n";
    html << "            color: #00ff00;\n";
    html << "            text-decoration: none;\n";
    html << "        }\n";
    html << "        .nav-buttons {\n";
    html << "            position: absolute;\n";
    html << "            top: 20px;\n";
    html << "            right: 20px;\n";
    html << "            display: flex;\n";
    html << "            gap: 10px;\n";
    html << "        }\n";
    html << "        .nav-button {\n";
    html << "            padding: 8px 16px;\n";
    html << "            background-color: rgba(33, 150, 243, 0.7);\n";
    html << "            color: white;\n";
    html << "            text-decoration: none;\n";
    html << "            border-radius: 4px;\n";
    html << "            transition: background-color 0.3s;\n";
    html << "        }\n";
    html << "        .nav-button:hover {\n";
    html << "            background-color: rgba(25, 118, 210, 0.9);\n";
    html << "        }\n";
    html << "    </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "    <video class=\"video-background\" autoplay muted loop playsinline>\n";
    html << "        <source src=\"/thegif.mp4\" type=\"video/mp4\">\n";
    html << "        Your browser does not support the video tag.\n";
    html << "    </video>\n";
    html << "    <div class=\"content-wrapper\">\n";
    html << "        <div class=\"nav-buttons\">\n";
    html << "            <a href=\"/\" class=\"nav-button\">Home</a>\n";
    html << "        </div>\n";
    html << "        <h1>Index of " << uri << "</h1>\n";
    html << "        <div class=\"directory-container\">\n";
    html << "            <table>\n";
    html << "        <tr><th>Name</th><th>Size</th><th>Last Modified</th></tr>\n";
    
    DIR* dir = opendir(path.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            
            std::string full_path = path + "/" + name;
            struct stat st;
            if (stat(full_path.c_str(), &st) == 0) {
                std::string link = uri;
                if (link[link.length() - 1] != '/') link += "/";
                link += name;
                
                html << "        <tr>\n";
                html << "            <td><a href=\"" << link << "\">" << name;
                if (S_ISDIR(st.st_mode)) html << "/";
                html << "</a></td>\n";
                
                if (S_ISDIR(st.st_mode)) {
                    html << "            <td>-</td>\n";
                } else {
                    html << "            <td>" << st.st_size << " bytes</td>\n";
                }
                
                char time_str[100];
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
                html << "            <td>" << time_str << "</td>\n";
                html << "        </tr>\n";
            }
        }
        closedir(dir);
    }
    
    html << "            </table>\n";
    html << "        </div>\n";
    html << "    </div>\n";
    html << "</body>\n";
    html << "</html>\n";
    return html.str();
}

void Response::sendFile(const std::string& filename) {
    if (!Utils::fileExists(filename)) {
        sendError(HTTP_NOT_FOUND);
        return;
    }
    
    if (!Utils::isReadable(filename)) {
        sendError(HTTP_FORBIDDEN);
        return;
    }
    
    setStatus(HTTP_OK);
    setBodyFromFile(filename);
}

void Response::sendError(int code, const std::string& custom_message) {
    setStatus(code);
    setHeader("Content-Type", CONTENT_TYPE_HTML);
    
    if (!custom_message.empty()) {
        setBody(custom_message);
    } else {
        setBody(generateErrorPage(code));
    }
}

void Response::sendError(int code, const ServerConfig& config, const std::string& root_path) {
    setStatus(code);
    setHeader("Content-Type", CONTENT_TYPE_HTML);
    
    // Try template-based error pages first
    std::string template_page = generateTemplateErrorPage(code, root_path);
    if (!template_page.empty()) {
        setBody(template_page);
        return;
    }
    
    // Try to use custom error page from config
    if (!tryCustomErrorPage(code, config.error_pages, root_path)) {
        // Fall back to default error page
        setBody(generateErrorPage(code));
    }
}

bool Response::tryCustomErrorPage(int code, const std::map<int, std::string>& error_pages, const std::string& root_path) {
    std::map<int, std::string>::const_iterator it = error_pages.find(code);
    if (it != error_pages.end()) {
        std::string error_page_path = root_path + it->second;
        if (Utils::fileExists(error_page_path)) {
            setBodyFromFile(error_page_path);
            return true;
        } else {
            Utils::logError("Custom error page not found: " + error_page_path);
        }
    }
    return false;
}

std::string Response::generateTemplateErrorPage(int code, const std::string& root_path) {
    // Determine which template to use based on error code
    std::string template_file;
    int code_class = code / 100;
    
    switch (code_class) {
        case 4:
            template_file = root_path + "/4xx.html";
            break;
        case 5:
            template_file = root_path + "/5xx.html";
            break;
        case 6:
            template_file = root_path + "/6xx.html";
            break;
        default:
            // Fall back to generating standard error page
            return generateErrorPage(code);
    }
    
    // Check if template exists
    if (Utils::fileExists(template_file)) {
        std::string template_content = Utils::readFile(template_file);
        if (!template_content.empty()) {
            return replacePlaceholders(template_content, code);
        }
    }
    
    // Fall back to generating standard error page
    return generateErrorPage(code);
}

std::string Response::replacePlaceholders(const std::string& template_content, int code) {
    std::string result = template_content;
    
    // Replace placeholders
    std::string code_str = Utils::toString(code);
    std::string message = Utils::getStatusMessage(code);
    std::string description = getErrorDescription(code);
    
    // Replace {{ERROR_CODE}}
    size_t pos = 0;
    while ((pos = result.find("{{ERROR_CODE}}", pos)) != std::string::npos) {
        result.replace(pos, 14, code_str);
        pos += code_str.length();
    }
    
    // Replace {{ERROR_MESSAGE}}
    pos = 0;
    while ((pos = result.find("{{ERROR_MESSAGE}}", pos)) != std::string::npos) {
        result.replace(pos, 17, message);
        pos += message.length();
    }
    
    // Replace {{ERROR_DESCRIPTION}}
    pos = 0;
    while ((pos = result.find("{{ERROR_DESCRIPTION}}", pos)) != std::string::npos) {
        result.replace(pos, 21, description);
        pos += description.length();
    }
    
    return result;
}

void Response::sendRedirect(const std::string& location) {
    setStatus(HTTP_FOUND);
    setHeader("Location", location);
    setHeader("Content-Type", CONTENT_TYPE_HTML);
    setBody("<html><body><h1>302 Found</h1><p>The document has moved <a href=\"" + location + "\">here</a>.</p></body></html>");
}

void Response::sendDirectoryListing(const std::string& path, const std::string& uri) {
    setStatus(HTTP_OK);
    setHeader("Content-Type", CONTENT_TYPE_HTML);
    setBody(generateDirectoryListing(path, uri));
}

// Getters
int Response::getStatusCode() const {
    return status_code;
}

const std::map<std::string, std::string>& Response::getHeaders() const {
    return headers;
}

const std::string& Response::getBody() const {
    return body;
}

const std::string& Response::getStatusMessage() const {
    return status_message;
}

bool Response::isSent() const {
    return is_sent;
}

// Response building
std::string Response::buildResponse() {
    return buildResponse("HTTP/1.1");
}

std::string Response::buildResponse(const std::string& request_version) {
    std::ostringstream response;
    
    // Determine response version based on request version
    std::string response_version = request_version;
    if (request_version == "HTTP/1.0") {
        response_version = "HTTP/1.0";
    } else {
        response_version = "HTTP/1.1";
    }
    
    // Status line
    response << response_version << " " << status_code << " " << status_message << "\r\n";
    
    // Add Server header (nginx-like)
    response << "Server: webserv/1.0\r\n";
    
    // Add Date header
    time_t now = time(0);
    struct tm* gmt = gmtime(&now);
    char date_buffer[100];
    strftime(date_buffer, sizeof(date_buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
    response << "Date: " << date_buffer << "\r\n";
    
    // Headers
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); 
         it != headers.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
    }
    
    // Handle Connection header based on HTTP version
    if (response_version == "HTTP/1.0") {
        // HTTP/1.0 defaults to Connection: close
        if (headers.find("Connection") == headers.end()) {
            response << "Connection: close\r\n";
        }
    } else {
        // HTTP/1.1 defaults to Connection: keep-alive, but we use close for simplicity
        response << "Connection: close\r\n";
    }
    
    // Empty line to separate headers from body
    response << "\r\n";
    
    // Body
    response << body;
    
    return response.str();
}

void Response::markAsSent() {
    is_sent = true;
}

void Response::reset() {
    status_code = HTTP_OK;
    headers.clear();
    body.clear();
    setStatusMessage();
    is_sent = false;
    http_version = "HTTP/1.1";
}

void Response::print() const {
    std::cout << "Response:" << std::endl;
    std::cout << "  Status: " << status_code << " " << status_message << std::endl;
    std::cout << "  Headers:" << std::endl;
    
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); 
         it != headers.end(); ++it) {
        std::cout << "    " << it->first << ": " << it->second << std::endl;
    }
    
    std::cout << "  Body Length: " << body.length() << std::endl;
    std::cout << "  Is Sent: " << (is_sent ? "Yes" : "No") << std::endl;
}

// Nginx-specific error responses
void Response::sendRequestHeaderTooLarge() {
    setStatus(HTTP_REQUEST_HEADER_TOO_LARGE);
    setHeader("Content-Type", CONTENT_TYPE_HTML);
    setHeader("Connection", "close");
    setBody(generateErrorPage(HTTP_REQUEST_HEADER_TOO_LARGE));
}

void Response::sendClientClosedRequest() {
    setStatus(HTTP_CLIENT_CLOSED_REQUEST);
    setHeader("Content-Type", CONTENT_TYPE_HTML);
    setHeader("Connection", "close");
    setBody(generateErrorPage(HTTP_CLIENT_CLOSED_REQUEST));
}

void Response::sendGatewayTimeout(const std::string& reason) {
    setStatus(HTTP_GATEWAY_TIMEOUT);
    setHeader("Content-Type", CONTENT_TYPE_HTML);
    setHeader("Connection", "close");
    
    if (!reason.empty()) {
        std::ostringstream html;
        html << "<!DOCTYPE html>\n";
        html << "<html lang=\"en\">\n";
        html << "<head>\n";
        html << "    <meta charset=\"UTF-8\">\n";
        html << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
        html << "    <title>504 Gateway Timeout - WebServ</title>\n";
        html << "    <style>\n";
        html << "        * { margin: 0; padding: 0; box-sizing: border-box; }\n";
        html << "        body {\n";
        html << "            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n";
        html << "            background: linear-gradient(135deg, #ff6b6b 0%, #ee5a24 100%);\n";
        html << "            min-height: 100vh;\n";
        html << "            display: flex;\n";
        html << "            align-items: center;\n";
        html << "            justify-content: center;\n";
        html << "            color: white;\n";
        html << "        }\n";
        html << "        .error-container {\n";
        html << "            background: rgba(255, 255, 255, 0.1);\n";
        html << "            backdrop-filter: blur(10px);\n";
        html << "            border-radius: 20px;\n";
        html << "            padding: 40px;\n";
        html << "            text-align: center;\n";
        html << "            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);\n";
        html << "            max-width: 500px;\n";
        html << "            width: 90%;\n";
        html << "        }\n";
        html << "        .error-code { font-size: 4rem; font-weight: bold; margin-bottom: 20px; }\n";
        html << "        .error-message { font-size: 1.5rem; margin-bottom: 20px; }\n";
        html << "        .error-reason { font-size: 1rem; margin-bottom: 30px; opacity: 0.9; }\n";
        html << "    </style>\n";
        html << "</head>\n";
        html << "<body>\n";
        html << "    <div class=\"error-container\">\n";
        html << "        <div class=\"error-code\">504</div>\n";
        html << "        <div class=\"error-message\">Gateway Timeout</div>\n";
        html << "        <div class=\"error-reason\">" << reason << "</div>\n";
        html << "        <div>WebServ - HTTP Server</div>\n";
        html << "    </div>\n";
        html << "</body>\n";
        html << "</html>\n";
        setBody(html.str());
    } else {
        setBody(generateErrorPage(HTTP_GATEWAY_TIMEOUT));
    }
}

void Response::sendRequestTimeout() {
    setStatus(HTTP_REQUEST_TIMEOUT);
    setHeader("Content-Type", CONTENT_TYPE_HTML);
    setHeader("Connection", "close");
    setBody(generateErrorPage(HTTP_REQUEST_TIMEOUT));
} 

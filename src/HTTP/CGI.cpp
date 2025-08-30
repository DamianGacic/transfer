#include "../../include/CGI.hpp"
#include "../../include/Utils.hpp"
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <cstring>
#include <ctime>
#include <sys/resource.h>

CGI::CGI() {
    gateway_interface = "CGI/1.1";
    server_software = "WebServer/1.0";
    server_protocol = "HTTP/1.1";
    remote_addr = "127.0.0.1";
    remote_host = "localhost";
    auth_type = "";
    remote_user = "";
    remote_ident = "";
}

CGI::~CGI() {
}

void CGI::setScriptPath(const std::string& path) {
    script_path = path;
    script_name = Utils::getBasename(path);
}

void CGI::setRequest(const Request& request) {
    request_method = request.getMethod();
    query_string = request.getQueryString();
    content_type = request.getHeader("Content-Type");
    content_length = request.getHeader("Content-Length");
    http_user_agent = request.getHeader("User-Agent");
    http_accept = request.getHeader("Accept");
    http_accept_language = request.getHeader("Accept-Language");
    http_accept_encoding = request.getHeader("Accept-Encoding");
    http_connection = request.getHeader("Connection");
    http_host = request.getHeader("Host");
    path_info = request.getPathInfo();
    request_uri = request.getUri();
    request_body = request.getBody();
}

void CGI::setDocumentRoot(const std::string& root) {
    document_root = root;
}

void CGI::setServerInfo(const std::string& name, const std::string& port) {
    server_name = name;
    server_port = port;
}

void CGI::setInterpreter(const std::string& interpreter) {
    interpreter_path = interpreter;
}

std::string CGI::execute() {
    if (!isValidScript(script_path)) {
        return "";
    }
    
    setupEnvironment();
    return executeScript();
}



void CGI::setupEnvironment() {
    env_vars.clear();
    setupStandardEnvironment();
    
    // Required CGI environment variables (RFC 3875)
    env_vars["GATEWAY_INTERFACE"] = gateway_interface;
    env_vars["SERVER_SOFTWARE"] = server_software;
    env_vars["SERVER_NAME"] = server_name;
    env_vars["SERVER_PORT"] = server_port;
    env_vars["SERVER_PROTOCOL"] = server_protocol;
    env_vars["REQUEST_METHOD"] = request_method;
    env_vars["SCRIPT_NAME"] = "/" + script_name;
    env_vars["QUERY_STRING"] = query_string;
    env_vars["REMOTE_ADDR"] = remote_addr;
    env_vars["REMOTE_HOST"] = remote_host;
    env_vars["REQUEST_URI"] = request_uri;
    env_vars["DOCUMENT_ROOT"] = document_root;
    
    // Optional meta-variables
    if (!path_info.empty()) {
        env_vars["PATH_INFO"] = path_info;
        env_vars["PATH_TRANSLATED"] = Utils::joinPath(document_root, path_info);
    }
    if (!content_type.empty()) env_vars["CONTENT_TYPE"] = content_type;
    if (!content_length.empty()) env_vars["CONTENT_LENGTH"] = content_length;
    if (!auth_type.empty()) env_vars["AUTH_TYPE"] = auth_type;
    if (!remote_user.empty()) env_vars["REMOTE_USER"] = remote_user;
    if (!remote_ident.empty()) env_vars["REMOTE_IDENT"] = remote_ident;
    
    // HTTP headers as environment variables
    if (!http_user_agent.empty()) env_vars["HTTP_USER_AGENT"] = http_user_agent;
    if (!http_accept.empty()) env_vars["HTTP_ACCEPT"] = http_accept;
    if (!http_accept_language.empty()) env_vars["HTTP_ACCEPT_LANGUAGE"] = http_accept_language;
    if (!http_accept_encoding.empty()) env_vars["HTTP_ACCEPT_ENCODING"] = http_accept_encoding;
    if (!http_connection.empty()) env_vars["HTTP_CONNECTION"] = http_connection;
    if (!http_host.empty()) env_vars["HTTP_HOST"] = http_host;
}

char** CGI::createEnvArray() {
    char** env_array = new char*[env_vars.size() + 1];
    int i = 0;
    
    for (std::map<std::string, std::string>::const_iterator it = env_vars.begin(); 
         it != env_vars.end(); ++it) {
        std::string env_var = it->first + "=" + it->second;
        env_array[i] = new char[env_var.length() + 1];
        strcpy(env_array[i], env_var.c_str());
        i++;
    }
    
    env_array[i] = NULL;
    return env_array;
}

void CGI::freeEnvArray(char** env) {
    for (int i = 0; env[i] != NULL; i++) {
        delete[] env[i];
    }
    delete[] env;
}

std::string CGI::executeScript() {
    int pipe_in[2], pipe_out[2], pipe_err[2];
    
    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1 || pipe(pipe_err) == -1) {
        return "";
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        close(pipe_in[0]); close(pipe_in[1]);
        close(pipe_out[0]); close(pipe_out[1]);
        close(pipe_err[0]); close(pipe_err[1]);
        return "";
    }
    
    if (pid == 0) {
        // Child process
        close(pipe_in[1]); // Close write end
        close(pipe_out[0]); // Close read end
        close(pipe_err[0]); // Close read end
        
        // Redirect stdin, stdout, and stderr
        dup2(pipe_in[0], STDIN_FILENO);
        dup2(pipe_out[1], STDOUT_FILENO);
        dup2(pipe_err[1], STDERR_FILENO);
        
        // Close unused file descriptors
        close(pipe_in[0]);
        close(pipe_out[1]);
        close(pipe_err[1]);
        
        // Set up environment
        char** env = createEnvArray();
        
        // Change to script directory for relative path resolution
        std::string script_dir = Utils::getDirname(script_path);
        if (!script_dir.empty()) {
            if (chdir(script_dir.c_str()) != 0) {
                // If chdir fails, exit child process
                freeEnvArray(env);
                exit(1);
            }
        }
        
        // Set resource limits for security
        struct rlimit rl;
        rl.rlim_cur = 60;  // 60 seconds CPU time limit
        rl.rlim_max = 60;
        setrlimit(RLIMIT_CPU, &rl);
        
        rl.rlim_cur = 50 * 1024 * 1024;  // 50MB memory limit
        rl.rlim_max = 50 * 1024 * 1024;
        setrlimit(RLIMIT_AS, &rl);
        
        // Execute the script with proper environment using execve
        if (!interpreter_path.empty()) {
            char* argv[] = { (char*)interpreter_path.c_str(), (char*)script_path.c_str(), NULL };
            execve(interpreter_path.c_str(), argv, env);
        } else {
            char* argv[] = { (char*)script_path.c_str(), NULL };
            execve(script_path.c_str(), argv, env);
        }

        // If we get here, exec failed
        freeEnvArray(env);
        exit(1);
    } else {
        // Parent process
        close(pipe_in[0]); // Close read end
        close(pipe_out[1]); // Close write end
        close(pipe_err[1]); // Close write end
        
        // Write request body to child
        if (!request_body.empty()) {
            ssize_t written = write(pipe_in[1], request_body.c_str(), request_body.length());
            (void)written; // Suppress unused variable warning
        }
        close(pipe_in[1]);
        
        // Read response from child with timeout
        std::string response;
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;
        
        // Set non-blocking I/O
        fcntl(pipe_out[0], F_SETFL, O_NONBLOCK);
        fcntl(pipe_err[0], F_SETFL, O_NONBLOCK);
        
        if (waitForChildWithTimeout(pid, CGI_TIMEOUT)) {
            // Read stdout
            while ((bytes_read = read(pipe_out[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[bytes_read] = '\0';
                response.append(buffer, bytes_read);
            }
        } else {
            // Timeout - kill the child process
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            close(pipe_out[0]);
            close(pipe_err[0]);
            std::string var;
            var = "timeout";
            return var;
            // Create a response object for proper timeout handling
            // Response timeout_response;
            // timeout_response.sendGatewayTimeout("CGI script execution timed out after " + Utils::toString(CGI_TIMEOUT) + " seconds.");
            // return timeout_response.buildResponse();
        }
        
        close(pipe_out[0]);
        close(pipe_err[0]);
        
        // Parse CGI output to separate headers and body
        if (!parseCGIOutput(response)) {
            // Create a response object for proper error handling
            Response error_response;
            error_response.sendError(HTTP_INTERNAL_SERVER_ERROR, "Invalid CGI output format from script.");
            return error_response.buildResponse();
        }
        
        return response;
    }
}

bool CGI::isValidScript(const std::string& path) {
    if (!Utils::fileExists(path)) {
        return false;
    }
    
    if (!Utils::isExecutable(path)) {
        return false;
    }
    
    // Security check: ensure path doesn't contain dangerous sequences
    if (path.find("..") != std::string::npos ||
        path.find("//") != std::string::npos ||
        path.find("./") == 0) {
        return false;
    }
    
    // Ensure the script is within allowed directories
    std::string abs_path = Utils::getAbsolutePath(path);
    if (abs_path.empty()) {
        return false;
    }
    
    return true;
}

std::string CGI::getScriptExtension(const std::string& filename) {
    return Utils::getExtension(filename);
}

bool CGI::isCGIScript(const std::string& filename, const std::map<std::string, std::string>& cgi_extensions) {
    std::string ext = getScriptExtension(filename);
    return cgi_extensions.find(ext) != cgi_extensions.end();
}

void CGI::setupStandardEnvironment() {
    // Standard environment variables that should always be set
    // Use a restricted PATH for security
    env_vars["PATH"] = "/usr/bin:/bin:/usr/local/bin";
    
    if (getenv("TZ")) {
        std::string tz = getenv("TZ");
        // Basic validation of TZ variable
        if (tz.length() < 100 && tz.find("..") == std::string::npos) {
            env_vars["TZ"] = tz;
        }
    }
    
    // Set safe defaults
    env_vars["IFS"] = " \t\n";
    
    // Clear potentially dangerous environment variables
    env_vars["LD_PRELOAD"] = "";
    env_vars["LD_LIBRARY_PATH"] = "";
}

bool CGI::waitForChildWithTimeout(pid_t pid, int timeout) {
    int status;
    time_t start_time = time(NULL);
    
    while (time(NULL) - start_time < timeout) {
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid) {
            // Child has finished
            return true;
        } else if (result == -1) {
            // Error occurred
            return false;
        }
        // Child is still running, sleep briefly
        usleep(100000); // 100ms
    }
    
    // Timeout occurred
    return false;
}

bool CGI::parseCGIOutput(const std::string& output) {
    cgi_headers.clear();
    cgi_body.clear();
    
    // Find the double CRLF that separates headers from body
    size_t header_end = output.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        // Try with just LF
        header_end = output.find("\n\n");
        if (header_end == std::string::npos) {
            // No header separation found - treat entire output as body
            cgi_body = output;
            // Add default content-type if missing
            if (output.find("Content-Type:") == std::string::npos) {
                cgi_headers = "Content-Type: text/html\r\n";
            }
            return true;
        }
        cgi_headers = output.substr(0, header_end);
        cgi_body = output.substr(header_end + 2);
    } else {
        cgi_headers = output.substr(0, header_end);
        cgi_body = output.substr(header_end + 4);
    }
    
    // Validate headers format
    std::istringstream header_stream(cgi_headers);
    std::string line;
    bool has_content_type = false;
    
    while (std::getline(header_stream, line)) {
        // Remove carriage return if present
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        
        if (line.empty()) continue;
        
        // Check for proper header format (Name: Value)
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) {
            return false;
        }
        
        std::string header_name = line.substr(0, colon_pos);
        if (header_name == "Content-Type") {
            has_content_type = true;
        }
    }
    
    // Add default Content-Type if not specified
    if (!has_content_type && cgi_headers.find("Status:") == std::string::npos) {
        if (!cgi_headers.empty()) {
            cgi_headers += "\r\n";
        }
        cgi_headers += "Content-Type: text/html";
    }
    
    return true;
}

std::string CGI::getHeaders() const {
    return cgi_headers;
}

std::string CGI::getBody() const {
    return cgi_body;
} 
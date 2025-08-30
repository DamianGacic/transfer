#include "../../include/Utils.hpp"
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <unistd.h>
#include <cstdlib>

// File operations
bool Utils::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool Utils::isDirectory(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0) {
        return S_ISDIR(buffer.st_mode);
    }
    return false;
}

bool Utils::isReadable(const std::string& path) {
    return (access(path.c_str(), R_OK) == 0);
}

bool Utils::isExecutable(const std::string& path) {
    return (access(path.c_str(), X_OK) == 0);
}

std::string Utils::readFile(const std::string& path) {
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

size_t Utils::getFileSize(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0) {
        return buffer.st_size;
    }
    return 0;
}

// String operations
std::vector<std::string> Utils::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

std::string Utils::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string Utils::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string Utils::toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

bool Utils::startsWith(const std::string& str, const std::string& prefix) {
    if (str.length() < prefix.length()) {
        return false;
    }
    return str.substr(0, prefix.length()) == prefix;
}

bool Utils::endsWith(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.substr(str.length() - suffix.length()) == suffix;
}

// URL operations
std::string Utils::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int value;
            std::istringstream iss(str.substr(i + 1, 2));
            iss >> std::hex >> value;
            result += static_cast<char>(value);
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string Utils::urlEncode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); i++) {
        char c = str[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += c;
        } else if (c == ' ') {
            result += '+';
        } else {
            std::ostringstream oss;
            oss << '%' << std::hex << std::uppercase << (int)(unsigned char)c;
            result += oss.str();
        }
    }
    return result;
}

// HTTP utilities
std::string Utils::getMimeType(const std::string& filename) {
    std::string ext = getExtension(filename);
    ext = toLower(ext);
    
    if (ext == ".html" || ext == ".htm") return CONTENT_TYPE_HTML;
    if (ext == ".css") return CONTENT_TYPE_CSS;
    if (ext == ".js") return CONTENT_TYPE_JS;
    if (ext == ".json") return CONTENT_TYPE_JSON;
    if (ext == ".txt") return CONTENT_TYPE_PLAIN;
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".xml") return "application/xml";
    if (ext == ".mp4") return "video/mp4";
    if (ext == ".avi") return "video/x-msvideo";
    if (ext == ".mov") return "video/quicktime";
    if (ext == ".wmv") return "video/x-ms-wmv";
    if (ext == ".flv") return "video/x-flv";
    if (ext == ".webm") return "video/webm";
    
    return CONTENT_TYPE_OCTET;
}

std::string Utils::getStatusMessage(int status_code) {
    switch (status_code) {
        // 1xx Informational
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        
        // 2xx Success
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 204: return "No Content";
        
        // 3xx Redirection
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        
        // 4xx Client Error
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Request Entity Too Large";
        case 414: return "Request-URI Too Long";
        case 415: return "Unsupported Media Type";
        case 416: return "Range Not Satisfiable";
        case 417: return "Expectation Failed";
        case 418: return "I'm a teapot";
        case 421: return "Misdirected Request";
        case 422: return "Unprocessable Entity";
        case 423: return "Locked";
        case 424: return "Failed Dependency";
        case 425: return "Too Early";
        case 426: return "Upgrade Required";
        case 428: return "Precondition Required";
        case 429: return "Too Many Requests";
        case 431: return "Request Header Fields Too Large";
        case 451: return "Unavailable For Legal Reasons";
        
        // 5xx Server Error
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        case 506: return "Variant Also Negotiates";
        case 507: return "Insufficient Storage";
        case 508: return "Loop Detected";
        case 510: return "Not Extended";
        case 511: return "Network Authentication Required";
        
        // Nginx-specific codes
        case 444: return "No Response";
        case 494: return "Request Header Too Large";
        case 499: return "Client Closed Request";
        
        // 6xx Future Extension (not standard HTTP but prepared for future use)
        case 600: return "Extended Error";
        
        default: 
            // Handle ranges for codes not explicitly defined
            if (status_code >= 400 && status_code < 500) {
                return "Client Error";
            } else if (status_code >= 500 && status_code < 600) {
                return "Server Error";
            } else if (status_code >= 600 && status_code < 700) {
                return "Extended Error";
            }
            return "Unknown";
    }
}

std::string Utils::getCurrentTime() {
    std::time_t now = std::time(NULL);
    std::tm* tm_info = std::localtime(&now);
    
    char buffer[80];
    std::strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", tm_info);
    return std::string(buffer);
}

// Path operations
std::string Utils::joinPath(const std::string& path1, const std::string& path2) {
    if (path1.empty()) return path2;
    if (path2.empty()) return path1;
    
    if (path1[path1.length() - 1] == '/') {
        return path1 + path2;
    } else {
        return path1 + "/" + path2;
    }
}

std::string Utils::getExtension(const std::string& filename) {
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos) {
        return filename.substr(pos);
    }
    return "";
}

std::string Utils::getBasename(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos != std::string::npos) {
        return path.substr(pos + 1);
    }
    return path;
}

std::string Utils::getDirname(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos != std::string::npos) {
        return path.substr(0, pos);
    }
    return "";
}

std::string Utils::getAbsolutePath(const std::string& path) {
    // If path is already absolute
    if (!path.empty() && path[0] == '/') {
        return path;
    }
    
    // Get current working directory
    char* cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        return "";
    }
    
    std::string result = std::string(cwd) + "/" + path;
    free(cwd);
    
    return result;
}

// Network utilities
std::string Utils::getClientIP(int client_fd) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    
    if (getpeername(client_fd, (struct sockaddr*)&addr, &addr_len) == 0) {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ip, INET_ADDRSTRLEN);
        return std::string(ip);
    }
    
    return "127.0.0.1";
}

int Utils::getClientPort(int client_fd) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    
    if (getpeername(client_fd, (struct sockaddr*)&addr, &addr_len) == 0) {
        return ntohs(addr.sin_port);
    }
    
    return 0;
}

// Error handling
void Utils::logError(const std::string& message) {
    std::cerr << "[ERROR] " << getCurrentTime() << ": " << message << std::endl;
}

void Utils::logInfo(const std::string& message) {
    std::cout << "[INFO] " << getCurrentTime() << ": " << message << std::endl;
}

std::string Utils::toString(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

std::string Utils::toString(size_t value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

std::string Utils::decodeBase64(const std::string& encoded) {
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string decoded;
    int val = 0, valb = -8;
    
    for (std::string::const_iterator it = encoded.begin(); it != encoded.end(); ++it) {
        if (chars.find(*it) == std::string::npos) break;
        val = (val << 6) + chars.find(*it);
        valb += 6;
        if (valb >= 0) {
            decoded.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return decoded;
}

bool Utils::validateBasicAuth(const std::string& authorization_header) {
    if (authorization_header.empty())
        return false;
    
    // Check if it starts with "Basic "
    if (authorization_header.substr(0, 6) != "Basic ")
        return false;
    
    // Decode the base64 part
    std::string credentials = decodeBase64(authorization_header.substr(6));
    
    // Find the colon separator
    size_t colon_pos = credentials.find(':');
    if (colon_pos == std::string::npos)
        return false;
    
    std::string username = credentials.substr(0, colon_pos);
    std::string password = credentials.substr(colon_pos + 1);
    
    // Hardcoded authentication credentials
    // Check against predefined username/password combinations
    if ((username == "webserv" && password == "upload123") ||
        (username == "admin" && password == "admin456") ||
        (username == "user" && password == "test789")) {
        return true;
    }
    
    return false;
}


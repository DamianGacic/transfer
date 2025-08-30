#ifndef UTILS_HPP
#define UTILS_HPP

#include "webserv.hpp"
#include <sstream>
#include <fstream>
#include <sys/stat.h>

class Utils {
public:
    // File operations
    static bool							fileExists(const std::string& path);
    static bool							isDirectory(const std::string& path);
    static bool							isReadable(const std::string& path);
    static bool							isExecutable(const std::string& path);
    static std::string					readFile(const std::string& path);
    static size_t						getFileSize(const std::string& path);
    
    // String operations
    static std::vector<std::string>		split(const std::string& str, char delimiter);
    static std::string					trim(const std::string& str);
    static std::string					toLower(const std::string& str);
    static std::string					toUpper(const std::string& str);
    static bool							startsWith(const std::string& str, const std::string& prefix);
    static bool							endsWith(const std::string& str, const std::string& suffix);
    
    // URL operations
    static std::string					urlDecode(const std::string& str);
    static std::string					urlEncode(const std::string& str);
    
    // HTTP utilities
    static std::string					getMimeType(const std::string& filename);
    static std::string					getStatusMessage(int status_code);
    static std::string					getCurrentTime();
    
    // Path operations
    static std::string					joinPath(const std::string& path1, const std::string& path2);
    static std::string					getExtension(const std::string& filename);
    static std::string					getBasename(const std::string& path);
    static std::string					getDirname(const std::string& path);
    static std::string					getAbsolutePath(const std::string& path);
    
    // Network utilities
    static std::string					getClientIP(int client_fd);
    static int							getClientPort(int client_fd);
    
    // Error handling
    static void							logError(const std::string& message);
    static void							logInfo(const std::string& message);
    
    // Conversion utilities
    static std::string					toString(int value);
    static std::string					toString(size_t value);
    
    // Authentication utilities
    static bool							validateBasicAuth(const std::string& authorization_header);
    static std::string					decodeBase64(const std::string& encoded);
};

#endif 
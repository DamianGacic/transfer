#include "../../include/Request.hpp"
#include "../../include/Utils.hpp"

Request::Request() : 
    method(""),
    uri(""),
    version(""),
    is_chunked(false),
    content_length(0),
    is_complete(false),
    expected_chunk_size(0),
    reading_chunk_size(true),
    headers_parsed(false),
    waiting_final_crlf(false),
    reading_trailing_headers(false),
    max_body_size(0) {
}

Request::~Request() {
}

void Request::parse(const std::string& raw_request) {
    buffer += raw_request;
    
    if (is_complete) {
        return;
    }
    
    if (!headers_parsed) {
        size_t pos = buffer.find("\r\n\r\n");
        if (pos == std::string::npos) {
            // Check if headers are too large
            if (buffer.length() > MAX_HEADER_SIZE) {
                // Headers too large - mark as complete with error
                is_complete = true;
                method = "ERROR_HEADER_TOO_LARGE";
                return;
            }
            return; // Incomplete request
        }
        
        std::string headers_part = buffer.substr(0, pos);
        std::string body_part = buffer.substr(pos + 4);
        
        // Parse headers
        std::istringstream header_stream(headers_part);
        std::string line;
        
        // Parse request line
        if (std::getline(header_stream, line)) {
            parseRequestLine(line);
        }
        
        // Parse headers
        while (std::getline(header_stream, line)) {
            if (line.empty() || line == "\r") {
                break;
            }
            parseHeader(line);
        }
        
        headers_parsed = true;
        
        // Remove processed headers from buffer
        buffer = body_part;
    }
    
    // Handle body
    if (is_chunked) {
        parseChunkedBody(buffer);
        buffer.clear(); // Clear buffer after processing chunked data
    } else {
        body = buffer;
        if (content_length > 0 && body.length() >= content_length) {
            body = body.substr(0, content_length);
            is_complete = true;
        } else if (content_length == 0) {
            is_complete = true;
        }
    }
}

void Request::appendData(const std::string& data) {
    parse(data);
}

void Request::parseRequestLine(const std::string& line) {
    std::vector<std::string> parts = Utils::split(line, ' ');
    if (parts.size() >= 3) {
        method = parts[0];
        uri = parts[1];
        version = Utils::trim(parts[2]); // Ensure version is trimmed
        
        // Parse query string
        size_t query_pos = uri.find('?');
        if (query_pos != std::string::npos) {
            query_string = uri.substr(query_pos + 1);
            uri = uri.substr(0, query_pos);
        }
    }
}

void Request::parseHeader(const std::string& line) {
    size_t colon_pos = line.find(':');
    if (colon_pos != std::string::npos) {
        std::string name = Utils::trim(line.substr(0, colon_pos));
        std::string value = Utils::trim(line.substr(colon_pos + 1));
        
        // Remove \r if present
        if (!value.empty() && value[value.length() - 1] == '\r') {
            value = value.substr(0, value.length() - 1);
        }
        
        headers[Utils::toLower(name)] = value;
        
        // Handle special headers
        if (Utils::toLower(name) == "content-length") {
            content_length = std::atoi(value.c_str());
            // Check content-length against max body size limit immediately
            if (max_body_size > 0 && content_length > max_body_size) {
                is_complete = true;
                method = "ERROR_REQUEST_ENTITY_TOO_LARGE";
                return;
            }
        } else if (Utils::toLower(name) == "transfer-encoding" && 
                   Utils::toLower(value) == "chunked") {
            is_chunked = true;
        }
    }
}

void Request::parseChunkedBody(const std::string& data) {
    chunk_buffer += data;
    
    while (!chunk_buffer.empty() && !is_complete) {
        if (reading_trailing_headers) {
            // Parse trailing headers after final chunk
            size_t crlf_pos = chunk_buffer.find("\r\n");
            if (crlf_pos == std::string::npos) {
                return; // Need more data
            }
            
            std::string header_line = chunk_buffer.substr(0, crlf_pos);
            chunk_buffer = chunk_buffer.substr(crlf_pos + 2);
            
            if (header_line.empty()) {
                // Empty line indicates end of trailing headers
                is_complete = true;
                return;
            }
            
            // Parse trailing header
            size_t colon_pos = header_line.find(':');
            if (colon_pos != std::string::npos) {
                std::string name = Utils::trim(header_line.substr(0, colon_pos));
                std::string value = Utils::trim(header_line.substr(colon_pos + 1));
                trailing_headers[Utils::toLower(name)] = value;
            }
            
        } else if (waiting_final_crlf) {
            // After zero chunk, check for trailing headers or final CRLF
            size_t crlf_pos = chunk_buffer.find("\r\n");
            if (crlf_pos == std::string::npos) {
                return; // Need more data
            }
            
            std::string line = chunk_buffer.substr(0, crlf_pos);
            
            if (line.empty()) {
                // Empty line = no trailing headers, complete
                is_complete = true;
                return;
            } else {
                // Has trailing headers
                reading_trailing_headers = true;
                waiting_final_crlf = false;
                continue; // Process the header line
            }
            
        } else if (reading_chunk_size) {
            // Look for CRLF to find end of chunk size line
            size_t crlf_pos = chunk_buffer.find("\r\n");
            if (crlf_pos == std::string::npos) {
                return; // Need more data
            }
            
            std::string size_line = chunk_buffer.substr(0, crlf_pos);
            chunk_buffer = chunk_buffer.substr(crlf_pos + 2);
            
            // Parse chunk size, handling extensions
            std::string hex_size = parseChunkSize(size_line);
            
            // Parse hex chunk size
            std::stringstream ss;
            ss << std::hex << hex_size;
            ss >> expected_chunk_size;
            
            // Check for parsing failure
            if (ss.fail()) {
                expected_chunk_size = 0;
            }
            
            // Check size limits
            if (expected_chunk_size > MAX_CHUNK_SIZE) {
                // Chunk too large, reject
                is_complete = true; // Mark as complete but body will be truncated
                return;
            }
            
            // Check total body size limit
            size_t max_size = (max_body_size > 0) ? max_body_size : MAX_BODY_SIZE;
            if (body.length() + expected_chunk_size > max_size) {
                // Total body too large, reject
                is_complete = true;
                method = "ERROR_REQUEST_ENTITY_TOO_LARGE";
                return;
            }
            
            // Check for final chunk (size 0)
            if (expected_chunk_size == 0) {
                waiting_final_crlf = true;
                reading_chunk_size = false;
                continue;
            }
            
            reading_chunk_size = false;
            
        } else {
            // Reading chunk data
            size_t needed = expected_chunk_size + 2; // +2 for trailing CRLF
            if (chunk_buffer.length() < needed) {
                return; // Need more data
            }
            
            // Extract chunk data (excluding trailing CRLF)
            std::string chunk_data = chunk_buffer.substr(0, expected_chunk_size);
            body += chunk_data;
            
            // Remove processed chunk data and trailing CRLF
            chunk_buffer = chunk_buffer.substr(needed);
            
            // Reset for next chunk
            reading_chunk_size = true;
            expected_chunk_size = 0;
        }
    }
}

std::string Request::parseChunkSize(const std::string& size_line) {
    // Parse chunk size, handling extensions (RFC 7230)
    // Format: chunk-size [ chunk-ext ] CRLF
    // chunk-ext = *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
    
    std::string hex_size;
    size_t semicolon_pos = size_line.find(';');
    
    if (semicolon_pos != std::string::npos) {
        // Has extensions, extract just the size part
        hex_size = Utils::trim(size_line.substr(0, semicolon_pos));
    } else {
        // No extensions
        hex_size = Utils::trim(size_line);
    }
    
    return hex_size;
}

std::string Request::urlDecode(const std::string& str) {
    return Utils::urlDecode(str);
}

bool Request::isComplete() const {
    return is_complete;
}

bool Request::hasError() const {
    return method.find("ERROR_") == 0;
}

std::string Request::getErrorType() const {
    if (hasError()) {
        return method;
    }
    return "";
}

void Request::reset() {
    method.clear();
    uri.clear();
    version.clear();
    headers.clear();
    body.clear();
    query_string.clear();
    path_info.clear();
    is_chunked = false;
    content_length = 0;
    is_complete = false;
    buffer.clear();
    chunk_buffer.clear();
    expected_chunk_size = 0;
    reading_chunk_size = true;
    headers_parsed = false;
    waiting_final_crlf = false;
    reading_trailing_headers = false;
    trailing_headers.clear();
    max_body_size = 0;
}

void Request::setMaxBodySize(size_t max_size) {
    max_body_size = max_size;
}

// Getters
const std::string& Request::getMethod() const { return method; }
const std::string& Request::getUri() const { return uri; }
const std::string& Request::getVersion() const { return version; }
const std::map<std::string, std::string>& Request::getHeaders() const { return headers; }
const std::string& Request::getBody() const { return body; }
const std::string& Request::getQueryString() const { return query_string; }
const std::string& Request::getPathInfo() const { return path_info; }
size_t Request::getContentLength() const { return content_length; }
bool Request::isChunked() const { return is_chunked; }

std::string Request::getHeader(const std::string& name) const {
    std::map<std::string, std::string>::const_iterator it = headers.find(Utils::toLower(name));
    return (it != headers.end()) ? it->second : "";
}

bool Request::hasHeader(const std::string& name) const {
    return headers.find(Utils::toLower(name)) != headers.end();
}

void Request::print() const {
    std::cout << "Request:" << std::endl;
    std::cout << "Method: " << method << std::endl;
    std::cout << "URI: " << uri << std::endl;
    std::cout << "Version: " << version << std::endl;
    std::cout << "Headers:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); 
         it != headers.end(); ++it) {
        std::cout << "  " << it->first << ": " << it->second << std::endl;
    }
    std::cout << "Body length: " << body.length() << std::endl;
    std::cout << "Complete: " << (is_complete ? "yes" : "no") << std::endl;
} 
#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "webserv.hpp"

// Chunked transfer encoding limits
#define MAX_CHUNK_SIZE (50 * 1024 * 1024)  // 50MB max chunk size
#define MAX_BODY_SIZE (100 * 1024 * 1024)  // 100MB max total body size
#define MAX_HEADER_SIZE (8 * 1024)  // 8KB max header size

class Request {
private:
	std::string							method;
	std::string							uri;
	std::string							version;
	std::map<std::string, std::string>	headers;
	std::string							body;
	std::string							query_string;
	std::string							path_info;
	bool								is_chunked;
	size_t								content_length;
	bool								is_complete;
	std::string							buffer;
	std::string							chunk_buffer;
	size_t								expected_chunk_size;
	bool								reading_chunk_size;
	bool								headers_parsed;
	bool								waiting_final_crlf;
	bool								reading_trailing_headers;
	std::map<std::string, std::string>	trailing_headers;
	size_t								max_body_size;
	
	void								parseRequestLine(const std::string& line);
	void								parseHeader(const std::string& line);
	void								parseChunkedBody(const std::string& data);
	std::string							parseChunkSize(const std::string& size_line);
	std::string							urlDecode(const std::string& str);

public:
	Request();
	~Request();
	void										parse(const std::string& raw_request);
	void										appendData(const std::string& data);
	bool										isComplete() const;
	bool										hasError() const;
	std::string									getErrorType() const;
	void										reset();
	void										setMaxBodySize(size_t max_size);
	// Getters
	const std::string&							getMethod() const;
	const std::string&							getUri() const;
	const std::string&							getVersion() const;
	const std::map<std::string, std::string>&	getHeaders() const;
	const std::string&				       		getBody() const;
	const std::string&				       		getQueryString() const;
	const std::string&				          	getPathInfo() const;
	size_t								       	getContentLength() const;
	bool										isChunked() const;
	// Header helpers
	std::string									getHeader(const std::string& name) const;
	bool										hasHeader(const std::string& name) const;
	void										print() const;
};

#endif 
#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "webserv.hpp"

class Response {
private:
	int									status_code;
	std::map<std::string, std::string>	headers;
	std::string							body;
	std::string							status_message;
	bool								is_sent;
	std::string							http_version;
	std::string							getContentType(const std::string& filename);
	std::string							generateErrorPage(int code);
	std::string							generateDirectoryListing(const std::string& path, const std::string& uri);
	std::string							getErrorDescription(int code);
	void								setStatusMessage();
	bool								tryCustomErrorPage(int code, const std::map<int, std::string>& error_pages, const std::string& root_path);
	std::string							generateTemplateErrorPage(int code, const std::string& root_path);
	std::string							replacePlaceholders(const std::string& template_content, int code);
public:
	Response();
	~Response();
	void										setStatus(int code);
	void										setHeader(const std::string& name, const std::string& value);
	void										setBody(const std::string& content);
	void										setBodyFromFile(const std::string& filename);
	// Response generation methods
	void										sendFile(const std::string& filename);
	void										sendError(int code, const std::string& custom_message = "");
	void										sendError(int code, const ServerConfig& config, const std::string& root_path = "./www");
	void										sendRedirect(const std::string& location);
	void										sendDirectoryListing(const std::string& path, const std::string& uri);
	// Nginx-specific error responses
	void										sendRequestHeaderTooLarge();
	void										sendClientClosedRequest();
	void										sendGatewayTimeout(const std::string& reason = "");
	// Timeout handling
	void										sendRequestTimeout();
	// Getters
	int											getStatusCode() const;
	const std::map<std::string, std::string>&	getHeaders() const;
	const std::string&							getBody() const;
	const std::string&							getStatusMessage() const;
	bool										isSent() const;
	// Response building
	std::string									buildResponse();
	std::string									buildResponse(const std::string& request_version);
	void										markAsSent();
	void										reset();
	void										print() const;
};

#endif 
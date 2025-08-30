#include "../../include/parse.hpp"
#include "../../include/Utils.hpp"
#include <set>
#include <climits>
#include <sstream>

void add_server_name(std::string serverNameValue, Config &configItem) {
	Server &serverConfig = static_cast<Server &>(configItem);

	if (serverNameValue.empty())
		throw std::invalid_argument("server_name directive cannot be empty.");

	serverConfig.set_server_name(serverNameValue);
}

void add_address(std::string addressValue, Config &configItem) {
	Server &serverConfig = static_cast<Server &>(configItem);

	if (addressValue.empty())
		throw std::invalid_argument("address directive cannot be empty.");

	if (addressValue.compare("localhost") == 0)
		serverConfig.set_ip(IP_DEFAULT);
	else if (is_valid_ipv4(addressValue))
		serverConfig.set_ip(addressValue);
	else
		throw std::invalid_argument("Invalid address directive. Address must be a valid IPv4.");
}

void add_listen(std::string portsValue, Config &configItem) {
	Server &serverConfig = static_cast<Server &>(configItem);

	if (portsValue.empty())
		throw std::invalid_argument("listen directive cannot be empty.");

	std::istringstream portsStream(portsValue);
	std::string portString;
	while (std::getline(portsStream, portString, ' ')) {
		if (is_valid_port(portString)) {
			char *conversionEnd;
			long portNumber = strtol(portString.c_str(), &conversionEnd, 10);
			serverConfig.add_port(static_cast<int>(portNumber));
		} else {
			throw std::invalid_argument("Invalid listen directive. Port must be a number between 0 to 65535.");
		}
	}
}


void add_root(std::string rootValue, Config &configItem) {
	if (rootValue.empty())
		throw std::invalid_argument("root directive cannot be empty.");

	std::string resolvedRootPath = rootValue;
	if (!is_valid_absolute_path(rootValue)) {
		resolvedRootPath = Utils::getAbsolutePath(rootValue);
		if (resolvedRootPath.empty()) {
			throw std::invalid_argument("Invalid root directive. Cannot resolve path.");
		}
	}
	resolvedRootPath.at(resolvedRootPath.size() - 1) != '/' ? configItem.set_root(resolvedRootPath) : configItem.set_root(resolvedRootPath.substr(0, resolvedRootPath.size() - 1));
}

void parseClientMaxBodySize(std::string sizeValue, Config &configItem) {
	if (sizeValue.empty() || sizeValue.at(0) == '-')
		throw std::invalid_argument("client_max_body_size directive cannot be empty or a negative number.");
	
	char* conversionEnd;
	size_t sizeInBytes = strtoul(sizeValue.c_str(), &conversionEnd, 10);

	// Before checking ULONG_MAX, verify if the input string represents ULONG_MAX
	std::stringstream ss;
	ss << ULONG_MAX;
	std::string ulongMaxStr = ss.str();
	if (sizeValue == ulongMaxStr) {
		// This is a legitimate ULONG_MAX value, allow it
	} else if (sizeInBytes == ULONG_MAX) {
		// This must be an overflow
		throw std::invalid_argument("client_max_body_size value is too large.");
	}

	std::string sizeSuffix(conversionEnd);

	if (sizeSuffix.size() == 0) {
		if (sizeInBytes > MAX_BODY_SIZE_BYTES)
			throw std::invalid_argument("client_max_body_size exceeds the maximum limit of 50 MB.");
		
		configItem.set_client_max_size(sizeInBytes);
		return;
	}

	if (sizeSuffix.size() != 1)
		throw std::invalid_argument("Invalid client_max_body_size directive. possible suffix letters are [ k, K, m, M, g, G ].");

	size_t sizeMultiplier = 0;
	switch (std::tolower(sizeSuffix[0])) {
	case 'k':
		sizeMultiplier = 1024;
		break;
	case 'm':
		sizeMultiplier = 1024 * 1024;
		break;
	case 'g':
		sizeMultiplier = 1024 * 1024 * 1024;
		break;
	default:
		throw std::invalid_argument("Invalid client_max_body_size directive. Accepted suffix are [ k, K, m, M, g, G ].");
	}

	if (sizeInBytes > ULONG_MAX / sizeMultiplier) {
		throw std::invalid_argument("client_max_body_size value causes overflow.");
	}

	size_t totalSizeInBytes = sizeInBytes * sizeMultiplier;

	if (totalSizeInBytes > MAX_BODY_SIZE_BYTES)
		throw std::invalid_argument("client_max_body_size exceeds the maximum limit of 50 MB.");

	configItem.set_client_max_size(totalSizeInBytes);
}

void parseErrorPageDirective(std::string errorPageValue, Config &configItem) {
	if (errorPageValue.empty())
		throw std::invalid_argument("error_page directive cannot be empty.");
	
	std::istringstream errorPageStream(errorPageValue);
	std::string currentToken;
	std::vector<int> errorCodes;
	std::string redirectPath;

	while (errorPageStream >> currentToken) {
		int httpStatusCode;
		if ((httpStatusCode = http_code(currentToken)) != -1 && httpStatusCode >= 300 && httpStatusCode <= 599) {
			errorCodes.push_back(httpStatusCode);
		} else if (is_valid_url_or_path(currentToken)) {
			redirectPath = currentToken;
			break;
		} else {
			throw std::invalid_argument("Invalid error_page directive. Format must follow: error_page [error_codes (300 - 599)] [path | URL];");
		}
	}

	if (errorCodes.empty() || redirectPath.empty() || errorPageStream >> currentToken)
		throw std::invalid_argument("Invalid error_page directive. Format must follow: error_page [error_codes (300 - 599)] [path | URL];");

	for (std::vector<int>::iterator codeIterator = errorCodes.begin(); codeIterator != errorCodes.end(); ++codeIterator) {
		configItem.add_error_page(*codeIterator, redirectPath);
	}
}

void add_index(std::string indexValue, Config &configItem) {
	if (indexValue.empty())
		throw std::invalid_argument("index directive cannot be empty.");

	std::istringstream indexStream(indexValue);
	std::string indexFileName;

	while (std::getline(indexStream, indexFileName, ' ')) {
		if (indexFileName.at(0) == '/')
			throw std::invalid_argument("Invalid index directive. Index cannot be an absolute path.");
		if (indexFileName.at(indexFileName.size() - 1) == '/')
			throw std::invalid_argument("Invalid index directive. Index cannot be a directory.");
		
		configItem.add_index(indexFileName);
	}
}

void add_autoindex(std::string autoindexValue, Config &configItem) {
	if (autoindexValue.empty())
		throw std::invalid_argument("autoindex directive cannot be empty.");

	std::string lowerCaseValue;
	for (std::string::const_iterator charIterator = autoindexValue.begin(); charIterator != autoindexValue.end(); ++charIterator) {
		lowerCaseValue += std::tolower(*charIterator);
	}

	if (lowerCaseValue.compare("true") == 0)
		configItem.set_autoindex(true);
	else if (lowerCaseValue.compare("false") == 0)
		configItem.set_autoindex(false);
	else
		throw std::invalid_argument("Invalid autoindex directive. Accepted values are [ true, false ].");
}



void add_return(std::string returnValue, Config &configItem) {
	if (returnValue.empty())
		throw std::invalid_argument("return directive cannot be empty.");

	std::istringstream returnStream(returnValue);
	std::string statusCodeString, returnText;

	if (std::getline(returnStream, statusCodeString, ' ') && std::getline(returnStream, returnText)) {
		Config::ReturnData returnData;

		if ((returnData.code = http_code(statusCodeString)) == -1)
			throw std::invalid_argument("Invalid return directive. Return code must be a valid HTTP code (between 100 and 599).");

		if (returnText.at(0) != '"' || returnText.at(returnText.size() - 1) != '"')
			throw std::invalid_argument("Invalid return directive. Return text must be all between quotes.");

		returnData.text = returnText.substr(1, returnText.size() - 2);
		configItem.set_return(returnData);
	} else {
		throw std::invalid_argument("Invalid return directive. Return directive contains exactly one status code and one URL/text.");
	}
}

void add_methods(std::string methodsValue, Config &configItem) {
	if (methodsValue.empty())
		throw std::invalid_argument("methods directive cannot be empty.");

	std::set<std::string> allowedHttpMethods;
	allowedHttpMethods.insert("GET");
	allowedHttpMethods.insert("POST");
	allowedHttpMethods.insert("DELETE");

	std::string upperCaseMethods;
	for (std::string::const_iterator charIterator = methodsValue.begin(); charIterator != methodsValue.end(); ++charIterator) {
		upperCaseMethods += std::toupper(*charIterator);
	}

	std::istringstream methodsStream(upperCaseMethods);
	std::string httpMethod;
	while (std::getline(methodsStream, httpMethod, ' ')) {
		if (allowedHttpMethods.find(httpMethod) == allowedHttpMethods.end())
			throw std::invalid_argument("Invalid methods directive. Accepted methods are [ GET, POST, DELETE ].");

		configItem.add_method(httpMethod);
	}
}

void add_alias(std::string aliasValue, Config &configItem) {
	Location &locationConfig = static_cast<Location &>(configItem);

	if (aliasValue.empty())
		throw std::invalid_argument("alias directive cannot be empty.");

	std::string resolvedAliasPath = aliasValue;
	if (aliasValue != "/" && !is_valid_absolute_path(aliasValue)) {
		resolvedAliasPath = Utils::getAbsolutePath(aliasValue);
		if (resolvedAliasPath.empty()) {
			throw std::invalid_argument("Invalid alias directive. Cannot resolve path.");
		}
	}

	if (resolvedAliasPath == "/")
		locationConfig.setAlias(resolvedAliasPath);
	else
		resolvedAliasPath.at(resolvedAliasPath.size() - 1) != '/' ? locationConfig.setAlias(resolvedAliasPath) : locationConfig.setAlias(resolvedAliasPath.substr(0, resolvedAliasPath.size() - 1));
}

void add_cgi_extension(std::string cgiValue, Config &configItem) {
	if (cgiValue.empty())
		throw std::invalid_argument("cgi_extension directive cannot be empty.");

	std::istringstream cgiStream(cgiValue);
	std::string fileExtension, interpreterPath;

	if (std::getline(cgiStream, fileExtension, ' ') && std::getline(cgiStream, interpreterPath)) {
		if (fileExtension.at(0) != '.')
			throw std::invalid_argument("Invalid cgi_extension directive. Extension must start with a dot.");

		configItem.add_cgi_extension(fileExtension, interpreterPath);
	} else {
		throw std::invalid_argument("Invalid cgi_extension directive. Format must be: cgi_extension .ext interpreter;");
	}
}

void add_auth_basic(std::string authValue, Config &configItem) {
	if (authValue.empty())
		throw std::invalid_argument("auth_basic directive cannot be empty.");

	if (authValue.size() >= 2 && authValue.at(0) == '"' && authValue.at(authValue.size() - 1) == '"')
		authValue = authValue.substr(1, authValue.size() - 2);

	configItem.set_auth_basic_realm(authValue);
}

void add_auth_basic_user_file(std::string userFilePath, Config &configItem) {
	if (userFilePath.empty())
		throw std::invalid_argument("auth_basic_user_file directive cannot be empty.");

	configItem.set_auth_basic_user_file(userFilePath);
}

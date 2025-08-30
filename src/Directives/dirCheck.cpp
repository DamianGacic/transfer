#include "../../include/parse.hpp"

bool is_valid_ipv4(std::string ipAddress) {
	std::istringstream ipStream(ipAddress);
	std::string octetString;
	std::vector<std::string> ipOctets;

	while (std::getline(ipStream, octetString, '.'))
		ipOctets.push_back(octetString);

	if (ipOctets.size() != 4)
		return false;

	for (size_t octetIndex = 0; octetIndex < ipOctets.size(); ++octetIndex) {
		const std::string& currentOctet = ipOctets[octetIndex];

		if (currentOctet.empty() || currentOctet.find_first_not_of("0123456789") != std::string::npos)
			return false;

		char *conversionEnd;
		long octetValue = strtol(currentOctet.c_str(), &conversionEnd, 10);

		if (*conversionEnd != '\0' || octetValue < 0 || octetValue > 255)
			return false;
	}

	return true;
}

bool is_valid_port(std::string portString) {
	if (portString.empty())
		return false;

	char* conversionEnd;
	long portNumber = strtol(portString.c_str(), &conversionEnd, 10);

	return !(*conversionEnd != '\0' || portNumber < 0 || portNumber > 65535);
}

bool is_valid_absolute_path(std::string filePath) {
	return filePath.at(0) == '/' && filePath.find(' ') == std::string::npos;
}

int http_code(std::string codeString) {
	char *conversionEnd;
	int httpStatusCode = std::strtol(codeString.c_str(), &conversionEnd, 10);

	return (!*conversionEnd && (httpStatusCode >= 100 && httpStatusCode <= 599)) ? httpStatusCode : -1;
}

bool is_valid_url_or_path(const std::string urlOrPath) {
	return !urlOrPath.empty() && ((urlOrPath[0] == '/' && urlOrPath[urlOrPath.size() - 1] != '/') || urlOrPath.substr(0, 4) == "http");
}
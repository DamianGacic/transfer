#include "../../include/parse.hpp"


std::vector<Server>	parse( std::string const& config_file )
{
	std::vector<Server> server_list;
	std::ifstream input_stream(config_file.c_str());
	
	server_list.clear();
	if (!input_stream.is_open()) {
		std::cerr << "couldnt open the following file: " << config_file << '\n';
		return server_list;
	}
	try
	{
		std::string current_line;
		bool expect_server = true;
		
		std::map<std::string, Function> server_handlers = get_server_directives();
		std::map<std::string, Function> location_handlers = get_location_directives();
		while (std::getline(input_stream, current_line)) {
			tokenize(current_line);
			if (current_line.empty() || current_line.at(0) == '#')
				continue;
			while (current_line.compare("server {") == 0)
				processServerBlock(server_list, input_stream, current_line, server_handlers, location_handlers, expect_server);
			if (!current_line.empty() && expect_server) {
				if (current_line.at(0) != '#')
					throw std::invalid_argument("Error while parsing configuration file. Check lines between servers.");
			}
		}
	}
	catch(const std::exception& parse_error)
	{
		server_list.clear();
		std::cerr << "[ ERROR ] " << config_file << ": " << parse_error.what() << '\n';
	}
	input_stream.close();
	return server_list;
}

// Produces a trimmed string with single spaces between tokens
void tokenize(std::string& input_str) {
	std::string clean_result;
	bool in_whitespace = false;

	for (size_t idx = 0; idx < input_str.size(); ++idx) {
		if (std::isspace(input_str[idx])) {
			if (!in_whitespace) {
				clean_result += ' ';
				in_whitespace = true;
			}
		} else {
			clean_result += input_str[idx];
			in_whitespace = false;
		}
	}

	size_t start_pos = 0;
	size_t end_pos = clean_result.size();
	while (start_pos < end_pos && std::isspace(clean_result[start_pos])) ++start_pos;
	while (end_pos > start_pos && std::isspace(clean_result[end_pos - 1])) --end_pos;
	clean_result = clean_result.substr(start_pos, end_pos - start_pos);
	input_str = clean_result;
}
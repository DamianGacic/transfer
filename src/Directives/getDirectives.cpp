#include "../../include/parse.hpp"
#include "../../include/CGI.hpp"

std::map<std::string, Function> get_server_directives() {
	std::map<std::string, Function> serverDirectiveHandlers;

	serverDirectiveHandlers["address "] = add_address;
	serverDirectiveHandlers["listen "] = add_listen;
	serverDirectiveHandlers["server_name "] = add_server_name;
	serverDirectiveHandlers["root "] = add_root;
	serverDirectiveHandlers["client_max_body_size "] = parseClientMaxBodySize;
	serverDirectiveHandlers["error_page "] = parseErrorPageDirective;
	serverDirectiveHandlers["index "] = add_index;
	serverDirectiveHandlers["autoindex "] = add_autoindex;
	serverDirectiveHandlers["return "] = add_return;
	serverDirectiveHandlers["methods "] = add_methods;

	return serverDirectiveHandlers;
}

std::map<std::string, Function> get_location_directives() {
	std::map<std::string, Function> locationDirectiveHandlers;

	locationDirectiveHandlers["alias "] = add_alias;
	locationDirectiveHandlers["root "] = add_root;
	locationDirectiveHandlers["client_max_body_size "] = parseClientMaxBodySize;
	locationDirectiveHandlers["error_page "] = parseErrorPageDirective;
	locationDirectiveHandlers["index "] = add_index;
	locationDirectiveHandlers["autoindex "] = add_autoindex;
	locationDirectiveHandlers["return "] = add_return;
	locationDirectiveHandlers["methods "] = add_methods;
	locationDirectiveHandlers["cgi_extension "] = add_cgi_extension;
	locationDirectiveHandlers["auth_basic "] = add_auth_basic;
	locationDirectiveHandlers["auth_basic_user_file "] = add_auth_basic_user_file;

	return locationDirectiveHandlers;
}

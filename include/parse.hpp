#pragma once
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include "Server.hpp"

typedef void (*Function)(const std::string, Config&);

std::vector<Server>				parse( std::string const& filename );
void							tokenize(std::string& str);
void							processServerBlock( std::vector<Server> &servers, std::ifstream &file, std::string &line, std::map<std::string, Function> server_directives, std::map<std::string, Function> location_directives, bool &line_after_server );
bool							is_valid_directive( const std::string line );
bool							is_location( const std::string line );
int								processLocationBlock( std::ifstream &file, std::string &line, Server &server, std::map<std::string, Function> location_directives );
std::map<std::string, Function>	get_server_directives(); // Returns map of server directive names to handler functions
std::map<std::string, Function>	get_location_directives(); // Returns map of location directive names to handler functions

template<typename T>
int								add_directive( std::string const line, T &item, std::map<std::string, Function> const directives_map ); // Generic directive processor
void							add_address( std::string line, Config &item ); // Parse server IP address
void							add_listen( std::string line, Config &item ); // Parse listen port
void							add_server_name( std::string line, Config &item ); // Parse server hostname
void							add_root( std::string line, Config &item ); // Parse document root path
void							parseClientMaxBodySize( std::string line, Config &item ); // Parse max request size
void							parseErrorPageDirective( std::string line, Config &item ); // Parse custom error pages
void							add_index( std::string line, Config &item ); // Parse index file names
void							add_autoindex( std::string line, Config &item ); // Parse directory listing setting
void							add_return( std::string line, Config &item ); // Parse HTTP redirect
void							add_methods( std::string line, Config &item ); // Parse allowed HTTP methods
void							add_alias( std::string line, Config &item ); // Parse location alias
void							add_cgi_extension( std::string line, Config &item ); // Parse CGI file extensions
void							add_auth_basic( std::string line, Config &item ); // Parse basic auth realm
void							add_auth_basic_user_file( std::string line, Config &item ); // Parse auth user file
bool							is_valid_ipv4( std::string line ); // Validate IPv4 address format
bool							is_valid_port( std::string line ); // Validate port number range
bool							is_valid_absolute_path( std::string line ); // Validate absolute file path
int								http_code( std::string line ); // Parse HTTP status code
bool							is_valid_url_or_path(const std::string line); // Validate URL or file path format

#include "parse.tpp"

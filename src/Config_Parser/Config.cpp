#include "../../include/Config.hpp"
#include <algorithm>
#include <sstream>
Config::Config( void ) :
	_client_max_body_size(CLIENT_MAX_BODY_SIZE_DEFAULT),
	_root(ROOT_DEFAULT),
	_autoindex(AUTOINDEX_DEFAULT)
{
	_indexes.clear();
	_error_pages.clear();
	_return_data.code = -1; // -1 indicates no redirect configured
	_return_data.text = "";
	_methods.push_back("GET"); // Default to GET method only
	for (int i = 0; i < TOTAL_INDEX; i++)
		_inicializated[i] = false; // Mark all fields as using default values
}
Config::~Config( void )
{
}
std::string	Config::printCfg( void ) const 
{ 
	return this->printCfg(""); 
}
std::string	Config::printCfg( std::string indent ) const
{
	std::stringstream result;
	std::string tab = "\t" + indent;
	std::string subtab = "\t\t" + indent;
	result << tab << "- Root: \"" << _root << "\"\n"; // Root directory
	result << tab << "- Client max body: " << _client_max_body_size << "\n"; // Client max body size
	result << tab << "- Autoindex: " << (_autoindex ? "true" : "false") << "\n"; // Autoindex setting
	result << tab << "- Index:"; // Index files
	if (_indexes.empty()) {
		result << " None\n";
	} else {
		result << "\n";
		for (std::vector<std::string>::const_iterator indexIt = _indexes.begin(); 
			 indexIt != _indexes.end(); ++indexIt) {
			result << subtab << "· \"" << *indexIt << "\"\n";
		}
	}
	result << tab << "- Methods:"; // HTTP methods
	if (_methods.empty()) {
		result << " None\n";
	} else {
		result << "\n";
		for (std::vector<std::string>::const_iterator methodIt = _methods.begin(); 
			 methodIt != _methods.end(); ++methodIt) {
			result << subtab << "· " << *methodIt << "\n";
		}
	}
	result << tab << "- Error pages:"; // Error pages
	if (_error_pages.empty()) {
		result << " None\n";
	} else {
		result << "\n";
		for (std::map<int, std::string>::const_iterator errorIt = _error_pages.begin(); 
			 errorIt != _error_pages.end(); ++errorIt) {
			result << subtab << "· " << errorIt->first << ": \"" << errorIt->second << "\"\n";
		}
	}
	result << tab << "- Return data:\n" // Return/redirect configuration
		   << subtab << "· Code: " << _return_data.code << "\n"
		   << subtab << "· Text: \"" << _return_data.text << "\"\n";
	if (!_cgi_extensions.empty()) { // CGI extensions
		result << tab << "- CGI extensions:\n";
		for (std::map<std::string, std::string>::const_iterator cgiIt = _cgi_extensions.begin(); 
			 cgiIt != _cgi_extensions.end(); ++cgiIt) {
			result << subtab << "· " << cgiIt->first << ": \"" << cgiIt->second << "\"\n";
		}
	}
	if (!_auth_basic_realm.empty() || !_auth_basic_user_file.empty()) { // Authentication settings
		result << tab << "- Authentication:\n";
		if (!_auth_basic_realm.empty()) {
			result << subtab << "· Realm: \"" << _auth_basic_realm << "\"\n";
		}
		if (!_auth_basic_user_file.empty()) {
			result << subtab << "· User file: \"" << _auth_basic_user_file << "\"\n";
		}
	}
	return result.str();
}
std::ostream&	operator<<( std::ostream& os, Config const& printObject )
{
	os << printObject.printCfg();
	return (os);
};
std::map<int, std::string>	Config::get_error_pages( void ) const	{ return _error_pages; }
std::string					Config::get_error_page( int code ) const
{
	std::map<int, std::string>::const_iterator it = _error_pages.find( code );
	return (it == _error_pages.end() ? "" : it->second);
}
void						Config::add_error_page( int code, std::string path )
{
	std::map<int, std::string>::iterator it = _error_pages.find(code);
	if (it == _error_pages.end())
		_error_pages.insert(std::pair<int, std::string>(code, path));
	else
		it->second = path;
}
size_t	Config::get_client_max_size( void ) const { return _client_max_body_size; }
void	Config::set_client_max_size( size_t client_max_size )
{
	_client_max_body_size = client_max_size;
	_inicializated[CLIENT_MAX_BODY_SIZE_INDEX] = true;
}
std::string	Config::get_root( void ) const	{ return _root; }
void		Config::set_root( std::string root )
{
	_root = root;
	_inicializated[ROOT_INDEX] = true;
}
std::vector<std::string>	Config::get_indexes( void ) const { return _indexes; }
void	Config::add_index( std::string index )
{
	if (std::find(_indexes.begin(), _indexes.end(), index) == _indexes.end())
		_indexes.push_back(index);
}
bool	Config::get_autoindex( void ) const		{ return _autoindex; }
void	Config::set_autoindex( bool autoindex )
{
	_autoindex = autoindex;
	_inicializated[AUTOINDEX_INDEX] = true;
}
Config::ReturnData const&	Config::get_return( void ) const { return _return_data; }
void							Config::set_return( ReturnData data )
{
	_return_data.code = data.code;
	_return_data.text = data.text;
}
std::vector<std::string>	Config::get_methods( void ) const { return _methods; }
bool						Config::has_method( std::string method ) const
{
	return std::find(_methods.begin(), _methods.end(), method) != _methods.end();
}
void						Config::add_method( std::string method )
{
	if (!_inicializated[METHODS_INDEX])
	{
		_methods.clear(); // Clear default GET method when first custom method added
		_inicializated[METHODS_INDEX] = true;
	}
	if (has_method(method))
		return; // Don't add duplicate methods
	_methods.push_back(method);
}
std::map<std::string, std::string>	Config::get_cgi_extensions( void ) const { return _cgi_extensions; }
void								Config::add_cgi_extension( std::string extension, std::string interpreter )
{
	_cgi_extensions[extension] = interpreter;
}
std::string	Config::get_auth_basic_realm( void ) const { return _auth_basic_realm; }
void		Config::set_auth_basic_realm( std::string realm ) { _auth_basic_realm = realm; }
std::string	Config::get_auth_basic_user_file( void ) const { return _auth_basic_user_file; }
void		Config::set_auth_basic_user_file( std::string user_file ) { _auth_basic_user_file = user_file; }
void	Config::inherit( Config const& src )
{
	if (!_inicializated[CLIENT_MAX_BODY_SIZE_INDEX]) // Only inherit if not explicitly set
		_client_max_body_size = src._client_max_body_size;
	if (!_inicializated[ROOT_INDEX])
		_root = src._root;
	if (!_inicializated[AUTOINDEX_INDEX])
		_autoindex = src._autoindex;
	for (std::map<int, std::string>::const_iterator it = src._error_pages.begin(); it != src._error_pages.end(); it++)
	{
		if (_error_pages.find(it->first) == _error_pages.end()) // Don't override existing error pages
			_error_pages.insert(std::pair<int, std::string>(it->first, it->second));
	}
	for (std::vector<std::string>::const_iterator it = src._indexes.begin(); it != src._indexes.end(); it++)
	{
		if (std::find(_indexes.begin(), _indexes.end(), *it) == _indexes.end()) // Avoid duplicates
			_indexes.push_back(*it);
	}
	if (_return_data.code == -1 && _return_data.text == "") // Only inherit if no redirect set
	{
		_return_data.code = src._return_data.code;
		_return_data.text = src._return_data.text;
	}
	if (!_inicializated[METHODS_INDEX]) // Only inherit methods if not explicitly configured
	{
		_methods.clear();
		for (std::vector<std::string>::const_iterator it = src._methods.begin(); it != src._methods.end(); it++)
		{
			if (!has_method(*it))
				_methods.push_back(*it);
		}
	}
}
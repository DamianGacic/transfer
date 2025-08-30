#pragma once
#include <string>
#include <map>
#include <vector>
#include "default.hpp"
class Config
{
	public:
		struct ReturnData {
			int			code;
			std::string	text;
		};
	protected:
		enum {
			CLIENT_MAX_BODY_SIZE_INDEX = 0,
			ROOT_INDEX,
			AUTOINDEX_INDEX,
			METHODS_INDEX,
			TOTAL_INDEX
		};
		bool	_inicializated[TOTAL_INDEX]; // Track which config fields have been explicitly set
		std::map<int, std::string>	_error_pages; // HTTP error code -> custom error page path
		size_t	_client_max_body_size; // Max request body size in bytes
		std::string	_root; // Document root directory path
		std::vector<std::string>	_indexes; // Default index files to serve
		bool	_autoindex; // Enable directory listing when no index file found
		ReturnData	_return_data; // HTTP redirect configuration
		std::vector<std::string> _methods; // Allowed HTTP methods
		std::map<std::string, std::string> _cgi_extensions; // File extension -> CGI interpreter path
		std::string _auth_basic_realm; // Basic auth realm name
		std::string _auth_basic_user_file; // Path to htpasswd-style user file
		Config();
		virtual ~Config();
	public:
		std::map<int, std::string>		get_error_pages() const;
		std::string						get_error_page( int code ) const;
		void							add_error_page( int code, std::string path );
		size_t							get_client_max_size() const;
		void							set_client_max_size( size_t client_max_size );
		std::string						get_root() const;
		void							set_root( std::string root );
		std::vector<std::string>		get_indexes() const ;
		void							add_index( std::string index );
		bool							get_autoindex() const;
		void							set_autoindex( bool autoindex );
		ReturnData const&				get_return() const;
		void							set_return( ReturnData data );
		std::vector<std::string>		get_methods() const;
		bool							has_method( std::string const method ) const;
		void							add_method( std::string method );
		std::map<std::string, std::string>	get_cgi_extensions() const;
		void							add_cgi_extension( std::string extension, std::string interpreter );
		std::string						get_auth_basic_realm() const;
		void							set_auth_basic_realm( std::string realm );
		std::string						get_auth_basic_user_file() const;
		void							set_auth_basic_user_file( std::string user_file );
		std::string						printCfg() const;
		std::string						printCfg( std::string preline ) const;
		void							inherit( Config const& src );
};
std::ostream&	operator<<( std::ostream& os, Config const& printObject );

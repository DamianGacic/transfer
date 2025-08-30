#pragma once

#include <cstdio>
#include "Config.hpp"
#include "Location.hpp"
#include "default.hpp"



class Server : public Config
{
	private:
		static int						num_servers; //single attributes
		bool							_active;
		std::string						_server_name;
		std::string						_ip;
		std::vector<int>                _ports;        //maps & vector attributes
		std::map<std::string, Location>	_locations;
		std::vector<int>				_sockets;


		// Sockets


	public:

		Server();
		~Server();
		int									is_active() const;
		std::string							get_server_name() const;
		void								set_server_name( std::string server_name );
		std::string							get_ip() const;
		void                                set_ip( std::string ip );

		std::vector<int>                    get_ports() const;
		void                                add_port( int port );
		bool                                has_port( int port );
		std::map<std::string, Location>     get_locations() const;
		std::pair<bool, Location const*>    get_location( std::string route ) const;
		void                                add_location( std::string route, Location location );
		std::vector<int>                    get_sockets() const;
		bool                                has_socket( int sock ) const; //has 1 socket at minimum
		std::string							printSrv() const; //maybe superfluous
		void								run();
		void								shutdown();


};

// non member opp overloading
std::ostream&    operator<<( std::ostream& os, Server const& printObject );

		class RuntimeException : std::exception
		{
			private:
				std::string _message;
			public:
				RuntimeException( std::string const str ) throw();
				RuntimeException( std::string const str, std::string const error ) throw();
				virtual ~RuntimeException() throw();
				virtual const char* what() const throw();
		};
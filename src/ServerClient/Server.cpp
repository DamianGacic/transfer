#include "../../include/Server.hpp"
#include <sstream>
#include <algorithm>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int Server::num_servers = 0;

Server::Server( void ) : Config(),
	_active(false),
	_ip(IP_DEFAULT)
{
	//set server name
	std::stringstream	ss;
	ss << ++Server::num_servers;
	_server_name = "Server " + ss.str();

	_ports.push_back(-1);
	_sockets.clear();
	_locations.clear();
}

// operator overloading
Server::~Server( void )
{
	_active = false;
	_locations.clear();
}

// getters setters

int	Server::is_active() const { return _active; }

//IPs
std::string	Server::get_ip() const { return _ip; }
void		Server::set_ip( std::string ip ) { _ip = ip; }

//Ports
std::vector<int>	Server::get_ports() const { return _ports; }
void	Server::add_port( int port )
{
	if (_ports.size() == 1 && _ports.at(0) == -1)
		_ports.clear();
	if (!has_port(port))
		_ports.push_back(port);
}
bool	Server::has_port( int port )
{
	return std::find(_ports.begin(), _ports.end(), port) != _ports.end();
}

//Server Names
std::string	Server::get_server_name() const { return _server_name; }
void		Server::set_server_name( std::string server_name ) { _server_name = server_name; }

// Sockets
std::vector<int>	Server::get_sockets() const { return _sockets; }
bool				Server::has_socket( int sock ) const
{
	return std::find(_sockets.begin(), _sockets.end(), sock) != _sockets.end();
}

// Locations
std::map<std::string, Location>	Server::get_locations() const { return _locations; }
std::pair<bool, Location const*>	Server::get_location( std::string route ) const
{
	std::map<std::string, Location>::const_iterator it;
	int index;

	// check if route exists in locations
	std::string last;
	while (1)
	{
		it = _locations.find(route);
		if (it != _locations.end())
			return std::pair<bool, Location const*>(true, &(it->second));

		index = route.rfind("/");
		last = route.substr(0, index);
		if (last.empty())
			last = "/";
		if (last == route)
			break ;
		route = last;
	};

	//location not found
	return std::pair<bool, Location const*>(false, NULL);
}

void	Server::add_location( std::string route, Location location )
{
	if (_locations.find( route ) == _locations.end())
	{
		location.inherit(*this);
		_locations.insert(std::pair<std::string, Location>(route, location));
	}
}

// class functions

void	Server::run()
{
	if (_active) 	// Check if the server is already running
		return ;
	_active = true;
	for (std::vector<int>::iterator it = _ports.begin(); it != _ports.end(); it++) 	// Create all the ports connections
	{
		int port = *it;
		if (port < 0 || port > 65535)
			throw RuntimeException("Invalid port");
		int socket_fd = socket(AF_INET, SOCK_STREAM, 0); 	// open socket
		if (socket_fd == -1)
			throw RuntimeException("Error while initializing socket", strerror(errno));
		int flags = fcntl(socket_fd, F_GETFL, 0); 		// set nonblocking
		if (flags == -1 || fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			close(socket_fd);
			throw RuntimeException("Error setting socket to non-blocking");
		}
		int optval = 1;
		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) 	//Configure Socket
		{
			close(socket_fd);
			throw RuntimeException("Error setting socket configuration");
		}
		struct sockaddr_in addr; 	//Bind socket to IP and Port
		bzero(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		if (inet_pton(AF_INET, _ip.c_str(), &addr.sin_addr) <= 0)
		{
			close(socket_fd);
			throw RuntimeException("Invalid server IP");
		}
		if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) //Bind Socket to port
		{
			close(socket_fd);
			throw RuntimeException("Error binding the socket");
		}
		if (listen(socket_fd, SERVERS_BACKLOG) == -1)	// start listening
		{
			close(socket_fd);
			throw RuntimeException("Error while starting the listen");
		}
		_sockets.push_back(socket_fd); // add to active (listening) sockets to vector
	}
}

void	Server::shutdown()
{
	if (!_active)
		return;
	_active = false;
	for (std::vector<int>::iterator it = _sockets.begin(); it != _sockets.end(); it++) 
		close(*it);
}
RuntimeException::RuntimeException( std::string const str ) throw():
	_message(str) {}
RuntimeException::RuntimeException( std::string const str, std::string const error ) throw():
	_message(str + ": " + error) {}
RuntimeException::~RuntimeException() throw() {}

const char* RuntimeException::what() const throw()
{ return _message.c_str(); }

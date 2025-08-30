#include "../../include/Server.hpp"
#include "../../include/signals.hpp"
#include "../../include/parse.hpp"
#include "../../include/polling.hpp"

int	help(char *cmd)
{
	std::cout <<
		"how to use: " << cmd << " [config_file.conf]" << std::endl <<
		"If no config file is provided, config/default.conf will be used."
	<< std::endl;
	return (1);
}


int main(int argc, char *argv[])
{
	std::vector<Server>	servers;
	const char* config_file;

 
	if (argc > 2) // 1. Check parameters
		return help(argv[0]);
	
	// Use provided config file or default to config/default.conf
	if (argc == 2)
		config_file = argv[1];
	else
		config_file = "config/default.conf";
		
	set_sigint(); // 2. Assign SIGINT
	
	// Display hardcoded authentication credentials
	std::cout << "\n========================================" << std::endl;
	std::cout << "        WEBSERV AUTHENTICATION         " << std::endl;
	std::cout << "========================================" << std::endl;
	std::cout << "For uploading files, use these credentials:" << std::endl;
	std::cout << "Username: webserv  | Password: upload123" << std::endl;
	std::cout << "Username: admin    | Password: admin456" << std::endl;
	std::cout << "Username: user     | Password: test789" << std::endl;
	std::cout << "========================================\n" << std::endl;
	
	// (3. Load our custom data, eg error files etc)
	servers = parse(config_file); // 4. Get the servers-vector
	if (servers.empty())
		return (1);
	{ // 5. run all valid servers
		try
		{
			for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); it++)
				it->run();
		}
		catch (RuntimeException& e)
		{
			std::cout << "Error while running a server: " << e.what() << std::endl;

			// close all running servers
			for (std::vector<Server>::iterator it = servers.begin(); it != servers.end() && (*it).is_active(); it++)
				(*it).shutdown();
			return (1);
		}
    }
	while (sigint_pressed == 0)
	{
	polling(servers); // 6. Loop (until SIGINT) and poll all configured servers
	}
	for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); it++) // 7. Stop the servers and free the ports
	it->shutdown();
	return (0);
}

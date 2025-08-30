#pragma once

#include "Server.hpp"
#include "Client.hpp"

void	polling(std::vector<Server> servers);
void	handle_http_request(Client& client);
void	handle_cgi_request(Client& client, const std::string& script_path, const Location& location, const Server& server);
bool	handle_client_data(int client_fd, Client& client, int epoll_fd);
void	process_epoll_events(int epoll_fd, struct epoll_event* events, int active_fds, std::map<int, Server>& servers, std::map<int, Client>& clients);
void	close_clients(std::map<int, Client>& clients);
void	check_client_timeouts(std::map<int, Client>& clients, int epoll_fd);

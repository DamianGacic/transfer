#pragma once

#include "Server.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include <ctime>

class Client
{
private:
	static int			client_count;
	const int			id;
	const int			socket_fd;
	const Server&		server;
	Request				request;
	Response			response;
	std::string			buffer;
	bool				request_ready;
	bool				response_sent;
	time_t				last_activity;
	int					send_retry_count;

public:
	Client(int fd, Server& server);
	~Client();

	int					getFd() const;
	const Server&		getServer() const;
	Request&			getRequest();
	Response&			getResponse();
	bool				isRequestReady() const;
	bool				isResponseSent() const;
	void				setRequestReady(bool ready);
	void				setResponseSent(bool sent);
	void				appendData(const std::string& data);
	void				reset();
	std::string			toString() const;
	void				updateLastActivity();
	time_t				getLastActivity() const;
	bool				isTimedOut(int timeout_seconds) const;
	void				incrementSendRetries();
	int					getSendRetries() const;
	void				resetSendRetries();
};

std::ostream& operator<<(std::ostream& os, const Client& client);
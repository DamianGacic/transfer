#include "../../include/Client.hpp"
#include <sstream>

int Client::client_count = 1;

Client::Client(int fd, Server& server)
	: id(client_count++), socket_fd(fd), server(server), 
	  request_ready(false), response_sent(false), send_retry_count(0)
{
	last_activity = time(NULL);
}

Client::~Client()
{
}

std::string Client::toString() const
{
	std::ostringstream oss;
	oss << "[ CLIENT ] ID: " << id << "\n"
		<< "\t- Socket FD: " << socket_fd << "\n"
		<< "\t- Server: " << server.get_server_name() << "\n"
		<< "\t- Request Ready: " << (request_ready ? "yes" : "no") << "\n"
		<< "\t- Response Sent: " << (response_sent ? "yes" : "no") << "\n";
	return oss.str();
}

std::ostream& operator<<(std::ostream& os, const Client& client)
{
	return os << client.toString();
}

int Client::getFd() const 
{ 
	return socket_fd; 
}

const Server& Client::getServer() const 
{ 
	return server; 
}

Request& Client::getRequest() 
{ 
	return request; 
}

Response& Client::getResponse() 
{ 
	return response; 
}

bool Client::isRequestReady() const 
{ 
	return request_ready; 
}

bool Client::isResponseSent() const 
{ 
	return response_sent; 
}

void Client::setRequestReady(bool ready) 
{ 
	request_ready = ready; 
}

void Client::setResponseSent(bool sent) 
{ 
	response_sent = sent; 
}

void Client::appendData(const std::string& data)
{
	buffer += data;
	request.appendData(data);
	request_ready = request.isComplete();
	updateLastActivity();
}

void Client::reset()
{
	request.reset();
	response.reset();
	buffer.clear();
	request_ready = false;
	response_sent = false;
	send_retry_count = 0;
	last_activity = time(NULL);
}

void Client::updateLastActivity()
{
	last_activity = time(NULL);
}

time_t Client::getLastActivity() const
{
	return last_activity;
}

bool Client::isTimedOut(int timeout_seconds) const
{
	return (time(NULL) - last_activity) > timeout_seconds;
}

void Client::incrementSendRetries()
{
	send_retry_count++;
}

int Client::getSendRetries() const
{
	return send_retry_count;
}

void Client::resetSendRetries()
{
	send_retry_count = 0;
}
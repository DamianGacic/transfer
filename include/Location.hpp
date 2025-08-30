#pragma once

#include <iostream>
#include "Config.hpp"
#include "default.hpp"

class Location : public Config
{
private:
	std::string		route;
	std::string		alias;

public:
	Location();
	Location(const std::string& route);
	~Location();

	const std::string&	getRoute() const;
	void				setRoute(const std::string& route);
	const std::string&	getAlias() const;
	void				setAlias(const std::string& alias);
	std::string			toString() const;
	void				inherit(const Config& src);
};

std::ostream& operator<<(std::ostream& os, const Location& location);
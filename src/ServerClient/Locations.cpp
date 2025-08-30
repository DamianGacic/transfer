#include "../../include/Location.hpp"

Location::Location() : Config(), route(""), alias(ALIAS_DEFAULT)
{
}

Location::Location(const std::string& route) : Config(), route(route), alias(ALIAS_DEFAULT)
{
}

Location::~Location()
{
}

std::string Location::toString() const
{
	std::string result = "\t[ LOCATION ] " + route + "\n";
	result += "\t\t· Alias: \"" + alias + "\"\n";
	result += static_cast<const Config&>(*this).printCfg("\t");
	return result;
}

std::ostream& operator<<(std::ostream& os, const Location& location)
{
	return os << location.toString();
}

const std::string& Location::getRoute() const 
{ 
	return route; 
}

void Location::setRoute(const std::string& route) 
{ 
	this->route = route; 
}

const std::string& Location::getAlias() const 
{ 
	return alias; 
}

void Location::setAlias(const std::string& alias) 
{ 
	this->alias = alias; 
}

void Location::inherit(const Config& src)
{
	std::string path = (_inicializated[ROOT_INDEX] ? _root : src.get_root());
	path += (alias.empty() ? route : alias);
	
	static_cast<Config*>(this)->inherit(src);
}
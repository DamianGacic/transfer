template<typename T>
int add_directive( std::string const line, T &item, std::map<std::string, Function> const directives_map )
{
	std::string key = line.substr( 0, line.find( ' ' ) + 1); // Substr form start to first space on line
	std::map<std::string, Function>::const_iterator it = directives_map.find(key); // Find directive on directives map
	if (it == directives_map.end())
	{
		tokenize(key);
		throw std::invalid_argument("Error while parsing configuration file. Invalid directive name [" + key + "]." );
	}
	std::string values = line.substr( key.size(), line.size() - key.size() - 1 ); // Create nwe string without key and normalize it
	tokenize(values);
	it->second(values , item); // Execute function associate with the key found
	return 1;
};

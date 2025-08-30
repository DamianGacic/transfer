#include "../../include/parse.hpp"

bool	is_location( const std::string directive_line ) {

	// Verify that it starts with the word location
	if ( directive_line.substr(0,9) != "location " ) return false;

	// Verify that there is a word in between and that the line ends with {
	size_t last_space = directive_line.rfind(' ');
	if ( last_space == std::string::npos || last_space == 8 || directive_line.at( directive_line.size() - 1 ) != '{' ) return false;

	// Verify that there is a valid name for the location
	std::string location_path = directive_line.substr(9, last_space - 9);
	// Path must be "/" or start with "/" and not end with "/" (except root)
	return !location_path.empty() && (!location_path.compare("/") || (location_path.at(0) == '/' && location_path.at( location_path.size() - 1 ) != '/'));
}

static bool check_duplicate_location( Server target_server, std::string location_path ) {

	// Get server locations
	std::map<std::string, Location>	existing_locations = target_server.get_locations();

	// Verify there is no duplicate locations
	std::map<std::string, Location>::const_iterator location_it = existing_locations.find( location_path );
	return !(location_it == existing_locations.end()); // Returns true if location already exists
}

int	processLocationBlock( std::ifstream &input_stream, std::string &current_line, Server &target_server, std::map<std::string, Function> location_handlers ) {
	

	// Get route of Location
	std::string location_path = current_line.substr( 9, current_line.rfind(' ') - 9 );

	// Verify duplicate Locations
	if ( check_duplicate_location( target_server, location_path ) )
		return 0; // Reject duplicate location

	// Create new Location
	Location new_location( location_path );

	// Read lines inside a location block
	while (std::getline( input_stream, current_line )) {

		// Normalize string
		tokenize( current_line );

		// Case end of Location
		if ( current_line.compare("}") == 0 ) {

			target_server.add_location( location_path, new_location );
			break;
		}
		// Case empty line or comment
		else if ( current_line.empty() || current_line.at(0) == '#' ) continue ;
		// Case valid directive on Location
		else if ( is_valid_directive( current_line ) && add_directive( current_line, new_location, location_handlers ) ) continue;
		// Case error
		else return 0; // Invalid syntax in location block
	}

	return 1; // Successfully parsed location

}
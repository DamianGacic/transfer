#include "../../include/parse.hpp"

bool	is_valid_directive( const std::string directive_line ) {
	/* Verify line finish with ";" */
	if ( directive_line.empty() || directive_line.at( directive_line.size() - 1 ) != ';' ) return false;
	
	/* Verify only one ";" and quotes are closed OK */
	int quote_count = 0;
	int semicolon_count = 0;

	for (size_t char_idx = 0; char_idx < directive_line.size(); ++char_idx) {
		if ( directive_line[char_idx] == '"' ) ++quote_count;
		// Only count semicolons outside of quoted strings
		if ( directive_line[char_idx] == ';' && quote_count % 2 == 0 ) ++semicolon_count;
	}

	return quote_count % 2 == 0 && semicolon_count == 1;
}

void	processServerBlock( std::vector<Server> &server_list, std::ifstream &input_stream, std::string &current_line, std::map<std::string, Function> server_handlers, std::map<std::string, Function> location_handlers, bool &expect_server ) {
	
	Server new_server;
	expect_server = false; // We're now inside a server block, not expecting new servers

	while (1) {

		std::getline(input_stream, current_line);
		tokenize(current_line);

		/* Case end of a Server */
		if ( current_line.compare("}") == 0 ) {

			server_list.push_back(new_server);

			// Read next line to check what comes after server block
			if ( std::getline(input_stream, current_line) ) {
				tokenize( current_line );
				expect_server = true; // Now we expect another server block or EOF
			}

			return ;
		}

		/* Case EOF without reaching end bracket */
		if ( input_stream.eof() )
			throw std::invalid_argument("Error while parsing configuration file. Check brackets are properly closed");
		
		/* Case empty line or comment in a Server */
		if ( current_line.empty() || current_line.at(0) == '#' ) continue ;

		/* Case Server directive found */
		if ( is_valid_directive( current_line ) && add_directive( current_line, new_server, server_handlers )) continue ;
		/* Case Location in a Server found */
		else if ( is_location( current_line ) && processLocationBlock( input_stream, current_line, new_server, location_handlers ) ) continue ;
		/* Case Syntax error found */
		else
			throw std::invalid_argument("Error while parsing configuration file. Syntax error.");
	}
}

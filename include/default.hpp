#pragma once

# define SERVERS_BACKLOG                10 // Max. number of  simultaneous connections per server
# define CLIENT_MAX_BODY_SIZE_DEFAULT	100 // Default HTTP request max size -> 100 bytes/characters
# define ALIAS_DEFAULT					""
# define ROOT_DEFAULT					"." // Default folder where the pages will be searched
# define AUTOINDEX_DEFAULT				false
# define IP_DEFAULT						"127.0.0.1"
# define MAX_BODY_SIZE_BYTES			52428800
# define SERVER_PROTOCOL				"HTTP/1.1"
# define POLL_TIMEOUT                   1000 // 1 second
# define CLIENT_TIMEOUT                 30   // 30 seconds

#ifndef LOG
# define LOG false
#endif

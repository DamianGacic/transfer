NAME = webserv

CXX := c++
CXXFLAGS := -g -Wall -Wextra -Werror -std=c++98 -g3 -fdiagnostics-color=always -DLOG=true
OBJ_FOLDER = obj

HEADERS = include/default.hpp include/Config.hpp include/Location.hpp include/Server.hpp include/Client.hpp include/webserv.hpp include/Request.hpp include/Response.hpp include/CGI.hpp include/Utils.hpp

SRC = \
    src/Main/main.cpp \
    src/Main/signals.cpp \
    src/Polling/polling.cpp \
    src/Polling/polling_utils.cpp \
    src/Config_Parser/parse.cpp \
    src/Config_Parser/Config.cpp \
    src/Parse_Utils/parseLocations.cpp \
    src/Parse_Utils/parseServers.cpp \
    src/Directives/directives.cpp \
    src/Directives/getDirectives.cpp \
    src/Directives/dirCheck.cpp \
    src/ServerClient/Locations.cpp \
    src/ServerClient/Server.cpp \
    src/ServerClient/Client.cpp \
    src/HTTP/Request.cpp \
    src/HTTP/Response.cpp \
    src/HTTP/CGI.cpp \
    src/HTTP/Utils.cpp


OBJ = $(patsubst %.cpp,$(OBJ_FOLDER)/%.o,$(SRC)) #create obj folder based on src

all: $(NAME)

test_response: src/HTTP/Response_test.cpp src/HTTP/Response.cpp src/HTTP/Utils.cpp
	@$(CXX) $(CXXFLAGS) $^ -o $@
	@./test_response
	@rm -f test_response

$(NAME): $(OBJ)
	@$(CXX) $(CXXFLAGS) $^ -o $@ # Added '@' to suppress command echo

$(OBJ_FOLDER)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	@echo "Compiling $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJ_FOLDER)

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
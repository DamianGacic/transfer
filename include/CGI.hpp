#ifndef CGI_HPP
#define CGI_HPP

#include "webserv.hpp"
#include "Request.hpp"
#include "Response.hpp"

class CGI
{
    private:
        std::string						script_path;
        std::string						script_name;
        std::string						query_string;
        std::string						request_method;
        std::string						content_type;
        std::string						content_length;
        std::string						server_protocol;
        std::string						server_software;
        std::string						server_name;
        std::string						server_port;
        std::string						remote_addr;
        std::string						remote_host;
        std::string						http_user_agent;
        std::string						http_accept;
        std::string						http_accept_language;
        std::string						http_accept_encoding;
        std::string						http_connection;
        std::string						http_host;
        std::string						path_info;
        std::string						path_translated;
        std::string						request_uri;
        std::string						document_root;
        std::string						gateway_interface;
        std::string						interpreter_path;
        std::string						auth_type;
        std::string						remote_user;
        std::string						remote_ident;
        std::map<std::string, std::string>	env_vars;
        std::string						request_body;
        std::string						cgi_headers;
        std::string						cgi_body;
        static const int				CGI_TIMEOUT = 30; // 30 seconds timeout

        void							setupEnvironment();
        char**							createEnvArray();
        void							freeEnvArray(char** env);
        std::string						executeScript();
        bool							isValidScript(const std::string& path);
        bool							parseCGIOutput(const std::string& output);
        void							setupStandardEnvironment();
        bool							waitForChildWithTimeout(pid_t pid, int timeout);

    public:
        CGI();
        ~CGI();
        void							setScriptPath(const std::string& path);
        void							setRequest(const Request& request);
        void							setDocumentRoot(const std::string& root);
        void							setServerInfo(const std::string& name, const std::string& port);
        void							setInterpreter(const std::string& interpreter);
        std::string						execute();
        std::string						getHeaders() const;
        std::string						getBody() const;
        // Static methods
        static std::string				getScriptExtension(const std::string& filename);
        static bool						isCGIScript(const std::string& filename, const std::map<std::string, std::string>& cgi_extensions);
};

#endif 
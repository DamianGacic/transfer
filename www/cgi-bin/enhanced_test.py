#!/usr/bin/env python3

import os
import sys
import urllib.parse

# CGI script to test the enhanced implementation
def main():
    # Get request method
    method = os.environ.get('REQUEST_METHOD', 'GET')
    
    # Output HTTP headers
    print("Content-Type: text/html")
    print("Cache-Control: no-cache")
    print()  # Empty line to separate headers from body
    
    # HTML output
    print("<!DOCTYPE html>")
    print("<html>")
    print("<head>")
    print("<title>Enhanced CGI Test</title>")
    print("<style>")
    print("body { font-family: Arial, sans-serif; margin: 20px; }")
    print("h1 { color: #333; }")
    print(".section { background: #f5f5f5; padding: 15px; margin: 10px 0; border-radius: 5px; }")
    print(".env-var { background: #fff; padding: 5px; margin: 2px 0; border-left: 3px solid #007cba; }")
    print("</style>")
    print("</head>")
    print("<body>")
    print("<h1>Enhanced CGI Test - Python Script</h1>")
    print(f"<p><strong>Request Method:</strong> {method}</p>")
    
    # Environment variables section
    print("<div class='section'>")
    print("<h2>CGI Environment Variables:</h2>")
    
    cgi_vars = [
        "GATEWAY_INTERFACE", "SERVER_SOFTWARE", "SERVER_NAME", "SERVER_PORT",
        "SERVER_PROTOCOL", "REQUEST_METHOD", "SCRIPT_NAME", "QUERY_STRING",
        "CONTENT_LENGTH", "CONTENT_TYPE", "REMOTE_ADDR", "REMOTE_HOST",
        "PATH_INFO", "PATH_TRANSLATED", "REQUEST_URI", "DOCUMENT_ROOT"
    ]
    
    for var in cgi_vars:
        value = os.environ.get(var, "Not set")
        print(f'<div class="env-var"><strong>{var}:</strong> {value}</div>')
    
    print("</div>")
    
    # HTTP headers section
    print("<div class='section'>")
    print("<h2>HTTP Headers:</h2>")
    
    http_vars = []
    for key, value in os.environ.items():
        if key.startswith('HTTP_'):
            http_vars.append((key, value))
    
    if http_vars:
        for key, value in sorted(http_vars):
            print(f'<div class="env-var"><strong>{key}:</strong> {value}</div>')
    else:
        print("<p>No HTTP headers found.</p>")
    
    print("</div>")
    
    # Query string section
    print("<div class='section'>")
    print("<h2>Query String Parameters:</h2>")
    
    query_string = os.environ.get("QUERY_STRING", "")
    if query_string:
        try:
            params = urllib.parse.parse_qs(query_string)
            for key, values in params.items():
                for value in values:
                    print(f'<div class="env-var"><strong>{key}:</strong> {value}</div>')
        except Exception as e:
            print(f"<p>Error parsing query string: {e}</p>")
    else:
        print("<p>No query string parameters.</p>")
    
    print("</div>")
    
    # POST data section (if applicable)
    if method == 'POST':
        print("<div class='section'>")
        print("<h2>POST Data:</h2>")
        
        try:
            content_length = int(os.environ.get('CONTENT_LENGTH', 0))
            if content_length > 0:
                post_data = sys.stdin.read(content_length)
                print(f'<div class="env-var"><strong>Raw POST Data:</strong> {post_data}</div>')
                
                # Try to parse as form data
                try:
                    if os.environ.get('CONTENT_TYPE', '').startswith('application/x-www-form-urlencoded'):
                        form_data = urllib.parse.parse_qs(post_data)
                        print("<h3>Parsed Form Data:</h3>")
                        for key, values in form_data.items():
                            for value in values:
                                print(f'<div class="env-var"><strong>{key}:</strong> {value}</div>')
                except Exception as e:
                    print(f"<p>Error parsing form data: {e}</p>")
            else:
                print("<p>No POST data received.</p>")
        except Exception as e:
            print(f"<p>Error reading POST data: {e}</p>")
        
        print("</div>")
    
    # Navigation
    print("<div class='section'>")
    print("<p><a href='/'>Back to Home</a> | <a href='/cgi-bin/test.py'>Original Test</a></p>")
    print("</div>")
    
    print("</body>")
    print("</html>")

if __name__ == "__main__":
    main()

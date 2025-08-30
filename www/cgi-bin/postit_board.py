#!/usr/bin/env python3
import cgi
import cgitb
import os
import json
import datetime
from urllib.parse import parse_qs

cgitb.enable()

# File to store the notes
NOTES_FILE = "../postit_notes.json"  # Path relative to cgi-bin directory

def load_notes():
    if os.path.exists(NOTES_FILE):
        try:
            with open(NOTES_FILE, 'r') as f:
                return json.load(f)
        except (json.JSONDecodeError, IOError):
            return []
    return []

def save_note(content):
    notes = load_notes()
    note = {
        'id': len(notes) + 1,
        'content': content,
        'timestamp': datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    }
    notes.append(note)
    try:
        with open(NOTES_FILE, 'w') as f:
            json.dump(notes, f, indent=2)  # Added indentation for better readability
        os.chmod(NOTES_FILE, 0o666)  # Make file readable and writable by everyone
    except IOError as e:
        print("Content-type: text/html\n")
        print(f"Error saving note: {str(e)}")
        return None
    return note

def generate_html(notes, message=""):
    print("Content-type: text/html\n")
    
    print("""<!DOCTYPE html>
<html>
<head>
    <title>Post-it Board</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            background-color: transparent;
            position: relative;
            color: white;
            min-height: 100vh;
        }
        .video-background {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            z-index: -1;
            object-fit: cover;
        }
        .content-wrapper {
            position: relative;
            z-index: 1;
            padding: 20px;
            background-color: rgba(0, 0, 0, 0.5);
            min-height: 100vh;
        }
        .nav-buttons {
            position: fixed;
            top: 20px;
            right: 20px;
            display: flex;
            gap: 10px;
            z-index: 1000;
        }
        .nav-button {
            padding: 8px 16px;
            background-color: #2196F3;
            color: white;
            text-decoration: none;
            border-radius: 4px;
            transition: background-color 0.3s;
        }
        .nav-button:hover {
            background-color: #1976D2;
        }
        .board {
            display: flex;
            flex-wrap: wrap;
            gap: 20px;
            margin-top: 20px;
        }
        .note {
            background-color: rgba(255, 215, 0, 0.9);
            padding: 15px;
            width: 200px;
            min-height: 150px;
            box-shadow: 3px 3px 15px rgba(0,0,0,0.4);
            transform: rotate(-1deg);
            transition: transform 0.2s;
            backdrop-filter: blur(3px);
        }
        .note:nth-child(even) {
            transform: rotate(1deg);
            background-color: rgba(255, 255, 153, 0.9);
        }
        .note:hover {
            transform: scale(1.05) rotate(0);
            z-index: 1;
        }
        .timestamp {
            font-size: 0.8em;
            color: #666;
            margin-top: 10px;
        }
        .form-container {
            background-color: rgba(255, 255, 255, 0.9);
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.3);
            margin-bottom: 20px;
            backdrop-filter: blur(5px);
        }
        textarea {
            width: 100%;
            padding: 10px;
            margin: 10px 0;
            border: 1px solid #ddd;
            border-radius: 4px;
            resize: vertical;
        }
        button {
            background-color: #4CAF50;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
        }
        button:hover {
            background-color: #45a049;
        }
        .message {
            padding: 10px;
            margin: 10px 0;
            border-radius: 4px;
            background-color: #e8f5e9;
            color: #2e7d32;
        }
    </style>
</head>
<body>
    <!-- Video Background -->
    <video class="video-background" autoplay muted loop playsinline>
        <source src="/thegif.mp4" type="video/mp4">
        Your browser does not support the video tag.
    </video>

    <div class="content-wrapper">
        <div class="nav-buttons">
            <a href="/" class="nav-button">Home</a>
            <a href="/cgi-bin/" class="nav-button">CGI Directory</a>
        </div>
        
        <h1>Post-it Board</h1>
    
    <div class="form-container">
        <form method="POST" action="/cgi-bin/postit_board.py">
            <textarea name="content" rows="4" placeholder="Write your note here..." required></textarea>
            <br>
            <button type="submit">Add Note</button>
        </form>
    </div>
""")

    if message:
        print(f'<div class="message">{message}</div>')

    print('<div class="board">')
    for note in reversed(notes):  # Show newest notes first
        # Wrap content to fit note boundaries
        wrapped_content = '<br>'.join([note['content'][i:i+40] for i in range(0, len(note['content']), 40)])
        print(f"""
        <div class="note">
            <div>{wrapped_content}</div>
            <div class="timestamp">{note['timestamp']}</div>
        </div>
        """)
    print('</div></div></body></html>')

def main():
    form = cgi.FieldStorage()
    notes = load_notes()
    message = ""

    # Handle POST request (new note)
    if os.environ.get('REQUEST_METHOD') == 'POST':
        content = form.getvalue('content')
        if content:
            save_note(content)
            message = "Note added successfully!"
            notes = load_notes()  # Reload notes after adding new one

    generate_html(notes, message)

if __name__ == "__main__":
    main()

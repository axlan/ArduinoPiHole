#!/usr/bin/env python3

from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import parse_qs

form = '''<!DOCTYPE html>
  <title>Message Board</title>
  <form method="POST" action="http://localhost:8000/">
    <textarea name="message"></textarea>
    <br>
    <button type="submit">Post it!</button>
  </form>'''

class MessageHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html; charset=utf-8')
        self.end_headers()
        self.wfile.write(form.encode())

if __name__ == '__main__':
    server_address = ('192.168.1.103', 8000)
    httpd = HTTPServer(server_address, MessageHandler)
    httpd.serve_forever()

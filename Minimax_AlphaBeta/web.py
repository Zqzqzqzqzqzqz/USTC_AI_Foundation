#!/usr/bin/env python3
"""
Connect Four Web Server
用法：python3 server.py [port]
默认端口 8080
"""

import http.server
import json
import os
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BIN        = os.path.join(SCRIPT_DIR, 'bin', 'web')
FRONTEND   = os.path.join(SCRIPT_DIR, '前端', 'index.html')
PORT       = int(sys.argv[1]) if len(sys.argv) > 1 else 8080


class GameHandler(http.server.BaseHTTPRequestHandler):

    def log_message(self, fmt, *args):
        print(f"[{self.address_string()}] {fmt % args}")

    # ── helpers ──────────────────────────────────────────────────────────────

    def send_json(self, data, status=200):
        body = json.dumps(data).encode()
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(body)))
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(body)

    def send_file(self, path, content_type):
        with open(path, 'rb') as f:
            body = f.read()
        self.send_response(200)
        self.send_header('Content-Type', content_type)
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    # ── GET ──────────────────────────────────────────────────────────────────

    def do_GET(self):
        if self.path in ('/', '/index.html'):
            self.send_file(FRONTEND, 'text/html; charset=utf-8')
        else:
            self.send_response(404)
            self.end_headers()

    # ── POST ─────────────────────────────────────────────────────────────────

    def do_POST(self):
        if self.path != '/api/ai_move':
            self.send_response(404)
            self.end_headers()
            return

        length  = int(self.headers.get('Content-Length', 0))
        payload = json.loads(self.rfile.read(length))
        player  = payload['player']          # 'X' or 'O'
        grid    = payload['grid']            # list of 6 strings (7 chars each)

        stdin_str = player + '\n' + '\n'.join(grid) + '\n'
        try:
            proc = subprocess.run(
                [BIN],
                input=stdin_str,
                capture_output=True,
                text=True,
                timeout=15,
            )
            if proc.returncode != 0:
                raise RuntimeError(proc.stderr.strip())
            col = int(proc.stdout.strip())
            self.send_json({'col': col})
        except Exception as exc:
            print(f'[ERROR] {exc}', file=sys.stderr)
            self.send_json({'error': str(exc)}, 500)


if __name__ == '__main__':
    for path, label in [(BIN, 'AI binary'), (FRONTEND, 'frontend')]:
        if not os.path.exists(path):
            print(f'Error: {label} not found at {path}', file=sys.stderr)
            print('Run:  bash run_advanced.sh serve', file=sys.stderr)
            sys.exit(1)

    server = http.server.HTTPServer(('localhost', PORT), GameHandler)
    print(f'Connect Four server  →  http://localhost:{PORT}')
    print('Press Ctrl+C to stop.')
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print('\nServer stopped.')

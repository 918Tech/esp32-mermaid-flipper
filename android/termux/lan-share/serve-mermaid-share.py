#!/data/data/com.termux/files/usr/bin/python
from __future__ import annotations

import hashlib
import json
import os
import socket
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

PORT = int(os.environ.get("MERMAID_SHARE_PORT", "9180"))
PACKAGE = Path.home() / "storage" / "downloads" / "mermaid-termux-complete.tar.gz"
ROOT = Path(__file__).resolve().parent
HTML = ROOT / "mermaid-share.html"
CHUNK = 1024 * 1024

def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(CHUNK), b""):
            h.update(chunk)
    return h.hexdigest()

def human_size(n: int) -> str:
    size = float(n)
    for unit in ("B","KB","MB","GB"):
        if size < 1024 or unit == "GB":
            return f"{size:.1f} {unit}"
        size /= 1024
    return f"{n} B"

def lan_ip() -> str:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        return s.getsockname()[0]
    except OSError:
        return "PHONE_IP"
    finally:
        s.close()

class Handler(BaseHTTPRequestHandler):
    server_version = "MermaidLANShare/1.0"

    def log_message(self, fmt, *args):
        print("[http]", fmt % args)

    def send_json(self, obj, code=200):
        body = json.dumps(obj, separators=(",", ":")).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        path = urlparse(self.path).path
        if path in ("/", "/index.html", "/mermaid-share.html"):
            if not HTML.is_file():
                self.send_error(404, "HTML missing")
                return
            body = HTML.read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return

        if path == "/api/info":
            ready = PACKAGE.is_file()
            info = {
                "ready": ready,
                "name": PACKAGE.name,
                "size": PACKAGE.stat().st_size if ready else 0,
                "size_human": human_size(PACKAGE.stat().st_size) if ready else "0 B",
                "sha256": sha256_file(PACKAGE) if ready else "",
            }
            self.send_json(info)
            return

        if path == "/download/mermaid-termux-complete.tar.gz":
            if not PACKAGE.is_file():
                self.send_error(404, f"Package missing: {PACKAGE}")
                return
            size = PACKAGE.stat().st_size
            self.send_response(200)
            self.send_header("Content-Type", "application/gzip")
            self.send_header("Content-Disposition", f'attachment; filename="{PACKAGE.name}"')
            self.send_header("Content-Length", str(size))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            with PACKAGE.open("rb") as f:
                while True:
                    chunk = f.read(CHUNK)
                    if not chunk:
                        break
                    self.wfile.write(chunk)
            return

        self.send_error(404)

def main():
    if not HTML.is_file():
        raise SystemExit(f"Missing HTML: {HTML}")
    ip = lan_ip()
    print()
    print("918 MERMAID TERMUX LAN SHARE")
    print("--------------------------------")
    print(f"Package : {PACKAGE}")
    print(f"LAN URL : http://{ip}:{PORT}/")
    print(f"Local   : http://127.0.0.1:{PORT}/")
    print()
    print("Keep this Termux session open during transfer.")
    print("Press Ctrl+C to stop sharing.")
    print()
    ThreadingHTTPServer(("0.0.0.0", PORT), Handler).serve_forever()

if __name__ == "__main__":
    main()

import http.server
import socketserver
import webbrowser
import os

PORT = 8000
DIRECTORY = r"C:\art_tech"

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

print("==================================================")
print(f"  실시간 키네틱 모니터링 로컬 서버 시작 (Port: {PORT})")
print("==================================================")

url = f"http://localhost:{PORT}/live_monitor.html"
print(f"브라우저에서 접속: {url}")
webbrowser.open(url)

with socketserver.TCPServer(("", PORT), Handler) as httpd:
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n서버를 종료합니다.")

import http.server
import socketserver
import threading
import serial
import time
import json
import os
import sys

PORT = 8000
DIRECTORY = os.path.dirname(os.path.abspath(__file__))

latest_telemetry = {
    "role": "Master Gateway (Real-Time Bridge)",
    "paused": False,
    "mode": "live",
    "score": 35.0,
    "dist": 3.0,
    "damp": 1.0,
    "d1": 3.0,
    "d2": 3.0,
    "d3": 3.0,
    "amp": 9.5,
    "speed": 2.5,
    "phase": 0.0,
    "ukraine_amp": 18.5,
    "ukraine_spd": 4.5,
    "iran_amp": 16.3,
    "iran_spd": 4.1,
    "peace_amp": 7.3,
    "peace_spd": 2.0,
    "custom_amp": 12.0,
    "custom_spd": 3.0,
    "is_manual": False,
    "manual_angle": 150.0,
    "post": "실시간 소셜 데이터 연동 준비 완료",
    "time": "Just now",
    "ip": "192.168.0.14"
}

ser_conn = None

def serial_reader_thread():
    global latest_telemetry, ser_conn
    while True:
        try:
            if ser_conn is None or not ser_conn.is_open:
                try:
                    ser_conn = serial.Serial('COM17', 115200, timeout=1)
                    ser_conn.dtr = False
                    ser_conn.rts = False
                    print("[Bridge] 🔌 COM17 마스터 보드 직결 연결 성공!")
                except Exception:
                    time.sleep(1)
                    continue

            line = ser_conn.readline().decode('utf-8', errors='ignore').strip()
            if line:
                if "S1:" in line and "융합거리:" in line:
                    try:
                        parts = line.split("|")
                        s1_val = float(parts[0].split("S1:")[1].replace("m", "").strip())
                        s2_val = float(parts[1].split("S2:")[1].replace("m", "").strip())
                        s3_val = float(parts[2].split("S3:")[1].replace("m", "").split("->")[0].strip())
                        dist_val = float(line.split("융합거리:")[1].split("m")[0].strip())
                        k_val = float(line.split("K:")[1].split("|")[0].strip())
                        amp_val = float(line.split("진폭:")[1].replace("±", "").replace("°", "").strip())

                        latest_telemetry["d1"] = s1_val
                        latest_telemetry["d2"] = s2_val
                        latest_telemetry["d3"] = s3_val
                        latest_telemetry["dist"] = dist_val
                        latest_telemetry["damp"] = k_val
                        latest_telemetry["amp"] = amp_val
                    except Exception:
                        pass
        except Exception:
            time.sleep(0.5)

class BridgeRequestHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def do_GET(self):
        global ser_conn
        if self.path.startswith('/api/status'):
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(json.dumps(latest_telemetry).encode('utf-8'))
        elif self.path.startswith('/api/pause'):
            latest_telemetry["paused"] = True
            if ser_conn and ser_conn.is_open:
                ser_conn.write(b"home\n")
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(json.dumps(latest_telemetry).encode('utf-8'))
        elif self.path.startswith('/api/resume'):
            latest_telemetry["paused"] = False
            if ser_conn and ser_conn.is_open:
                ser_conn.write(b"auto\n")
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(json.dumps(latest_telemetry).encode('utf-8'))
        else:
            if self.path == '/':
                self.path = '/control_dashboard.html'
            super().do_GET()

    def log_message(self, format, *args):
        return

if __name__ == '__main__':
    t = threading.Thread(target=serial_reader_thread, daemon=True)
    t.start()
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), BridgeRequestHandler) as httpd:
        print(f"Server started on http://localhost:{PORT}")
        httpd.serve_forever()

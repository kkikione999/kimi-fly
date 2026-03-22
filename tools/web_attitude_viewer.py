#!/usr/bin/env python3
"""
ESP32 WiFi 姿态数据网页查看器
在浏览器中打开 http://localhost:8080 查看实时姿态数据
"""

import socket
import struct
import time
import threading
import http.server
import socketserver
from urllib.parse import parse_qs

# 协议定义
PROTOCOL_VERSION = 1
FRAME_START = 0xAA
CMD_ATTITUDE = 0x01
CMD_STATUS = 0x02

# 全局数据
latest_attitude = {
    'roll': 0, 'pitch': 0, 'yaw': 0,
    'roll_rate': 0, 'pitch_rate': 0, 'yaw_rate': 0,
    'armed': False, 'mode': 0, 'error': 0
}
last_update = time.time()
packet_count = 0

def decode_protocol(data):
    """解析协议帧"""
    frames = []
    i = 0
    while i < len(data):
        if data[i] != FRAME_START:
            i += 1
            continue
        if i + 4 > len(data):
            break
        cmd = data[i + 1]
        len_byte = data[i + 2]
        payload_len = len_byte & 0x7F
        # checksum at index i + 3 + payload_len, need i + 4 + payload_len bytes total
        if i + 4 + payload_len > len(data):
            break
        checksum_idx = i + 3 + payload_len
        checksum = data[checksum_idx]
        # checksum covers CMD + LEN + DATA (NOT including checksum byte itself)
        calc_checksum = sum(data[i+1:checksum_idx]) & 0xFF
        if checksum == calc_checksum:
            payload = data[i+3:i+3+payload_len]
            frames.append((cmd, payload))
            i += 4 + payload_len  # skip past header(1) + cmd(1) + len(1) + data(payload_len) + checksum(1)
        else:
            i += 1  # skip bad byte and try again
    return frames

def parse_attitude(payload):
    """解析姿态数据"""
    if len(payload) < 12:
        return None
    roll = struct.unpack('<h', payload[0:2])[0] / 100.0
    pitch = struct.unpack('<h', payload[2:4])[0] / 100.0
    yaw = struct.unpack('<h', payload[4:6])[0] / 100.0
    roll_rate = struct.unpack('<h', payload[6:8])[0] / 10.0
    pitch_rate = struct.unpack('<h', payload[8:10])[0] / 10.0
    yaw_rate = struct.unpack('<h', payload[10:12])[0] / 10.0
    return {
        'roll': roll, 'pitch': pitch, 'yaw': yaw,
        'roll_rate': roll_rate, 'pitch_rate': pitch_rate, 'yaw_rate': yaw_rate
    }

def parse_status(payload):
    """解析状态数据"""
    if len(payload) < 5:
        return None
    version, armed, mode, status, error = struct.unpack('<BBBBB', payload[:5])
    return {'version': version, 'armed': bool(armed), 'mode': mode, 'error': error}

def connect_and_read():
    """连接ESP32并读取数据"""
    global latest_attitude, last_update, packet_count
    while True:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(10)
            sock.connect(('192.168.50.132', 8888))
            sock.settimeout(1)
            print("已连接到 ESP32")
            while True:
                try:
                    data = sock.recv(1024)
                    if not data:
                        break
                    frames = decode_protocol(data)
                    for cmd, payload in frames:
                        if cmd == CMD_ATTITUDE:
                            att = parse_attitude(payload)
                            if att:
                                latest_attitude.update(att)
                                last_update = time.time()
                                packet_count += 1
                        elif cmd == CMD_STATUS:
                            st = parse_status(payload)
                            if st:
                                latest_attitude['armed'] = st['armed']
                                latest_attitude['mode'] = st['mode']
                                latest_attitude['error'] = st['error']
                except socket.timeout:
                    continue
                except Exception as e:
                    print(f"读取错误: {e}")
                    break
            sock.close()
        except Exception as e:
            print(f"连接失败: {e}")
        time.sleep(2)

HTML_TEMPLATE = """<!DOCTYPE html>
<html>
<head>
    <title>ESP32 姿态监控</title>
    <meta charset="utf-8">
    <meta http-equiv="refresh" content="1">
    <style>
        body {{
            font-family: Arial, sans-serif;
            margin: 20px;
            background: #1a1a2e;
            color: #eee;
        }}
        .container {{
            max-width: 900px;
            margin: 0 auto;
        }}
        h1 {{
            text-align: center;
            color: #00d4ff;
        }}
        .status-bar {{
            background: #16213e;
            padding: 15px;
            border-radius: 10px;
            margin-bottom: 20px;
            display: flex;
            justify-content: space-around;
        }}
        .status-item {{
            text-align: center;
        }}
        .status-value {{
            font-size: 24px;
            font-weight: bold;
            color: #00d4ff;
        }}
        .status-label {{
            font-size: 12px;
            color: #888;
        }}
        .armed {{ color: #00ff00; }}
        .disarmed {{ color: #ff4444; }}
        .attitude-container {{
            display: flex;
            justify-content: space-around;
            gap: 20px;
        }}
        .attitude-card {{
            background: #16213e;
            padding: 20px;
            border-radius: 10px;
            text-align: center;
            flex: 1;
        }}
        .attitude-value {{
            font-size: 36px;
            font-weight: bold;
            margin: 10px 0;
        }}
        .roll {{ color: #ff6b6b; }}
        .pitch {{ color: #4ecdc4; }}
        .yaw {{ color: #ffe66d; }}
        .rate-section {{
            margin-top: 20px;
            background: #16213e;
            padding: 15px;
            border-radius: 10px;
        }}
        .rate-grid {{
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 10px;
            text-align: center;
        }}
        .rate-item {{
            padding: 10px;
        }}
        .rate-value {{
            font-size: 20px;
            font-weight: bold;
            color: #00d4ff;
        }}
        .info {{
            text-align: center;
            color: #666;
            margin-top: 20px;
            font-size: 12px;
        }}
    </style>
</head>
<body>
    <div class="container">
        <h1>🚀 ESP32 姿态监控</h1>

        <div class="status-bar">
            <div class="status-item">
                <div class="status-value {armed_class}">{armed_text}</div>
                <div class="status-label">状态</div>
            </div>
            <div class="status-item">
                <div class="status-value">{mode_text}</div>
                <div class="status-label">模式</div>
            </div>
            <div class="status-item">
                <div class="status-value">{packet_count}</div>
                <div class="status-label">数据包</div>
            </div>
            <div class="status-item">
                <div class="status-value">{rssi}%</div>
                <div class="status-label">信号</div>
            </div>
        </div>

        <div class="attitude-container">
            <div class="attitude-card">
                <div class="attitude-label">Roll 横滚</div>
                <div class="attitude-value roll">{roll}°</div>
            </div>
            <div class="attitude-card">
                <div class="attitude-label">Pitch 俯仰</div>
                <div class="attitude-value pitch">{pitch}°</div>
            </div>
            <div class="attitude-card">
                <div class="attitude-label">Yaw 偏航</div>
                <div class="attitude-value yaw">{yaw}°</div>
            </div>
        </div>

        <div class="rate-section">
            <div style="text-align:center;margin-bottom:10px;color:#888">角速度 (°/s)</div>
            <div class="rate-grid">
                <div class="rate-item">
                    <div>Roll Rate</div>
                    <div class="rate-value">{roll_rate}</div>
                </div>
                <div class="rate-item">
                    <div>Pitch Rate</div>
                    <div class="rate-value">{pitch_rate}</div>
                </div>
                <div class="rate-item">
                    <div>Yaw Rate</div>
                    <div class="rate-value">{yaw_rate}</div>
                </div>
            </div>
        </div>

        <div class="info">
            ESP32 IP: 192.168.50.132 | 端口: 8888 | 最后更新: {last_update}s前
        </div>
    </div>
</body>
</html>"""

class Handler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        global latest_attitude, last_update, packet_count
        if self.path == '/' or self.path == '/index.html':
            age = time.time() - last_update
            armed_class = 'armed' if latest_attitude['armed'] else 'disarmed'
            armed_text = '已解锁' if latest_attitude['armed'] else '锁定'
            mode_names = ['手动', '定高', 'GPS', '悬停', '返航']
            mode_text = mode_names[latest_attitude['mode']] if latest_attitude['mode'] < len(mode_names) else '未知'
            rssi = max(0, min(100, 100 + (-37)))  # -37 is signal strength

            html = HTML_TEMPLATE.format(
                roll=f"{latest_attitude['roll']:.2f}",
                pitch=f"{latest_attitude['pitch']:.2f}",
                yaw=f"{latest_attitude['yaw']:.2f}",
                roll_rate=f"{latest_attitude['roll_rate']:.2f}",
                pitch_rate=f"{latest_attitude['pitch_rate']:.2f}",
                yaw_rate=f"{latest_attitude['yaw_rate']:.2f}",
                armed_class=armed_class,
                armed_text=armed_text,
                mode_text=mode_text,
                packet_count=packet_count,
                rssi=rssi,
                last_update=f"{age:.1f}"
            )
            self.send_response(200)
            self.send_header('Content-type', 'text/html; charset=utf-8')
            self.end_headers()
            self.wfile.write(html.encode())
        else:
            super().do_GET()

if __name__ == '__main__':
    # 启动TCP连接线程
    tcp_thread = threading.Thread(target=connect_and_read, daemon=True)
    tcp_thread.start()

    # 启动HTTP服务器
    PORT = 8080
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        print(f"🌐 网页查看器已启动: http://localhost:{PORT}")
        print(f"📡 正在连接 ESP32 (192.168.50.132:8888)...")
        print(f"   按 Ctrl+C 停止")
        httpd.serve_forever()

#!/usr/bin/env python3
"""
ESP32/STM32 姿态数据网页查看器
默认从当前连接电脑的 ESP32 串口日志中提取 STM32 姿态遥测帧。
在浏览器中打开 http://localhost:8080 查看实时姿态数据。
"""

import struct
import time
import threading
import http.server
import socketserver
import re
import json
import os
from pathlib import Path

import serial

# 当前 STM32 <-> ESP32 协议定义，见 firmware/stm32/comm/protocol.h
FRAME_HEADER_H = 0xAA
FRAME_HEADER_L = 0x55
CMD_ATTITUDE = 0x20
CMD_STATUS = 0x04
SERIAL_CANDIDATES = [
    "/dev/cu.usbmodem212403",
    "/dev/tty.usbmodem212403",
    "/dev/cu.usbmodem212301",
    "/dev/tty.usbmodem212301",
]
SERIAL_BAUDRATE = 460800
SERIAL_RETRY_DELAY_SEC = 1.0
SERIAL_OPEN_SETTLE_SEC = 0.2
SERIAL_NO_DATA_REOPEN_SEC = 2.0
HEX_BYTE_RE = re.compile(r"\b[0-9A-Fa-f]{2}\b")
ATT_CDEG_RE = re.compile(
    r"\[ATT_CDEG\]\s+t=(\d+)\s+r=(-?\d+)\s+p=(-?\d+)\s+y=(-?\d+)"
    r"(?:\s+rr=(-?\d+)\s+pr=(-?\d+)\s+yr=(-?\d+))?"
    r"(?:\s+tr=(-?\d+)\s+tp=(-?\d+))?"
    r"(?:\s+rrs=(-?\d+)\s+prs=(-?\d+)\s+ro=(-?\d+)\s+po=(-?\d+))?"
    r"\s+m=(\d+)"
)
MAG_DBG_RE = re.compile(
    r"\[MAG_DBG\]\s+t=(\d+)\s+addr=0x([0-9A-Fa-f]+)\s+lay=(\d+)\s+"
    r"mx=(-?\d+)\s+my=(-?\d+)\s+mz=(-?\d+)\s+hx=(-?\d+)"
)
ATTITUDE_SERIAL_PORT = os.environ.get("ATTITUDE_SERIAL_PORT", "").strip()

# 全局数据
latest_attitude = {
    'roll': 0, 'pitch': 0, 'yaw': 0,
    'roll_rate': 0, 'pitch_rate': 0, 'yaw_rate': 0,
    'trim_roll': 0.0, 'trim_pitch': 0.0,
    'mag_heading': 0.0,
    'mag_x': 0, 'mag_y': 0, 'mag_z': 0,
    'armed': False, 'mode': 0, 'error': 0
}
last_update = time.time()
packet_count = 0
latest_source = "serial"
history_path = Path("/Users/ll/kimi-fly/tools/attitude_logs/latest_static_30s.json")


def find_serial_port(preferred_port=None):
    """选择当前最可能的 ESP32/调试串口。"""
    if ATTITUDE_SERIAL_PORT:
        return ATTITUDE_SERIAL_PORT if Path(ATTITUDE_SERIAL_PORT).exists() else None

    candidates = []

    if preferred_port:
        candidates.append(preferred_port)

    candidates.extend(SERIAL_CANDIDATES)

    seen = set()
    for candidate in candidates:
        if candidate in seen:
            continue
        seen.add(candidate)
        if Path(candidate).exists():
            return candidate
    return None


def decode_protocol(data):
    """解析当前二进制协议帧 [AA 55][LEN][CMD][DATA][CRC16]。"""
    frames = []
    i = 0
    while i + 6 <= len(data):
        if data[i] != FRAME_HEADER_H or data[i + 1] != FRAME_HEADER_L:
            i += 1
            continue

        payload_len = data[i + 2]
        total_len = payload_len + 6
        if i + total_len > len(data):
            break

        cmd = data[i + 3]
        payload = data[i + 4:i + 4 + payload_len]
        # 已有串口日志来源和帧头校验，网页侧不再重复实现 CRC16，仅做轻量解包。
        if payload_len <= 64:
            frames.append((cmd, payload))
        i += total_len
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
    if len(payload) < 6:
        return None
    version, armed, mode, status, error_flags = struct.unpack('<BBBBH', payload[:6])
    return {'version': version, 'armed': bool(armed), 'mode': mode, 'error': error_flags, 'status': status}


def serial_and_read():
    """从 ESP32 串口日志中提取 STM32 姿态遥测。"""
    global latest_attitude, last_update, packet_count, latest_source
    last_port = None

    while True:
        port = find_serial_port(last_port)
        if port is None:
            print("未找到串口设备，1 秒后重试")
            time.sleep(SERIAL_RETRY_DELAY_SEC)
            continue

        ser = None
        try:
            # 串口重连后丢弃旧文本/旧二进制缓存，避免跨重启混包。
            line_buffer = ""
            byte_buffer = bytearray()
            ser = serial.Serial(port, SERIAL_BAUDRATE, timeout=0.1, rtscts=False, dsrdtr=False)
            # 避免部分 USB CDC 设备在打开串口时因 DTR/RTS 翻转而复位。
            ser.dtr = False
            ser.rts = False
            time.sleep(SERIAL_OPEN_SETTLE_SEC)
            latest_source = port
            last_port = port
            last_chunk_time = time.time()
            print(f"已连接串口: {port}")

            while True:
                chunk = ser.read(4096)
                if not chunk:
                    if (time.time() - last_chunk_time) >= SERIAL_NO_DATA_REOPEN_SEC:
                        raise serial.SerialException(
                            f"serial stream stalled for {SERIAL_NO_DATA_REOPEN_SEC:.1f}s"
                        )
                    continue
                last_chunk_time = time.time()

                line_buffer += chunk.decode("utf-8", errors="ignore")
                lines = line_buffer.split("\n")
                line_buffer = lines.pop() if lines else ""

                for line in lines:
                    att_match = ATT_CDEG_RE.search(line)
                    if att_match:
                        groups = att_match.groups()
                        t_ms = int(groups[0])
                        roll_cd = int(groups[1])
                        pitch_cd = int(groups[2])
                        yaw_cd = int(groups[3])
                        roll_rate_dd = int(groups[4]) if groups[4] is not None else None
                        pitch_rate_dd = int(groups[5]) if groups[5] is not None else None
                        yaw_rate_dd = int(groups[6]) if groups[6] is not None else None
                        trim_roll_cd = int(groups[7]) if groups[7] is not None else 0
                        trim_pitch_cd = int(groups[8]) if groups[8] is not None else 0
                        mag_valid = int(groups[13])
                        prev_roll = latest_attitude['roll']
                        prev_pitch = latest_attitude['pitch']
                        prev_yaw = latest_attitude['yaw']
                        prev_update = last_update
                        latest_attitude['roll'] = roll_cd / 100.0
                        latest_attitude['pitch'] = pitch_cd / 100.0
                        latest_attitude['yaw'] = yaw_cd / 100.0
                        latest_attitude['trim_roll'] = trim_roll_cd / 100.0
                        latest_attitude['trim_pitch'] = trim_pitch_cd / 100.0
                        now = time.time()
                        dt = max(now - prev_update, 1e-3)
                        if roll_rate_dd is not None and pitch_rate_dd is not None and yaw_rate_dd is not None:
                            latest_attitude['roll_rate'] = roll_rate_dd / 10.0
                            latest_attitude['pitch_rate'] = pitch_rate_dd / 10.0
                            latest_attitude['yaw_rate'] = yaw_rate_dd / 10.0
                        else:
                            latest_attitude['roll_rate'] = (latest_attitude['roll'] - prev_roll) / dt
                            latest_attitude['pitch_rate'] = (latest_attitude['pitch'] - prev_pitch) / dt
                            latest_attitude['yaw_rate'] = (latest_attitude['yaw'] - prev_yaw) / dt
                        latest_attitude['armed'] = bool(mag_valid)
                        last_update = now
                        packet_count += 1
                        continue

                    mag_match = MAG_DBG_RE.search(line)
                    if mag_match:
                        groups = mag_match.groups()
                        latest_attitude['mag_x'] = int(groups[3])
                        latest_attitude['mag_y'] = int(groups[4])
                        latest_attitude['mag_z'] = int(groups[5])
                        latest_attitude['mag_heading'] = int(groups[6]) / 100.0
                        continue

                    if "RX raw" in line:
                        hex_bytes = HEX_BYTE_RE.findall(line)
                        for item in hex_bytes:
                            byte_buffer.append(int(item, 16))

                frames = decode_protocol(byte_buffer)
                if frames:
                    # 已消费帧后丢弃缓存，避免日志持续膨胀。
                    byte_buffer.clear()

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
        except Exception as e:
            print(f"串口读取失败: {e}")
            if ser is not None:
                try:
                    ser.close()
                except Exception:
                    pass
        time.sleep(SERIAL_RETRY_DELAY_SEC)

HTML_TEMPLATE = """<!DOCTYPE html>
<html>
<head>
    <title>ESP32 姿态监控</title>
    <meta charset="utf-8">
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
        .debug-section {{
            margin-top: 20px;
            background: #16213e;
            padding: 15px;
            border-radius: 10px;
        }}
        .debug-grid {{
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 10px;
            text-align: center;
        }}
        .debug-item {{
            padding: 10px;
            background: #0f3460;
            border-radius: 8px;
        }}
        .debug-value {{
            font-size: 20px;
            font-weight: bold;
            color: #7dd3fc;
        }}
        .history-section {{
            margin-top: 20px;
            background: #16213e;
            padding: 15px;
            border-radius: 10px;
        }}
        .summary-grid {{
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 10px;
            text-align: center;
            margin-bottom: 12px;
        }}
        .summary-item {{
            background: #0f3460;
            border-radius: 8px;
            padding: 10px;
        }}
        .summary-value {{
            font-size: 22px;
            font-weight: bold;
        }}
        .chart {{
            width: 100%;
            height: 320px;
            background: #0f172a;
            border-radius: 8px;
        }}
        .legend {{
            display: flex;
            gap: 16px;
            justify-content: center;
            margin-top: 10px;
            color: #a0aec0;
            font-size: 12px;
        }}
        .dot {{
            display: inline-block;
            width: 10px;
            height: 10px;
            border-radius: 50%;
            margin-right: 6px;
        }}
        .stale {{
            color: #f59e0b;
        }}
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 姿态监控</h1>

        <div class="status-bar">
            <div class="status-item">
                <div class="status-value {armed_class}" id="armed-text">{armed_text}</div>
                <div class="status-label">状态</div>
            </div>
            <div class="status-item">
                <div class="status-value" id="mode-text">{mode_text}</div>
                <div class="status-label">模式</div>
            </div>
            <div class="status-item">
                <div class="status-value" id="packet-count">{packet_count}</div>
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
                <div class="attitude-value roll" id="roll-value">{roll}°</div>
            </div>
            <div class="attitude-card">
                <div class="attitude-label">Pitch 俯仰</div>
                <div class="attitude-value pitch" id="pitch-value">{pitch}°</div>
            </div>
            <div class="attitude-card">
                <div class="attitude-label">Yaw 偏航</div>
                <div class="attitude-value yaw" id="yaw-value">{yaw}°</div>
            </div>
        </div>

        <div class="rate-section">
            <div style="text-align:center;margin-bottom:10px;color:#888">角速度 (°/s)</div>
            <div class="rate-grid">
                <div class="rate-item">
                    <div>Roll Rate</div>
                    <div class="rate-value" id="roll-rate">{roll_rate}</div>
                </div>
                <div class="rate-item">
                    <div>Pitch Rate</div>
                    <div class="rate-value" id="pitch-rate">{pitch_rate}</div>
                </div>
                <div class="rate-item">
                    <div>Yaw Rate</div>
                    <div class="rate-value" id="yaw-rate">{yaw_rate}</div>
                </div>
            </div>
        </div>

        <div class="info" id="live-info">
            数据源: {latest_source} | 最后更新: {last_update}s前
        </div>

        <div class="debug-section">
            <div style="text-align:center;margin-bottom:10px;color:#7dd3fc">IMU / 磁力计调试</div>
            <div class="debug-grid">
                <div class="debug-item">
                    <div>Mag Heading</div>
                    <div class="debug-value" id="mag-heading">{mag_heading}°</div>
                </div>
                <div class="debug-item">
                    <div>Mag X</div>
                    <div class="debug-value" id="mag-x">{mag_x}</div>
                </div>
                <div class="debug-item">
                    <div>Mag Y</div>
                    <div class="debug-value" id="mag-y">{mag_y}</div>
                </div>
                <div class="debug-item">
                    <div>Mag Z</div>
                    <div class="debug-value" id="mag-z">{mag_z}</div>
                </div>
            </div>
        </div>

        <div class="history-section">
            <div style="text-align:center;margin-bottom:10px;color:#00d4ff">静止 30s 验证</div>
            <div class="summary-grid">
                <div class="summary-item">
                    <div>Roll 最大偏移</div>
                    <div class="summary-value roll" id="roll-drift">--</div>
                </div>
                <div class="summary-item">
                    <div>Pitch 最大偏移</div>
                    <div class="summary-value pitch" id="pitch-drift">--</div>
                </div>
                <div class="summary-item">
                    <div>Yaw 最大漂移</div>
                    <div class="summary-value yaw" id="yaw-drift">--</div>
                </div>
            </div>
            <svg id="history-chart" class="chart" viewBox="0 0 900 320" preserveAspectRatio="none"></svg>
            <div class="legend">
                <span><span class="dot" style="background:#ff6b6b"></span>Roll</span>
                <span><span class="dot" style="background:#4ecdc4"></span>Pitch</span>
                <span><span class="dot" style="background:#ffe66d"></span>Yaw</span>
            </div>
        </div>
    </div>
    <script>
        async function loadLive() {{
            const res = await fetch('/live.json', {{ cache: 'no-store' }});
            if (!res.ok) {{
                return;
            }}
            const data = await res.json();
            document.getElementById('roll-value').textContent = `${{data.roll.toFixed(2)}}°`;
            document.getElementById('pitch-value').textContent = `${{data.pitch.toFixed(2)}}°`;
            document.getElementById('yaw-value').textContent = `${{data.yaw.toFixed(2)}}°`;
            document.getElementById('roll-rate').textContent = data.roll_rate.toFixed(2);
            document.getElementById('pitch-rate').textContent = data.pitch_rate.toFixed(2);
            document.getElementById('yaw-rate').textContent = data.yaw_rate.toFixed(2);
            document.getElementById('mag-heading').textContent = `${{data.mag_heading.toFixed(2)}}°`;
            document.getElementById('mag-x').textContent = `${{data.mag_x}}`;
            document.getElementById('mag-y').textContent = `${{data.mag_y}}`;
            document.getElementById('mag-z').textContent = `${{data.mag_z}}`;
            document.getElementById('packet-count').textContent = `${{data.packet_count}}`;
            document.getElementById('armed-text').textContent = data.armed ? '在线' : '离线';
            document.getElementById('armed-text').className = `status-value ${{data.stale ? 'stale' : (data.armed ? 'armed' : 'disarmed')}}`;
            document.getElementById('mode-text').textContent = data.source;
            document.getElementById('live-info').textContent = `数据源: ${{data.source}} | 最后更新: ${{data.age_sec.toFixed(1)}}s前`;
        }}

        async function loadHistory() {{
            const res = await fetch('/history.json', {{ cache: 'no-store' }});
            if (!res.ok) {{
                return;
            }}
            const data = await res.json();
            const metrics = data.metrics || {{}};
            document.getElementById('roll-drift').textContent = `${{(metrics.roll_max_drift_deg ?? 0).toFixed(2)}}°`;
            document.getElementById('pitch-drift').textContent = `${{(metrics.pitch_max_drift_deg ?? 0).toFixed(2)}}°`;
            document.getElementById('yaw-drift').textContent = `${{(metrics.yaw_max_drift_deg ?? 0).toFixed(2)}}°`;

            const samples = data.samples || [];
            const svg = document.getElementById('history-chart');
            if (!samples.length) {{
                return;
            }}

            const width = 900;
            const height = 320;
            const pad = 18;
            const values = samples.flatMap(s => [s.roll_deg, s.pitch_deg, s.yaw_deg]);
            const minV = Math.min(...values);
            const maxV = Math.max(...values);
            const spanV = Math.max(maxV - minV, 1);
            const t0 = samples[0].t_ms;
            const t1 = samples[samples.length - 1].t_ms;
            const spanT = Math.max(t1 - t0, 1);

            function pt(sample, key) {{
                const x = pad + ((sample.t_ms - t0) / spanT) * (width - pad * 2);
                const y = height - pad - ((sample[key] - minV) / spanV) * (height - pad * 2);
                return `${{x.toFixed(2)}},${{y.toFixed(2)}}`;
            }}

            function poly(key, color) {{
                const points = samples.map(s => pt(s, key)).join(' ');
                return `<polyline fill="none" stroke="${{color}}" stroke-width="2" points="${{points}}" />`;
            }}

            const midY = height - pad - ((0 - minV) / spanV) * (height - pad * 2);
            svg.innerHTML =
                `<rect x="0" y="0" width="${{width}}" height="${{height}}" fill="#0f172a" rx="8" />` +
                `<line x1="${{pad}}" y1="${{midY.toFixed(2)}}" x2="${{width - pad}}" y2="${{midY.toFixed(2)}}" stroke="#334155" stroke-width="1" />` +
                poly('roll_deg', '#ff6b6b') +
                poly('pitch_deg', '#4ecdc4') +
                poly('yaw_deg', '#ffe66d');
        }}

        loadLive();
        loadHistory();
        setInterval(loadLive, 500);
        setInterval(loadHistory, 2000);
    </script>
</body>
</html>"""

class Handler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, format, *args):
        # Keep the terminal readable; serial reconnect errors matter more here.
        return

    def do_GET(self):
        global latest_attitude, last_update, packet_count
        if self.path == '/live.json':
            age = time.time() - last_update
            payload = json.dumps({
                'roll': latest_attitude['roll'],
                'pitch': latest_attitude['pitch'],
                'yaw': latest_attitude['yaw'],
                'roll_rate': latest_attitude['roll_rate'],
                'pitch_rate': latest_attitude['pitch_rate'],
                'yaw_rate': latest_attitude['yaw_rate'],
                'mag_heading': latest_attitude['mag_heading'],
                'mag_x': latest_attitude['mag_x'],
                'mag_y': latest_attitude['mag_y'],
                'mag_z': latest_attitude['mag_z'],
                'armed': packet_count > 0,
                'mode': latest_attitude['mode'],
                'error': latest_attitude['error'],
                'packet_count': packet_count,
                'source': latest_source,
                'age_sec': age,
                'stale': age > 1.0,
            }, ensure_ascii=False)
            self.send_response(200)
            self.send_header('Content-type', 'application/json; charset=utf-8')
            self.end_headers()
            self.wfile.write(payload.encode('utf-8'))
        elif self.path == '/history.json':
            if history_path.exists():
                payload = history_path.read_text(encoding='utf-8')
            else:
                payload = json.dumps({'samples': [], 'metrics': {}}, ensure_ascii=False)
            self.send_response(200)
            self.send_header('Content-type', 'application/json; charset=utf-8')
            self.end_headers()
            self.wfile.write(payload.encode('utf-8'))
        elif self.path == '/' or self.path == '/index.html':
            age = time.time() - last_update
            armed_class = 'armed' if latest_attitude['armed'] else 'disarmed'
            armed_text = '已解锁' if latest_attitude['armed'] else '锁定'
            mode_names = ['手动', '定高', 'GPS', '悬停', '返航']
            mode_text = mode_names[latest_attitude['mode']] if latest_attitude['mode'] < len(mode_names) else '未知'
            rssi = 100

            html = HTML_TEMPLATE.format(
                roll=f"{latest_attitude['roll']:.2f}",
                pitch=f"{latest_attitude['pitch']:.2f}",
                yaw=f"{latest_attitude['yaw']:.2f}",
                roll_rate=f"{latest_attitude['roll_rate']:.2f}",
                pitch_rate=f"{latest_attitude['pitch_rate']:.2f}",
                yaw_rate=f"{latest_attitude['yaw_rate']:.2f}",
                mag_heading=f"{latest_attitude['mag_heading']:.2f}",
                mag_x=latest_attitude['mag_x'],
                mag_y=latest_attitude['mag_y'],
                mag_z=latest_attitude['mag_z'],
                armed_class=armed_class,
                armed_text=armed_text,
                mode_text=mode_text,
                packet_count=packet_count,
                rssi=rssi,
                last_update=f"{age:.1f}",
                latest_source=latest_source,
            )
            self.send_response(200)
            self.send_header('Content-type', 'text/html; charset=utf-8')
            self.end_headers()
            self.wfile.write(html.encode())
        else:
            super().do_GET()


class ReusableTCPServer(socketserver.TCPServer):
    allow_reuse_address = True

if __name__ == '__main__':
    reader_thread = threading.Thread(target=serial_and_read, daemon=True)
    reader_thread.start()

    # 启动HTTP服务器
    PORT = int(os.environ.get("ATTITUDE_VIEWER_PORT", "8080"))
    with ReusableTCPServer(("", PORT), Handler) as httpd:
        print(f"🌐 网页查看器已启动: http://localhost:{PORT}")
        print("📡 正在从 ESP32 串口日志提取姿态遥测...")
        print(f"📍 当前数据源: {latest_source}")
        print("   按 Ctrl+C 停止")
        httpd.serve_forever()

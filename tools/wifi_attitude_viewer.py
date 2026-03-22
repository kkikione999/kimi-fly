#!/usr/bin/env python3
"""
ESP32 WiFi 姿态数据查看器
连接ESP32的TCP服务器，解析并显示姿态遥测数据
"""

import socket
import struct
import time
import sys
from collections import deque

# 协议定义 (参考 protocol.h)
PROTOCOL_VERSION = 1
FRAME_START = 0xAA

# CMD定义
CMD_ATTITUDE = 0x01
CMD_STATUS = 0x02
CMD_ARM = 0x03
CMD_DISARM = 0x04
CMD_HEARTBEAT = 0x05
CMD_ACK = 0x06
CMD_NACK = 0x07

def decode_protocol(data):
    """解析协议帧"""
    frames = []
    i = 0
    while i < len(data):
        # 查找帧头
        if data[i] != FRAME_START:
            i += 1
            continue

        if i + 4 > len(data):
            break

        # 解析帧头
        cmd = data[i + 1]
        len_byte = data[i + 2]
        payload_len = len_byte & 0x7F

        if i + 4 + payload_len + 2 > len(data):
            break

        # 计算校验和 (简单XOR)
        checksum = data[i + 3 + payload_len]
        calc_checksum = sum(data[i+1:i+3+payload_len]) & 0xFF

        if checksum == calc_checksum:
            payload = data[i+3:i+3+payload_len]
            frames.append((cmd, payload))

        i += 4 + payload_len + 2  # 帧头(4) + payload + 校验和(2)

    return frames

def parse_attitude(payload):
    """解析姿态数据 (protocol_attitude_t)"""
    if len(payload) < 13:
        return None

    # 格式: roll, pitch, yaw, roll_rate, pitch_rate, yaw_rate
    # 每个是 int16_t * 100 (0.01度)
    roll = struct.unpack('<h', payload[0:2])[0] / 100.0
    pitch = struct.unpack('<h', payload[2:4])[0] / 100.0
    yaw = struct.unpack('<h', payload[4:6])[0] / 100.0
    roll_rate = struct.unpack('<h', payload[6:8])[0] / 10.0  # deg/s
    pitch_rate = struct.unpack('<h', payload[8:10])[0] / 10.0
    yaw_rate = struct.unpack('<h', payload[10:12])[0] / 10.0

    return {
        'roll': roll,
        'pitch': pitch,
        'yaw': yaw,
        'roll_rate': roll_rate,
        'pitch_rate': pitch_rate,
        'yaw_rate': yaw_rate
    }

def parse_status(payload):
    """解析状态数据"""
    if len(payload) < 6:
        return None

    version, armed, mode, status, error = struct.unpack('<BBBBB', payload[:5])
    return {
        'version': version,
        'armed': bool(armed),
        'mode': mode,
        'status': status,
        'error': error
    }

def main():
    host = sys.argv[1] if len(sys.argv) > 1 else "192.168.4.1"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8888

    print(f"连接到 {host}:{port}...")
    print("按 Ctrl+C 退出")
    print()
    print("=" * 60)
    print(f"{'Time':>8} | {'Roll':>8} | {'Pitch':>8} | {'Yaw':>8} | {'RollRate':>9} | {'PitchRate':>9} | {'YawRate':>9}")
    print("-" * 60)

    # 滚动窗口用于平滑显示
    roll_history = deque(maxlen=10)
    pitch_history = deque(maxlen=10)
    yaw_history = deque(maxlen=10)

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10.0)
        sock.connect((host, port))
        sock.settimeout(1.0)

        print("已连接! 等待数据...")
        print()

        last_update = time.time()
        packet_count = 0

        while True:
            try:
                data = sock.recv(1024)
                if not data:
                    print("连接关闭")
                    break

                frames = decode_protocol(data)

                for cmd, payload in frames:
                    if cmd == CMD_ATTITUDE:
                        att = parse_attitude(payload)
                        if att:
                            packet_count += 1

                            # 平滑
                            roll_history.append(att['roll'])
                            pitch_history.append(att['pitch'])
                            yaw_history.append(att['yaw'])

                            roll_avg = sum(roll_history) / len(roll_history)
                            pitch_avg = sum(pitch_history) / len(pitch_history)
                            yaw_avg = sum(yaw_history) / len(yaw_history)

                            elapsed = time.time() - last_update
                            if elapsed >= 0.2:  # 5Hz显示
                                print(f"{elapsed:>8.2f} | {roll_avg:>8.2f} | {pitch_avg:>8.2f} | {yaw_avg:>8.2f} | {att['roll_rate']:>9.2f} | {att['pitch_rate']:>9.2f} | {att['yaw_rate']:>9.2f}")
                                last_update = time.time()

                    elif cmd == CMD_STATUS:
                        st = parse_status(payload)
                        if st:
                            print(f"[STATUS] Armed={st['armed']}, Mode={st['mode']}, Error={st['error']}")

                    elif cmd == CMD_ACK:
                        print(f"[ACK] CMD=0x{payload[0]:02X}")

                    elif cmd == CMD_NACK:
                        print(f"[NACK] CMD=0x{payload[0]:02X}, ERR=0x{payload[1]:02X}")

            except socket.timeout:
                continue

    except KeyboardInterrupt:
        print("\n退出")
    except Exception as e:
        print(f"错误: {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    main()

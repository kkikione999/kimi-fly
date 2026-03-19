#!/usr/bin/env python3
"""
Drone WiFi Ground Station
Ground control station for WiFi drone

Usage:
    python ground_station.py --host 192.168.4.1 --port 8888

Controls:
    w/s - Throttle up/down
    a/d - Roll left/right
    i/k - Pitch forward/backward
    j/l - Yaw left/right
    space - ARM/DISARM toggle
    m - Cycle flight modes
    q - Quit
"""

import argparse
import socket
import struct
import sys
import termios
import tty
import select
import threading
import time
from dataclasses import dataclass
from typing import Optional

from protocol import (
    ProtocolFrame, Command, ResponseCode, FlightMode,
    RCData, AttitudeData, MotorData, StatusData
)


@dataclass
class Telemetry:
    """Telemetry data container"""
    attitude: Optional[AttitudeData] = None
    motor: Optional[MotorData] = None
    status: Optional[StatusData] = None
    last_update: float = 0.0


class DroneConnection:
    """TCP connection to drone"""

    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port
        self.socket: Optional[socket.socket] = None
        self.connected = False
        self.rx_buffer = b''
        self.lock = threading.Lock()

    def connect(self) -> bool:
        """Connect to drone"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(5.0)
            self.socket.connect((self.host, self.port))
            self.socket.setblocking(False)
            self.connected = True
            return True
        except Exception as e:
            print(f"Connection failed: {e}")
            return False

    def disconnect(self):
        """Disconnect from drone"""
        self.connected = False
        if self.socket:
            self.socket.close()
            self.socket = None

    def send(self, frame: ProtocolFrame) -> bool:
        """Send frame to drone"""
        if not self.connected or not self.socket:
            return False
        try:
            data = frame.encode()
            self.socket.sendall(data)
            return True
        except Exception as e:
            print(f"Send error: {e}")
            self.connected = False
            return False

    def receive(self) -> Optional[ProtocolFrame]:
        """Receive and parse frame from drone"""
        if not self.connected or not self.socket:
            return None

        try:
            # Read available data
            data = self.socket.recv(256)
            if not data:
                self.connected = False
                return None

            self.rx_buffer += data

            # Try to decode frame
            while len(self.rx_buffer) >= 6:
                result = ProtocolFrame.decode(self.rx_buffer)
                if result is None:
                    # Not enough data or CRC error, skip first byte and retry
                    self.rx_buffer = self.rx_buffer[1:]
                    continue

                frame, consumed = result
                self.rx_buffer = self.rx_buffer[consumed:]
                return frame

        except BlockingIOError:
            pass
        except Exception as e:
            print(f"Receive error: {e}")
            self.connected = False

        return None


class GroundStation:
    """Ground station main class"""

    def __init__(self, host: str, port: int):
        self.conn = DroneConnection(host, port)
        self.telemetry = Telemetry()
        self.running = False
        self.rc = RCData()
        self.armed = False
        self.current_mode = FlightMode.DISARMED

        # Control rates
        self.throttle_step = 50
        self.stick_step = 50

    def run(self):
        """Main run loop"""
        print("=" * 60)
        print("     Drone WiFi Ground Station")
        print("=" * 60)
        print(f"Connecting to {self.conn.host}:{self.conn.port}...")

        if not self.conn.connect():
            print("Failed to connect to drone!")
            return

        print("Connected!")
        print()
        print("Controls:")
        print("  w/s - Throttle up/down")
        print("  a/d - Roll left/right")
        print("  i/k - Pitch forward/backward")
        print("  j/l - Yaw left/right")
        print("  space - ARM/DISARM toggle")
        print("  m - Cycle flight modes")
        print("  v - Request version")
        print("  t - Request status")
        print("  r - Reset sticks to center")
        print("  q - Quit")
        print()

        self.running = True

        # Start receive thread
        rx_thread = threading.Thread(target=self._receive_loop, daemon=True)
        rx_thread.start()

        # Start heartbeat thread
        hb_thread = threading.Thread(target=self._heartbeat_loop, daemon=True)
        hb_thread.start()

        # Main control loop
        self._control_loop()

        self.conn.disconnect()
        print("Disconnected.")

    def _receive_loop(self):
        """Background receive loop"""
        while self.running:
            frame = self.conn.receive()
            if frame:
                self._handle_frame(frame)
            time.sleep(0.001)

    def _handle_frame(self, frame: ProtocolFrame):
        """Handle received frame"""
        if frame.cmd == Command.TELEMETRY_ATT and len(frame.data) >= 12:
            self.telemetry.attitude = AttitudeData.from_bytes(frame.data)
            self.telemetry.last_update = time.time()

        elif frame.cmd == Command.TELEMETRY_MOTOR and len(frame.data) >= 10:
            self.telemetry.motor = MotorData.from_bytes(frame.data)

        elif frame.cmd == Command.STATUS and len(frame.data) >= 6:
            self.telemetry.status = StatusData.from_bytes(frame.data)
            self.armed = self.telemetry.status.armed
            self.current_mode = FlightMode(self.telemetry.status.mode)

        elif frame.cmd == Command.ACK:
            pass  # Ack received

        elif frame.cmd == Command.NACK:
            if len(frame.data) >= 2:
                cmd, error = frame.data[0], frame.data[1]
                print(f"NACK: cmd=0x{cmd:02X}, error=0x{error:02X}")

    def _heartbeat_loop(self):
        """Send periodic heartbeats"""
        while self.running:
            self.conn.send(ProtocolFrame(Command.HEARTBEAT))
            time.sleep(0.5)

    def _send_rc_command(self):
        """Send RC input command"""
        frame = ProtocolFrame(Command.RC_INPUT, self.rc.to_bytes())
        self.conn.send(frame)

    def _control_loop(self):
        """Main keyboard control loop"""
        # Set terminal to raw mode for single character input
        old_settings = termios.tcgetattr(sys.stdin)
        try:
            tty.setcbreak(sys.stdin.fileno())

            last_rc_send = 0
            display_counter = 0

            while self.running:
                # Check for keyboard input
                if select.select([sys.stdin], [], [], 0.01)[0]:
                    key = sys.stdin.read(1)

                    if key == 'q':
                        self.running = False
                        break

                    elif key == ' ':
                        # ARM/DISARM toggle
                        if self.armed:
                            self.conn.send(ProtocolFrame(Command.DISARM))
                            print("-> DISARM")
                        else:
                            self.conn.send(ProtocolFrame(Command.ARM))
                            print("-> ARM")

                    elif key == 'm':
                        # Cycle modes
                        modes = [FlightMode.ARMED, FlightMode.STABILIZE, FlightMode.ACRO]
                        try:
                            current_idx = modes.index(self.current_mode)
                            next_mode = modes[(current_idx + 1) % len(modes)]
                        except ValueError:
                            next_mode = FlightMode.ARMED

                        frame = ProtocolFrame(Command.MODE, bytes([next_mode]))
                        self.conn.send(frame)
                        print(f"-> MODE: {next_mode.name}")

                    elif key == 'v':
                        self.conn.send(ProtocolFrame(Command.VERSION))
                        print("-> Request version")

                    elif key == 't':
                        self.conn.send(ProtocolFrame(Command.STATUS))
                        print("-> Request status")

                    elif key == 'r':
                        # Reset sticks
                        self.rc = RCData()
                        self._send_rc_command()
                        print("-> Reset sticks")

                    # Throttle
                    elif key == 'w':
                        self.rc.throttle = min(1000, self.rc.throttle + self.throttle_step)
                    elif key == 's':
                        self.rc.throttle = max(0, self.rc.throttle - self.throttle_step)

                    # Roll
                    elif key == 'a':
                        self.rc.roll = max(-500, self.rc.roll - self.stick_step)
                    elif key == 'd':
                        self.rc.roll = min(500, self.rc.roll + self.stick_step)

                    # Pitch
                    elif key == 'i':
                        self.rc.pitch = min(500, self.rc.pitch + self.stick_step)
                    elif key == 'k':
                        self.rc.pitch = max(-500, self.rc.pitch - self.stick_step)

                    # Yaw
                    elif key == 'j':
                        self.rc.yaw = max(-500, self.rc.yaw - self.stick_step)
                    elif key == 'l':
                        self.rc.yaw = min(500, self.rc.yaw + self.stick_step)

                # Send RC command at 20Hz
                now = time.time()
                if now - last_rc_send >= 0.05:
                    self._send_rc_command()
                    last_rc_send = now

                # Display telemetry at 5Hz
                display_counter += 1
                if display_counter >= 10:
                    self._display_status()
                    display_counter = 0

        finally:
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)

    def _display_status(self):
        """Display current status"""
        # Clear line and print status
        status_line = f"\rRC[T:{self.rc.throttle:4} R:{self.rc.roll:4} P:{self.rc.pitch:4} Y:{self.rc.yaw:4}] "

        if self.telemetry.attitude:
            a = self.telemetry.attitude
            status_line += f"Att[R:{a.roll:6.1f} P:{a.pitch:6.1f} Y:{a.yaw:6.1f}] "

        if self.telemetry.motor:
            m = self.telemetry.motor
            status_line += f"Mot[{m.motor1:3},{m.motor2:3},{m.motor3:3},{m.motor4:3}] "

        mode_str = self.current_mode.name if self.current_mode else "UNKNOWN"
        status_line += f"Mode:{mode_str} Armed:{self.armed}"

        # Pad to clear line
        status_line += " " * max(0, 120 - len(status_line))
        print(status_line[:120], end='', flush=True)


def main():
    parser = argparse.ArgumentParser(description='Drone WiFi Ground Station')
    parser.add_argument('--host', default='192.168.4.1',
                        help='Drone IP address (default: 192.168.4.1)')
    parser.add_argument('--port', type=int, default=8888,
                        help='Drone port (default: 8888)')

    args = parser.parse_args()

    gs = GroundStation(args.host, args.port)

    try:
        gs.run()
    except KeyboardInterrupt:
        print("\nInterrupted by user")
    finally:
        gs.running = False


if __name__ == '__main__':
    main()

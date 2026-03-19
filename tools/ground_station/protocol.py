"""
Drone WiFi Communication Protocol - Python Implementation
Compatible with firmware/stm32/comm/protocol.h

Frame format: [HEADER:2][LEN:1][CMD:1][DATA:N][CRC:2]
"""

import struct
from enum import IntEnum
from typing import Optional, Tuple

# Protocol constants
PROTOCOL_FRAME_HEADER = 0xAA55
PROTOCOL_VERSION = 0x01


class Command(IntEnum):
    """Command types matching protocol_cmd_t"""
    # System commands (0x00-0x0F)
    HEARTBEAT = 0x00
    ACK = 0x01
    NACK = 0x02
    VERSION = 0x03
    STATUS = 0x04

    # Flight control commands (0x10-0x1F)
    ARM = 0x10
    DISARM = 0x11
    MODE = 0x12
    RC_INPUT = 0x13

    # Telemetry commands (0x20-0x2F)
    TELEMETRY_ATT = 0x20
    TELEMETRY_IMU = 0x21
    TELEMETRY_MOTOR = 0x22
    TELEMETRY_BATTERY = 0x23

    # Configuration commands (0x30-0x3F)
    PID_GET = 0x30
    PID_SET = 0x31
    CALIBRATE = 0x32


class ResponseCode(IntEnum):
    """Response status codes"""
    OK = 0x00
    ERR_INVALID_CMD = 0x01
    ERR_INVALID_PARAM = 0x02
    ERR_BUSY = 0x03
    ERR_NOT_ARMED = 0x04
    ERR_ARMED = 0x05
    ERR_CRC = 0x06
    ERR_TIMEOUT = 0x07
    ERR_INTERNAL = 0x08


class FlightMode(IntEnum):
    """Flight modes matching flight_mode_t"""
    DISARMED = 0
    ARMED = 1
    STABILIZE = 2
    ACRO = 3


class CRC16:
    """CRC-16/CCITT-FALSE implementation"""
    TABLE = [
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
        0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
        0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
        0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
        0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
        0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
        0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
        0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
        0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
        0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
        0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
        0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
        0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
        0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
        0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
        0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
        0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
        0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
        0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
        0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
        0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
        0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
        0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
        0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
        0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
        0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
        0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
        0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
        0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
        0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
        0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
        0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
    ]

    @classmethod
    def calc(cls, data: bytes) -> int:
        """Calculate CRC16 for data"""
        crc = 0xFFFF
        for byte in data:
            crc = ((crc << 8) ^ cls.TABLE[((crc >> 8) ^ byte) & 0xFF]) & 0xFFFF
        return crc


class ProtocolFrame:
    """Protocol frame structure"""

    def __init__(self, cmd: Command = Command.HEARTBEAT, data: bytes = b''):
        self.cmd = cmd
        self.data = data

    def encode(self) -> bytes:
        """Encode frame to bytes"""
        # Build frame without CRC: [HEADER][LEN][CMD][DATA]
        frame = struct.pack('>HBB', PROTOCOL_FRAME_HEADER, len(self.data), self.cmd)
        frame += self.data

        # Calculate and append CRC
        crc = CRC16.calc(frame)
        frame += struct.pack('>H', crc)

        return frame

    @classmethod
    def decode(cls, data: bytes) -> Optional[Tuple['ProtocolFrame', int]]:
        """
        Decode frame from bytes
        Returns (frame, consumed_bytes) or None if not enough data
        """
        # Find header
        header_pos = -1
        for i in range(len(data) - 1):
            if struct.unpack('>H', data[i:i+2])[0] == PROTOCOL_FRAME_HEADER:
                header_pos = i
                break

        if header_pos == -1:
            return None

        # Check minimum frame size
        if len(data) - header_pos < 6:
            return None

        # Parse length
        payload_len = data[header_pos + 2]
        total_len = 6 + payload_len

        # Check if complete frame available
        if len(data) - header_pos < total_len:
            return None

        # Extract frame data
        frame_data = data[header_pos:header_pos + total_len]

        # Verify CRC
        calc_crc = CRC16.calc(frame_data[:-2])
        rx_crc = struct.unpack('>H', frame_data[-2:])[0]

        if calc_crc != rx_crc:
            # CRC error, skip header and try again
            return None

        # Parse frame
        cmd = frame_data[3]
        payload = frame_data[4:4 + payload_len]

        return cls(Command(cmd), payload), header_pos + total_len


class RCData:
    """RC input data structure"""

    def __init__(self, throttle: int = 0, roll: int = 0, pitch: int = 0,
                 yaw: int = 0, buttons: int = 0):
        self.throttle = max(0, min(1000, throttle))  # 0-1000
        self.roll = max(-500, min(500, roll))        # -500 to +500
        self.pitch = max(-500, min(500, pitch))      # -500 to +500
        self.yaw = max(-500, min(500, yaw))          # -500 to +500
        self.buttons = buttons

    def to_bytes(self) -> bytes:
        """Pack to bytes"""
        return struct.pack('<hhhhB', self.throttle, self.roll,
                          self.pitch, self.yaw, self.buttons)

    @classmethod
    def from_bytes(cls, data: bytes) -> 'RCData':
        """Unpack from bytes"""
        t, r, p, y, b = struct.unpack('<hhhhB', data[:9])
        return cls(t, r, p, y, b)


class AttitudeData:
    """Attitude telemetry data"""

    def __init__(self, roll: float = 0, pitch: float = 0, yaw: float = 0,
                 roll_rate: float = 0, pitch_rate: float = 0, yaw_rate: float = 0):
        self.roll = roll          # degrees
        self.pitch = pitch        # degrees
        self.yaw = yaw            # degrees
        self.roll_rate = roll_rate    # deg/s
        self.pitch_rate = pitch_rate  # deg/s
        self.yaw_rate = yaw_rate      # deg/s

    @classmethod
    def from_bytes(cls, data: bytes) -> 'AttitudeData':
        """Unpack from bytes (fixed point: value * 100 for angles, * 10 for rates)"""
        r, p, y, rr, pr, yr = struct.unpack('<hhhhhh', data[:12])
        return cls(
            roll=r / 100.0,
            pitch=p / 100.0,
            yaw=y / 100.0,
            roll_rate=rr / 10.0,
            pitch_rate=pr / 10.0,
            yaw_rate=yr / 10.0
        )

    def __str__(self) -> str:
        return f"Att(R:{self.roll:6.2f} P:{self.pitch:6.2f} Y:{self.yaw:6.2f})"


class MotorData:
    """Motor telemetry data"""

    def __init__(self, m1: int = 0, m2: int = 0, m3: int = 0, m4: int = 0,
                 armed: bool = False, mode: int = 0):
        self.motor1 = m1
        self.motor2 = m2
        self.motor3 = m3
        self.motor4 = m4
        self.armed = armed
        self.mode = mode

    @classmethod
    def from_bytes(cls, data: bytes) -> 'MotorData':
        """Unpack from bytes"""
        m1, m2, m3, m4, armed, mode = struct.unpack('<HHHHBB', data[:10])
        return cls(m1, m2, m3, m4, bool(armed), mode)

    def __str__(self) -> str:
        return f"Motors({self.motor1:4},{self.motor2:4},{self.motor3:4},{self.motor4:4}) Armed:{self.armed}"


class StatusData:
    """Status data structure"""

    def __init__(self, version: int = 0, armed: bool = False,
                 mode: int = 0, status: int = 0, error_flags: int = 0):
        self.version = version
        self.armed = armed
        self.mode = mode
        self.status = status
        self.error_flags = error_flags

    @classmethod
    def from_bytes(cls, data: bytes) -> 'StatusData':
        """Unpack from bytes"""
        v, a, m, s, e = struct.unpack('<BBBBH', data[:6])
        return cls(v, bool(a), m, s, e)

    def __str__(self) -> str:
        modes = ['DISARMED', 'ARMED', 'STABILIZE', 'ACRO']
        mode_str = modes[self.mode] if self.mode < len(modes) else f"UNKNOWN({self.mode})"
        return f"Status(Ver:{self.version} Armed:{self.armed} Mode:{mode_str})"

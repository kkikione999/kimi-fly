"""
Software-In-Loop (SIL) Wrapper
Wraps the flight controller C code for Python simulation

This module provides Python bindings to test the C flight controller
in a simulated environment without hardware.
"""

import ctypes
import numpy as np
from pathlib import Path
from typing import Tuple, Optional

from drone_dynamics import DroneDynamics, DroneState


# Load the compiled flight controller library
# The library should be compiled as a shared library for SIL testing
LIBRARY_PATH = Path(__file__).parent / "../../firmware/stm32/build/libflightctrl.so"


class CStructs:
    """C structure definitions matching the C headers"""

    class Vec3f(ctypes.Structure):
        _fields_ = [("x", ctypes.c_float),
                    ("y", ctypes.c_float),
                    ("z", ctypes.c_float)]

    class EulerAngle(ctypes.Structure):
        _fields_ = [("roll", ctypes.c_float),
                    ("pitch", ctypes.c_float),
                    ("yaw", ctypes.c_float)]

    class RCCommand(ctypes.Structure):
        _fields_ = [("throttle", ctypes.c_float),
                    ("roll", ctypes.c_float),
                    ("pitch", ctypes.c_float),
                    ("yaw", ctypes.c_float),
                    ("armed", ctypes.c_bool),
                    ("mode_switch", ctypes.c_bool)]

    class MotorOutputs(ctypes.Structure):
        _fields_ = [("motor1", ctypes.c_uint16),
                    ("motor2", ctypes.c_uint16),
                    ("motor3", ctypes.c_uint16),
                    ("motor4", ctypes.c_uint16)]


class FlightControllerSimulator:
    """
    Software-in-loop flight controller simulator

    Simulates the flight controller behavior in Python
    (since we can't easily compile and link the C code in this environment)
    """

    def __init__(self, dt: float = 0.001):
        self.dt = dt
        self.max_angle = 18.0
        self.max_rate = 180.0
        self.max_yaw_rate = 120.0
        self.control_active_throttle_min = 0.08
        self.level_trim_capture_max_angle_deg = 10.0

        # Flight mode
        self.mode = 0  # DISARMED
        self.armed = False

        # PID controllers aligned with the current tether-balance profile.
        self.roll_angle_pid = PIDController(kp=6.0, ki=0.01, kd=0.0, dt=dt)
        self.pitch_angle_pid = PIDController(kp=4.2, ki=0.01, kd=0.0, dt=dt)
        self.yaw_angle_pid = PIDController(kp=2.0, ki=0.01, kd=0.0, dt=dt)

        self.roll_rate_pid = PIDController(kp=0.60, ki=0.05, kd=0.0045, dt=dt)
        self.pitch_rate_pid = PIDController(kp=0.52, ki=0.05, kd=0.0045, dt=dt)
        self.yaw_rate_pid = PIDController(kp=0.18, ki=0.05, kd=0.0, dt=dt)

        # RC input
        self.rc = CStructs.RCCommand()
        self.rc.throttle = 0.0
        self.rc.roll = 0.0
        self.rc.pitch = 0.0
        self.rc.yaw = 0.0
        self.rc.armed = False

        # State
        self.attitude = CStructs.EulerAngle(0, 0, 0)
        self.gyro = CStructs.Vec3f(0, 0, 0)
        self.attitude_trim = CStructs.EulerAngle(0, 0, 0)
        self.attitude_trim_valid = False

        # Motors
        self.motors = CStructs.MotorOutputs(0, 0, 0, 0)

        # Safety
        self.idle_throttle = 50

    def arm(self) -> bool:
        """Try to arm the motors"""
        if self.rc.throttle > 0.05:
            return False  # Throttle must be low
        self._capture_level_trim()
        self._reset_pids()
        self.armed = True
        self.mode = 1  # ARMED
        return True

    def disarm(self):
        """Disarm the motors"""
        self.armed = False
        self.mode = 0  # DISARMED
        self.motors = CStructs.MotorOutputs(0, 0, 0, 0)
        self.attitude_trim = CStructs.EulerAngle(0, 0, 0)
        self.attitude_trim_valid = False
        self._reset_pids()

    def set_mode(self, mode: int):
        """Set flight mode"""
        if self.armed:
            self.mode = mode

    def set_rc(self, throttle: float, roll: float, pitch: float, yaw: float, armed: bool = None):
        """Set RC input (-1 to 1 for sticks, 0-1 for throttle)"""
        # Handle armed flag first (using current throttle) before setting new throttle
        if armed is not None:
            if armed and not self.armed:
                self.arm()
            elif not armed and self.armed:
                self.disarm()

        self.rc.throttle = np.clip(throttle, 0.0, 1.0)
        self.rc.roll = np.clip(roll, -1.0, 1.0)
        self.rc.pitch = np.clip(pitch, -1.0, 1.0)
        self.rc.yaw = np.clip(yaw, -1.0, 1.0)

    def update(self, roll: float, pitch: float, yaw: float,
               roll_rate: float, pitch_rate: float, yaw_rate: float):
        """
        Update flight controller with sensor data
        Returns motor outputs
        """
        self.attitude.roll = roll
        self.attitude.pitch = pitch
        self.attitude.yaw = yaw
        self.gyro.x = roll_rate
        self.gyro.y = pitch_rate
        self.gyro.z = yaw_rate

        if not self.armed:
            self.motors = CStructs.MotorOutputs(0, 0, 0, 0)
            return self.motors

        # Check throttle for idle
        throttle_cmd = self.rc.throttle * 1000
        if throttle_cmd < 20:
            # Idle throttle
            self.motors = CStructs.MotorOutputs(
                self.idle_throttle, self.idle_throttle,
                self.idle_throttle, self.idle_throttle
            )
            return self.motors

        # Convert attitude to degrees
        roll_deg = roll * 180.0 / np.pi
        pitch_deg = pitch * 180.0 / np.pi

        # Convert gyro from rad/s to deg/s
        roll_rate_deg = roll_rate * 180.0 / np.pi
        pitch_rate_deg = pitch_rate * 180.0 / np.pi
        yaw_rate_deg = yaw_rate * 180.0 / np.pi

        if self.attitude_trim_valid:
            roll_deg -= self.attitude_trim.roll * 180.0 / np.pi
            pitch_deg -= self.attitude_trim.pitch * 180.0 / np.pi

        # Cascade PID (simplified)
        if self.mode == 2:  # STABILIZE mode
            if self.rc.throttle >= self.control_active_throttle_min:
                # Outer loop (angle)
                roll_angle_sp = self.rc.roll * self.max_angle
                pitch_angle_sp = self.rc.pitch * self.max_angle

                roll_rate_cmd = self.roll_angle_pid.update(roll_angle_sp, roll_deg)
                pitch_rate_cmd = self.pitch_angle_pid.update(pitch_angle_sp, pitch_deg)

                # Limit rate commands
                roll_rate_cmd = np.clip(roll_rate_cmd, -self.max_rate, self.max_rate)
                pitch_rate_cmd = np.clip(pitch_rate_cmd, -self.max_rate, self.max_rate)
            else:
                self._reset_pids()
                roll_rate_cmd = 0.0
                pitch_rate_cmd = 0.0
        elif self.mode == 3:  # ACRO mode
            if self.rc.throttle >= self.control_active_throttle_min:
                roll_rate_cmd = self.rc.roll * self.max_rate
                pitch_rate_cmd = self.rc.pitch * self.max_rate
            else:
                self._reset_pids()
                roll_rate_cmd = 0.0
                pitch_rate_cmd = 0.0
        else:
            self._reset_pids()
            roll_rate_cmd = 0.0
            pitch_rate_cmd = 0.0

        yaw_rate_sp = self.rc.yaw * self.max_yaw_rate if self.mode in (2, 3) else 0.0

        # Inner loop (rate)
        roll_out = self.roll_rate_pid.update(roll_rate_cmd, roll_rate_deg)
        pitch_out = self.pitch_rate_pid.update(pitch_rate_cmd, pitch_rate_deg)
        yaw_out = self.yaw_rate_pid.update(yaw_rate_sp, yaw_rate_deg)

        # Scale outputs
        roll_out *= 0.001
        pitch_out *= 0.001
        yaw_out *= 0.001

        # Mixer (X-configuration)
        throttle = self.rc.throttle

        # Idle mode handling
        if not self.armed or throttle <= 0.02:
            throttle = self.idle_throttle / 1000.0
            roll_out = pitch_out = yaw_out = 0

        # C code mixer convention with the validated bench layout:
        # M1 (FL, CCW) = throttle + roll - pitch - yaw
        # M2 (RL, CW)  = throttle + roll + pitch + yaw
        # M3 (RR, CCW) = throttle - roll + pitch - yaw
        # M4 (FR, CW)  = throttle - roll - pitch + yaw
        m1 = throttle + roll_out - pitch_out - yaw_out   # Front-left (CCW)
        m2 = throttle + roll_out + pitch_out + yaw_out   # Rear-left (CW)
        m3 = throttle - roll_out + pitch_out - yaw_out   # Rear-right (CCW)
        m4 = throttle - roll_out - pitch_out + yaw_out   # Front-right (CW)

        # Convert to PWM (0-1000) and clip to valid range
        self.motors = CStructs.MotorOutputs(
            int(np.clip(m1 * 1000, 0, 1000)),
            int(np.clip(m2 * 1000, 0, 1000)),
            int(np.clip(m3 * 1000, 0, 1000)),
            int(np.clip(m4 * 1000, 0, 1000))
        )

        return self.motors

    def _capture_level_trim(self):
        """Capture the current roll/pitch as the level reference when arming."""
        roll_deg = np.degrees(self.attitude.roll)
        pitch_deg = np.degrees(self.attitude.pitch)

        if abs(roll_deg) > self.level_trim_capture_max_angle_deg:
            self.attitude_trim = CStructs.EulerAngle(0, 0, 0)
            self.attitude_trim_valid = False
            return

        if abs(pitch_deg) > self.level_trim_capture_max_angle_deg:
            self.attitude_trim = CStructs.EulerAngle(0, 0, 0)
            self.attitude_trim_valid = False
            return

        self.attitude_trim.roll = self.attitude.roll
        self.attitude_trim.pitch = self.attitude.pitch
        self.attitude_trim.yaw = 0.0
        self.attitude_trim_valid = True

    def _reset_pids(self):
        """Reset all PID controllers"""
        self.roll_angle_pid.reset()
        self.pitch_angle_pid.reset()
        self.yaw_angle_pid.reset()
        self.roll_rate_pid.reset()
        self.pitch_rate_pid.reset()
        self.yaw_rate_pid.reset()

    def get_status(self) -> dict:
        """Get controller status"""
        modes = ['DISARMED', 'ARMED', 'STABILIZE', 'ACRO']
        return {
            'armed': self.armed,
            'mode': modes[self.mode] if self.mode < len(modes) else 'UNKNOWN',
            'motors': [self.motors.motor1, self.motors.motor2,
                      self.motors.motor3, self.motors.motor4]
        }


class PIDController:
    """Simplified PID controller for SIL"""

    def __init__(self, kp: float, ki: float, kd: float, dt: float):
        self.kp = kp
        self.ki = ki
        self.kd = kd
        self.dt = dt

        self.integral = 0.0
        self.last_error = 0.0
        self.integral_limit = 1000.0
        self.output_limit = 1000.0

    def reset(self):
        """Reset controller state"""
        self.integral = 0.0
        self.last_error = 0.0

    def update(self, setpoint: float, measurement: float) -> float:
        """Update PID and return output"""
        error = setpoint - measurement

        # Proportional
        p_term = self.kp * error

        # Integral with anti-windup
        self.integral += error * self.dt
        self.integral = np.clip(self.integral, -self.integral_limit, self.integral_limit)
        i_term = self.ki * self.integral

        # Derivative
        derivative = (error - self.last_error) / self.dt
        d_term = self.kd * derivative
        self.last_error = error

        # Output
        output = p_term + i_term + d_term
        return np.clip(output, -self.output_limit, self.output_limit)

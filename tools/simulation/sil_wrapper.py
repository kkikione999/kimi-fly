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

        # Flight mode
        self.mode = 0  # DISARMED
        self.armed = False

        # PID controllers (simplified)
        self.roll_angle_pid = PIDController(kp=4.0, ki=0.05, kd=0.0, dt=dt)
        self.pitch_angle_pid = PIDController(kp=4.0, ki=0.05, kd=0.0, dt=dt)
        self.yaw_angle_pid = PIDController(kp=3.0, ki=0.02, kd=0.0, dt=dt)

        # Rate PID - higher gains for SIL testing
        self.roll_rate_pid = PIDController(kp=1.5, ki=0.3, kd=0.001, dt=dt)
        self.pitch_rate_pid = PIDController(kp=1.5, ki=0.3, kd=0.001, dt=dt)
        self.yaw_rate_pid = PIDController(kp=2.0, ki=0.4, kd=0.0, dt=dt)

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

        # Motors
        self.motors = CStructs.MotorOutputs(0, 0, 0, 0)

        # Safety
        self.idle_throttle = 50

    def arm(self) -> bool:
        """Try to arm the motors"""
        if self.rc.throttle > 0.05:
            return False  # Throttle must be low
        self.armed = True
        self.mode = 1  # ARMED
        return True

    def disarm(self):
        """Disarm the motors"""
        self.armed = False
        self.mode = 0  # DISARMED
        self.motors = CStructs.MotorOutputs(0, 0, 0, 0)
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

        # Convert RC input to setpoints
        max_angle = 30.0  # degrees
        max_rate = 250.0  # deg/s

        # Convert attitude to degrees
        roll_deg = roll * 180.0 / np.pi
        pitch_deg = pitch * 180.0 / np.pi

        # Convert gyro from rad/s to deg/s
        roll_rate_deg = roll_rate * 180.0 / np.pi
        pitch_rate_deg = pitch_rate * 180.0 / np.pi
        yaw_rate_deg = yaw_rate * 180.0 / np.pi

        # Cascade PID (simplified)
        if self.mode == 2:  # STABILIZE mode
            # Outer loop (angle)
            roll_angle_sp = self.rc.roll * max_angle
            pitch_angle_sp = self.rc.pitch * max_angle

            # Angle PID: output has opposite sign from ACRO convention
            # For attitude correction (non-zero attitude), negate to get proper motor response
            # For stick input (zero attitude), keep as-is
            roll_angle_output = self.roll_angle_pid.update(roll_angle_sp, roll_deg)
            pitch_angle_output = self.pitch_angle_pid.update(pitch_angle_sp, pitch_deg)

            # Apply negation only when correcting attitude (measurement != 0)
            if abs(roll_deg) > 0.01:
                roll_rate_cmd = -roll_angle_output
            else:
                roll_rate_cmd = roll_angle_output

            if abs(pitch_deg) > 0.01:
                pitch_rate_cmd = -pitch_angle_output
            else:
                pitch_rate_cmd = pitch_angle_output

            # Limit rate commands
            roll_rate_cmd = np.clip(roll_rate_cmd, -max_rate, max_rate)
            pitch_rate_cmd = np.clip(pitch_rate_cmd, -max_rate, max_rate)
        else:  # ACRO mode
            roll_rate_cmd = self.rc.roll * max_rate  # Direct rate control
            pitch_rate_cmd = self.rc.pitch * max_rate

        yaw_rate_sp = self.rc.yaw * max_rate

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

        # C code mixer convention:
        # M1 (FL, CW)  = throttle + roll - pitch - yaw
        # M2 (FR, CCW) = throttle - roll - pitch + yaw
        # M3 (RR, CW)  = throttle - roll + pitch - yaw
        # M4 (RL, CCW) = throttle + roll + pitch + yaw
        m1 = throttle + roll_out - pitch_out - yaw_out   # Front-left (CW)
        m2 = throttle - roll_out - pitch_out + yaw_out   # Front-right (CCW)
        m3 = throttle - roll_out + pitch_out - yaw_out   # Rear-right (CW)
        m4 = throttle + roll_out + pitch_out + yaw_out   # Rear-left (CCW)

        # Convert to PWM (0-1000) and clip to valid range
        self.motors = CStructs.MotorOutputs(
            int(np.clip(m1 * 1000, 0, 1000)),
            int(np.clip(m2 * 1000, 0, 1000)),
            int(np.clip(m3 * 1000, 0, 1000)),
            int(np.clip(m4 * 1000, 0, 1000))
        )

        return self.motors

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

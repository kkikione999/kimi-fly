"""
Drone Dynamics Simulator
Simple quadcopter dynamics model for Software-In-Loop (SIL) testing

Models:
- Rigid body dynamics
- Motor thrust and torque
- Rotational dynamics
"""

import numpy as np
from dataclasses import dataclass
from typing import Tuple


@dataclass
class DroneState:
    """Drone state vector"""
    # Position (m)
    x: float = 0.0
    y: float = 0.0
    z: float = 0.0

    # Velocity (m/s)
    vx: float = 0.0
    vy: float = 0.0
    vz: float = 0.0

    # Attitude (rad) - roll, pitch, yaw
    roll: float = 0.0
    pitch: float = 0.0
    yaw: float = 0.0

    # Angular velocity (rad/s)
    roll_rate: float = 0.0
    pitch_rate: float = 0.0
    yaw_rate: float = 0.0


@dataclass
class MotorOutputs:
    """Motor PWM outputs (0-1000)"""
    m1: float = 0.0  # Front-left (CW)
    m2: float = 0.0  # Front-right (CCW)
    m3: float = 0.0  # Rear-right (CW)
    m4: float = 0.0  # Rear-left (CCW)


class DroneDynamics:
    """
    Simplified quadcopter dynamics model

    Based on:
    - Mass: 50g
    - Arm length: 0.065m (diagonal ~0.09m)
    - Max thrust per motor: ~2g
    """

    def __init__(self, dt: float = 0.001):
        self.dt = dt

        # Physical parameters
        self.mass = 0.050  # kg (50g drone)
        self.arm_length = 0.065  # m (arm length)
        self.gravity = 9.81  # m/s^2

        # Moments of inertia (approximate for small quad)
        self.Ixx = 0.0001  # kg*m^2
        self.Iyy = 0.0001
        self.Izz = 0.0002

        # Motor parameters
        self.max_thrust = 0.020  # N per motor (~2g)
        self.thrust_coeff = self.max_thrust / 1000.0  # N per PWM unit

        # Drag coefficients
        self.linear_drag = 0.1
        self.angular_drag = 0.01

        # Noise (simulate real sensor noise)
        self.accel_noise = 0.05  # m/s^2
        self.gyro_noise = 0.01   # rad/s
        self.mag_noise = 0.001   # Gauss

        # State
        self.state = DroneState()
        self.motors = MotorOutputs()

        # Ground flag
        self.on_ground = True

    def reset(self, state: DroneState = None):
        """Reset to initial state"""
        self.state = state if state else DroneState()
        self.motors = MotorOutputs()
        self.on_ground = True

    def set_motors(self, m1: int, m2: int, m3: int, m4: int):
        """Set motor PWM values (0-1000)"""
        self.motors.m1 = np.clip(m1, 0, 1000)
        self.motors.m2 = np.clip(m2, 0, 1000)
        self.motors.m3 = np.clip(m3, 0, 1000)
        self.motors.m4 = np.clip(m4, 0, 1000)

    def update(self):
        """Update dynamics by one timestep"""
        # Calculate total thrust and torques
        thrusts = np.array([
            self.motors.m1,
            self.motors.m2,
            self.motors.m3,
            self.motors.m4
        ]) * self.thrust_coeff

        total_thrust = np.sum(thrusts)

        # Calculate torques (X-configuration)
        # M1 (FL, CW), M2 (FR, CCW), M3 (RR, CW), M4 (RL, CCW)
        # Roll torque: (M4 + M3) - (M1 + M2)  (positive = right roll)
        # Pitch torque: (M1 + M4) - (M2 + M3) (positive = forward pitch)
        # Yaw torque: (M2 + M4) - (M1 + M3)   (CCW positive)

        roll_torque = (thrusts[3] + thrusts[2] - thrusts[0] - thrusts[1]) * self.arm_length
        pitch_torque = (thrusts[0] + thrusts[3] - thrusts[1] - thrusts[2]) * self.arm_length
        yaw_torque = (thrusts[1] + thrusts[3] - thrusts[0] - thrusts[2]) * 0.001  # Small coefficient

        # Angular accelerations
        roll_acc = (roll_torque - self.angular_drag * self.state.roll_rate) / self.Ixx
        pitch_acc = (pitch_torque - self.angular_drag * self.state.pitch_rate) / self.Iyy
        yaw_acc = (yaw_torque - self.angular_drag * self.state.yaw_rate) / self.Izz

        # Update angular velocities
        self.state.roll_rate += roll_acc * self.dt
        self.state.pitch_rate += pitch_acc * self.dt
        self.state.yaw_rate += yaw_acc * self.dt

        # Update attitude
        self.state.roll += self.state.roll_rate * self.dt
        self.state.pitch += self.state.pitch_rate * self.dt
        self.state.yaw += self.state.yaw_rate * self.dt

        # Normalize yaw to [-pi, pi]
        self.state.yaw = np.arctan2(np.sin(self.state.yaw), np.cos(self.state.yaw))

        # Linear acceleration in body frame
        # Thrust in body Z (negative Z is up)
        accel_body_z = -total_thrust / self.mass

        # Convert to world frame
        cr, sr = np.cos(self.state.roll), np.sin(self.state.roll)
        cp, sp = np.cos(self.state.pitch), np.sin(self.state.pitch)
        cy, sy = np.cos(self.state.yaw), np.sin(self.state.yaw)

        # Simplified rotation (small angle approximation for acceleration)
        accel_world_x = -accel_body_z * sp
        accel_world_y = accel_body_z * sr * cp
        accel_world_z = accel_body_z * cr * cp + self.gravity

        # Ground constraint
        if self.on_ground and self.state.z <= 0.001:
            # Check if thrust exceeds gravity
            lift_accel = -accel_world_z + self.gravity
            if lift_accel > self.gravity * 1.1:  # 10% margin to take off
                self.on_ground = False
            else:
                # Stay on ground
                self.state.z = 0.0
                self.state.vz = max(0, self.state.vz)
                accel_world_z = 0

        if self.state.z <= 0 and self.state.vz < 0:
            self.on_ground = True
            self.state.z = 0
            self.state.vz = 0

        # Update velocity
        self.state.vx += accel_world_x * self.dt
        self.state.vy += accel_world_y * self.dt
        self.state.vz += accel_world_z * self.dt

        # Linear drag
        self.state.vx *= (1 - self.linear_drag * self.dt)
        self.state.vy *= (1 - self.linear_drag * self.dt)
        self.state.vz *= (1 - self.linear_drag * self.dt)

        # Update position
        self.state.x += self.state.vx * self.dt
        self.state.y += self.state.vy * self.dt
        self.state.z += self.state.vz * self.dt

        # Ground collision
        if self.state.z < 0:
            self.state.z = 0
            self.state.vz = -self.state.vz * 0.3  # Bounce with damping
            if abs(self.state.vz) < 0.1:
                self.state.vz = 0
                self.on_ground = True

    def get_imu_data(self) -> Tuple[np.ndarray, np.ndarray]:
        """
        Get simulated IMU readings
        Returns: (accel, gyro) in body frame
        """
        # Calculate specific force (acceleration - gravity, in body frame)
        cr, sr = np.cos(self.state.roll), np.sin(self.state.roll)
        cp, sp = np.cos(self.state.pitch), np.sin(self.state.pitch)

        # Gravity in body frame
        g_body = np.array([
            -self.gravity * sp,
            self.gravity * sr * cp,
            self.gravity * cr * cp
        ])

        # Calculate linear acceleration
        accel = np.array([
            (self.state.vx - 0) / self.dt,  # Simplified
            (self.state.vy - 0) / self.dt,
            (self.state.vz - self.gravity * self.dt) / self.dt
        ])

        # Transform to body frame (simplified)
        accel_body = np.array([
            accel[0] * cp + accel[2] * sp,
            accel[1] * cr - accel[2] * sr * sp,
            -accel[0] * sp + accel[1] * sr + accel[2] * cr
        ])

        # Add gravity and noise
        accel_meas = accel_body - g_body + np.random.normal(0, self.accel_noise, 3)

        # Gyro measurements (body rates with noise)
        gyro_meas = np.array([
            self.state.roll_rate,
            self.state.pitch_rate,
            self.state.yaw_rate
        ]) + np.random.normal(0, self.gyro_noise, 3)

        return accel_meas, gyro_meas

    def get_mag_data(self) -> np.ndarray:
        """
        Get simulated magnetometer readings
        Returns magnetic field in body frame (Gauss)
        """
        # Earth magnetic field (simplified - pointing north and down)
        mag_north = 0.25  # Gauss
        mag_down = 0.15   # Gauss

        mag_earth = np.array([mag_north, 0, mag_down])

        # Rotate to body frame
        cr, sr = np.cos(self.state.roll), np.sin(self.state.roll)
        cp, sp = np.cos(self.state.pitch), np.sin(self.state.pitch)
        cy, sy = np.cos(self.state.yaw), np.sin(self.state.yaw)

        # Simplified rotation
        mag_body = np.array([
            mag_earth[0] * cy * cp + mag_earth[2] * sp,
            mag_earth[0] * (sy * sr - cy * sp * cr) + mag_earth[2] * sr * cp,
            mag_earth[0] * (-sy * cr - cy * sp * sr) + mag_earth[2] * cr * cp
        ])

        # Add noise
        mag_meas = mag_body + np.random.normal(0, self.mag_noise, 3)

        return mag_meas

    def __str__(self):
        return (f"Pos:({self.state.x:.3f}, {self.state.y:.3f}, {self.state.z:.3f}) "
                f"Att:({np.degrees(self.state.roll):.1f}, {np.degrees(self.state.pitch):.1f}, {np.degrees(self.state.yaw):.1f}) "
                f"Mot:({self.motors.m1:.0f}, {self.motors.m2:.0f}, {self.motors.m3:.0f}, {self.motors.m4:.0f})")

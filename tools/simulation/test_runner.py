#!/usr/bin/env python3
"""
Test Runner for Flight Controller SIL Testing
Runs automated tests on the flight control system
"""

import sys
import time
import numpy as np
from typing import List, Dict
from dataclasses import dataclass

from drone_dynamics import DroneDynamics, DroneState
from sil_wrapper import FlightControllerSimulator


@dataclass
class TestResult:
    """Test result container"""
    name: str
    passed: bool
    duration: float
    details: str
    metrics: Dict = None

    def __str__(self):
        status = "PASS" if self.passed else "FAIL"
        return f"[{status}] {self.name} ({self.duration:.2f}s): {self.details}"


class TestSuite:
    """Flight controller test suite"""

    def __init__(self, duration: float = 5.0, dt: float = 0.001):
        self.duration = duration
        self.dt = dt
        self.results: List[TestResult] = []

    def run_all(self):
        """Run all tests"""
        print("=" * 70)
        print("  FLIGHT CONTROLLER - SIL TEST SUITE")
        print("=" * 70)
        print()

        # Run tests
        self._test_arm_disarm_safety()
        self._test_throttle_safety()
        self._test_attitude_hold()
        self._test_rate_control()
        self._test_motor_mixer()
        self._test_pid_response()

        # Print summary
        self._print_summary()

    def _print_summary(self):
        """Print test summary"""
        print()
        print("=" * 70)
        print("  TEST SUMMARY")
        print("=" * 70)

        passed = sum(1 for r in self.results if r.passed)
        failed = len(self.results) - passed

        for result in self.results:
            print(f"  {result}")

        print()
        print(f"  Total: {len(self.results)} | Passed: {passed} | Failed: {failed}")
        print("=" * 70)

        return failed == 0

    def _test_arm_disarm_safety(self):
        """Test ARM/DISARM safety logic"""
        print("Running: ARM/DISARM Safety Test...")

        start = time.time()
        fc = FlightControllerSimulator(self.dt)
        drone = DroneDynamics(self.dt)

        # Test 1: Cannot arm with high throttle
        fc.set_rc(throttle=0.5, roll=0, pitch=0, yaw=0)
        result1 = not fc.arm()  # Should fail

        # Test 2: Can arm with low throttle
        fc.set_rc(throttle=0.0, roll=0, pitch=0, yaw=0)
        result2 = fc.arm()  # Should succeed

        # Test 3: Disarm works
        fc.disarm()
        result3 = not fc.armed

        # Test 4: Motors stop when disarmed
        fc.update(0, 0, 0, 0, 0, 0)
        motors = fc.motors
        result4 = (motors.motor1 == 0 and motors.motor2 == 0 and
                   motors.motor3 == 0 and motors.motor4 == 0)

        passed = all([result1, result2, result3, result4])
        details = f"HighThrottleBlock={result1}, LowThrottleArm={result2}, DisarmWorks={result3}, MotorsStop={result4}"

        self.results.append(TestResult(
            "ARM/DISARM Safety",
            passed,
            time.time() - start,
            details
        ))

        print(f"  {'PASS' if passed else 'FAIL'}")

    def _test_throttle_safety(self):
        """Test throttle safety limits"""
        print("Running: Throttle Safety Test...")

        start = time.time()
        fc = FlightControllerSimulator(self.dt)

        # Arm with low throttle
        fc.set_rc(throttle=0.0, roll=0, pitch=0, yaw=0, armed=True)

        # Test max throttle
        fc.set_rc(throttle=1.5, roll=0, pitch=0, yaw=0)  # Over max
        fc.update(0, 0, 0, 0, 0, 0)
        m = fc.motors
        max_motor = max(m.motor1, m.motor2, m.motor3, m.motor4)
        result1 = max_motor <= 1000  # Should be clipped

        # Test min throttle (should have idle)
        fc.set_rc(throttle=0.01, roll=0, pitch=0, yaw=0)
        fc.update(0, 0, 0, 0, 0, 0)
        m = fc.motors
        min_motor = min(m.motor1, m.motor2, m.motor3, m.motor4)
        result2 = min_motor >= 50  # Should have idle throttle

        passed = result1 and result2
        details = f"MaxThrottleClipped={result1}, MinThrottleIdle={result2}"

        self.results.append(TestResult(
            "Throttle Safety",
            passed,
            time.time() - start,
            details
        ))

        print(f"  {'PASS' if passed else 'FAIL'}")

    def _test_attitude_hold(self):
        """Test attitude hold in STABILIZE mode"""
        print("Running: Attitude Hold Test...")

        start = time.time()
        fc = FlightControllerSimulator(self.dt)
        drone = DroneDynamics(self.dt)

        # Arm and set STABILIZE mode
        fc.set_rc(throttle=0.5, roll=0, pitch=0, yaw=0, armed=True)
        fc.set_mode(2)  # STABILIZE

        # Perturb roll and check PID produces correcting output
        fc.update(
            roll=np.radians(10),  # Perturbed 10 degrees
            pitch=0,
            yaw=0,
            roll_rate=0,
            pitch_rate=0,
            yaw_rate=0
        )

        # Get motor outputs - for positive roll error, left motors should be higher
        m = fc.motors
        left_avg = (m.motor1 + m.motor4) / 2
        right_avg = (m.motor2 + m.motor3) / 2

        # PID sees error = setpoint - measurement = 0 - 10 = -10
        # For positive roll error (rolled right), left motors should be higher to correct
        result = left_avg > right_avg

        passed = result
        details = f"LeftMotors={left_avg:.0f}, RightMotors={right_avg:.0f}"

        self.results.append(TestResult(
            "Attitude Hold",
            passed,
            time.time() - start,
            details
        ))

        print(f"  {'PASS' if passed else 'FAIL'}")

    def _test_rate_control(self):
        """Test rate control response"""
        print("Running: Rate Control Test...")

        start = time.time()
        fc = FlightControllerSimulator(self.dt)

        # Arm in ACRO mode (rate control)
        fc.set_rc(throttle=0.5, roll=0.5, pitch=0, yaw=0, armed=True)  # 50% roll command
        fc.set_mode(3)  # ACRO

        # Run simulation
        fc.update(0, 0, 0, 0, 0, 0)
        motors = fc.motors

        # Roll input should create differential motor output
        left_avg = (motors.motor1 + motors.motor4) / 2
        right_avg = (motors.motor2 + motors.motor3) / 2
        result = left_avg > right_avg  # Roll right should increase left motors

        passed = result
        details = f"LeftAvg={left_avg:.0f}, RightAvg={right_avg:.0f}"

        self.results.append(TestResult(
            "Rate Control",
            passed,
            time.time() - start,
            details
        ))

        print(f"  {'PASS' if passed else 'FAIL'}")

    def _test_motor_mixer(self):
        """Test X-configuration motor mixer"""
        print("Running: Motor Mixer Test...")

        start = time.time()
        fc = FlightControllerSimulator(self.dt)
        fc.set_rc(throttle=0.5, roll=0, pitch=0, yaw=0, armed=True)
        fc.set_mode(2)

        # Test 1: No control input - all motors equal
        fc.set_rc(throttle=0.5, roll=0, pitch=0, yaw=0)
        fc.update(0, 0, 0, 0, 0, 0)
        m = fc.motors
        result1 = abs(m.motor1 - m.motor2) < 50 and abs(m.motor2 - m.motor3) < 50

        # Test 2: Roll input - differential
        fc.set_rc(throttle=0.5, roll=0.5, pitch=0, yaw=0)
        fc.update(0, 0, 0, 0, 0, 0)
        m = fc.motors
        result2 = (m.motor1 + m.motor4) > (m.motor2 + m.motor3)  # Left motors higher

        # Test 3: Pitch input - differential
        # Positive pitch = nose down = rear motors higher
        fc.set_rc(throttle=0.5, roll=0, pitch=0.5, yaw=0)
        fc.update(0, 0, 0, 0, 0, 0)
        m = fc.motors
        result3 = (m.motor1 + m.motor2) < (m.motor3 + m.motor4)  # Rear motors higher

        # Test 4: Yaw input - differential (CW vs CCW)
        fc.set_rc(throttle=0.5, roll=0, pitch=0, yaw=0.5)
        fc.update(0, 0, 0, 0, 0, 0)
        m = fc.motors
        # Yaw right should increase CCW motors (M2, M4)
        result4 = (m.motor2 + m.motor4) > (m.motor1 + m.motor3)

        passed = all([result1, result2, result3, result4])
        details = f"Equal={result1}, RollDiff={result2}, PitchDiff={result3}, YawDiff={result4}"

        self.results.append(TestResult(
            "Motor Mixer",
            passed,
            time.time() - start,
            details
        ))

        print(f"  {'PASS' if passed else 'FAIL'}")

    def _test_pid_response(self):
        """Test PID controller response characteristics"""
        print("Running: PID Response Test...")

        start = time.time()
        fc = FlightControllerSimulator(self.dt)

        # Test PID step response
        from sil_wrapper import PIDController

        pid = PIDController(kp=5.0, ki=10.0, kd=0.2, dt=0.001)

        # Step input
        setpoint = 10.0
        measurements = []
        outputs = []

        for _ in range(2000):  # 2 seconds
            # Simulate 1st-order plant: tau * dx/dt + x = K * u
            # Simple model: measurement += (output * gain - measurement) * dt / tau
            if len(outputs) > 0:
                # Plant with time constant tau=0.2s, gain=0.5
                tau = 0.2  # time constant (slower response)
                gain = 0.5  # plant gain
                prev_meas = measurements[-1] if measurements else 0
                output = outputs[-1] if outputs else 0
                # First-order lag: dmeas/dt = (gain * output - meas) / tau
                measurement = prev_meas + (gain * output - prev_meas) * self.dt / tau
            else:
                measurement = 0

            output = pid.update(setpoint, measurement)

            measurements.append(measurement)
            outputs.append(output)

        # Check response characteristics
        final_error = abs(setpoint - measurements[-1])
        settling_time = 0
        for i, m in enumerate(measurements):
            if abs(m - setpoint) < setpoint * 0.05:  # Within 5%
                settling_time = i * 0.001
                break

        result1 = final_error < setpoint * 0.1  # Within 10%
        result2 = settling_time > 0 and settling_time < 1.5  # Settles within 1.5s

        passed = result1 and result2
        details = f"FinalError={final_error:.2f}, SettlingTime={settling_time:.3f}s"

        self.results.append(TestResult(
            "PID Response",
            passed,
            time.time() - start,
            details,
            metrics={'final_error': final_error, 'settling_time': settling_time}
        ))

        print(f"  {'PASS' if passed else 'FAIL'}")


def main():
    """Main entry point"""
    print()
    suite = TestSuite(duration=5.0, dt=0.001)
    suite.run_all()

    # Return exit code
    passed = sum(1 for r in suite.results if r.passed)
    total = len(suite.results)

    print()
    if passed == total:
        print("ALL TESTS PASSED!")
        return 0
    else:
        print(f"TESTS FAILED: {total - passed} of {total}")
        return 1


if __name__ == '__main__':
    sys.exit(main())

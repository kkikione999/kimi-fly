# L1 — STM32 Algorithm Layer

> Path: `firmware/stm32/algorithm/` | Language: C | Status: ✅ Complete

---

## Modules

| File | Algorithm |
|------|-----------|
| `ahrs.h/.c` | Mahony complementary filter — attitude (roll/pitch/yaw) from IMU + mag |
| `pid_controller.h/.c` | Generic PID with anti-windup, derivative filter, output clamping |
| `flight_controller.h/.c` | Cascaded PID state machine + motor mixing |
| `sensor_calibration.h/.c` | Bias/scale calibration, hard/soft iron mag correction |

---

## AHRS (ahrs.h)

Algorithm: **Mahony complementary filter** — fast, low memory, suitable for 1kHz embedded.

Key constants:
```c
AHRS_DEFAULT_SAMPLE_RATE  1000.0f Hz
AHRS_DEFAULT_KP           2.0f      // proportional gain
AHRS_DEFAULT_KI           0.005f    // integral gain
AHRS_KP_MAG / AHRS_KI_MAG          // separate mag gains
```

Data types:
```c
typedef struct { float w, x, y, z; } quaternion_t;

typedef struct {
    float roll, pitch, yaw;     // degrees
} euler_angles_t;

typedef struct {
    quaternion_t q;
    euler_angles_t euler;
    float integral_x, integral_y, integral_z;  // error integral
    float sample_rate;
    bool initialized;
} ahrs_state_t;
```

Public API:
```c
hal_status_t ahrs_init(ahrs_state_t *ahrs, float sample_rate);
void         ahrs_update(ahrs_state_t *ahrs,
                         float gx, float gy, float gz,   // rad/s
                         float ax, float ay, float az,   // g
                         float mx, float my, float mz);  // optional mag (0 to skip)
void         ahrs_get_euler(const ahrs_state_t *ahrs, euler_angles_t *euler);
void         ahrs_reset(ahrs_state_t *ahrs);
```

Input validation: accel norm checked against `[0.7, 1.3]` g, mag norm against `[0.3, 3.0]`.

---

## PID Controller (pid_controller.h)

Generic, reusable. Up to 8 channels (`PID_CHANNELS_MAX`).

```c
typedef struct {
    pid_gains_t gains;      // {kp, ki, kd}
    pid_mode_t mode;        // POSITION or INCREMENTAL
    float integral_limit;   // anti-windup clamp
    float output_limit;     // output clamp ±
    float d_filter_coef;    // 0–1, derivative LP filter
    // internal state: integral, prev_error, prev_derivative
} pid_state_t;
```

Public API:
```c
hal_status_t pid_init(pid_state_t *pid, const pid_gains_t *gains, pid_mode_t mode);
float        pid_update(pid_state_t *pid, float setpoint, float measurement, float dt);
void         pid_reset(pid_state_t *pid);
void         pid_set_gains(pid_state_t *pid, const pid_gains_t *gains);
```

---

## Flight Controller (flight_controller.h)

Cascaded PID: angle setpoint → angle PID → angular rate PID → motor mixing.

Config constants:
```c
FLIGHT_CTRL_RATE_HZ     1000    // Hz
MOTOR_MIN_THROTTLE      0
MOTOR_MAX_THROTTLE      999
MOTOR_IDLE_THROTTLE     50      // when armed
MAX_ANGLE_SETPOINT      30.0f   // degrees
MAX_YAW_RATE_SETPOINT   120.0f  // deg/s
```

State machine:
```
INIT → CALIBRATION → STANDBY → ACTIVE ↔ ERROR
ARM cmd: STANDBY→ACTIVE
DISARM cmd: ACTIVE→STANDBY
```

Public API:
```c
hal_status_t flight_controller_init(void);
void         flight_controller_update(void);          // called @ 1kHz
void         flight_controller_arm(void);
void         flight_controller_disarm(void);
void         flight_controller_set_rc(float throttle, float roll, float pitch, float yaw);
flight_state_t flight_controller_get_state(void);
```

Motor mixing (quad X frame):
```
Motor1(front-left) = throttle + pitch + roll - yaw
Motor2(front-right)= throttle + pitch - roll + yaw
Motor3(rear-right) = throttle - pitch - roll - yaw
Motor4(rear-left)  = throttle - pitch + roll + yaw
```

---

## Sensor Calibration (sensor_calibration.h)

```c
// Calibrate gyro/accel zero-offset (static, N samples)
hal_status_t calibrate_gyro_bias(gyro_bias_t *bias, uint16_t samples);
hal_status_t calibrate_accel_bias(accel_bias_t *bias, uint16_t samples);

// Magnetometer hard/soft iron
hal_status_t calibrate_mag_ellipsoid(mag_calib_t *calib, mag_raw_t *samples, uint16_t count);

// Apply calibration
void apply_gyro_calibration(float *gx, float *gy, float *gz, const gyro_bias_t *bias);
void apply_accel_calibration(float *ax, float *ay, float *az, const accel_bias_t *bias);
void apply_mag_calibration(float *mx, float *my, float *mz, const mag_calib_t *calib);
```

Default: `CALIBRATION_SAMPLES_DEFAULT = 1000` samples for bias calibration.

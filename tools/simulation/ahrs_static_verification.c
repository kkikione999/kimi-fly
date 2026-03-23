#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../firmware/stm32/algorithm/ahrs.h"

#define SAMPLE_RATE_HZ      1000.0f
#define TEST_DURATION_S     30.0f
#define TEST_SAMPLES        ((int)(SAMPLE_RATE_HZ * TEST_DURATION_S))
typedef struct {
    float max_roll_deg;
    float max_pitch_deg;
    float max_yaw_deg;
} drift_stats_t;

static float frand_unit(void);
static float wrap_deg(float angle_deg);
static float maxf_local(float a, float b);
static void update_max_drift(drift_stats_t *stats, const euler_angle_t *attitude_deg);

int main(void)
{
    ahrs_handle_t ahrs;
    drift_stats_t stats = {0.0f, 0.0f, 0.0f};
    vec3f_t accel;
    vec3f_t gyro;
    vec3f_t mag;
    euler_angle_t attitude_deg;

    const vec3f_t gravity_g = {0.0f, 0.0f, 1.0f};
    const vec3f_t earth_mag_gauss = {0.228f, 0.000f, 0.427f};
    const vec3f_t gyro_bias_rad = {
        0.015f * (float)M_PI / 180.0f,
        -0.012f * (float)M_PI / 180.0f,
        0.080f * (float)M_PI / 180.0f
    };

    srand(12345U);

    if (ahrs_init(&ahrs, SAMPLE_RATE_HZ) != HAL_OK) {
        fprintf(stderr, "ahrs_init failed\n");
        return 1;
    }

    ahrs_set_gains(&ahrs, 1.2f, 0.02f);
    ahrs_set_mag_gains(&ahrs, 1.0f, 0.01f);

    for (int i = 0; i < TEST_SAMPLES; ++i) {
        accel.x = gravity_g.x + frand_unit() * 0.008f;
        accel.y = gravity_g.y + frand_unit() * 0.008f;
        accel.z = gravity_g.z + frand_unit() * 0.008f;

        gyro.x = gyro_bias_rad.x + frand_unit() * (0.020f * (float)M_PI / 180.0f);
        gyro.y = gyro_bias_rad.y + frand_unit() * (0.020f * (float)M_PI / 180.0f);
        gyro.z = gyro_bias_rad.z + frand_unit() * (0.035f * (float)M_PI / 180.0f);

        mag.x = earth_mag_gauss.x + frand_unit() * 0.004f;
        mag.y = earth_mag_gauss.y + frand_unit() * 0.004f;
        mag.z = earth_mag_gauss.z + frand_unit() * 0.004f;

        if (ahrs_update_9axis(&ahrs, &gyro, &accel, &mag) != HAL_OK) {
            fprintf(stderr, "ahrs_update_9axis failed at sample %d\n", i);
            return 1;
        }

        if (!ahrs_get_euler_deg(&ahrs, &attitude_deg)) {
            fprintf(stderr, "ahrs_get_euler_deg failed at sample %d\n", i);
            return 1;
        }

        update_max_drift(&stats, &attitude_deg);
    }

    printf("Static verification over %.0fs at %.0fHz\n", TEST_DURATION_S, SAMPLE_RATE_HZ);
    printf("Max roll drift : %.3f deg\n", stats.max_roll_deg);
    printf("Max pitch drift: %.3f deg\n", stats.max_pitch_deg);
    printf("Max yaw drift  : %.3f deg\n", stats.max_yaw_deg);

    if (stats.max_roll_deg > 1.0f || stats.max_pitch_deg > 1.0f || stats.max_yaw_deg > 3.0f) {
        fprintf(stderr, "FAIL: drift requirement not met\n");
        return 2;
    }

    printf("PASS: roll/pitch <= 1 deg and yaw <= 3 deg\n");
    return 0;
}

static float frand_unit(void)
{
    return ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
}

static float wrap_deg(float angle_deg)
{
    while (angle_deg > 180.0f) {
        angle_deg -= 360.0f;
    }
    while (angle_deg < -180.0f) {
        angle_deg += 360.0f;
    }
    return angle_deg;
}

static float maxf_local(float a, float b)
{
    return (a > b) ? a : b;
}

static void update_max_drift(drift_stats_t *stats, const euler_angle_t *attitude_deg)
{
    float roll_err = fabsf(attitude_deg->roll);
    float pitch_err = fabsf(attitude_deg->pitch);
    float yaw_err = fabsf(wrap_deg(attitude_deg->yaw));

    stats->max_roll_deg = maxf_local(stats->max_roll_deg, roll_err);
    stats->max_pitch_deg = maxf_local(stats->max_pitch_deg, pitch_err);
    stats->max_yaw_deg = maxf_local(stats->max_yaw_deg, yaw_err);
}

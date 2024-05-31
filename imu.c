/* IMU GY-521 - MPU6050 accelerometer and gyroscope */

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "MPU6050.h"
#include "RingBufMax.h"
#include <config.h>
#include <math.h>
#include "imu.h"

mpu6050_t mpu6050;
RingBufMax vel_ringbuf;     // Ring buffer that returns its highest value.
float gyro_multiplier;      // Will be set to 1 / (clock frequency * gyro's 131 LSB/°/sec).

inline float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

inline int16_t clamp(int16_t x, int16_t min, int16_t max) {
    return x < min ? min : x > max ? max : x;
}

void imu_init(){
    mpu6050 = mpu6050_init(MPU6050_I2C_PORT, MPU6050_ADDRESS);

    if (mpu6050_begin(&mpu6050)) {
        mpu6050_set_scale(&mpu6050, MPU6050_SCALE_250DPS); // Gyroscope scale. ±250 °/sec = 131 LSB/°/sec
        mpu6050_set_range(&mpu6050, MPU6050_RANGE_2G); // Accelerometer range. ±2 g = 16384 LSB/g

        mpu6050_set_gyroscope_measuring(&mpu6050, true);
        mpu6050_set_accelerometer_measuring(&mpu6050, true);

        mpu6050_set_dhpf_mode(&mpu6050, MPU6050_DHPF_2_5HZ);
        mpu6050_set_dlpf_mode(&mpu6050, MPU6050_DLPF_3);

        mpu6050_calibrate_gyro(&mpu6050, 50);
    }
    float clk_sys_hz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS) / 1000.0f;  
    gyro_multiplier = 1 / (clk_sys_hz * 131.0f);
}

void imu_task(Imu_data * data){
    static float pitch;
    static float pitch_accel;
    static float accel_total;
    static uint16_t velocity_raw;

    mpu6050_event(&mpu6050);
    mpu6050_vectorf_t *accel = mpu6050_get_accelerometer(&mpu6050);
    mpu6050_vectorf_t *gyro = mpu6050_get_gyroscope(&mpu6050);

    pitch += gyro->x * gyro_multiplier;
    accel_total = sqrt((accel->x*accel->x)+(accel->y*accel->y)+(accel->z*accel->z)); 
    pitch_accel = asin((float)accel->y/accel_total) * RAD_TO_DEG;                                         
    pitch = (pitch + pitch_accel) / 2;
    // Clamp the velocity range
    // velocity_raw = (uint16_t)fmap(accel_total - GRAVITY_CONSTANT, 0.0f, TAP_SENSITIVITY, 0.0f, 63.0f);
    // uint16_t velocity_corrected = 64 + ringBufMax(&vel_ringbuf, velocity_raw);
    // velocity_corrected = clamp(velocity_corrected, 64, 127);
    // data->velocity = velocity_corrected;
    data->velocity = ringBufMax(&vel_ringbuf, data->velocity);
    data->deviation = (((pitch + REST_ANGLE) * 2.0 / 180.0f) - 1.0f) * BENDING_FACTOR;
}
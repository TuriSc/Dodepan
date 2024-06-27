/* IMU GY-521 - MPU6050 accelerometer and gyroscope */

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "MPU6050.h"
#include <config.h>
#include <math.h>
#include "imu.h"

mpu6050_t mpu6050;
float gyro_multiplier;      // Will be set to 1 / (clock frequency * gyro's 131 LSB/°/sec).

inline float fclamp(float x, float min, float max) { return x < min ? min : x > max ? max : x; }

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
    mpu6050_event(&mpu6050);
    mpu6050_vectorf_t *accel = mpu6050_get_accelerometer(&mpu6050);
    mpu6050_vectorf_t *gyro = mpu6050_get_gyroscope(&mpu6050);

    // Calculate the Euclidean norm of the vector (x, y, z)
    float acceleration = sqrt((accel->x*accel->x)+(accel->y*accel->y)+(accel->z*accel->z));
    // Remove the gravitational pull from the calculations
    float accel_total = acceleration - GRAVITY_CONSTANT;
    // Normalize and clamp outputs
    accel_total = (fabs(accel_total) + TAP_SENSITIVITY) / TAP_SENSITIVITY -1.0f;

    float roll = (gyro->x * gyro_multiplier) + asin((float)accel->x/acceleration) * RAD_TO_DEG;      
    // By 'pitch' here we mean the tilt angle, not the frequency of a note
    float pitch = (gyro->y * gyro_multiplier) + asin((float)accel->y/acceleration) * RAD_TO_DEG;

    // Map [-90.0, +90.0] to [0, 16383]
    data->deviation_x = (uint16_t)((roll + 90.0) / 180.0 * 16383.0) + 1;
    // Map [-90.0, +90.0] to [0, 127]
    data->deviation_y = (uint8_t)((pitch + 90.0) / 180.0 * 127.0) + 1;

    accel_total = fclamp(accel_total, -2.0f, 2.0f);
    // Map [-2.0, +2.0] to [0, 64]
    data->acceleration = (uint8_t)((accel_total + 2.0) / 4.0 * 64.0) + 1;
}
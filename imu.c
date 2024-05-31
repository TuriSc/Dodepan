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

    // By 'pitch' here we mean the tilt angle, not the frequency of a note
    float pitch = gyro->x * gyro_multiplier;
    // Calculate the Euclidean norm of the vector (x, y, z)
    float accel_total = sqrt((accel->x*accel->x)+(accel->y*accel->y)+(accel->z*accel->z));
    float pitch_accel = asin((float)accel->y/accel_total) * RAD_TO_DEG;                                       
    pitch = (pitch + pitch_accel) / 2.0f;
    // Remove the gravitational pull from the calculations
    accel_total -= GRAVITY_CONSTANT;
    // Normalize and clamp outputs
    accel_total = (fabs(accel_total) + TAP_SENSITIVITY) / TAP_SENSITIVITY -1.0f;
    float deviation = (((pitch + 90.0f) * 2.0f / 180.0f) - 1.0f);
    data->acceleration = fclamp(accel_total, 0.0f, 1.0f);
    data->deviation = fclamp(deviation, -1.0f, 1.0f);
}
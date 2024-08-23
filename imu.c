/* IMU GY-521 - MPU6050 accelerometer and gyroscope */

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "MPU6050.h"
#include <config.h>
#include "imu.h"

mpu6050_t mpu6050;

// The lack of an FPU on the Pico makes it hard to process IMU data on the fly
// while also running the synth and i/o. We'll have to resort to fixed-point math.

#define FIXED_POINT_BITS 16
#define FIXED_POINT_SCALE (1 << FIXED_POINT_BITS)

#define PEAK_HOLD_WINDOW        10

typedef struct {
    int16_t peak;
    int16_t window[PEAK_HOLD_WINDOW];
    int8_t window_index;
    int8_t reset_value;
    int8_t reset_counter;
} peak_hold_state_t;

peak_hold_state_t peak_hold;

void peak_hold_init(peak_hold_state_t *state) {
    state->peak = 0;
    for (int i = 0; i < PEAK_HOLD_WINDOW; i++) {
        state->window[i] = 0;
    }
    state->window_index = 0;
    state->reset_value = VELOCITY_HOLD_SAMPLES;
    state->reset_counter = 0;
}

void peak_hold_update(peak_hold_state_t *state, int16_t value) {
    state->window[state->window_index] = value;
    state->window_index = (state->window_index + 1) % PEAK_HOLD_WINDOW;

    // Find maximum value in window
    int16_t max_value = state->window[0];
    for (int i = 1; i < PEAK_HOLD_WINDOW; i++) {
        if (state->window[i] > max_value) {
            max_value = state->window[i];
        }
    }

    // Update peak if new maximum is higher
    if (max_value > state->peak) {
        state->peak = max_value;
    }

    // Reset the peak after the timeout
    if (++state->reset_counter == state->reset_value) {
        peak_hold_init(state);
    }
}

int16_t peak_hold_get(peak_hold_state_t *state) {
    return state->peak;
}

inline int16_t mul_fixed(int16_t a, int16_t b) {
    return (a * b) >> FIXED_POINT_BITS;
}

inline int16_t sqrt_fixed(int16_t x) {
    int16_t y = x;
    int32_t e = FIXED_POINT_SCALE;
    while (e > 1) {
        e >>= 1;
        y = (y + mul_fixed(x, FIXED_POINT_SCALE / y)) >> 1;
    }
    return y;
}

void imu_init(){
    mpu6050 = mpu6050_init(MPU6050_I2C_PORT, MPU6050_ADDRESS);

    if (mpu6050_begin(&mpu6050)) {
        mpu6050_set_range(&mpu6050, MPU6050_RANGE_2G); // Accelerometer range. ±2 g = 16384 LSB/g

        // Set digital filters
        mpu6050_set_dhpf_mode(&mpu6050, MPU6050_DHPF_2_5HZ);
        mpu6050_set_dlpf_mode(&mpu6050, MPU6050_DLPF_3);

        mpu6050_set_accelerometer_measuring(&mpu6050, true);

        // We're not calibrating the IMU on Pico RP2040, and we're not fusing gyro and accelerometer data
        // to account for gravitational compensation.
    }

    peak_hold_init(&peak_hold);
}

// Overriding rpi-pico-mpu6050 library methods for minimal, fixed-point calculations
void read_raw_accel_fixed(struct mpu6050 *self) {
    struct i2c_information *i2c = &self->i2c;
    uint8_t data[6];

    self->ra.x = 0;
    self->ra.y = 0;
    self->ra.z = 0;

    // Read raw accelerometer data
    uint8_t reg = 0x3B; // ACCEL_XOUT_H
    i2c_write_timeout_us(i2c->instance, i2c->address, &reg, 1, true, 3000);
    i2c_read_timeout_us(i2c->instance, i2c->address, data, 6, false, 3000);

    self->ra.x = data[0] << 8 | data[1];
    self->ra.y = data[2] << 8 | data[3];
    self->ra.z = data[4] << 8 | data[5];
}

int16_t abs_fixed(int16_t x) {
    return (x < 0) ? -x : x;
}

int8_t map_7(int16_t value) {
    int16_t result = (value + 128) * 127 / 256;
    return (result < 0) ? 0 : (result > 127) ? 127 : result;
}

int16_t map_14(int16_t value) {
    int32_t result = (value + 128) * 16383 / 256;
    return (result < 0) ? 0 : (result > 16383) ? 16383 : result;
}

void imu_task(Imu_data * data) {
    read_raw_accel_fixed(&mpu6050);
    struct mpu6050_vector16 *accel = &mpu6050.ra;

    // Scale the raw readings.
    // 493 = (MPU6050_SCALE_250DPS * (1 << 16)) / 1
    int16_t ax = (accel->x * 493) / FIXED_POINT_SCALE;
    int16_t ay = (accel->y * 493) / FIXED_POINT_SCALE;
    int16_t az = (accel->z * 493) / FIXED_POINT_SCALE;

    // Calculate the total motion acceleration (which is not linear since we have not accounted for gravity)
    int16_t tot_accel = sqrt_fixed(mul_fixed(accel->x, accel->x) +
                               mul_fixed(accel->y, accel->y) +
                               mul_fixed(accel->z, accel->z));

    static int16_t prev_tot_accel;
    int16_t delta_accel = abs_fixed(prev_tot_accel - tot_accel);
    prev_tot_accel = tot_accel;

    peak_hold_update(&peak_hold, delta_accel);

#if defined (MPU6050_FLIP_X)
    ax = -ax;
#endif
#if defined (MPU6050_FLIP_Y)
    ay = -ay;
#endif

#if defined (MPU6050_SWAP_X_Y)
    int16_t temp = ax;
    ax = ay;
    ay = temp;
#endif

    // X goes to pitch bending, Y goes to cutoff
    data->deviation_x = map_14(ax);
    data->deviation_y = map_7(LPF_MIN + ay);

    // Acceleration goes to velocity
    data->acceleration = (map_7(peak_hold_get(&peak_hold) * VELOCITY_MULTIPLIER));
}
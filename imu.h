#ifndef IMU_H
#define IMU_H
#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {            // Accelerometer/gyroscope data, with fallback.
    uint16_t velocity;      // Velocity for Midi ranges between 64 and 127.
    float deviation;        // Detuning amount.
} Imu_data;

void imu_init();
void imu_task(Imu_data * data);

#ifdef __cplusplus
}
#endif

#endif
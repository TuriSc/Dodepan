#ifndef IMU_H
#define IMU_H
#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {            // Accelerometer/gyroscope data, with fallback.
    float acceleration;     // Also used to calculate velocity for Midi.
    float deviation_x;
    float deviation_y;
} Imu_data;

void imu_init();
void imu_task(Imu_data * data);

#ifdef __cplusplus
}
#endif

#endif
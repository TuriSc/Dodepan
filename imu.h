#ifndef IMU_H
#define IMU_H
#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {            // Accelerometer/gyroscope data, with fallback.
    uint8_t acceleration;     // Also used to calculate velocity for Midi.
    uint16_t deviation_x;
    uint8_t deviation_y;
} Imu_data;

void imu_init();
void imu_task(Imu_data * data);

#ifdef __cplusplus
}
#endif

#endif
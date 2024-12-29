#ifndef TOF_H
#define TOF_H
#include "pico/stdlib.h"
#include "VL53L0X.h"
#include "imu.h"

void tof_init(VL53L0X* sensor);
void tof_scale(Imu_data * data, uint16_t distance_mm);

#endif

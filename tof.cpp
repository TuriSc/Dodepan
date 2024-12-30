#include "pico/stdlib.h"
#include "config.h"
#include "imu.h"
#include "VL53L0X.h"

void tof_init(VL53L0X* sensor) {
    sensor->init();
    sensor->setTimeout(2);
    sensor->startContinuous();
}

void tof_scale(Imu_data * data, uint16_t distance_mm) {
    if (distance_mm < TOF_DISTANCE_MIN) {
        distance_mm = TOF_DISTANCE_MIN;
    }
    if (distance_mm > TOF_DISTANCE_MAX) {
        distance_mm = TOF_DISTANCE_MAX;
    }
    uint16_t tof;
    if (TOF_DIRECTION == 1) { // 0 = far to near bends down, 1 = far to near bends up
        tof = (uint16_t)(0x3FFF - ((distance_mm - TOF_DISTANCE_MIN) * 0x1FFF / TOF_INPUT_RANGE));
    } else {
        tof = (uint16_t)((distance_mm - TOF_DISTANCE_MIN) * 0x2000 / TOF_INPUT_RANGE);
    }
    data->deviation_x = tof;
}

#ifndef TOUCH_H
#define TOUCH_H
#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void mpr121_i2c_init();
void mpr121_task();

extern void touch_on(uint8_t id);
extern void touch_off(uint8_t id);

#ifdef __cplusplus
}
#endif

#endif
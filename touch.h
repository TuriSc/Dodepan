#ifndef TOUCH_H
#define TOUCH_H
#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void mpr121_i2c_init();
void mpr121_task();

extern uint8_t get_note_by_id(uint8_t n);
extern void trigger_note_on(uint8_t note);
extern void trigger_note_off(uint8_t note);

#ifdef __cplusplus
}
#endif

#endif
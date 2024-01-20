#ifndef DEMUXER_H
#define DEMUXER_H
#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCALE       0
#define KEY         1
#define ALTERATION  2

void demuxer_init(uint8_t en, uint8_t sig, int8_t s0, int8_t s1, int8_t s2, int8_t s3);
void demuxer_task();
void set_led(uint8_t type, uint8_t led);

#ifdef __cplusplus
}
#endif

#endif
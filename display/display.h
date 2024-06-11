#ifndef DISPLAY_H
#define DISPLAY_H
#include "pico/stdlib.h"
#include "ssd1306.h"        // https://github.com/daschr/pico-ssd1306

#ifdef __cplusplus
extern "C" {
#endif

void display_init(ssd1306_t *p);
void display_update(ssd1306_t *p);
void ssd1306_flip(ssd1306_t *p);
void intro_animation(ssd1306_t *p);

#ifdef __cplusplus
}
#endif

#endif


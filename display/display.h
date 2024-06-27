#ifndef DISPLAY_H
#define DISPLAY_H
#include "pico/stdlib.h"
#include "ssd1306.h"        // https://github.com/daschr/pico-ssd1306
#include "state.h"

#ifdef __cplusplus
extern "C" {
#endif

void display_init(ssd1306_t *p);
void display_draw(ssd1306_t *p, state_t *st);
void ssd1306_flip(ssd1306_t *p);
void intro_animation(ssd1306_t *p, void (*callback)(void));

#ifdef __cplusplus
}
#endif

#endif


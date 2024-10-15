#ifndef DISPLAY_H
#define DISPLAY_H
#include "pico/stdlib.h"
#include "ssd1306.h"
#include "state.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONTRAST_MIN    0
#define CONTRAST_MED    1
#define CONTRAST_MAX    2
#define CONTRAST_AUTO   3

void display_init(ssd1306_t *p);
void display_draw(ssd1306_t *p);
void display_update_contrast(ssd1306_t *p);
void display_dim(ssd1306_t *p);
void display_wake(ssd1306_t *p);
void intro_animation(ssd1306_t *p, void (*callback)(void));

#ifdef __cplusplus
}
#endif

#endif


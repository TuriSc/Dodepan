#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "config.h"
#include "ssd1306.h"        // https://github.com/daschr/pico-ssd1306
#include "intro_frames.h"
#include "display_fonts.h"
#include "display_strings.h"
#include "state.h"

void display_init(ssd1306_t *p) {
    p->external_vcc=false;
    ssd1306_init(p, SSD1306_WIDTH, SSD1306_HEIGHT, SSD1306_ADDRESS, SSD1306_I2C_PORT);
}

void intro_animation(ssd1306_t *p) {
    for(uint8_t current_frame=0;current_frame<16;current_frame++) {
        ssd1306_bmp_show_image(p, intro_frames[current_frame], intro_frames_size);
        ssd1306_show(p);
        sleep_ms(82);
        ssd1306_clear(p);
    }
}

void display_draw(ssd1306_t *p, state_t *st) {
    ssd1306_clear(p);
    ssd1306_draw_string_with_font(p, 0, 0, 1, key_font, note_names[st->tonic]);
    if(st->is_alteration) { ssd1306_draw_string_with_font(p, 15, 0, 1, diesis_font, diesis); }
    ssd1306_draw_string_with_font(p, 15, 16, 1, octave_font, octave_names[st->octave]);
    ssd1306_draw_string(p, 40, 0, 1, scale_names[st->scale]);
    ssd1306_draw_string(p, 40, 19, 1, instrument_names[st->instrument]);
    
    switch(st->context) {
        case CTX_KEY:
            ssd1306_draw_line(p, 0, 30, 25, 30);
            ssd1306_draw_line(p, 0, 31, 25, 31);
        break;
        case CTX_SCALE:
            ssd1306_draw_line(p, 40, 11, 127, 11);
            ssd1306_draw_line(p, 40, 12, 127, 12);
        break;
        case CTX_INSTRUMENT:
            ssd1306_draw_line(p, 40, 30, 127, 30);
            ssd1306_draw_line(p, 40, 31, 127, 31);
        break;
    }

    ssd1306_show(p);
}
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "config.h"
#include "ssd1306.h"        // https://github.com/daschr/pico-ssd1306
#include "intro_frames.h"
#include "icon_off.h"
#include "icon_on_x.h"
#include "icon_on_y.h"
#include "icon_on_x_y.h"
#include "icon_low_batt.h"
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

#define OFFSET_X    32
void display_draw(ssd1306_t *p, state_t *st) {
    ssd1306_clear(p);
    switch(st->context) {
        case CTX_KEY:
        case CTX_SCALE:
        case CTX_INSTRUMENT:
        {
            ssd1306_draw_string_with_font(p, 0, 4, 1, key_font, note_names[st->tonic]);
            if(st->is_alteration) { ssd1306_draw_string_with_font(p, 15, 0, 1, diesis_font, diesis); }
            ssd1306_draw_string_with_font(p, 15, 16, 1, octave_font, octave_names[st->octave]);
            ssd1306_draw_string(p, OFFSET_X, 19, 1, scale_names[st->scale]);
            ssd1306_draw_string(p, OFFSET_X, 0, 1, instrument_names[st->instrument]);
            
            switch(st->context) {
                case CTX_KEY:
                    ssd1306_draw_line(p, 0, 30, 25, 30);
                    ssd1306_draw_line(p, 0, 31, 25, 31);
                break;
                case CTX_SCALE:
                    ssd1306_draw_line(p, OFFSET_X, 30, 120, 30);
                    ssd1306_draw_line(p, OFFSET_X, 31, 120, 31);
                break;
                case CTX_INSTRUMENT:
                    ssd1306_draw_line(p, OFFSET_X, 11, 120, 11);
                    ssd1306_draw_line(p, OFFSET_X, 12, 120, 12);
                break;
            }
        }
        break;
        case CTX_IMU_CONFIG:
            switch (st->imu_dest) {
                case 0x0:
                    ssd1306_bmp_show_image_with_offset(p, icon_off_data, icon_off_size, 48, 0);
                break;
                case 0x1:
                    ssd1306_bmp_show_image_with_offset(p, icon_on_x_data, icon_on_x_size, 48, 0);
                break;
                case 0x2:
                    ssd1306_bmp_show_image_with_offset(p, icon_on_y_data, icon_on_y_size, 48, 0);
                break;
                case 0x3:
                    ssd1306_bmp_show_image_with_offset(p, icon_on_x_y_data, icon_on_x_y_size, 48, 0);
                break;
            }
        break;
        case CTX_VOLUME:
        {
            // Draw the volume bars
            uint8_t increments = 128 / VOL_INCR;
            uint8_t gap = (128 / increments / 4);
            uint8_t width = 128 / increments - gap;
            // for(uint8_t i=VOL_MIN; i <= 127; i += VOL_INCR){
            for (uint8_t i = 0; i < increments; i++) {
                if(st->volume >= VOL_MIN + i * VOL_INCR) {
                    uint8_t x = i * width + i * gap;
                    ssd1306_draw_square(p, x, 4, width, 24);
                }
            }
        }
        break;
    }


    if(st->low_batt) {
        // Clear an outline around the icon to avoid overlaps
        ssd1306_clear_square(p, 119, 8, 9, 16);
        ssd1306_bmp_show_image_with_offset(p, icon_low_batt_data, icon_low_batt_size, 121, 9);
    }

    ssd1306_show(p);
}
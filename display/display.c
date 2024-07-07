#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "config.h"
#include "ssd1306.h"        // https://github.com/daschr/pico-ssd1306
#include "state.h"

// Include assets
#include "display_fonts.h"
#include "display_strings.h"
#include "intro.h"
#include "icon_gyro_off.h"
#include "icon_gyro_on_x.h"
#include "icon_gyro_on_y.h"
#include "icon_gyro_on_x_y.h"
#include "icon_bar.h"
#include "icon_close.h"
#include "icon_low_batt.h"

void display_init(ssd1306_t *p) {
    p->external_vcc=false;
    ssd1306_init(p, SSD1306_WIDTH, SSD1306_HEIGHT, SSD1306_ADDRESS, SSD1306_I2C_PORT);
    ssd1306_clear(p);
    ssd1306_show(p);
}

static inline void draw_info_screen(ssd1306_t *p) {
    uint8_t baseline = 0;
    char version_number[18];
    snprintf(version_number, sizeof(version_number), "%s%s", "Version ", PROGRAM_VERSION);
    ssd1306_draw_string_with_font(p, 8, baseline, 1, spaced_font, version_number);
    baseline += 14;

    // Split the program URL into multiple lines
    uint8_t len = strlen(PROGRAM_URL);
    uint8_t max_chars = 17;
    uint8_t num_substr = (len + max_chars -1) / max_chars; // Calculate the number of substrings
    char **substrings = malloc(num_substr * sizeof(char *));
    uint8_t s = 0;

    for (uint8_t i = 0; i < len; i += max_chars) {
        uint8_t substr_len = (i + max_chars <= len) ? max_chars : len - i;
        substrings[s] = malloc((substr_len + 1) * sizeof(char));
        strncpy(substrings[s], PROGRAM_URL + i, substr_len);
        substrings[s][substr_len] = '\0'; // Terminate the substring
        s++;
    }

    // Print the lines to screen
    for (uint8_t i = 0; i < num_substr; i++) {
        ssd1306_draw_string_with_font(p, 8, baseline, 1, spaced_font, substrings[i]);
        baseline += 10;
    }
}

static inline void draw_volume_screen(ssd1306_t *p){
    // Draw the volume bars
    uint8_t increments = 9; // 0 to 8
    uint8_t gap = (128 / increments / 4);
    uint8_t width = 128 / increments - gap;

    for (uint8_t i = 1; i < increments; i++) {
        uint8_t x = i * width + i * gap;
        if(get_volume() >= i) {
            ssd1306_draw_square(p, x, 8, width, 16);
        } else {
            ssd1306_draw_empty_square(p, x, 8, width, 16);
        }
    }
}

static inline void draw_imu_axes_screen(ssd1306_t *p){
    switch (get_imu_axes()) {
        case 0x0:
            ssd1306_bmp_show_image_with_offset(p, icon_gyro_off_data, icon_gyro_off_size, 48, 0);
        break;
        case 0x1:
            ssd1306_bmp_show_image_with_offset(p, icon_gyro_on_x_data, icon_gyro_on_x_size, 48, 0);
        break;
        case 0x2:
            ssd1306_bmp_show_image_with_offset(p, icon_gyro_on_y_data, icon_gyro_on_y_size, 48, 0);
        break;
        case 0x3:
            ssd1306_bmp_show_image_with_offset(p, icon_gyro_on_x_y_data, icon_gyro_on_x_y_size, 48, 0);
        break;
    }
}

static inline void draw_synth_edit_screen(ssd1306_t *p) {
    char str[3];
    sprintf(str, "%d", get_argument());
    ssd1306_draw_string(p, 4, 0, 1, parameter_names[get_parameter()]);

    // Center the string
    uint8_t margin_x = 38; // 3 digits
    if(get_argument() < 100) { // 2 digits
        margin_x = 50;
    } else if(get_argument() < 10) { // 1 digit
        margin_x = 62;
    }
    ssd1306_draw_string(p, margin_x, 18, 2, str);

    // Draw selection mark
    if(get_context() == CTX_SYNTH_EDIT_PARAM) {
        ssd1306_draw_square(p, 0, 0, 2, 7);
    } else {
        ssd1306_draw_square(p, 0, 18, 2, 14);
    }
}

// If you changed NUM_PRESET_SLOTS in config.h you will have to adjust this function accordingly
static inline void draw_synth_store_screen(ssd1306_t *p) {
    int8_t slot = get_preset_slot();

    uint8_t position = slot + 1;
    // Close icon
    ssd1306_bmp_show_image_with_offset(p, icon_close_data, icon_close_size, 8, 11);

    ssd1306_draw_string_with_font(p,  34, 11, 1, octave_font, "1");
    ssd1306_draw_string_with_font(p,  60, 11, 1, octave_font, "2");
    ssd1306_draw_string_with_font(p,  86, 11, 1, octave_font, "3");
    ssd1306_draw_string_with_font(p, 112, 11, 1, octave_font, "4");
    
    // Underline
    ssd1306_draw_square(p, 8 + 26 * position, 25, 12, 2);

}

#define ICON_CENTERED_MARGIN_X ((SSD1306_WIDTH / 2) - (32 / 2))
void intro_animation(ssd1306_t *p, void (*callback)(void)) {
    for(uint8_t current_frame=0; current_frame < INTRO_FRAMES_NUM; current_frame++) {
        ssd1306_bmp_show_image_with_offset(p, intro_frames[current_frame], INTRO_FRAME_SIZE, ICON_CENTERED_MARGIN_X, 0);
        ssd1306_show(p);
        busy_wait_ms(42); // About 24fps
        ssd1306_clear(p);
    }
    callback();
}

#define OFFSET_X    32
#define CHAR_W      7 // Includes spacing
static inline void draw_main_screen(ssd1306_t *p) {
    // Key
    ssd1306_draw_string_with_font(p, 0, 4, 1, key_font, note_names[get_tonic()]);
    if (get_alteration()) { ssd1306_draw_string_with_font(p, 15, 0, 1, diesis_font, diesis); }
    ssd1306_draw_string_with_font(p, 15, 16, 1, octave_font, octave_names[get_octave()]);

    // Scale
    ssd1306_draw_string_with_font(p, OFFSET_X, 4, 1, spaced_font, scale_names[get_scale()]);
    uint8_t scale_name_width = strlen(scale_names[get_scale()]);
    if (scale_name_width > 12) { scale_name_width = 12; }

    // Instrument
    ssd1306_draw_string_with_font(p, OFFSET_X, 21, 1, spaced_font, instrument_names[get_instrument()]);
    uint8_t instrument_name_width = strlen(instrument_names[get_instrument()]);
    if (instrument_name_width > 12) { instrument_name_width = 12; }

    // Clear an area around the icon bar to avoid overlaps
    ssd1306_clear_square(p, 116, 0, 12, 32);
    
    // Draw selection mark and underline
    switch(get_context()) {
        case CTX_SELECTION: {
            switch(get_selection()) {
                case SELECTION_KEY:
                    ssd1306_draw_square(p, 0, 30, 6, 2);
                break;
                case SELECTION_SCALE:
                    ssd1306_draw_square(p, OFFSET_X, 13, 6, 2);
                break;
                case SELECTION_INSTRUMENT:
                    ssd1306_draw_square(p, OFFSET_X, 30, 6, 2);
                break;
                case SELECTION_VOLUME:
                    ssd1306_draw_square(p, 117, 0, 2, 6);
                break;
                case SELECTION_IMU_CONFIG:
                    ssd1306_draw_square(p, 117, 7, 2, 6);
                break;
                case SELECTION_SYNTH_EDIT:
                    ssd1306_draw_square(p, 117, 14, 2, 6);
                break;
            }
        }
        break;
        case CTX_KEY:
            ssd1306_draw_square(p, 0, 30, 25, 2);
        break;
        case CTX_SCALE:
            ssd1306_draw_square(p, OFFSET_X, 13, CHAR_W * scale_name_width, 2);
        break;
        case CTX_INSTRUMENT:
            ssd1306_draw_square(p, OFFSET_X, 30, CHAR_W * instrument_name_width, 2);
        break;
    }

    // Icons
    ssd1306_bmp_show_image_with_offset(p, icon_bar_data, icon_bar_size, 120, 0);

    if(get_low_batt()) {
        ssd1306_bmp_show_image_with_offset(p, icon_low_batt_data, icon_low_batt_size, 120, 21);
    }
}

void display_draw(ssd1306_t *p) {

    state_t* state = get_state();

    ssd1306_clear(p);
    switch(get_context()) {
        case CTX_SELECTION:
        case CTX_KEY:
        case CTX_SCALE:
        case CTX_INSTRUMENT:
            draw_main_screen(p);
        break;
        case CTX_IMU_CONFIG:
            draw_imu_axes_screen(p);
        break;
        case CTX_VOLUME:
            draw_volume_screen(p);
        break;
        case CTX_SYNTH_EDIT_PARAM:
        case CTX_SYNTH_EDIT_ARG:
            draw_synth_edit_screen(p);
        break;
        case CTX_SYNTH_EDIT_STORE:
            draw_synth_store_screen(p);
        break;
        case CTX_INFO:
            draw_info_screen(p);
        break;
    }

    ssd1306_show(p);
}
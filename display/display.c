#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "config.h"
#include "ssd1306.h"        // https://github.com/TuriSc/pico-ssd1306
#include "state.h"
#include "looper.h"
#include "display.h"

// Include assets
#include "display_fonts.h"
#include "display_strings.h"
#include "intro.h"
#include "icon_contrast_max.h"
#include "icon_contrast_med.h"
#include "icon_contrast_min.h"
#include "icon_imu_off.h"
#include "icon_imu_on_x.h"
#include "icon_imu_on_y.h"
#include "icon_imu_on_x_y.h"
#include "icon_close.h"
#include "icon_looper.h"
#include "icon_rec.h"
#include "icon_play.h"
#include "icon_ready.h"
#include "icon_pause.h"
#include "icon_transpose.h"
// #include "icon_parameters.h" // Unused asset
#include "icon_low_batt.h"

#define ICON_CENTERED_MARGIN_X ((SSD1306_WIDTH / 2) - (32 / 2))

static alarm_id_t display_dim_alarm_id;

void display_init(ssd1306_t *p) {
    p->external_vcc=false;
    ssd1306_init(p, SSD1306_WIDTH, SSD1306_HEIGHT, SSD1306_ADDRESS, SSD1306_I2C_PORT);
#if defined (SSD1306_ROTATE)
    ssd1306_rotate(p, 1);
#endif
    ssd1306_clear(p);
    ssd1306_show(p);
}

static inline void draw_info_screen(ssd1306_t *p) {
    uint8_t baseline = 0;
    char version_number[18];
    snprintf(version_number, sizeof(version_number), "%s%s", string_version, PROGRAM_VERSION);
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
        free(substrings[i]);
    }
    free(substrings);
}

const uint8_t transpose_coords[12][2] = {
    {15,  1},
    {22,  3},
    {27,  8},
    {29, 15},
    {27, 22},
    {22, 27},
    {15, 29},
    { 8, 27},
    { 3, 22},
    { 1, 15},
    { 3,  8},
    { 8,  3}
};

static inline void draw_looper_screen(ssd1306_t *p) {
    // Draw only the looper icon if in page selection mode
    if(get_context() == CTX_SELECTION) {
        ssd1306_bmp_show_image_with_offset(p, icon_looper_data, icon_looper_size, ICON_CENTERED_MARGIN_X, 0);
        return;
    }

    // Rec
    if(looper_is_recording()) {
        ssd1306_bmp_show_image_with_offset(p, icon_rec_data, icon_rec_size, 58, 10);
    } else if(looper_is_playing()) {
        ssd1306_bmp_show_image_with_offset(p, icon_play_data, icon_play_size, 59, 10);
    } else {
        if(looper_has_recording()) {
            ssd1306_bmp_show_image_with_offset(p, icon_pause_data, icon_pause_size, 58, 10);
        } else {
            ssd1306_bmp_show_image_with_offset(p, icon_ready_data, icon_ready_size, 58, 10);
        }
    }

    // Transpose
    ssd1306_bmp_show_image_with_offset(p, icon_transpose_data, icon_transpose_size, ICON_CENTERED_MARGIN_X, 0);
    uint8_t t = looper_get_transpose();

    // Draw transposition marker
    ssd1306_draw_square(p, ICON_CENTERED_MARGIN_X + transpose_coords[t][0],
                                            transpose_coords[t][1], 2, 2);

    // Draw selection mark
    if(get_context() == CTX_LOOPER) {
        ssd1306_draw_square(p, 0, 0, 2, 32);
    }
}

static inline void draw_imu_axes_screen(ssd1306_t *p) {
    switch (get_imu_axes()) {
        case 0x0:
            ssd1306_bmp_show_image_with_offset(p, icon_imu_off_data, icon_imu_off_size, 48, 0);
        break;
        case 0x1:
            ssd1306_bmp_show_image_with_offset(p, icon_imu_on_x_data, icon_imu_on_x_size, 48, 0);
        break;
        case 0x2:
            ssd1306_bmp_show_image_with_offset(p, icon_imu_on_y_data, icon_imu_on_y_size, 48, 0);
        break;
        case 0x3:
            ssd1306_bmp_show_image_with_offset(p, icon_imu_on_x_y_data, icon_imu_on_x_y_size, 48, 0);
        break;
    }

    // Draw selection mark
    if(get_context() == CTX_IMU_CONFIG) {
        ssd1306_draw_square(p, 0, 0, 2, 32);
    }
}

static inline void draw_contrast_screen(ssd1306_t *p) {
    uint8_t contrast = get_contrast();
    switch (contrast) {
        case CONTRAST_MIN:
            ssd1306_bmp_show_image_with_offset(p, icon_contrast_min_data, icon_contrast_min_size, 48, 0);
        break;
        case CONTRAST_MED:
            ssd1306_bmp_show_image_with_offset(p, icon_contrast_med_data, icon_contrast_med_size, 48, 0);
        break;
        case CONTRAST_MAX:
            ssd1306_bmp_show_image_with_offset(p, icon_contrast_max_data, icon_contrast_max_size, 48, 0);
        break;
        case CONTRAST_AUTO:
            ssd1306_bmp_show_image_with_offset(p, icon_contrast_max_data, icon_contrast_max_size, 48, 0);
            ssd1306_clear_square(p, 48, 11, 32, 10);
            ssd1306_draw_string(p, 53, 13, 1, "AUTO");
        break;
    }

    // Draw selection mark
    ssd1306_draw_square(p, 0, 0, 2, 32);
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

    ssd1306_draw_string(p, 0, 0, 1, "Store preset in slot:");

    // Close icon
    ssd1306_bmp_show_image_with_offset(p, icon_close_data, icon_close_size, 8, 11);

    ssd1306_draw_string_with_font(p,  34, 12, 1, octave_font, "1");
    ssd1306_draw_string_with_font(p,  60, 12, 1, octave_font, "2");
    ssd1306_draw_string_with_font(p,  86, 12, 1, octave_font, "3");
    ssd1306_draw_string_with_font(p, 112, 12, 1, octave_font, "4");
    
    // Underline
    ssd1306_draw_square(p, 8 + 26 * position, 25, 12, 2);
}

static inline void draw_scale_edit_screen(ssd1306_t *p) {
    uint8_t spacing = 18;
    uint8_t line_height = 11;
    uint8_t margin = 2;
    ssd1306_draw_string(p, 0, 0, 1, "Edit scale");

    char str[2];
    for (uint8_t i = 0; i < 6; i++) {
        sprintf(str, "%d", get_degree(i) + 1);
        ssd1306_draw_string(p, margin + i * spacing, line_height, 1, str);
    }

    for (uint8_t i = 0; i < 6; i++) {
        sprintf(str, "%d", get_degree(i+6) + 1);
        ssd1306_draw_string(p, margin + i * spacing, line_height*2, 1, str);
    }

    uint8_t step = get_step();
    uint8_t newline = (step >= 6);
    uint8_t x = step * spacing;
    if(newline) { x -= 6 * spacing; }
    uint8_t y = 8 + line_height + line_height * newline;

    if(get_context() == CTX_SCALE_EDIT_STEP) {
        ssd1306_draw_square(p, margin + x, y, 11, 2);
    } else { // CTX_SCALE_EDIT_DEG
        ssd1306_draw_empty_square(p, x, y-line_height, 15, 11);
    }
}

// If you changed NUM_SCALE_SLOTS in config.h you will have to adjust this function accordingly
static inline void draw_scale_store_screen(ssd1306_t *p) {
    int8_t slot = get_scale_slot();

    uint8_t position = slot + 1;

    ssd1306_draw_string(p, 0, 0, 1, "Store scale in slot:");

    // Close icon
    ssd1306_bmp_show_image_with_offset(p, icon_close_data, icon_close_size, 8, 11);

    ssd1306_draw_string_with_font(p,  34, 12, 1, octave_font, "1");
    ssd1306_draw_string_with_font(p,  60, 12, 1, octave_font, "2");
    ssd1306_draw_string_with_font(p,  86, 12, 1, octave_font, "3");
    ssd1306_draw_string_with_font(p, 112, 12, 1, octave_font, "4");
    
    // Underline
    ssd1306_draw_square(p, 8 + 26 * position, 25, 12, 2);
}

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
    if (get_scale_unsaved()) {
        ssd1306_draw_string_with_font(p, OFFSET_X, 4, 1, spaced_font, "Custom");
    } else {
        ssd1306_draw_string_with_font(p, OFFSET_X, 4, 1, spaced_font, scale_names[get_scale()]);
    }
    uint8_t scale_name_width = strlen(scale_names[get_scale()]);
    if (scale_name_width > 12) { scale_name_width = 12; }

    // Instrument
    ssd1306_draw_string_with_font(p, OFFSET_X, 21, 1, spaced_font, instrument_names[get_instrument()]);
    uint8_t instrument_name_width = strlen(instrument_names[get_instrument()]);
    if (instrument_name_width > 12) { instrument_name_width = 12; }

    // Volume
    uint8_t increments = 9; // 0 to 8
    uint8_t bar_height = 3;

    for (uint8_t i = 1; i < increments; i++) {
        uint8_t y = 32 - (bar_height * i + i);
        if(get_volume() >= i) {
            ssd1306_draw_square(p, 120, y, 7, bar_height);
        } 
    }
    if(get_volume() == 0) {
        ssd1306_draw_string(p, 122, 13, 1, "X");
    }
    
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
        case CTX_VOLUME:
            ssd1306_draw_square(p, 117, 0, 2, 32);
        break;
    }

    // Low battery icon
    if(get_low_batt()) {
        // ssd1306_bmp_show_image_with_offset(p, icon_low_batt_data, icon_low_batt_size, 120, 21);
        // Clear an outline around the icon to avoid overlaps
        ssd1306_clear_square(p, 119, 17, 9, 15);
        ssd1306_bmp_show_image_with_offset(p, icon_low_batt_data, icon_low_batt_size, 121, 18);
    }
}

void display_dim(ssd1306_t *p) {
    ssd1306_contrast(p, 0);
}

int64_t display_dim_callback(alarm_id_t id, void * p) {
    display_dim(p);
}

void display_wake(ssd1306_t *p) {
    ssd1306_contrast(p, 255);
    if (display_dim_alarm_id) cancel_alarm(display_dim_alarm_id);
    display_dim_alarm_id = add_alarm_in_ms(DISPLAY_DIM_DELAY * 1000, display_dim_callback, p, true);
}

void display_refresh(ssd1306_t *p) {
    ssd1306_reset(p);
    ssd1306_show(p);
}

void display_update_contrast(ssd1306_t *p) {
    if (display_dim_alarm_id) cancel_alarm(display_dim_alarm_id);
    uint8_t contrast = get_contrast();
    switch (contrast) {
        case CONTRAST_MIN:
            ssd1306_contrast(p, 0);
        break;
        case CONTRAST_MED:
            ssd1306_contrast(p, 127);
        break;
        case CONTRAST_MAX:
            ssd1306_contrast(p, 255);
        break;
        case CONTRAST_AUTO:
            display_wake(p);
        break;
    }
}

void display_draw(ssd1306_t *p) {
    selection_t selection = get_selection();
    context_t context = get_context();

    ssd1306_clear(p);

    switch(context) {
        case CTX_SELECTION: {
            switch (selection) {
                case SELECTION_KEY:
                case SELECTION_SCALE:
                case SELECTION_INSTRUMENT:
                case SELECTION_VOLUME:
                    draw_main_screen(p);
                break;
                case SELECTION_LOOPER:
                    draw_looper_screen(p);
                break;
                case SELECTION_IMU_CONFIG:
                    draw_imu_axes_screen(p);
                break;
            }
            break;
        }
        case CTX_KEY:
        case CTX_SCALE:
        case CTX_INSTRUMENT:
        case CTX_VOLUME:
            draw_main_screen(p);
        break;
        case CTX_CONTRAST:
            draw_contrast_screen(p);
        break;
        case CTX_LOOPER:
            draw_looper_screen(p);
        break;
        case CTX_IMU_CONFIG:
            draw_imu_axes_screen(p);
        break;
        case CTX_SYNTH_EDIT_PARAM:
        case CTX_SYNTH_EDIT_ARG:
            draw_synth_edit_screen(p);
        break;
        case CTX_SYNTH_EDIT_STORE:
            draw_synth_store_screen(p);
        break;
        case CTX_SCALE_EDIT_STEP:
        case CTX_SCALE_EDIT_DEG:
            draw_scale_edit_screen(p);
        break;
        case CTX_SCALE_EDIT_STORE:
            draw_scale_store_screen(p);
        break;
        case CTX_INFO:
            draw_info_screen(p);
        break;
    }

    ssd1306_show(p);
}

/* Dodepan
 * Digital musical instrument. Touch-enabled, with multiple tunings, pitch bending and Midi out.
 * By Turi Scandurra – https://turiscandurra.com/circuits
 * v2.1.2b - 2024.06.11
 */

#include <stdio.h>
#include "config.h"         // Most configurable options are here
#include "pico/stdlib.h"
// Arduino types added for compatibility
typedef bool boolean;
typedef uint8_t byte;
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "sound_i2s.h"
#include "pra32-u-common.h" // https://github.com/risgk/digital-synth-pra32-u
#include "pra32-u-synth.h"  // PRA32-U version 2.3.1
#include "instrument_preset.h"
#include "encoder.h"        // https://github.com/TuriSc/RP2040-Rotary-Encoder
#include "button.h"         // https://github.com/TuriSc/RP2040-Button
// #include "bsp/board.h"      // For Midi
// #include "tusb.h"           // For Midi
#include "scales.h"
#include "battery-check.h"
#include "imu.h"
#include "touch.h"
#include "display/display.h"
#include "state.h"

/* Audio */
    static const struct sound_i2s_config sound_config = {
        .pio_num         = I2S_PIO_NUM,
        .pin_scl         = I2S_CLOCK_PIN_BASE,
        .pin_sda         = I2S_DATA_PIN,
        .pin_ws          = I2S_CLOCK_PIN_BASE + 1,
        .sample_rate     = SOUND_OUTPUT_FREQUENCY,
    };

/* Globals */
PRA32_U_Synth g_synth;

state_t state;

struct audio_buffer_pool *ap;

Imu_data imu_data;

#if defined (USE_DISPLAY)
ssd1306_t display;
#endif

static alarm_id_t blink_alarm_id;
static alarm_id_t power_on_alarm_id;

uint8_t audio_pin_slice;
uint8_t num_scales = sizeof(scales)/sizeof(scales[0]);

void blink();

void core1_main() {
    while(true) { g_synth.secondary_core_process(); }
}

/* Note and audio functions */

uint8_t get_note_by_id(uint8_t n) {
    return state.key + state.extended_scale[n];
}

uint8_t get_scale_size(uint8_t s) {
    for (uint8_t i=1; i<12; i++) {
        if (scales[s][i] == 0) return i;
    }
    return 12;
}

void update_key() {
    uint8_t tonic = state.key % 12;
    state.tonic = tonic;
    state.octave = state.key / 12;  // C3 is in octave 5 in this system because
                                    // octave -1 is the first element
    // Map key indices to alterations
    static bool key_to_alteration_map[12] = {0,1,0,1,0,0,1,0,1,0,1,0};
    state.is_alteration = (key_to_alteration_map[tonic] ? 1 : 0);
}

void update_scale() {
    uint8_t scale_size = get_scale_size(state.scale);
    uint8_t j = 0;
    uint8_t octave_shift = 0;
    for (uint8_t i=0; i<12; i++) {
        state.extended_scale[i] = (scales[state.scale][j] + octave_shift);
        j++;
        if (j >= scale_size) {j=0; octave_shift+=12;}
    }
}

void set_instrument(uint8_t instr) {
    switch (instr) {
        case 0: // Load custom Dodepan preset
            for (uint32_t i = 0; i < sizeof(dodepan_program_parameters) / sizeof(dodepan_program_parameters[0]); ++i) {
                g_synth.control_change(dodepan_program_parameters[i], dodepan_program[i]);
            }
        break;
        default: // Load PRA32-U presets
            g_synth.program_change(instr - 1); 
        break;
    }
    state.instrument = instr;
}

// static inline uint32_t tudi_midi_write24 (uint8_t jack_id, uint8_t b1, uint8_t b2, uint8_t b3) {
//     uint8_t msg[3] = { b1, b2, b3 };
//     return tud_midi_stream_write(jack_id, msg, 3);
// }

void trigger_note_on(uint8_t note) {
    // Set the decay according to accelerometer data.
    // The range of decay is 0-64, but here we're clamping it to 32-48.
    int8_t decay = 32 + (16 * imu_data.acceleration);
    // set_parameter(EG_DECAY_TIME, decay); // TODO
    // The range of velocity is 0-127, but here we're clamping it to 64-127
    uint8_t velocity = 64 + (63 * imu_data.acceleration);
    // tudi_midi_write24(0, 0x90, note, velocity);
    g_synth.note_on(note, velocity);
    blink();
}

void trigger_note_off(uint8_t note) {
    g_synth.note_off(note);
    // tudi_midi_write24(0, 0x80, note, 0);
}

void bending(float deviation) {
    // Use the IMU to alter a parameter according to device tilting.
    // The range of the deviation parameter is between -1 and 1 inclusive.
    // Parameters have a minimum, a maximum, and a center value, for example
    // FILTER_CUTOFF goes from 0 to 120, with 60 being the center value.
    // int8_t cutoff = 60 + (60 * deviation);
    // set_parameter(FILTER_CUTOFF, cutoff);

    // For pitch bending use this instead:
    // int8_t pitch = 16 + (32 * deviation);
    // set_parameter(OSC_2_FINE_PITCH, pitch);
    // g_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);

    // Prepare Midi message
    static uint8_t throttle;
    if(throttle++ % 10 != 0) return; // Further limit the message rate
    // Pitch wheel range is between 0 and 16383 (0x0000 to 0x3FFF),
    // with 8192 (0x2000) being the center value.
    unsigned int pitchval = (deviation+1.f)*8192;
    if (pitchval > 16383) pitchval = 16383;
    // Send Midi message
    // tudi_midi_write24 (0, 0xE0, (pitchval & 0x7F), (pitchval >> 7) & 0x7F);
}

/* I/O functions */

int64_t blink_complete(alarm_id_t, void *) {
    gpio_put(LED_PIN, 0);
    return 0;
}

int64_t power_on_complete(alarm_id_t, void *) {
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    return 0;
}

void blink() { // LED blinks for 0.1 seconds when a note is played
    gpio_put(LED_PIN, 1);
    if (blink_alarm_id) cancel_alarm(blink_alarm_id);
    blink_alarm_id = add_alarm_in_us(100000, blink_complete, NULL, true);
}

void encoder_onchange(rotary_encoder_t *encoder) {
    static long int last_position;
    long int position = encoder->position / 4; // Adjust the encoder sensitivity here
    int8_t direction = 0;
    if(last_position != position) {
        direction = (last_position < position ? 1 : -1);
    }
    last_position = position;
    uint8_t control_number;
    uint8_t num_parameters = sizeof(dodepan_program_parameters) / sizeof(dodepan_program_parameters[0]);

    if (direction == 1) {
        switch (state.context) {
            case CTX_KEY:
                // D#7 is the highest note that can be set as root note
                if(state.key < 99) state.key++;
                update_key();
            break;
            case CTX_SCALE:
                if(state.scale < num_scales - 1) state.scale++;
                update_scale();
            break;
            case CTX_INSTRUMENT:
                if(state.instrument < PRESET_PROGRAM_NUMBER_MAX + 1) set_instrument(state.instrument + 1);
            break;
            case CTX_PARAMETER:
                if(state.parameter < num_parameters - 1) state.parameter++;
                control_number = dodepan_program_parameters[state.parameter];
                state.argument = g_synth.current_controller_value(control_number);
            break;
            case CTX_ARGUMENT:
                if(state.argument < 128) state.argument++;
                control_number = dodepan_program_parameters[state.parameter];
                g_synth.control_change(control_number, state.argument);
            break;
        }
    } else if (direction == -1) {
        switch (state.context) {
            case CTX_KEY:
                if(state.key > 0) state.key--;
                update_key();
            break;
            case CTX_SCALE:
                if(state.scale > 0) state.scale--;
                update_scale();
            break;
            case CTX_INSTRUMENT:
                if(state.instrument > 0) set_instrument(state.instrument - 1);
            break;
            case CTX_PARAMETER:
                if(state.parameter > 0) state.parameter--;
                control_number = dodepan_program_parameters[state.parameter];
                state.argument = g_synth.current_controller_value(control_number);
            break;
            case CTX_ARGUMENT:
                if(state.argument > 0) state.argument--;
                control_number = dodepan_program_parameters[state.parameter];
                g_synth.control_change(control_number, state.argument);
            break;
        }
    }
    display_draw(&display, &state);
}

void button_onchange(button_t *button_p) {
    button_t *button = (button_t*)button_p;
    static uint64_t pressed_time;
    uint8_t num_parameters = sizeof(dodepan_program_parameters) / sizeof(dodepan_program_parameters[0]);
    if(!button->state) { // Button pressed
        pressed_time = time_us_64();
        switch(state.context){
            case CTX_KEY:
            case CTX_SCALE:
            case CTX_INSTRUMENT:
                state.context++;
                if (state.context == CTX_INSTRUMENT + 1) { state.context = CTX_KEY; }
            break;
            case CTX_PARAMETER:
                state.context = CTX_ARGUMENT;
            break;
            case CTX_ARGUMENT:
                state.context = CTX_PARAMETER;
            break;
        }
        display_draw(&display, &state);
    } else { // Button released
        uint64_t released_time = time_us_64();
        if(released_time - pressed_time > 2000*1000) { // Held for more than 2 seconds
            switch(state.context){
                case CTX_KEY:
                case CTX_SCALE:
                case CTX_INSTRUMENT:
                    state.context = CTX_PARAMETER;
                break;
                case CTX_PARAMETER:
                case CTX_ARGUMENT:
                    state.context = CTX_KEY;
                break;
            }

            display_draw(&display, &state);
        }
    }
}

void battery_low_detected() { // TODO, new battery level display
    // Low battery detected
    // gpio_put(LOW_BATT_LED_PIN, 1);
    battery_check_stop(); // Stop the timer
}

void bi_decl_all() {
    bi_decl(bi_program_name(PROGRAM_NAME));
    bi_decl(bi_program_description(PROGRAM_DESCRIPTION));
    bi_decl(bi_program_version_string(PROGRAM_VERSION));
    bi_decl(bi_program_url(PROGRAM_URL));
    bi_decl(bi_1pin_with_name(LED_PIN, LED_DESCRIPTION));
    bi_decl(bi_1pin_with_name(MPR121_SDA_PIN, MPR121_SDA_DESCRIPTION));
    bi_decl(bi_1pin_with_name(MPR121_SCL_PIN, MPR121_SCL_DESCRIPTION));
    bi_decl(bi_1pin_with_name(ENCODER_DT_PIN, ENCODER_DT_DESCRIPTION));
    bi_decl(bi_1pin_with_name(ENCODER_CLK_PIN, ENCODER_CLK_DESCRIPTION));
    bi_decl(bi_1pin_with_name(ENCODER_SWITCH_PIN, ENCODER_SWITCH_DESCRIPTION));
#if defined USE_DISPLAY && defined USE_GYRO
    bi_decl(bi_1pin_with_name(SSD1306_SDA_PIN, SSD1306_MPU6050_SDA_DESCRIPTION));
    bi_decl(bi_1pin_with_name(SSD1306_SCL_PIN, SSD1306_MPU6050_SCL_DESCRIPTION));
#elif defined USE_DISPLAY
    bi_decl(bi_1pin_with_name(SSD1306_SDA_PIN, SSD1306_SDA_DESCRIPTION));
    bi_decl(bi_1pin_with_name(SSD1306_SCL_PIN, SSD1306_SCL_DESCRIPTION));
#elif defined USE_GYRO
    bi_decl(bi_1pin_with_name(SSD1306_SDA_PIN, MPU6050_SDA_DESCRIPTION));
    bi_decl(bi_1pin_with_name(SSD1306_SCL_PIN, MPU6050_SCL_DESCRIPTION));
#endif
    bi_decl(bi_3pins_with_names(I2S_DATA_PIN, I2S_DATA_DESCRIPTION,
    I2S_CLOCK_PIN_BASE, I2S_BCK_DESCRIPTION,
    I2S_CLOCK_PIN_BASE+1, I2S_LRCK_DESCRIPTION));
}

static inline void i2s_audio_task() {
    static int16_t *last_buffer;
    int16_t *buffer = sound_i2s_get_next_buffer();
    if (buffer != last_buffer) {
        last_buffer = buffer;
        int16_t right_buffer; // Necessary quirk for compatibility with
                              // the original PRA32-U code
        for (int i = 0; i < AUDIO_BUFFER_LENGTH; i++) {
            uint16_t level = g_synth.process(right_buffer);
            // level = (int64_t)level * (int32_t)volume >> 16; // TODO, use main volume
            // Copy to I2S buffer
            *buffer++ = level;
            *buffer++ = level;
        }
    }
}

int main() {
    stdio_init_all();

    bi_decl_all();

    // Start the audio engine.
    sound_i2s_init(&sound_config);
    sound_i2s_playback_start();
    // Start the synth
    g_synth.initialize();

    // Launch the routine on the second core
    multicore_launch_core1(core1_main);

    state.context = CTX_KEY;
    state.key = 48; // C3
    update_key();
    update_scale();
    set_instrument(0);
    uint8_t control_number = dodepan_program_parameters[state.parameter];
    state.argument = g_synth.current_controller_value(control_number);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    mpr121_i2c_init();

    rotary_encoder_t *encoder = create_encoder(ENCODER_DT_PIN, ENCODER_CLK_PIN, encoder_onchange);
    button_t *button = create_button(ENCODER_SWITCH_PIN, button_onchange);

#if defined (USE_DISPLAY) || defined (USE_GYRO)
    gpio_init(SSD1306_SDA_PIN);
    gpio_init(SSD1306_SCL_PIN);
    gpio_set_function(SSD1306_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SSD1306_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SSD1306_SDA_PIN);
    gpio_pull_up(SSD1306_SCL_PIN);
#endif

    /* Display */
#if defined (USE_DISPLAY)
    i2c_init(SSD1306_I2C_PORT, SSD1306_I2C_FREQ);
    display_init(&display);
    display_draw(&display, &state);
#endif

#if defined (USE_GYRO)
    imu_init(); // MPU6050
#endif

    // Falloff values in case the IMU is disabled
    imu_data.deviation = 0.0f;
    imu_data.acceleration = 1.0f;

    // board_init(); // Midi
    // tusb_init(); // tinyusb

    // Non-time-critical routine, run by timer
    // battery_check_init(5000, NULL, (void*)battery_low_detected);

    // Use the onboard LED as a power-on indicator
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    power_on_alarm_id = add_alarm_in_ms(500, power_on_complete, NULL, true);

    static uint8_t throttle;
    while (true) { // Main loop
        mpr121_task();
        sleep_ms(1);
#if defined (USE_GYRO)
        if(throttle++ % 10 == 0) { // Limit the call rate
            imu_task(&imu_data);
            bending(imu_data.deviation);
        } 
#endif
        // tud_task(); // tinyusb device task
        i2s_audio_task();
    }
}

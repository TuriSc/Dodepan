/* Dodepan
 * Digital musical instrument. Touch-enabled, with multiple tunings, pitch bending and Midi out.
 * By Turi Scandurra – https://turiscandurra.com/circuits
 * v2.2.2b - 2024.06.21
 */

#include <stdio.h>
#include "config.h"         // Most configurable options are here
#include "pico/stdlib.h"
// Arduino types added for compatibility
typedef bool boolean;
typedef uint8_t byte;
#include "hardware/gpio.h"
#include "hardware/adc.h"   // Used for low battery detection
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
    .bits_per_sample = 16,
    .samples_per_buffer = AUDIO_BUFFER_LENGTH,
};

/* Globals */
PRA32_U_Synth g_synth;

state_t state;

Imu_data imu_data;

#if defined (USE_DISPLAY)
ssd1306_t display;
#endif

static alarm_id_t power_on_alarm_id;
static alarm_id_t long_press_alarm_id;

uint8_t num_scales = sizeof(scales)/sizeof(scales[0]);

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

void update_volume() {
    // Clip values
    if (state.volume < VOL_MIN) {
        state.volume = VOL_MIN;
    } else if (state.volume > 127) {
        state.volume = 127;
    }
    g_synth.control_change(dodepan_program_parameters[AMP_GAIN], state.volume);
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
    // Set the velocity according to accelerometer data.
    // The range of velocity is 0-127, but here we're clamping it to 64-127
    uint8_t velocity = 64 + imu_data.acceleration;

    g_synth.note_on(note, velocity);
    // tudi_midi_write24(0, 0x90, note, velocity);
}

void trigger_note_off(uint8_t note) {
    g_synth.note_off(note);
    // tudi_midi_write24(0, 0x80, note, 0);
}

void bending() {
    // Use the IMU to alter a parameter according to device tilting.
    // The range of the deviation is between -0.5 and +0.5.
    if(state.imu_dest & 0x02) {
        g_synth.control_change(FILTER_CUTOFF, imu_data.deviation_y);
    }

    // Split the bytes
    uint8_t bending_lsb = imu_data.deviation_x & 0x7F;
    uint8_t bending_msb = (imu_data.deviation_x >> 7) & 0x7F;

    // Send the instruction to the synth
    if(state.imu_dest & 0x01) {
        g_synth.pitch_bend(bending_lsb, bending_msb);
    }

    // Prepare the Midi message
    static uint8_t throttle;
    if(throttle++ % 10 != 0) return; // Limit the message rate
    // Pitch wheel range is between 0 and 16383 (0x0000 to 0x3FFF),
    // with 8192 (0x2000) being the center value.
    // Send the Midi message
    // tudi_midi_write24 (0, 0xE0, bending_lsb, bending_msb);
}

static inline void i2s_audio_task(void) {
    static int16_t *last_buffer;
    int16_t *buffer = sound_i2s_get_next_buffer();
    int16_t right_buffer; // Necessary quirk for compatibility with
                          // the original PRA32-U code
            
    if (buffer != last_buffer) { 
        last_buffer = buffer;
        for (int i = 0; i < AUDIO_BUFFER_LENGTH; i++) {
        short sample = g_synth.process(right_buffer);
        *buffer++ = sample;
        *buffer++ = sample;
        }
    }
}

/* I/O functions */

int64_t power_on_complete(alarm_id_t, void *) {
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    return 0;
}

int64_t on_long_press(alarm_id_t, void *) {
     switch(state.context) {
        case CTX_KEY:
        case CTX_SCALE:
        case CTX_INSTRUMENT:
            state.context = CTX_IMU_CONFIG;
        break;
        case CTX_IMU_CONFIG:
            state.context = CTX_VOLUME;
        break;
        case CTX_VOLUME:
        case CTX_INIT:
        default:
            ; // Do nothing
        break;
     }

#if defined (USE_DISPLAY)
    display_draw(&display, &state);
#endif
    return 0;
}

void encoder_onchange(rotary_encoder_t *encoder) {
    static long int last_position;
    long int position = encoder->position / 4; // Adjust the encoder sensitivity here
    int8_t direction = 0;
    if(last_position != position) {
        direction = (last_position < position ? 1 : -1);
    }
    last_position = position;

    if (direction == 1) {
        switch (state.context) {
            case CTX_KEY:
                // D#7 is the highest note that can be set as root note
                if(state.key < 99) { state.key++; }
                update_key();
            break;
            case CTX_SCALE:
                if(state.scale < num_scales - 1) { state.scale++; }
                update_scale();
            break;
            case CTX_INSTRUMENT:
                if(state.instrument < PRESET_PROGRAM_NUMBER_MAX + 1) { set_instrument(state.instrument + 1); }
            break;
            case CTX_IMU_CONFIG:
                state.imu_dest = (state.imu_dest == 0x3) ? 0x0 : state.imu_dest + 0x1;
            break;
            case CTX_VOLUME:
                if(state.volume < 127) { state.volume += VOL_INCR; }
                update_volume();
            break;
            case CTX_INIT:
            default:
                ; // Do nothing
            break;
        }
    } else if (direction == -1) {
        switch (state.context) {
            case CTX_KEY:
                if(state.key > 0) { state.key--; }
                update_key();
            break;
            case CTX_SCALE:
                if(state.scale > 0) { state.scale--; }
                update_scale();
            break;
            case CTX_INSTRUMENT:
                if(state.instrument > 0) { set_instrument(state.instrument - 1); }
            break;
            case CTX_IMU_CONFIG:
                state.imu_dest = (state.imu_dest == 0x0) ? 0x3 : state.imu_dest - 0x1;
            break;
            case CTX_VOLUME:
                if(state.volume > VOL_MIN) { state.volume -= VOL_INCR; }
                update_volume();
            break;
            case CTX_INIT:
            default:
                ; // Do nothing
            break;
        }
    }
#if defined (USE_DISPLAY)
    display_draw(&display, &state);
#endif
}

void intro_complete() {
    state.context = CTX_KEY;
}

void button_onchange(button_t *button_p) {
    button_t *button = (button_t*)button_p;
    if (long_press_alarm_id) cancel_alarm(long_press_alarm_id);
    if(button->state) return; // Ignore button release
    long_press_alarm_id = add_alarm_in_ms(1000, on_long_press, NULL, true);
    switch(state.context){
        case CTX_KEY:
        case CTX_SCALE:
        case CTX_INSTRUMENT:
            state.context++;
            if (state.context == CTX_INSTRUMENT + 1) { state.context = CTX_KEY; }
        break;
        case CTX_IMU_CONFIG:
            state.context = CTX_VOLUME;
            state.low_batt=true;// TODO Test
        break;
        case CTX_VOLUME:
            state.context = CTX_KEY;
        break;
        case CTX_INIT:
        default:
            ; // Do nothing
        break;
    }
#if defined (USE_DISPLAY)
    display_draw(&display, &state);
#endif
}

void battery_low_detected() {
    state.low_batt = true;
    battery_check_stop(); // Stop the timer
}

void bi_decl_all() {
    // Declare some binary information
    bi_decl(bi_program_name(PROGRAM_NAME));
    bi_decl(bi_program_description(PROGRAM_DESCRIPTION));
    bi_decl(bi_program_version_string(PROGRAM_VERSION));
    bi_decl(bi_program_url(PROGRAM_URL));
    bi_decl(bi_1pin_with_name(MPR121_SDA_PIN, MPR121_SDA_DESCRIPTION));
    bi_decl(bi_1pin_with_name(MPR121_SCL_PIN, MPR121_SCL_DESCRIPTION));
    bi_decl(bi_1pin_with_name(ENCODER_DT_PIN, ENCODER_DT_DESCRIPTION));
    bi_decl(bi_1pin_with_name(ENCODER_CLK_PIN, ENCODER_CLK_DESCRIPTION));
    bi_decl(bi_1pin_with_name(ENCODER_SWITCH_PIN, ENCODER_SWITCH_DESCRIPTION));
    bi_decl(bi_1pin_with_name(BATT_LVL_PIN, BATT_LVL_DESCRIPTION));
    
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

void core1_main() {
    while(true) {
        g_synth.secondary_core_process();
        i2s_audio_task();
    }
}

int main() {
    // Adjust the clock speed to be an even multiplier of the audio
    // sampling frequency
    if (SOUND_OUTPUT_FREQUENCY % 11025 == 0) { // For 22.05, 44.1, 88.2 kHz
        set_sys_clock_khz(135600, false);
    } else if (SOUND_OUTPUT_FREQUENCY % 8000 == 0) { // For 8, 16, 32, 48, 96, 192 kHz
        set_sys_clock_khz(147600, false);
    }
    stdio_init_all();

    bi_decl_all();

    // Initialize display and IMU (sharing an I²C bus)
#if defined (USE_DISPLAY) || defined (USE_GYRO)
    gpio_init(SSD1306_SDA_PIN);
    gpio_init(SSD1306_SCL_PIN);
    gpio_set_function(SSD1306_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SSD1306_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SSD1306_SDA_PIN);
    gpio_pull_up(SSD1306_SCL_PIN);

    i2c_init(SSD1306_I2C_PORT, SSD1306_I2C_FREQ);
#endif

#if defined (USE_DISPLAY)
    display_init(&display);
#endif

#if defined (USE_GYRO)
    imu_init(); // MPU6050
#endif

    // Start the audio engine.
    sound_i2s_init(&sound_config);

    // Start the synth
    g_synth.initialize();

    // Initialize state
    state.context = CTX_INIT;
    state.key = 48; // C3
    update_key();
    state.volume = 127; // Max
    update_volume();
    update_scale();
    set_instrument(0);
    state.imu_dest = 0x3; // Both effects are active

    // Launch the routine on the second core
    multicore_launch_core1(core1_main);

    // Initialize the touch module
    mpr121_i2c_init();

    // Initialize the rotary encoder and switch
    rotary_encoder_t *encoder = create_encoder(ENCODER_DT_PIN, ENCODER_CLK_PIN, encoder_onchange);
    button_t *button = create_button(ENCODER_SWITCH_PIN, button_onchange);

    // Falloff values in case the IMU is disabled
    imu_data.acceleration = 127;    // Max value
    imu_data.deviation_x = 0x2000;  // Center value
    imu_data.deviation_y = 64;      // Center value

    // board_init(); // Midi
    // tusb_init(); // tinyusb

    // Use the onboard LED as a power-on indicator
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    power_on_alarm_id = add_alarm_in_ms(500, power_on_complete, NULL, true);

    // Initialize the ADC, used for voltage sensing
    adc_init();
    // Non-time-critical routine, run by timer
    battery_check_init(5000, NULL, (void*)battery_low_detected);

#if defined (USE_DISPLAY)
    // Show a short intro animation. This will distract the user
    // while the hardware is calibrating
    intro_animation(&display, intro_complete);
    display_draw(&display, &state);
#else
    // Since there's no intro animation without a display,
    // let's trigger the new state manually
    state.context = CTX_KEY;
#endif

    while (true) { // Main loop
        mpr121_task();
    static uint8_t throttle;
#if defined (USE_GYRO)
        if(++throttle % 10 == 0) { // Limit the call rate
            imu_task(&imu_data);
            bending();
        }
#endif
        // tud_task(); // tinyusb device task
    }
}

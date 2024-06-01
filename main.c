/* Dodepan
 * Digital musical instrument. Touch-enabled, with multiple tunings, pitch bending and Midi out.
 * By Turi Scandurra – https://turiscandurra.com/circuits
 * v2.1.1b - 2024.05.27
 */

#include <stdio.h>
#include "config.h"         // Most configurable options are here
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/binary_info.h"
#include "pico_synth_ex.h"  // https://github.com/TuriSc/RP2040-pico_synth_ex
#include "encoder.h"        // https://github.com/TuriSc/RP2040-Rotary-Encoder
#include "button.h"         // https://github.com/TuriSc/RP2040-Button
// #include "bsp/board.h"      // For Midi
// #include "tusb.h"           // For Midi
#include "scales.h"
#include "battery-check.h"
#include "imu.h"
#include "demuxer.h"
#include "touch.h"

/* Audio setup */
#if USE_AUDIO_I2S
    #include "sound_i2s.h"
    static const struct sound_i2s_config sound_config = {
        .pin_sda         = I2S_DATA_PIN,
        .pin_scl         = I2S_CLOCK_PIN_BASE,
        .pin_ws          = I2S_CLOCK_PIN_BASE + 1,
        .sample_rate     = SOUND_OUTPUT_FREQUENCY,
        .pio_num         = 0, // 0 for pio0, 1 for pio1
    };

    repeating_timer_t i2s_timer;
#endif

/* Globals */
typedef struct {
    uint8_t key;
    uint8_t scale;
    uint8_t extended_scale[12];     // A copy of the scale array, padded with
                                    // repeated notes shifted up in octaves,
                                    // so that we always end up with 12 notes.
} Tuning;
Tuning tuning;

uint8_t instrument;

struct audio_buffer_pool *ap;

Imu_data imu_data;

// The following two arrays map key indices to notes and alterations
static uint8_t key_to_note_map[12] =    {0,0,1,1,2,3,3,4,4,5,5,6};
static bool key_to_alteration_map[12] = {0,1,0,1,0,0,1,0,1,0,1,0};

static alarm_id_t blink_alarm_id;
static alarm_id_t power_on_alarm_id;

uint8_t audio_pin_slice;
uint8_t num_scales = sizeof(scales)/sizeof(scales[0]);

// The encoder affects different parameters according to its current context
uint8_t encoder_context;
#define CTX_KEY 0
#define CTX_SCALE 1
#define CTX_INSTRUMENT 2

void blink();

/* Note and audio functions */

uint8_t get_note_by_id(uint8_t n) {
    return tuning.key + tuning.extended_scale[n];
}

uint8_t get_scale_size(uint8_t s) {
    for (uint8_t i=1; i<12; i++) {
        if (scales[s][i] == 0) return i;
    }
    return 12;
}

void update_key() {
    uint8_t key = tuning.key % 12;
    set_led(KEY, (9 + key_to_note_map[key])); // LEDs on C9 to C15
    set_led(ALTERATION, (key_to_alteration_map[key] ? 8 : -1)); // LED on C8, off if not an alteration
}

void update_scale() {
    uint8_t scale_size = get_scale_size(tuning.scale);
    uint8_t j = 0;
    uint8_t octave_shift = 0;
    for (uint8_t i=0; i<12; i++) {
        tuning.extended_scale[i] = (scales[tuning.scale][j] + octave_shift);
        j++;
        if (j >= scale_size) {j=0; octave_shift+=12;}
    }
    set_led(SCALE, tuning.scale % 8);
}

void set_instrument(uint8_t instr) {
    switch (instr) {
        case 0:
            load_preset((Preset_t) { 0, 1, 12, 16, 32, 72, 5, 19, 38, 14, 2, 2}); // Default
        break;
        case 1:
            control_message(PRESET_0);
        break;
        case 2:
            control_message(PRESET_1);
        break;
        case 3:
            control_message(PRESET_2);
        break;
        case 4:
            control_message(PRESET_3);
        break;
        case 5:
            control_message(PRESET_5);
        break;
        case 6:
            control_message(PRESET_6);
        break;
        case 7:
            control_message(PRESET_7);
        break;
        case 8:
            control_message(PRESET_8);
        break;
        case 9:
            control_message(PRESET_9);
        break;
    }
    instrument = instr;
}

// static inline uint32_t tudi_midi_write24 (uint8_t jack_id, uint8_t b1, uint8_t b2, uint8_t b3) {
//     uint8_t msg[3] = { b1, b2, b3 };
//     return tud_midi_stream_write(jack_id, msg, 3);
// }

void trigger_note_on(uint8_t note) {
    // Set the decay according to accelerometer data.
    // The range of decay is 0-64, but here we're clamping it to 32-48.
    int8_t decay = 32 + (16 * imu_data.acceleration);
    set_parameter(EG_DECAY_TIME, decay);
    // The range of velocity is 0-127, but here we're clamping it to 64-127
    uint8_t velocity = 64 + (63 * imu_data.acceleration);
    // tudi_midi_write24(0, 0x90, note, velocity);
    note_on(note);
    blink();
}

void trigger_note_off(uint8_t note) {
    note_off(note);
    // tudi_midi_write24(0, 0x80, note, 0);
}

void bending(float deviation) {
    // Use the IMU to alter a parameter according to device tilting.
    // The range of the deviation parameter is between -1 and 1 inclusive.
    // Parameters have a minimum, a maximum, and a center value, for example
    // FILTER_CUTOFF goes from 0 to 120, with 60 being the center value.
    int8_t cutoff = 60 + (60 * deviation);
    set_parameter(FILTER_CUTOFF, cutoff);

    // For pitch bending use this instead:
    // int8_t pitch = 16 + (32 * deviation);
    // set_parameter(OSC_2_FINE_PITCH, pitch);

    // Prepare Midi message
    static uint8_t throttle;
    if(throttle++ % 10 != 0) return; // Limit the message rate
    // Pitch wheel range is between 0 and 16383 (0x0000 to 0x3FFF),
    // with 8192 (0x2000) being the center value.
    unsigned int pitchval = (deviation+1.f)*8192;
    if (pitchval > 16383) pitchval = 16383;
    // Send Midi message
    // tudi_midi_write24 (0, 0xE0, (pitchval & 0x7F), (pitchval >> 7) & 0x7F);
}

/* I/O functions */

int64_t blink_complete() {
    gpio_put(LED_PIN, 0);
    return 0;
}

int64_t power_on_complete() {
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

    if (direction == 1) {
        switch (encoder_context) {
            case CTX_KEY:
                // 100 (E7) is the highest root note that can be set
                if(tuning.key < 99) tuning.key++;
                update_key();
            break;
            case CTX_SCALE:
                if(tuning.scale < num_scales - 1) tuning.scale++;
                update_scale();
            break;
            case CTX_INSTRUMENT:
                if(instrument < 9) set_instrument(instrument + 1);
            break;
        }
    } else if (direction == -1) {
        switch (encoder_context) {
            case CTX_KEY:
                if(tuning.key > 0) tuning.key--;
                update_key();
            break;
            case CTX_SCALE:
                if(tuning.scale > 0) tuning.scale--;
                update_scale();
            break;
            case CTX_INSTRUMENT:
                if(instrument > 0) set_instrument(instrument - 1);
            break;
        }
    }
}

void button_onchange(button_t *button_p) {
    button_t *button = (button_t*)button_p;
    if(!button->state) { // Button pressed
        encoder_context++;
        if (encoder_context > 3) { encoder_context = CTX_KEY; }
    }
}

void battery_low_callback(uint16_t battery_mv) {
    // Low battery detected
    gpio_put(LOW_BATT_LED_PIN, 1);
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
    bi_decl(bi_1pin_with_name(DEMUX_EN, DEMUX_EN_DESCRIPTION));
    bi_decl(bi_1pin_with_name(DEMUX_SIG, DEMUX_SIG_DESCRIPTION));
    bi_decl(bi_1pin_with_name(DEMUX_S0, DEMUX_S0_DESCRIPTION));
    bi_decl(bi_1pin_with_name(DEMUX_S1, DEMUX_S1_DESCRIPTION));
    bi_decl(bi_1pin_with_name(DEMUX_S2, DEMUX_S2_DESCRIPTION));
    bi_decl(bi_1pin_with_name(DEMUX_S3, DEMUX_S3_DESCRIPTION));
    bi_decl(bi_1pin_with_name(ENCODER_DT_PIN, ENCODER_DT_DESCRIPTION));
    bi_decl(bi_1pin_with_name(ENCODER_CLK_PIN, ENCODER_CLK_DESCRIPTION));
    bi_decl(bi_1pin_with_name(ENCODER_SWITCH_PIN, ENCODER_SWITCH_DESCRIPTION));
    #ifdef USE_GYRO
    bi_decl(bi_1pin_with_name(MPU6050_SDA_PIN, MPU6050_SDA_DESCRIPTION));
    bi_decl(bi_1pin_with_name(MPU6050_SCL_PIN, MPU6050_SCL_DESCRIPTION));
    #endif
    #if USE_AUDIO_PWM
    bi_decl(bi_2pins_with_names(PWM_AUDIO_PIN_RIGHT, PWM_AUDIO_RIGHT_DESCRIPTION,
    PWM_AUDIO_PIN_LEFT, PWM_AUDIO_LEFT_DESCRIPTION));
    #elif USE_AUDIO_I2S
    bi_decl(bi_3pins_with_names(I2S_DATA_PIN, I2S_DATA_DESCRIPTION,
    I2S_CLOCK_PIN_BASE, I2S_BCK_DESCRIPTION,
    I2S_CLOCK_PIN_BASE+1, I2S_LRCK_DESCRIPTION));
    #endif
}

int main() {
    // Set the system clock.
    // You can run it at the default speed, but expect
    // an offset with the output frequencies.
    set_sys_clock_khz(FCLKSYS / 1000, true);

    stdio_init_all();

    bi_decl_all();

    // Start the synth.
    #if USE_AUDIO_PWM
        // Pass the two output GPIOs as arguments.
        // Left channel must be active.
        // For a mono setup, the right channel can be disabled by passing -1.
        PWMA_init(PWM_AUDIO_PIN_RIGHT, PWM_AUDIO_PIN_LEFT);
    #elif USE_AUDIO_I2S
        sound_i2s_init(&sound_config);
        sound_i2s_playback_start();
        add_repeating_timer_ms(10, i2s_timer_callback, NULL, &i2s_timer);
    #endif
    set_instrument(0);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(LOW_BATT_LED_PIN);
    gpio_set_dir(LOW_BATT_LED_PIN, GPIO_OUT);

    #ifdef USE_GYRO
    gpio_init(MPU6050_SDA_PIN);
    gpio_init(MPU6050_SCL_PIN);
    gpio_set_function(MPU6050_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(MPU6050_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(MPU6050_SDA_PIN);
    gpio_pull_up(MPU6050_SCL_PIN);

    imu_init(); // MPU6050
    #endif

    // Falloff values in case IMU is disabled
    imu_data.deviation = 0.0f;
    imu_data.acceleration = 1.0f;

    mpr121_i2c_init();

    adc_init();
    adc_gpio_init(PIN_BATT_LVL);

    demuxer_init(DEMUX_EN, DEMUX_SIG, DEMUX_S0, DEMUX_S1, DEMUX_S2, DEMUX_S3);
    rotary_encoder_t *encoder = create_encoder(ENCODER_DT_PIN, ENCODER_CLK_PIN, encoder_onchange);
    button_t *button = create_button(ENCODER_SWITCH_PIN, button_onchange);

    // board_init(); // Midi
    // tusb_init(); // tinyusb

    // Non-time-critical routine, run by timer
    battery_check_init(5000, NULL, battery_low_callback);

    encoder_context = CTX_KEY;
    tuning.key = 48; // C3
    update_key();
    update_scale();

    // Use the onboard LED as a power-on indicator
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    power_on_alarm_id = add_alarm_in_ms(500, power_on_complete, NULL, true);
    
    while (1) { // Main loop
        mpr121_task();
        #ifdef USE_GYRO
        imu_task(&imu_data);
        bending(imu_data.deviation);
        #endif
        // tud_task(); // tinyusb device task
        demuxer_task();
    }
}

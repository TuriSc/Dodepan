/* Dodepan
 * Digital musical instrument. Touch-enabled, with multiple tunings, pitch bending and Midi out.
 * By Turi Scandurra – https://turiscandurra.com/circuits
 * v2.1.0b - 2024.01.20
 */

#include <stdio.h>
#include "config.h"         // Most configurable options are here
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/binary_info.h"
#include "mixer.h"          // https://github.com/TuriSc/RP2040-I2S-Audio-Mixer
#include "sound_i2s.h"
#include "button.h"         // https://github.com/TuriSc/RP2040-Button
// #include "bsp/board.h"      // For Midi
// #include "tusb.h"           // For Midi
#include "scales.h"
#include "samples/notes.h"
#include "battery-check.h"
#include "imu.h"
#include "demuxer.h"
#include "touch.h"

/* Globals */
typedef struct {
    uint8_t key;
    uint8_t scale;
    uint8_t extended_scale[12];     // A copy of the scale array, padded with
                                    // repeated notes shifted up in octaves,
                                    // so that we always end up with 12 notes.
} Tuning;

Tuning tuning;

struct audio_buffer_pool *ap;

Imu_data imu_data;

// The following two arrays map key indices to notes and alterations
static uint8_t key_to_note_map[12] =    {0,0,1,1,2,3,3,4,4,5,5,6};
static bool key_to_alteration_map[12] = {0,1,0,1,0,0,1,0,1,0,1,0};

static alarm_id_t blink_alarm_id;
static alarm_id_t power_on_alarm_id;

uint8_t audio_pin_slice;
uint8_t num_scales = sizeof(scales)/sizeof(scales[0]);

static const struct sound_i2s_config sound_config = {
  .pin_sda         = I2S_DATA_PIN,
  .pin_scl         = I2S_CLOCK_PIN_BASE,
  .pin_ws          = I2S_CLOCK_PIN_BASE + 1,
  .sample_rate     = SOUND_OUTPUT_FREQUENCY,
  .bits_per_sample = 16,
  .pio_num         = 0, // 0 for pio0, 1 for pio1
};

void blink();

/* Math functions */

inline int map(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/* Note and audio functions */

uint8_t get_note_by_id(uint8_t n) {
    return LOWEST_NOTE + tuning.key + tuning.extended_scale[n];
}

int play_note(uint8_t n, uint16_t velocity) {
    if (n < LOWEST_NOTE || n > HIGHEST_NOTE) return -1;
    uint16_t volume = map(velocity, 64, 127, 128, 1024); // Audio volume ranges between 128 and 1024.
    // int id = audio_play_once(notes[n-LOWEST_NOTE], SAMPLE_LENGTH);
    int id = audio_play_once(notes[24], SAMPLE_LENGTH/2);
    // if (id >= 0) audio_source_set_volume(id, volume);
    if (id >= 0) audio_source_set_volume(id, 128);
    return id;
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

// static inline uint32_t tudi_midi_write24 (uint8_t jack_id, uint8_t b1, uint8_t b2, uint8_t b3) {
//     uint8_t msg[3] = { b1, b2, b3 };
//     return tud_midi_stream_write(jack_id, msg, 3);
// }

void sendPitchWheelMessage(float deviation) {
    static uint8_t throttle;
    if(throttle++ % 10 != 0) return;
    // Pitch wheel range is between 0 and 16383 (0x0000 to 0x3FFF),
    // with 8192 (0x2000) being the center value.
    unsigned int pitchval = (deviation+1.f)*8192;
    if (pitchval > 16383) pitchval = 16383;
    // tudi_midi_write24 (0, 0xE0, (pitchval & 0x7F), (pitchval >> 7) & 0x7F);
}

void note_on(uint8_t note) {
    play_note(note, imu_data.velocity);
    // tudi_midi_write24(0, 0x90, note, imu_data.velocity);
    blink();
}

void note_off(uint8_t note) {
    // tudi_midi_write24(0, 0x80, note, 0);
}

// Detuning affects the entire audio engine
void detune(float deviation) { // The range of the parameter is between -1 and 1 inclusive.
    // TODO: detune via I2S
    // static float divider;
    // static const float unaltered_divider = 5.5796f;
    // if (deviation == 0) divider = unaltered_divider;
    // else if (deviation == 1) divider = 5.91138f;
    // else if (deviation == -1) divider = 5.26644f;
    // else divider = unaltered_divider * pow(1.05946f, deviation); // Chromatic scaling of the divider
    // pwm_set_clkdiv(audio_pin_slice, divider);
    sendPitchWheelMessage(deviation);
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

void blink() { // LED blinks for 0.1 seconds after a note is played
    gpio_put(LED_PIN, 1);
    if (blink_alarm_id) cancel_alarm(blink_alarm_id);
    blink_alarm_id = add_alarm_in_us(100000, blink_complete, NULL, true);
}

void button_onchange(button_t *button_p) {
  button_t *button = (button_t*)button_p;

  if(button->state) return; // Ignore button release, acting only on button press

  switch(button->pin) {
    case BTN_KEY_DOWN_PIN:
        if(tuning.key > 0) tuning.key--;
        update_key();
    break;
    case BTN_KEY_UP_PIN:
        if(tuning.key < NUM_ROOT_KEYS) tuning.key++;
        update_key();
    break;
    case BTN_SCALE_DOWN_PIN:
        if(tuning.scale > 0) tuning.scale--;
        update_scale();
    break;
    case BTN_SCALE_UP_PIN:
        if(tuning.scale < num_scales - 1) tuning.scale++;
        update_scale();
    break;
  }
}

void battery_low_callback(uint16_t battery_mv) {
    // Low battery detected
    gpio_put(LOW_BATT_LED_PIN, 1);
}

void bi_decl_all() {
    bi_decl(bi_program_name(PROGRAM_NAME));
    bi_decl(bi_program_description(PROGRAM_DESCRIPTION));
    bi_decl(bi_program_version_string(PROGRAM_VERSION));
    bi_decl(bi_program_url(PROGRAM_URL));
    bi_decl(bi_1pin_with_name(SOUND_PIN, SOUND_PIN_DESCRIPTION));
    bi_decl(bi_1pin_with_name(LED_PIN, LED_PIN_DESCRIPTION));
    bi_decl(bi_1pin_with_name(MPR121_SDA_PIN, MPR121_SDA_DESCRIPTION));
    bi_decl(bi_1pin_with_name(MPR121_SCL_PIN, MPR121_SCL_DESCRIPTION));
    bi_decl(bi_1pin_with_name(BTN_KEY_DOWN_PIN, BTN_KEY_DOWN_DESCRIPTION));
    bi_decl(bi_1pin_with_name(BTN_KEY_UP_PIN, BTN_KEY_UP_DESCRIPTION));
    bi_decl(bi_1pin_with_name(BTN_SCALE_DOWN_PIN, BTN_SCALE_DOWN_DESCRIPTION));
    bi_decl(bi_1pin_with_name(BTN_SCALE_UP_PIN, BTN_SCALE_UP_DESCRIPTION));
    bi_decl(bi_1pin_with_name(DEMUX_EN, DEMUX_EN_DESCRIPTION));
    bi_decl(bi_1pin_with_name(DEMUX_SIG, DEMUX_SIG_DESCRIPTION));
    bi_decl(bi_1pin_with_name(DEMUX_S0, DEMUX_S0_DESCRIPTION));
    bi_decl(bi_1pin_with_name(DEMUX_S1, DEMUX_S1_DESCRIPTION));
    bi_decl(bi_1pin_with_name(DEMUX_S2, DEMUX_S2_DESCRIPTION));
    bi_decl(bi_1pin_with_name(DEMUX_S3, DEMUX_S3_DESCRIPTION));
    #ifdef USE_GYRO
    bi_decl(bi_1pin_with_name(MPU6050_SDA_PIN, MPU6050_SDA_DESCRIPTION));
    bi_decl(bi_1pin_with_name(MPU6050_SCL_PIN, MPU6050_SCL_DESCRIPTION));
    #endif
    bi_decl(bi_3pins_with_names(I2S_DATA_PIN, I2S_DATA_DESCRIPTION,
    I2S_CLOCK_PIN_BASE, I2S_BCK_DESCRIPTION,
    I2S_CLOCK_PIN_BASE+1, I2S_LRCK_DESCRIPTION));
}

int main() {
    stdio_init_all();

    bi_decl_all();

    sound_i2s_init(&sound_config);
    sound_i2s_playback_start();

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
    imu_data.velocity = 127;
    imu_data.deviation = 0.0f;

    mpr121_i2c_init();

    adc_init();
    adc_gpio_init(PIN_BATT_LVL);

    button_t *key_down_b = create_button(BTN_KEY_DOWN_PIN, button_onchange);
    button_t *key_up_b = create_button(BTN_KEY_UP_PIN, button_onchange);
    button_t *scale_down_b = create_button(BTN_SCALE_DOWN_PIN, button_onchange);
    button_t *scale_up_b = create_button(BTN_SCALE_UP_PIN, button_onchange);

    demuxer_init(DEMUX_EN, DEMUX_SIG, DEMUX_S0, DEMUX_S1, DEMUX_S2, DEMUX_S3);

    // board_init(); // Midi
    // tusb_init(); // tinyusb

    // Non-time-critical routine, run by timer
    battery_check_init(5000, NULL, battery_low_callback);

    tuning.key = 12; // C3
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
        detune(imu_data.deviation);
        #endif
        // tud_task(); // tinyusb device task
        demuxer_task();
        audio_i2s_step();
        sleep_ms(1); // Things tend to break without this line
    }
}

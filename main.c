/* Dodepan
 * Digital musical instrument. Touch-enabled, with multiple tunings, pitch bending and Midi out.
 * By Turi Scandurra – https://turiscandurra.com/circuits
 * v2.0.1b - 2023.03.19
 */

#include <stdio.h>
#include "config.h"         // Most configurable options are here
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "RingBufMax.h"
#include "mixer.h"          // https://github.com/TuriSc/RP2040-I2S-Audio-Mixer
#include "button.h"         // https://github.com/TuriSc/RP2040-Button
#include "mpr121.h"         // https://github.com/antgon/pico-mpr121
#include "bsp/board.h"      // For Midi
#include "tusb.h"           // For Midi
#include "scales.h"
#include "samples/notes.h"

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

// The following two arrays map key indices to notes and alterations
static uint8_t key_to_note_map[12] =    {0,0,1,1,2,3,3,4,4,5,5,6};
static bool key_to_alteration_map[12] = {0,1,0,1,0,0,1,0,1,0,1,0};

static alarm_id_t blink_alarm_id;
static alarm_id_t power_on_alarm_id;
static repeating_timer_t low_battery_check_timer;

uint8_t audio_pin_slice;
uint8_t num_scales = sizeof(scales)/sizeof(scales[0]);

struct mpr121_sensor mpr121;

typedef struct {            // Accelerometer/gyroscope data, with fallback.
    uint16_t velocity;      // Velocity for Midi ranges between 64 and 127.
    float deviation;        // Detuning amount.
} Imu_data;
Imu_data imu_data;

#ifdef USE_GYRO
mpu6050_t mpu6050;
RingBufMax vel_ringbuf;     // Ring buffer that returns its highest value.
float gyro_multiplier;      // Will be set to 1 / (clock frequency * gyro's 131 LSB/°/sec).
#endif

#ifdef USE_DEMUX
int8_t leds_on[3] =                 {-1, -1, -1};
int8_t control_pins[4] =            {DEMUX_S0, DEMUX_S1, DEMUX_S2, DEMUX_S3};

uint8_t enable_pin;
int8_t signal_pin;
#endif

void blink();

/* Math functions */

inline int map(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

inline float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

inline int16_t clamp(int16_t x, int16_t min, int16_t max) {
    return x < min ? min : x > max ? max : x;
}


/* Note and audio functions */

uint8_t get_note_by_id(uint8_t n){
    return LOWEST_NOTE + tuning.key + tuning.extended_scale[n];
}

int play_note(uint8_t n, uint16_t velocity){
    if (n < LOWEST_NOTE || n > HIGHEST_NOTE) return -1;
    uint16_t volume = map(velocity, 64, 127, 128, 1024); // Audio volume ranges between 128 and 1024.
    int id = audio_play_once(notes[n-LOWEST_NOTE], SAMPLE_LENGTH);
    if (id >= 0) audio_source_set_volume(id, volume);
    return id;
}

uint8_t get_scale_size(uint8_t s){
    for (uint8_t i=1; i<12; i++) {
        if (scales[s][i] == 0) return i;
    }
    return 12;
}

void update_key(){
    #ifdef USE_DEMUX
    uint8_t key = tuning.key % 12;
    leds_on[1] = 9 + key_to_note_map[key];              // LEDs on C9 to C15
    leds_on[2] = (key_to_alteration_map[key] ? 8 : -1); // LED on C8, off if not an alteration
    #endif
}

void update_scale(){
    uint8_t scale_size = get_scale_size(tuning.scale);
    uint8_t j = 0;
    uint8_t octave_shift = 0;
    for (uint8_t i=0; i<12; i++) {
        tuning.extended_scale[i] = (scales[tuning.scale][j] + octave_shift);
        j++;
        if (j >= scale_size) {j=0; octave_shift+=12;}
    }
    #ifdef USE_DEMUX
    leds_on[0] = tuning.scale % 8;
    #endif
}

static inline uint32_t tudi_midi_write24 (uint8_t jack_id, uint8_t b1, uint8_t b2, uint8_t b3){
    uint8_t msg[3] = { b1, b2, b3 };
    return tud_midi_stream_write(jack_id, msg, 3);
}

void sendPitchWheelMessage(float deviation){
    static uint8_t throttle;
    if(throttle++ % 10 != 0) return;
    // Pitch wheel range is between 0 and 16383 (0x0000 to 0x3FFF),
    // with 8192 (0x2000) being the center value.
    unsigned int pitchval = (deviation+1.f)*8192;
    if (pitchval > 16383) pitchval = 16383;
    tudi_midi_write24 (0, 0xE0, (pitchval & 0x7F), (pitchval >> 7) & 0x7F);
}

void note_on(uint8_t note){
    play_note(note, imu_data.velocity);
    tudi_midi_write24(0, 0x90, note, imu_data.velocity);
    blink();
}

void note_off(uint8_t note){
    tudi_midi_write24(0, 0x80, note, 0);
}

// Detuning affects the entire audio engine
void detune(float deviation){ // The range of the parameter is between -1 and 1 inclusive.
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

#ifdef USE_DEMUX
/* 74HC4067 Multiplexer/Demultiplexer functions */
// The number of LEDs that are simultaneously on is always between 2 and 3.
// leds_on[0] for the scale (LEDs on C0 to C7),
// leds_on[1] for the key (LEDs on C9 to C15),
// leds_on[2] for alterations (LED on C8).

void demuxer_init(uint8_t en, uint8_t sig, int8_t s0, int8_t s1, int8_t s2, int8_t s3) {
	enable_pin = en;
    gpio_init(en);
    gpio_set_dir(en, GPIO_OUT);
    gpio_put(en, 1); // 1 = disable

	signal_pin = sig;
	gpio_init(signal_pin);
	gpio_set_dir(signal_pin, GPIO_OUT);

	control_pins[0] = s0;
    control_pins[1] = s1;
    control_pins[2] = s2;
    control_pins[3] = s3;
	for (uint8_t i = 0; i < 4; ++i){
		gpio_init(control_pins[i]);
		gpio_set_dir(control_pins[i], GPIO_OUT);
	}
    gpio_put(en, 0); // 0 = enable
    gpio_put(sig, 1);
}

void demuxer_task(){
    static uint8_t ticker;
    int8_t selected_led = leds_on[ticker++ % 3];

    if (selected_led > -1){
        for (uint8_t i = 0; i < 4; ++i)
        {
            gpio_put(control_pins[i], selected_led & 0x01);
            selected_led >>= 1;
        }
    }
}
#endif

/* MPR121 functions */

void mpr121_i2c_init(){
    i2c_init(MPR121_I2C_PORT, MPR121_I2C_FREQ);
    gpio_set_function(MPR121_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(MPR121_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(MPR121_SDA_PIN);
    gpio_pull_up(MPR121_SCL_PIN);

    mpr121_init(MPR121_I2C_PORT, MPR121_ADDR, &mpr121);
    mpr121_set_thresholds(MPR121_TOUCH_THRESHOLD,
                          MPR121_RELEASE_THRESHOLD, &mpr121);
    
    mpr121_enable_electrodes(12, &mpr121);
}

void mpr121_task(){
    bool is_touched;
    static bool was_touched[12];
    for(uint8_t i=0; i<12; i++) {
        mpr121_is_touched(i, &is_touched, &mpr121);
        if (is_touched != was_touched[i]){
            if(time_us_32() < 500000) return;  // Ignore readings for half a second,
                                                // allowing the MPR121 to calibrate.
            if (is_touched){
                note_on(get_note_by_id(i));
            } else {
                note_off(get_note_by_id(i));
            }
            was_touched[i] = is_touched;
        }
    }
}

/* IMU GY-521 - MPU6050 accelerometer and gyroscope */

#ifdef USE_GYRO
void imu_init(){
    mpu6050 = mpu6050_init(MPU6050_I2C_PORT, MPU6050_ADDRESS);

    if (mpu6050_begin(&mpu6050)) {
        mpu6050_set_scale(&mpu6050, MPU6050_SCALE_250DPS); // Gyroscope scale. ±250 °/sec = 131 LSB/°/sec
        mpu6050_set_range(&mpu6050, MPU6050_RANGE_2G); // Accelerometer range. ±2 g = 16384 LSB/g

        mpu6050_set_gyroscope_measuring(&mpu6050, true);
        mpu6050_set_accelerometer_measuring(&mpu6050, true);

        mpu6050_set_dhpf_mode(&mpu6050, MPU6050_DHPF_2_5HZ);
        mpu6050_set_dlpf_mode(&mpu6050, MPU6050_DLPF_3);

        mpu6050_calibrate_gyro(&mpu6050, 50);
    }
    float clk_sys_hz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS) / 1000.0f;  
    gyro_multiplier = 1 / (clk_sys_hz * 131.0f);
}

void imu_task(Imu_data * data){
    static float pitch;
    static float pitch_accel;
    static float accel_total;
    static uint16_t velocity_raw;

    mpu6050_event(&mpu6050);
    mpu6050_vectorf_t *accel = mpu6050_get_accelerometer(&mpu6050);
    mpu6050_vectorf_t *gyro = mpu6050_get_gyroscope(&mpu6050);

    pitch += gyro->x * gyro_multiplier;
    accel_total = sqrt((accel->x*accel->x)+(accel->y*accel->y)+(accel->z*accel->z)); 
    pitch_accel = asin((float)accel->y/accel_total) * RAD_TO_DEG;                                         
    pitch = (pitch + pitch_accel) / 2;
    // Clamp the range of velocity.
    velocity_raw = (uint16_t)fmap(accel_total - GRAVITY_CONSTANT, 0.0f, TAP_SENSITIVITY, 0.0f, 63.0f);
    uint16_t velocity_corrected = 64 + ringBufMax(&vel_ringbuf, velocity_raw);
    velocity_corrected = clamp(velocity_corrected, 64, 127);
    data->velocity = velocity_corrected;
    data->deviation = (((pitch + 90.0f) * 2.0 / 180.0f) - 1.0f) * DETUNE_FACTOR;
}
#endif

/* I/O functions */

int64_t blink_complete() {
    gpio_put(LED_PIN, 0);
    return 0;
}

int64_t power_on_complete() {
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    return 0;
}

void blink(){ // LED blinks for 0.1 seconds after a note is played
    gpio_put(LED_PIN, 1);
    if (blink_alarm_id) cancel_alarm(blink_alarm_id);
    blink_alarm_id = add_alarm_in_us(100000, blink_complete, NULL, true);
}

void button_onchange(button_t *button_p) {
  button_t *button = (button_t*)button_p;

  if(button->state) return; // Ignore button release, acting only on button press

  switch(button->pin){
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

static bool low_battery_check_task() {
   adc_select_input(3); // Battery level input
   // Coefficients from https://github.com/elehobica/pico_battery_op/
   uint16_t battery_mv = adc_read() * 9875 / (1<<12) - 20;
   if (battery_mv < LOW_BATT_THRESHOLD){ // Low battery
      gpio_put(LOW_BATT_LED_PIN, 1);
   } else {
      gpio_put(LOW_BATT_LED_PIN, 0);
   }
}

int main() {
    stdio_init_all();

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
    #ifdef USE_DEMUX
    bi_decl(bi_1pin_with_name(DEMUX_EN, DEMUX_EN_DESCRIPTION));
    bi_decl(bi_1pin_with_name(DEMUX_SIG, DEMUX_SIG_DESCRIPTION));
    bi_decl(bi_1pin_with_name(DEMUX_S0, DEMUX_S0_DESCRIPTION));
    bi_decl(bi_1pin_with_name(DEMUX_S1, DEMUX_S1_DESCRIPTION));
    bi_decl(bi_1pin_with_name(DEMUX_S2, DEMUX_S2_DESCRIPTION));
    bi_decl(bi_1pin_with_name(DEMUX_S3, DEMUX_S3_DESCRIPTION));
    #endif
    #ifdef USE_GYRO
    bi_decl(bi_1pin_with_name(MPU6050_SDA_PIN, MPU6050_SDA_DESCRIPTION));
    bi_decl(bi_1pin_with_name(MPU6050_SCL_PIN, MPU6050_SCL_DESCRIPTION));
    #endif

    ap = init_audio();

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

    #ifdef USE_DEMUX
    demuxer_init(DEMUX_EN, DEMUX_SIG, DEMUX_S0, DEMUX_S1, DEMUX_S2, DEMUX_S3);
    #endif

    board_init(); // Midi
    tusb_init(); // tinyusb

    // Non-time-critical routine, run by timer
    add_repeating_timer_ms(5000, low_battery_check_task, 0, &low_battery_check_timer);
    low_battery_check_task(); // First run

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
        tud_task(); // tinyusb device task
        #ifdef USE_DEMUX
        demuxer_task();
        #endif
        audio_i2s_step(ap);
        sleep_ms(1); // Things tend to break without this line
    }
}

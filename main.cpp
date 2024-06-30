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
#include "hardware/flash.h"
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

/* Globals */

PRA32_U_Synth g_synth;

state_t* state;

Imu_data imu_data;

#if defined (USE_DISPLAY)
ssd1306_t display;
#endif

static alarm_id_t power_on_alarm_id;
static alarm_id_t long_press_alarm_id;
static alarm_id_t flash_write_alarm_id;

/* Note and audio */

uint8_t get_note_by_id(uint8_t id) {
    return get_key() + get_extended_scale(id);
}

static const struct sound_i2s_config sound_config = {
    .pio_num         = I2S_PIO_NUM,
    .pin_scl         = I2S_CLOCK_PIN_BASE,
    .pin_sda         = I2S_DATA_PIN,
    .pin_ws          = I2S_CLOCK_PIN_BASE + 1,
    .sample_rate     = SOUND_OUTPUT_FREQUENCY,
    .bits_per_sample = 16,
    .samples_per_buffer = AUDIO_BUFFER_LENGTH,
};

void update_volume(uint8_t volume) {
    if(volume == get_volume()) { return; } // Nothing to do
    set_volume(volume);
    g_synth.control_change(dodepan_program_parameters[AMP_GAIN], get_volume());
}

void update_instrument(uint8_t instr) {
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
    set_instrument(instr);
}

// static inline uint32_t tudi_midi_write24 (uint8_t jack_id, uint8_t b1, uint8_t b2, uint8_t b3) {
//     uint8_t msg[3] = { b1, b2, b3 };
//     return tud_midi_stream_write(jack_id, msg, 3);
// }

void trigger_note_on(uint8_t note) {
    // Set the velocity according to accelerometer data.
    // The range of velocity is 0-127, but here it's clamped to 64-127
    uint8_t velocity = imu_data.acceleration;

    g_synth.note_on(note, velocity);
    // tudi_midi_write24(0, 0x90, note, velocity);
}

void trigger_note_off(uint8_t note) {
    g_synth.note_off(note);
    // tudi_midi_write24(0, 0x80, note, 0);
}

bool load_flash_data() { // Only called at startup
    // Read address is different than write address
    const uint8_t *stored_data = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

    // Validation
    uint8_t magic[MAGIC_NUMBER_LENGTH] = MAGIC_NUMBER;
    bool invalid_data = false;
    for(uint8_t i=0; i<MAGIC_NUMBER_LENGTH; i++){
        if(stored_data[i] != magic[i]){ return false; } // Invalid data
    }
    
    if((stored_data[MAGIC_NUMBER_LENGTH + 0] > 99)   || // Validate key
       (stored_data[MAGIC_NUMBER_LENGTH + 1] > 15)   || // Validate scale
       (stored_data[MAGIC_NUMBER_LENGTH + 2] > 8)    || // Validate instrument
       (stored_data[MAGIC_NUMBER_LENGTH + 3] > 0x03) || // Validate IMU configuration
       (stored_data[MAGIC_NUMBER_LENGTH + 3] > 127)     // Validate volume
    ) { return false; } // Invalid data

    // Data is valid and can be loaded safely
    set_key(            stored_data[MAGIC_NUMBER_LENGTH + 0]);
    set_scale(          stored_data[MAGIC_NUMBER_LENGTH + 1]);
    update_instrument(  stored_data[MAGIC_NUMBER_LENGTH + 2]);
    set_imu_axes(       stored_data[MAGIC_NUMBER_LENGTH + 3]);
    update_volume(      stored_data[MAGIC_NUMBER_LENGTH + 4]);

    return true;
}

void core1_main();

int64_t write_flash_data(alarm_id_t, void *) {
    // Initialize the buffer with a signature
    uint8_t flash_buffer[FLASH_PAGE_SIZE] = MAGIC_NUMBER;

    // Gather the rest of the data
    flash_buffer[MAGIC_NUMBER_LENGTH + 0] = get_key();
    flash_buffer[MAGIC_NUMBER_LENGTH + 1] = get_scale();
    flash_buffer[MAGIC_NUMBER_LENGTH + 2] = get_instrument();
    flash_buffer[MAGIC_NUMBER_LENGTH + 3] = get_imu_axes();
    flash_buffer[MAGIC_NUMBER_LENGTH + 4] = get_volume();

    // Make sure that the stored data is different than what we're about to write
    const uint8_t *stored_data = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
    if( stored_data[MAGIC_NUMBER_LENGTH + 0] == flash_buffer[MAGIC_NUMBER_LENGTH + 0] &&
        stored_data[MAGIC_NUMBER_LENGTH + 1] == flash_buffer[MAGIC_NUMBER_LENGTH + 1] &&
        stored_data[MAGIC_NUMBER_LENGTH + 2] == flash_buffer[MAGIC_NUMBER_LENGTH + 2] &&
        stored_data[MAGIC_NUMBER_LENGTH + 3] == flash_buffer[MAGIC_NUMBER_LENGTH + 3] &&
        stored_data[MAGIC_NUMBER_LENGTH + 4] == flash_buffer[MAGIC_NUMBER_LENGTH + 4] ) { return 0; }


    // Stop audio and synth processes on core1
    multicore_reset_core1();

    // Disable interrupts, write, and restore interrupts
    uint32_t ints_id = save_and_disable_interrupts();
	flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE); // Required for flash_range_program to work
	flash_range_program(FLASH_TARGET_OFFSET, flash_buffer, FLASH_PAGE_SIZE);
	restore_interrupts (ints_id);

    // Restart processes on core1
    multicore_launch_core1(core1_main);

    return 0;
}

// Use the IMU to alter parameters according to device tilting
void tilt_process() {
    if(get_imu_axes() & 0x02) {
        g_synth.control_change(FILTER_CUTOFF, imu_data.deviation_y);
    }

    // Split the bytes
    uint8_t bending_lsb = imu_data.deviation_x & 0x7F;
    uint8_t bending_msb = (imu_data.deviation_x >> 7) & 0x7F;

    // Send the instruction to the synth
    if(get_imu_axes() & 0x01) {
        g_synth.pitch_bend(bending_lsb, bending_msb);
    }

    // static uint8_t throttle;
    // if(throttle++ % 10 != 0) return; // Limit the message rate
    // Pitch wheel range is between 0 and 16383 (0x0000 to 0x3FFF),
    // with 8192 (0x2000) being the center value.
    // Send the Midi message
    // tudi_midi_write24 (0, 0xE0, bending_lsb, bending_msb);
}

/*
static void __not_in_flash_func(wait_for_reboot)(void) {
	__asm volatile ( " cpsid i " );
	while ( true )  {};					// reboot should happen here when watchdog times out
}
*/
static void __not_in_flash_func(i2s_audio_task)(void) {
// static inline void i2s_audio_task(void) {
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

int64_t power_on_complete(alarm_id_t, void *) {
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    return 0;
}

void intro_complete() {
    set_context(CTX_KEY);
}

/* I/O functions */

int64_t on_long_press(alarm_id_t, void *) {
    switch(get_context()) {
        case CTX_KEY:
        case CTX_SCALE:
        case CTX_INSTRUMENT:
            set_context(CTX_IMU_CONFIG);
        break;
        case CTX_IMU_CONFIG:
            set_context(CTX_VOLUME);
        break;
        case CTX_VOLUME:
        case CTX_INIT:
        default:
            ; // Do nothing
        break;
    }

#if defined (USE_DISPLAY)
    display_draw(&display, state);
#endif
    return 0;
}

void encoder_up() {
    uint8_t context = get_context();
    switch (context) {
        case CTX_KEY:
            // D#7 is the highest note that can be set as root note
            set_key_up();
            g_synth.all_notes_off();
        break;
        case CTX_SCALE:
            set_scale_up();
            g_synth.all_notes_off();
        break;
        case CTX_INSTRUMENT:
            update_instrument(get_instrument_up());
        break;
        case CTX_IMU_CONFIG:
            set_imu_axes_up();
        break;
        case CTX_VOLUME:
            update_volume(get_volume_up());
        break;
        case CTX_INIT:
        default:
            ; // Do nothing
        break;
    }
#if defined (USE_DISPLAY)
    display_draw(&display, state);
#endif
}

void encoder_down() {
    uint8_t context = get_context();
    switch (get_context()) {
        case CTX_KEY:
            set_key_down();
            g_synth.all_notes_off();
        break;
        case CTX_SCALE:
            set_scale_down();
            g_synth.all_notes_off();
        break;
        case CTX_INSTRUMENT:
            update_instrument(get_instrument_down());
        break;
        case CTX_IMU_CONFIG:
            set_imu_axes_down();
        break;
        case CTX_VOLUME:
            update_volume(get_volume_down());
        break;
        case CTX_INIT:
        default:
            ; // Do nothing
        break;
    }
#if defined (USE_DISPLAY)
    display_draw(&display, state);
#endif
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
        encoder_up();
    } else if (direction == -1) {
        encoder_down();
    }

    // Schedule writing settings to flash.
    // This delay is introduced to minimize write operations.
    if (flash_write_alarm_id) cancel_alarm(flash_write_alarm_id);
    flash_write_alarm_id = add_alarm_in_ms(FLASH_WRITE_DELAY_S * 1000, write_flash_data, NULL, true);
}

void button_onchange(button_t *button_p) {
    button_t *button = (button_t*)button_p;
    if (long_press_alarm_id) cancel_alarm(long_press_alarm_id);
    if(button->state) return; // Ignore button release
    long_press_alarm_id = add_alarm_in_ms(1000, on_long_press, NULL, true);

    uint8_t context = get_context();
    switch(context){
        case CTX_KEY:
        case CTX_SCALE:
        case CTX_INSTRUMENT:
            set_context_up(); 
        break;
        case CTX_IMU_CONFIG:
            set_context(CTX_VOLUME);
        break;
        case CTX_VOLUME:
            set_context(CTX_KEY);
        break;
        case CTX_INIT:
        default:
            ; // Do nothing
        break;
    }
#if defined (USE_DISPLAY)
    display_draw(&display, state);
#endif
}

void battery_low_detected() {
    set_low_batt(true);
    battery_check_stop(); // Stop the timer
}

// Declare binary information
void bi_decl_all() {
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

// Secondary core task
void core1_main() {
    while(true) {
        g_synth.secondary_core_process();
        i2s_audio_task();
    }
}

int main() {
    // Adjust the clock speed to be an even multiplier
    // of the audio sampling frequency
    if (SOUND_OUTPUT_FREQUENCY % 11025 == 0) { // For 22.05, 44.1, 88.2 kHz
        set_sys_clock_khz(135600, false);
    } else if (SOUND_OUTPUT_FREQUENCY % 8000 == 0) { // For 8, 16, 32, 48, 96, 192 kHz
        set_sys_clock_khz(147600, false);
    }
    stdio_init_all();
    sleep_ms(2000);

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

    // Initialize the state
    state = get_state();
    set_context(CTX_INIT);
    set_low_batt(false);

    // Attempt to load previous settings, if stored on flash
    bool data_loaded = load_flash_data();
    if(!data_loaded) {
        // Settings not loaded, initialize state with default values
        set_key(48); // C3
        set_scale(0); // Major
        update_instrument(0); // Dodepan custom preset
        set_imu_axes(0x3); // Both effects are active
        update_volume(127); // Max value
    }

    // Launch the routine on the second core
    multicore_launch_core1(core1_main);

    // Initialize the touch module
    mpr121_i2c_init();

    // Initialize the rotary encoder and switch
    rotary_encoder_t *encoder = create_encoder(ENCODER_DT_PIN, ENCODER_CLK_PIN, encoder_onchange);
    button_t *button = create_button(ENCODER_SWITCH_PIN, button_onchange);

    // Falloff values in case the IMU is disabled
    imu_data.deviation_x = 0x2000;  // Center value
    imu_data.deviation_y = 64;      // Center value
    imu_data.acceleration = 127;    // Max value

    // board_init(); // Midi
    // tusb_init(); // tinyusb

    // Use the onboard LED as a power-on indicator
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    power_on_alarm_id = add_alarm_in_ms(500, power_on_complete, NULL, true);

    // Initialize the ADC, used for voltage sensing
    adc_init();
    // Launch the battery check timed task
    battery_check_init(5000, NULL, (void*)battery_low_detected);

#if defined (USE_DISPLAY)
    // Show a short intro animation. This will distract the user
    // while the hardware is calibrating
    intro_animation(&display, intro_complete);
    display_draw(&display, state);
#else
    // Since there's no intro animation without a display,
    // let's trigger the new state manually
    set_context(CTX_KEY);
#endif

    while (true) { // Main loop
        mpr121_task();

#if defined (USE_GYRO)
        imu_task(&imu_data);
        tilt_process();
#endif
        // tud_task(); // tinyusb device task
    }
}

/* Dodepan
 * An expressive electronic instrument and Midi controller built on Raspberry Pi Pico".
 * By Turi Scandurra – https://turiscandurra.com/circuits
 * v2.4.0 - 2024.08.16
 */

#include "config.h"         // Most configurable options are here
#include "pico/stdlib.h"
// Arduino types added for compatibility
typedef bool boolean;
typedef uint8_t byte;
#include "hardware/clocks.h"
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
#include "bsp/board_api.h"  // For TinyUSB Midi
#include "tusb.h"           // For TinyUSB Midi
#include "scales.h"
#include "battery-check.h"
#include "imu.h"
#include "touch.h"
#include "looper.h"
#include "display/display.h"
#include "state.h"

/* Globals */

PRA32_U_Synth g_synth;

state_t* state;

Imu_data imu_data;

uint8_t **user_presets;
uint8_t **user_scales;

#if defined (USE_DISPLAY)
ssd1306_t display;
#endif

static alarm_id_t power_on_alarm_id;
static alarm_id_t long_press_alarm_id;
static alarm_id_t flash_write_alarm_id;

void core1_main();

/* User Presets and flash memory */
static inline uint8_t get_argument_from_parameter(uint8_t parameter) {
    uint8_t control_number = dodepan_program_parameters[parameter];
    return g_synth.current_controller_value(control_number);
}

static inline void update_argument_from_parameter(uint8_t parameter) {
    uint8_t control_number = dodepan_program_parameters[parameter];
    uint8_t argument = g_synth.current_controller_value(control_number);
    set_argument(argument);
}

static inline void update_degree() {
    uint8_t step = get_step();
    uint8_t degree = get_degree(step);
    set_extended_scale(step, degree);
    set_scale_unsaved(true); // Used when the user modifies a scale but does not save it,
                             // and the name of the scale must be changed to "Custom"
}

void load_user_preset(uint8_t instrument) {
    uint8_t preset_num = instrument - 9; // Subtracting the 9 default presets
    for (uint32_t i = 0; i < PROGRAM_PARAMS_NUM; i++) {
        g_synth.control_change(dodepan_program_parameters[i], user_presets[preset_num][i]);
    }
}

static inline void sync_control_change() {
    uint8_t parameter = get_parameter();
    uint8_t control_number = dodepan_program_parameters[parameter];
    uint8_t argument = get_argument();
    g_synth.control_change(control_number, argument);
}

void update_instrument() {
    uint8_t instrument = get_instrument();
    switch (instrument) {
        case 0: // Load custom Dodepan preset
            for (uint32_t i = 0; i < PROGRAM_PARAMS_NUM; i++) {
                g_synth.control_change(dodepan_program_parameters[i], dodepan_preset[i]);
            }
            set_preset_slot(-1); // No slot selected
        break;
        case 9:
        case 10:
        case 11:
        case 12:
            load_user_preset(instrument);
            // Set preset_slot selection to match loaded instrument
            set_preset_slot(instrument - 9);
        break;
        default: // case 1-8: load PRA32-U presets
            g_synth.program_change(instrument - 1);
            set_preset_slot(-1); // No slot selected
        break;
    }
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
    
    if((stored_data[MAGIC_NUMBER_LENGTH + 0] > HIGHEST_KEY)          || // Validate key
       (stored_data[MAGIC_NUMBER_LENGTH + 1] > NUM_SCALES -1)        || // Validate scale
       (stored_data[MAGIC_NUMBER_LENGTH + 2] > 8 + NUM_PRESET_SLOTS) || // Validate instrument
       (stored_data[MAGIC_NUMBER_LENGTH + 3] > 0x03)                 || // Validate IMU configuration
       (stored_data[MAGIC_NUMBER_LENGTH + 4] > 8)                       // Validate volume
    ) { return false; } // Invalid data

    // Data is valid and can be loaded safely
    set_key(             stored_data[MAGIC_NUMBER_LENGTH + 0]);
    uint8_t scale =      stored_data[MAGIC_NUMBER_LENGTH + 1] ;
    set_instrument(      stored_data[MAGIC_NUMBER_LENGTH + 2]);
    set_imu_axes(        stored_data[MAGIC_NUMBER_LENGTH + 3]);
    set_volume(          stored_data[MAGIC_NUMBER_LENGTH + 4]);

    // Load user presets
    uint8_t offset = MAGIC_NUMBER_LENGTH + 5;
    for (uint8_t i = 0; i < NUM_PRESET_SLOTS; i++) {
        for (uint8_t j = 0; j < PROGRAM_PARAMS_NUM; j++) {
            user_presets[i][j] = stored_data[offset++];
        }
    }
    update_instrument();

    // Load user scales
    for (uint8_t i = 0; i < NUM_SCALE_SLOTS; i++) {
        for (uint8_t j = 0; j < 12; j++) {
            user_scales[i][j] = stored_data[offset++];
        }
    }
    set_and_extend_scale(scale);

    return true;
}

int64_t write_flash_data(alarm_id_t, void *) {
    // Initialize the buffer with a signature
    uint8_t flash_buffer[FLASH_PAGE_SIZE] = MAGIC_NUMBER;
    uint8_t index = MAGIC_NUMBER_LENGTH;

    // Gather the rest of the data
    flash_buffer[MAGIC_NUMBER_LENGTH + 0] = get_key();
    flash_buffer[MAGIC_NUMBER_LENGTH + 1] = get_scale();
    flash_buffer[MAGIC_NUMBER_LENGTH + 2] = get_instrument();
    flash_buffer[MAGIC_NUMBER_LENGTH + 3] = get_imu_axes();
    flash_buffer[MAGIC_NUMBER_LENGTH + 4] = get_volume();

    // Stop here if the stored data is the same as what we're about to write
    const uint8_t *stored_data = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
    if( stored_data[MAGIC_NUMBER_LENGTH + 0] == flash_buffer[MAGIC_NUMBER_LENGTH + 0] &&
        stored_data[MAGIC_NUMBER_LENGTH + 1] == flash_buffer[MAGIC_NUMBER_LENGTH + 1] &&
        stored_data[MAGIC_NUMBER_LENGTH + 2] == flash_buffer[MAGIC_NUMBER_LENGTH + 2] &&
        stored_data[MAGIC_NUMBER_LENGTH + 3] == flash_buffer[MAGIC_NUMBER_LENGTH + 3] &&
        stored_data[MAGIC_NUMBER_LENGTH + 4] == flash_buffer[MAGIC_NUMBER_LENGTH + 4] &&
        get_preset_has_changes() == false &&
        get_scale_has_changes()  == false) { return 0; }

    // Add user presets to the write buffer
    uint8_t offset = MAGIC_NUMBER_LENGTH + 5;
    for (uint8_t i = 0; i < NUM_PRESET_SLOTS; i++) {
        for (uint8_t j = 0; j < PROGRAM_PARAMS_NUM; j++) {
            flash_buffer[offset++] = user_presets[i][j];
        }
    }

    // Add user scales to the write buffer
    for (uint8_t i = 0; i < NUM_SCALE_SLOTS; i++) {
        for (uint8_t j = 0; j < 12; j++) {
            flash_buffer[offset++] = user_scales[i][j];
        }
    }

    // Turn on built-in LED
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    // Stop audio and synth processes on core1
    multicore_reset_core1();

    // Disable interrupts, write, and restore interrupts
    uint32_t ints_id = save_and_disable_interrupts();
	flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE); // Required for flash_range_program to work
	flash_range_program(FLASH_TARGET_OFFSET, flash_buffer, FLASH_PAGE_SIZE);
	restore_interrupts (ints_id);

    // Restart processes on core1
    multicore_launch_core1(core1_main);

    // Wash "dirty" flags
    set_preset_has_changes(false);
    set_scale_has_changes(false);

    // Turn off built-in LED
    gpio_put(PICO_DEFAULT_LED_PIN, 0);

    return 0;
}

void request_flash_write() {
    // Schedule writing settings to flash.
    // This delay is introduced to minimize write operations.
    if (flash_write_alarm_id) cancel_alarm(flash_write_alarm_id);
    flash_write_alarm_id = add_alarm_in_ms(FLASH_WRITE_DELAY_S * 1000, write_flash_data, NULL, true);
}

void submit_preset_slot() {
    int8_t slot = get_preset_slot();
    if(slot == -1) { return; }
    for (uint8_t i = 0; i < PROGRAM_PARAMS_NUM; i++) {
        user_presets[slot][i] = get_argument_from_parameter(i);
    }
    set_preset_has_changes(true);
    request_flash_write();

    // Since we've written a preset, let's select it on the main screen
    set_instrument(9 + slot);
}

void submit_scale_slot() {
    int8_t slot = get_scale_slot();
    if(slot == -1) { return; }
    for (uint8_t i = 0; i < 12; i++) {
        user_scales[slot][i] = get_extended_scale(i);
    }
    set_scale_has_changes(true);
    set_scale_unsaved(false); // Used when the user modifies a scale but does not save it,
                              // and the name of the scale must be changed to "Custom"

    request_flash_write();

    // Since we've written a scale, let's select it on the main screen
    set_scale(NUM_SCALES_BUILTIN + slot);
}

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

static inline uint32_t tudi_midi_write24 (uint8_t jack_id, uint8_t b1, uint8_t b2, uint8_t b3) {
    uint8_t msg[3] = { b1, b2, b3 };
    return tud_midi_stream_write(jack_id, msg, 3);
}

void note_on(uint8_t id, uint8_t velocity) {
    uint8_t note = get_note_by_id(id);
    g_synth.note_on(note, velocity);
    tudi_midi_write24(0, 0x90, note, velocity);
}

void note_off(uint8_t id) {
    uint8_t note = get_note_by_id(id);
    g_synth.note_off(note);
    tudi_midi_write24(0, 0x80, note, 0);
}

void touch_on(uint8_t id) {
    // Set the velocity according to accelerometer data.
    // The range of velocity is 0-127, but here it's clamped to 64-127
    uint8_t velocity = imu_data.acceleration;

    note_on(id, velocity);
    looper_record(id, velocity, true);

    // Since a note_on event can start the looper recording,
    // a display draw needs to be called here.
#if defined (USE_DISPLAY)
    display_draw(&display, state);
#endif
}

void touch_off(uint8_t id) {
    note_off(id);
    looper_record(id, 0, false);
}

void all_notes_off() {
    g_synth.all_notes_off();
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

        static uint8_t throttle;
        if(throttle++ % 10 != 0) return; // Limit the message rate
        // Pitch wheel range is between 0 and 16383 (0x0000 to 0x3FFF),
        // with 8192 (0x2000) being the center value.
        // Send the Midi message
        tudi_midi_write24 (0, 0xE0, bending_lsb, bending_msb);
    }
}

static void __not_in_flash_func(i2s_audio_task)(void) {
    static int16_t *last_buffer;
    int16_t *buffer = sound_i2s_get_next_buffer();
    int16_t right_buffer; // Necessary quirk for compatibility with
                          // the original PRA32-U code
            
    if (buffer != last_buffer) { 
        last_buffer = buffer;
        for (int i = 0; i < AUDIO_BUFFER_LENGTH; i++) {
        short sample = g_synth.process(right_buffer);
        int temp = (int)sample * get_volume();
        short output = (short)(temp >> 3);
        *buffer++ = output;
        *buffer++ = output;
        }
    }
}

int64_t power_on_complete(alarm_id_t, void *) {
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    return 0;
}

void intro_complete() {
    set_context(CTX_SELECTION);
}

/* I/O functions */

void encoder_up() {
    context_t context = get_context();
    switch (context) {
        case CTX_SELECTION:
            set_selection_up();
        break;
        case CTX_KEY:
            // D#7 is the highest note that can be set as root note
            set_key_up();
            all_notes_off();
        break;
        case CTX_SCALE:
            set_scale_up();
            all_notes_off();
        break;
        case CTX_INSTRUMENT:
            set_instrument_up();
            update_instrument();
        break;
        case CTX_IMU_CONFIG:
            set_imu_axes_up();
        break;
        case CTX_VOLUME:
            set_volume_up();
        break;
        case CTX_SYNTH_EDIT_PARAM:
            set_parameter_up();
            update_argument_from_parameter(get_parameter());
        break;
        case CTX_SYNTH_EDIT_ARG:
            set_argument_up();
            sync_control_change();
        break;
        case CTX_SYNTH_EDIT_STORE:
            set_preset_slot_up();
        break;
        case CTX_LOOPER:
            looper_transpose_up();
        break;
        case CTX_SCALE_EDIT_STEP:
            set_step_up();
        break;
        case CTX_SCALE_EDIT_DEG:
            set_degree_up();
            update_degree();
        break;
        case CTX_SCALE_EDIT_STORE:
            set_scale_slot_up();
        break;
        case CTX_INIT:
        case CTX_INFO:
        default:
            ; // Do nothing
        break;
    }
#if defined (USE_DISPLAY)
    display_draw(&display, state);
#endif
}

void encoder_down() {
    context_t context = get_context();
    switch (get_context()) {
        case CTX_SELECTION:
            set_selection_down();
        break;
        case CTX_KEY:
            set_key_down();
            all_notes_off();
        break;
        case CTX_SCALE:
            set_scale_down();
            all_notes_off();
        break;
        case CTX_INSTRUMENT:
            set_instrument_down();
            update_instrument();
        break;
        case CTX_IMU_CONFIG:
            set_imu_axes_down();
        break;
        case CTX_VOLUME:
            set_volume_down();
        break;
        case CTX_SYNTH_EDIT_PARAM:
            set_parameter_down();
            update_argument_from_parameter(get_parameter());
        break;
        case CTX_SYNTH_EDIT_ARG:
            set_argument_down();
            sync_control_change();
        break;
        case CTX_SYNTH_EDIT_STORE:
            set_preset_slot_down();
        break;
        case CTX_LOOPER:
            looper_transpose_down();
        break;
        case CTX_SCALE_EDIT_STEP:
            set_step_down();
        break;
        case CTX_SCALE_EDIT_DEG:
            set_degree_down();
            update_degree();
        break;
        case CTX_SCALE_EDIT_STORE:
            set_scale_slot_down();
        break;
        case CTX_INIT:
        case CTX_INFO:
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
}

int64_t on_long_press(alarm_id_t, void *) {
    context_t context = get_context();
    selection_t selection = get_selection();
    switch(context) {
        case CTX_SELECTION:
            switch (selection) {
                case SELECTION_SCALE:
                    set_context(CTX_SCALE_EDIT_STEP);
                break;
                default:
                    set_context(CTX_INFO);
                break;
                }
        break;
        case CTX_KEY:
        case CTX_VOLUME:
        case CTX_IMU_CONFIG:
            set_context(CTX_SELECTION);
        break;
        case CTX_INSTRUMENT:
            set_context(CTX_SYNTH_EDIT_PARAM);
            update_argument_from_parameter(get_parameter());
        break;
        case CTX_SCALE:
            set_context(CTX_SCALE_EDIT_STEP);
        break;
        case CTX_SCALE_EDIT_STEP:
        case CTX_SCALE_EDIT_DEG:
            set_context(CTX_SCALE_EDIT_STORE);
        break;
        case CTX_SYNTH_EDIT_PARAM:
        case CTX_SYNTH_EDIT_ARG:
            set_context(CTX_SYNTH_EDIT_STORE);
        break;
        case CTX_INFO:
            set_context(CTX_SELECTION);
        break;
        case CTX_LOOPER:
            looper_disable();
            set_context(CTX_SELECTION);
        break;
        case CTX_INIT:
        case CTX_SYNTH_EDIT_STORE:
        case CTX_SCALE_EDIT_STORE:
        default:
            ; // Do nothing
        break;
    }

#if defined (USE_DISPLAY)
    display_draw(&display, state);
#endif
    return 0;
}

void button_onchange(button_t *button_p) {
    button_t *button = (button_t*)button_p;
    if (long_press_alarm_id) cancel_alarm(long_press_alarm_id);
    if (button->state) return; // Ignore button release
    long_press_alarm_id = add_alarm_in_ms(LONG_PRESS_THRESHOLD, on_long_press, NULL, true);

    context_t context = get_context();
    selection_t selection = get_selection();

    switch(context){
        case CTX_SELECTION: {
            switch (selection) {
                case SELECTION_KEY:
                    set_context(CTX_KEY);
                break;
                case SELECTION_SCALE:
                    set_context(CTX_SCALE);
                break;
                case SELECTION_INSTRUMENT:
                    set_context(CTX_INSTRUMENT);
                break;
                case SELECTION_VOLUME:
                    set_context(CTX_VOLUME);
                break;
                case SELECTION_LOOPER:
                    set_context(CTX_LOOPER);
                    looper_enable();
                break;
                case SELECTION_IMU_CONFIG:
                    set_context(CTX_IMU_CONFIG);
                break;
            }
        }
        break;
        case CTX_INFO:
        case CTX_KEY:
        case CTX_SCALE:
        case CTX_INSTRUMENT:
        case CTX_VOLUME:
        case CTX_IMU_CONFIG:
            set_context(CTX_SELECTION);
            request_flash_write();
        break;
        case CTX_SYNTH_EDIT_PARAM:
            set_context(CTX_SYNTH_EDIT_ARG);
        break;
        case CTX_SYNTH_EDIT_ARG:
            set_context(CTX_SYNTH_EDIT_PARAM);
        break;
        case CTX_SYNTH_EDIT_STORE:
            submit_preset_slot();
            set_context(CTX_SELECTION);
            set_selection(SELECTION_INSTRUMENT);
        break;
        case CTX_LOOPER:
            looper_onpress();
        break;
        case CTX_SCALE_EDIT_STEP:
            set_context(CTX_SCALE_EDIT_DEG);
        break;
        case CTX_SCALE_EDIT_DEG:
            set_context(CTX_SCALE_EDIT_STEP);
        break;
        case CTX_SCALE_EDIT_STORE:
            submit_scale_slot();
            set_context(CTX_SELECTION);
            set_selection(SELECTION_SCALE);
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
    
#if defined USE_DISPLAY && defined USE_IMU
    bi_decl(bi_1pin_with_name(SSD1306_SDA_PIN, SSD1306_MPU6050_SDA_DESCRIPTION));
    bi_decl(bi_1pin_with_name(SSD1306_SCL_PIN, SSD1306_MPU6050_SCL_DESCRIPTION));
#elif defined USE_DISPLAY
    bi_decl(bi_1pin_with_name(SSD1306_SDA_PIN, SSD1306_SDA_DESCRIPTION));
    bi_decl(bi_1pin_with_name(SSD1306_SCL_PIN, SSD1306_SCL_DESCRIPTION));
#elif defined USE_IMU
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

    bi_decl_all();

    // Enable Midi device functionality
    board_init();
    tud_init(BOARD_TUD_RHPORT);

    if (board_init_after_tusb) {
        board_init_after_tusb();
    }

    // Initialize display and IMU (sharing an I²C bus)
#if defined (USE_DISPLAY) || defined (USE_IMU)
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

#if defined (USE_IMU)
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
    set_preset_slot(-1); // No slot selected
    set_scale_slot(-1);  // No slot selected
    set_preset_has_changes(false);
    set_scale_has_changes(false);

    // Allocate memory for the user presets and scales arrays
    user_presets = (uint8_t **)malloc(NUM_PRESET_SLOTS * sizeof(uint8_t *));
    for (uint8_t i = 0; i < NUM_PRESET_SLOTS; i++) {
        user_presets[i] = (uint8_t *)malloc(PROGRAM_PARAMS_NUM * sizeof(uint8_t));
    }

    user_scales = (uint8_t **)malloc(NUM_SCALE_SLOTS * sizeof(uint8_t *));
    for (uint8_t i = 0; i < NUM_SCALE_SLOTS; i++) {
        user_scales[i] = (uint8_t *)malloc(12 * sizeof(uint8_t));
    }

    // Attempt to load previous settings, if stored on flash
    bool data_loaded = load_flash_data();
    if(!data_loaded) {
        // Settings not loaded, initialize state with default values
        set_key(60); // C4
        set_and_extend_scale(0); // Major
        set_scale_unsaved(false);
        set_instrument(0); // Dodepan custom preset
        update_instrument();
        set_imu_axes(0x02); // Filter cutoff modulation enabled, pitch bending disabled
        set_volume(8); // Max value
        // Copy the custom preset to the four user preset slots
        for (uint8_t i = 0; i < NUM_PRESET_SLOTS; i++) {
            for (uint8_t j = 0; j < PROGRAM_PARAMS_NUM; j++) {
                user_presets[i][j] = dodepan_preset[j];
            }
        }
        // Set all user scales to chromatic
        for (uint8_t i = 0; i < NUM_SCALE_SLOTS; i++) {
            for (uint8_t j = 0; j < 12; j++) {
                user_scales[i][j] = j;
            }
        }
    }

    // Launch the routine on the second core
    multicore_launch_core1(core1_main);

    // Initialize the touch module
    mpr121_i2c_init();

    // Initialize the looper with a maximum of 512 note events
    looper_init(512);

    // Initialize the rotary encoder and switch
#if defined (ENCODER_USE_PULLUPS)
    gpio_pull_up(ENCODER_DT_PIN);
    gpio_pull_up(ENCODER_CLK_PIN);
    gpio_pull_up(ENCODER_SWITCH_PIN);
#endif
    rotary_encoder_t *encoder = create_encoder(ENCODER_DT_PIN, ENCODER_CLK_PIN, encoder_onchange);
    button_t *button = create_button(ENCODER_SWITCH_PIN, button_onchange);

    // Falloff values in case the IMU is disabled
    imu_data.deviation_x = 0x2000;  // Center value
    imu_data.deviation_y = 64;      // Center value
    imu_data.acceleration = 127;    // Max value

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
    set_context(CTX_SELECTION);
#endif

    while (true) { // Main loop
        mpr121_task();

#if defined (USE_IMU)
        if(get_imu_axes() > 0) {
            imu_task(&imu_data);
            tilt_process();
        }
#endif
        looper_task();
        tud_task(); // tinyusb device task
    }
}

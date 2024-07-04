#include "pico/stdlib.h"
#include "state.h"
#include "config.h"
#include "scales.h"
#include "instrument_preset.h"

// Declare the static state instance
static state_t state;

state_t* get_state(void) {
    return &state;
}

/* Key */

uint8_t get_key() {
    return state.key;
}

void set_key(uint8_t key) {
    state.key = key;
    set_tonic(key % 12);
    set_octave(key / 12); // C3 is on octave 5 in this system because
                          // octave -1 is the first element
    // Map key indices to alterations
    static bool key_to_alteration_map[12] = {0,1,0,1,0,0,1,0,1,0,1,0};
    bool is_alteration = (key_to_alteration_map[get_tonic()] ? 1 : 0);
    set_alteration(is_alteration);
}

void set_key_up() {
    uint8_t key = get_key();
    if(key < 99) {
        key++;
        set_key(key);
    }
}

void set_key_down() {
    uint8_t key = get_key();
    if(key > 0) {
        key--;
        set_key(key);
    }
}

/* Scale and extended scale*/

#define NUM_SCALES sizeof(scales)/sizeof(scales[0])

static inline uint8_t get_scale_size(uint8_t scale) {
    // Find the first zero element and return its index
    for (uint8_t i=1; i<12; i++) {
        if (scales[scale][i] == 0) { return i; }
    }
    return 12;
}

// Compute the extended scale
static inline void update_extended_scale() {
    uint8_t scale_size = get_scale_size(get_scale());
    uint8_t j = 0;
    uint8_t octave_shift = 0;
    for (uint8_t i=0; i<12; i++) {
        uint8_t note = scales[get_scale()][j] + octave_shift;
        set_extended_scale(i, note);
        j++;
        // Repeat the scale on higher octaves to fill all the available positions
        if (j >= scale_size) { j=0; octave_shift += 12; }
    }
}

uint8_t get_scale() {
    return state.scale;
}

void set_scale(uint8_t scale) {
    state.scale = scale;
    update_extended_scale();
}

void set_scale_up() {
    uint8_t scale = get_scale();
    if(scale < NUM_SCALES - 1) {
        scale++;
        set_scale(scale);
    }
}

void set_scale_down() {
    uint8_t scale = get_scale();
    if(scale > 0) {
        scale--;
        set_scale(scale);
    }
}

uint8_t get_extended_scale(uint8_t index) {
    return state.extended_scale[index];
}

void set_extended_scale(uint8_t index, uint8_t note) {
    state.extended_scale[index] = note;
}

/* Instrument */

uint8_t get_instrument() {
    return state.instrument;
}

void set_instrument(uint8_t instrument) {
    state.instrument = instrument;
}

void set_instrument_up() {
    uint8_t instrument = get_instrument();
    if(instrument < 8) { instrument++; } // TODO User presets
    set_instrument(instrument);
}

void set_instrument_down() {
    uint8_t instrument = get_instrument();
    if(instrument > 0) { instrument--; }
    set_instrument(instrument);
}

/* Context */
uint8_t get_context() {
    return state.context;
}

void set_context(context_t context) {
    state.context = context;
}

/* Selection */
uint8_t get_selection() {
    return state.selection;
}

void set_selection(selection_t selection) {
    state.selection = selection;
}

void set_selection_up() {
    selection_t selection = get_selection();
    // Wrap around
    selection = (selection_t)(((int)selection + 1) % SELECTION_LAST);
    set_selection(selection);
}

void set_selection_down() {
    selection_t selection = get_selection();
    // Wrap around
    selection = (selection_t)(((int)selection - 1 + SELECTION_LAST) % SELECTION_LAST);
    set_selection(selection);
}

/* IMU axes */

uint8_t get_imu_axes() {
    return state.imu_axes;
}

void set_imu_axes(uint8_t imu_axes) {
    state.imu_axes = imu_axes;
}

void set_imu_axes_up() {
    uint8_t imu_axes = get_imu_axes();
    imu_axes = (imu_axes == 0x3) ? 0x0 : imu_axes + 0x1;
    set_imu_axes(imu_axes);
}

void set_imu_axes_down() {
    uint8_t imu_axes = get_imu_axes();
    imu_axes = (imu_axes == 0x0) ? 0x3 : imu_axes - 0x1;
    set_imu_axes(imu_axes);
}

/* Tonic */

uint8_t get_tonic() {
    return state.tonic;
}

void set_tonic(uint8_t tonic) {
    state.tonic = tonic;
}

/* Octave */
uint8_t get_octave() {
    return state.octave;
}

void set_octave(uint8_t octave) {
    state.octave = octave;
}

/* Alteration */

uint8_t get_alteration() {
    return state.is_alteration;
}

void set_alteration(uint8_t alteration) {
    state.is_alteration = alteration;
}

/* Volume */

uint8_t get_volume() {
    return state.volume;
}

void set_volume(uint8_t vol) {
    uint8_t volume = vol;
    // Clip values
    if (volume < 1) {
        volume = 1;
    } else if (volume > 8) {
        volume = 8;
    }
    state.volume = volume;
}

void set_volume_up() {
    uint8_t volume = get_volume() + 1;
    set_volume(volume);
}

void set_volume_down() {
    uint8_t volume = get_volume() - 1;
    set_volume(volume);
}

/* Low batt */

bool get_low_batt() {
    return state.low_batt;
}

void set_low_batt(bool low_batt) {
    state.low_batt = low_batt;
}

/* Parameter */

uint8_t get_parameter() {
    return state.parameter;
}

void set_parameter(uint8_t param) {
    uint8_t parameter = param;
    uint8_t num_parameters = sizeof(dodepan_program_parameters) / sizeof(dodepan_program_parameters[0]);
    // Clip values
    if (parameter >= num_parameters) {
        parameter = 0;
    }
    state.parameter = parameter;
}

void set_parameter_up() {
    uint8_t parameter = get_parameter() + 1;
    // Wrap around
    if (parameter > 127) {
        parameter = 0;
    }
    set_parameter(parameter);
}

void set_parameter_down() {
    uint8_t parameter = get_parameter() - 1;
    uint8_t num_parameters = sizeof(dodepan_program_parameters) / sizeof(dodepan_program_parameters[0]);
    // Wrap around
    if (parameter >= num_parameters) {
        parameter = num_parameters - 1;
    }
    set_parameter(parameter);
}

/* Argument */

uint8_t get_argument() {
    return state.argument;
}

void set_argument(uint8_t arg) {
    uint8_t argument = arg;
    // Clip values
    if (argument > 127) {
        argument = 127;
    }
    state.argument = argument;
}

void set_argument_up() {
    uint8_t argument = get_argument();
    // Do not wrap around
    if (argument < 127) {
        argument++;
    }
    set_argument(argument);
}

void set_argument_down() {
    uint8_t argument = get_argument();
    // Do not wrap around
    if (argument > 0) {
        argument--;
    }
    set_argument(argument);
}
#ifndef STATE_H
#define STATE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum context {
    CTX_INIT,
    CTX_INFO,
    CTX_SELECTION,
    CTX_KEY,
    CTX_SCALE,
    CTX_INSTRUMENT,
    CTX_VOLUME,
    CTX_CONTRAST,
    CTX_LOOPER,
    CTX_IMU_CONFIG,
    CTX_SYNTH_EDIT_PARAM,
    CTX_SYNTH_EDIT_ARG,
    CTX_SYNTH_EDIT_STORE,
    CTX_SCALE_EDIT_STEP,
    CTX_SCALE_EDIT_DEG,
    CTX_SCALE_EDIT_STORE,
} context_t;

typedef enum selection {
    SELECTION_KEY,
    SELECTION_SCALE,
    SELECTION_INSTRUMENT,
    SELECTION_VOLUME,
    SELECTION_LOOPER,
    SELECTION_IMU_CONFIG,
    SELECTION_LAST,
} selection_t;

typedef struct state {
    uint8_t key;
    uint8_t scale;
    uint8_t tonic;                  // The 'base' note
    uint8_t extended_scale[12];     // A copy of the scale array, padded with
                                    // repeated notes shifted up in octaves,
                                    // so that we always end up with 12 notes.
    bool is_alteration;
    uint8_t octave;
    uint8_t instrument;
    context_t context;              // The encoder affects different parameters
                                    // according to its current context
    selection_t selection;
    uint8_t volume;
    uint8_t contrast;               // Value to control the SSD1306 display brightness (aka "contrast")

    // Instrument presets
    uint8_t parameter;
    uint8_t argument;
    int8_t preset_slot;
    bool preset_has_changes;        // Used to determine whether a flash write is required

    // Custom scales
    int8_t scale_slot;
    bool scale_has_changes;         // Used to determine whether a flash write is required
    bool scale_unsaved;             // Only used to manage the scale display name
    uint8_t step;

    uint8_t imu_axes;               // Determines what the IMU is controlling:
                                    // 0x0 - no effect
                                    // 0x1 - rolling the device bends the pitch
                                    // 0x2 - pitching (as in tilting) the device changes the filter cutoff frequency
                                    // 0x3 - (default) both effects are active

    bool low_batt;                  // Low battery detected
} state_t;

extern uint8_t get_note_by_id(uint8_t id);

state_t* get_state(void);

uint8_t get_key();
void set_key(uint8_t key);
void set_key_up();
void set_key_down();

uint8_t get_scale();
void set_scale(uint8_t scale);
void set_and_extend_scale(uint8_t scale);
void set_scale_up();
void set_scale_down();

uint8_t get_extended_scale(uint8_t index);
void set_extended_scale(uint8_t index, uint8_t degree);

uint8_t get_instrument();
void set_instrument(uint8_t instrument);
void set_instrument_up();
void set_instrument_down();

context_t get_context();
void set_context(context_t context);

selection_t get_selection();
void set_selection(selection_t selection);
void set_selection_up();
void set_selection_down();

uint8_t get_imu_axes();
void set_imu_axes(uint8_t imu_axes);
void set_imu_axes_up();
void set_imu_axes_down();

uint8_t get_tonic();
void set_tonic(uint8_t tonic);

uint8_t get_octave();
void set_octave(uint8_t octave);

uint8_t get_alteration();
void set_alteration(uint8_t alteration);

uint8_t get_volume();
void set_volume(uint8_t volume);
void set_volume_up();
void set_volume_down();

uint8_t get_contrast();
void set_contrast(uint8_t contrast);
void set_contrast_up();
void set_contrast_down();

bool get_low_batt();
void set_low_batt(bool low_batt);

uint8_t get_parameter();
void set_parameter(uint8_t param);
void set_parameter_up();
void set_parameter_down();

uint8_t get_argument();
void set_argument(uint8_t arg);
void set_argument_up();
void set_argument_down();

int8_t get_preset_slot();
void set_preset_slot(int8_t slot);
void set_preset_slot_up();
void set_preset_slot_down();
bool get_preset_has_changes();
void set_preset_has_changes(bool flag);

int8_t get_scale_slot();
void set_scale_slot(int8_t slot);
void set_scale_slot_up();
void set_scale_slot_down();
bool get_scale_has_changes();
void set_scale_has_changes(bool flag);
bool get_scale_unsaved();
void set_scale_unsaved(bool flag);

uint8_t get_step();
void set_step(uint8_t step);
void set_step_up();
void set_step_down();

uint8_t get_degree(uint8_t step);
void set_degree(uint8_t step, uint8_t degree);
void set_degree_up();
void set_degree_down();

#ifdef __cplusplus
}
#endif

#endif

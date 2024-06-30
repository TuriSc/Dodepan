#ifndef STATE_H
#define STATE_H

#ifdef __cplusplus
extern "C" {
#endif

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
    uint8_t context;                // The encoder affects different parameters
                                    // according to its current context
    uint8_t volume;

    uint8_t imu_axes;               // Determines what the IMU is controlling:
                                    // 0x0 - no effect
                                    // 0x1 - rolling the device bends the pitch
                                    // 0x2 - pitching (as in tilting) the device changes the filter cutoff frequency
                                    // 0x3 - (default) both effects are active

    bool low_batt;                  // Low battery detected
} state_t;

#define CTX_INIT        0
#define CTX_KEY         1
#define CTX_SCALE       2
#define CTX_INSTRUMENT  3
#define CTX_IMU_CONFIG  4
#define CTX_VOLUME      5

extern uint8_t get_note_by_id(uint8_t id);

state_t* get_state(void);

uint8_t get_key();
void set_key(uint8_t key);
void set_key_up();
void set_key_down();

uint8_t get_scale();
void set_scale(uint8_t scale);
void set_scale_up();
void set_scale_down();

uint8_t get_extended_scale(uint8_t index);
void set_extended_scale(uint8_t index, uint8_t note);

uint8_t get_instrument();
void set_instrument(uint8_t instrument);
uint8_t get_instrument_up();
uint8_t get_instrument_down();

uint8_t get_context();
void set_context(uint8_t context);
void set_context_up();

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
uint8_t get_volume_up();
uint8_t get_volume_down();

bool get_low_batt();
void set_low_batt(bool low_batt);

#ifdef __cplusplus
}
#endif

#endif

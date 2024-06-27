#ifndef STATE_H
#define STATE_H

typedef struct {
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
    uint8_t parameter;
    uint8_t argument;
    uint8_t volume;

    uint8_t imu_dest;               // Determines what the IMU is controlling:
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

#endif

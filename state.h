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
} state_t;

#define CTX_KEY         0
#define CTX_SCALE       1
#define CTX_INSTRUMENT  2
#define CTX_PARAMETER   3
#define CTX_ARGUMENT    4

#endif

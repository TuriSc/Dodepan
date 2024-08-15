#ifndef LOOPER_H
#define LOOPER_H
#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum looper_state {
    LOOP_OFF,
    LOOP_READY,
    LOOP_REC,
    LOOP_PLAY,
} looper_state_t;

// Structure to represent a recorded note event
typedef struct {
    uint32_t timestamp; // timestamp in microseconds
    uint8_t note;
    uint8_t velocity;
    bool is_on; // true for note on, false for note off
} note_event_t;

typedef struct looper {
    looper_state_t state;
    note_event_t* events; // Recorded note events
    uint16_t rec_index; // Number of recorded note events
    uint16_t events_max; // Max number of recorded note events
    uint16_t play_index; // Number of replayed note events
    uint32_t rec_start_timestamp; // Timestamp of the beginning of the recording
    uint32_t play_start_timestamp; // Timestamp of the beginning of the playback
    uint32_t loop_duration; // Duration of the loop, in microseconds
} looper_t;

void looper_onpress();
void looper_enable();
void looper_disable();
bool looper_is_disabled();
bool looper_is_ready();
bool looper_is_recording();
bool looper_is_playing();
void looper_set_state(looper_state_t state);
void looper_init(uint16_t events_max);
void looper_record(uint8_t note, uint8_t velocity, bool is_on);
void looper_start_playback();
void looper_task();

extern void note_on(uint8_t note, uint8_t velocity);
extern void note_off(uint8_t note);
extern void all_notes_off();

#ifdef __cplusplus
}
#endif

#endif
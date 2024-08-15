#include "pico/stdlib.h"
#include <stdlib.h>
#include "looper.h"

// Declare the static looper instance
static looper_t looper;

// Handle button presses
void looper_onpress() {
    switch(looper.state) {
        case LOOP_READY:
            // If there is a recording, start playing it.
            // If there is no recording, do nothing
            looper_start_playback();
        break;
        case LOOP_REC:
            // Stop recording, start playing
            looper.loop_duration = time_us_32() - looper.rec_start_timestamp;
            looper_start_playback();
        break;
        case LOOP_PLAY:
            // Stop playing and reset the playhead
            looper_set_state(LOOP_READY);
        break;
        case LOOP_OFF:
            // Do nothing
        break;
    }
}

// Initialize the looper
void looper_init(uint16_t events_max) {
    looper.events = malloc(sizeof(note_event_t) * events_max);
    looper.rec_index = 0;
    looper.events_max = events_max;
    looper.rec_start_timestamp = 0;
    looper.play_start_timestamp = 0;
    looper.loop_duration = 0;
}

/* OK ABOVE */

void looper_start_playback() {
    if(looper.rec_index > 1) { // At least two entries
        // Start playback
        looper_set_state(LOOP_PLAY);
    } else {
        // No note events on record
        looper_set_state(LOOP_READY);
    }
}

// Record a note event
void looper_record(uint8_t note, uint8_t velocity, bool is_on) {
    if (looper_is_disabled() || looper_is_playing()) { return; }
    else if (looper_is_ready()) {
        looper.rec_index = 0;
        looper.rec_start_timestamp = 0;
        looper_set_state(LOOP_REC);
    }

    if (looper.rec_index < looper.events_max) {
        uint32_t now = time_us_32();
        if(looper.rec_start_timestamp == 0) { looper.rec_start_timestamp = now; }
        note_event_t* event = &looper.events[looper.rec_index];
        event->timestamp = now - looper.rec_start_timestamp;
        event->note = note;
        event->velocity = velocity;
        event->is_on = is_on;
        looper.rec_index++;
    }
}

void looper_task() {
    if(!looper_is_playing()) { return; }
    uint32_t now = time_us_32();
    if(now - looper.play_start_timestamp > looper.loop_duration){ // Loop start or restart
        looper.play_start_timestamp = now;
        looper.play_index = 0;
    }
    // Scan for recorded events to replay
    for (uint16_t i = looper.play_index; i < looper.rec_index; i++) {
        note_event_t* event = &looper.events[i];
        // Check if it's time to replay the event
        if (now >= event->timestamp + looper.play_start_timestamp) {
            if (event->is_on) {
                note_on(event->note, event->velocity);
            } else {
                note_off(event->note);
            }
            // Increase the counter, to avoid processing events more than once
            looper.play_index++;
        }

        }
/* OK BELOW */
}

void looper_enable() {
    looper_set_state(LOOP_READY);
}

void looper_disable() {
    looper_set_state(LOOP_OFF);
}

void looper_set_state(looper_state_t state) {
    looper.state = state;
}

bool looper_is_disabled() {
    return (looper.state == LOOP_OFF);
}

bool looper_is_ready() {
    return (looper.state == LOOP_READY);
}

bool looper_is_recording() {
    return (looper.state == LOOP_REC);
}

bool looper_is_playing() {
    return (looper.state == LOOP_PLAY);
}
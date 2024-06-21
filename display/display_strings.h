#ifndef DISPLAY_STRINGS_H
#define DISPLAY_STRINGS_H

const char *note_names[] = {
    "C",
    "C",
    "D",
    "D",
    "E",
    "F",
    "F",
    "G",
    "G",
    "A",
    "A",
    "B"
};

const char *diesis = "#";

const char *scale_names[] = {
    "Major",
    "Natural Minor",
    "Harmonic Minor",
    "Dorian",
    "Lydian",
    "Mixolydian",
    "Locrian",
    "Phrygian",
    "Phrygian Domin.",
    "Pentatonic Maj.",
    "Pentatonic Min.",
    "Pentatonic Blues",
    "Arabian",
    "Oriental",
    "Japanese",
    "Chromatic"
};

const char *octave_names[] = {
    "/",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9"
};

const char *instrument_names[] = {
    "Dodepan",
    "Fifth Lead",
    "Strings",
    "Brass",
    "Pad",
    "Synth Bass",
    "PWM Lead",
    "Electric Piano",
    "Square Tone",
    };

const char *parameter_names[] = {
    "OSC_1_WAVE",
    "OSC_1_SHAPE",
    "OSC_1_MORPH",
    "MIXER_SUB_OSC",

    "OSC_2_WAVE",
    "OSC_2_COARSE",
    "OSC_2_PITCH",
    "MIXER_OSC_MIX",

    "FILTER_CUTOFF",
    "FILTER_RESO",
    "FILTER_EG_AMT",
    "FILTER_KEY_TRK",

    "EG_ATTACK",
    "EG_DECAY",
    "EG_SUSTAIN",
    "EG_RELEASE",

    "EG_OSC_AMT",
    "EG_OSC_DST",
    "VOICE_MODE",
    "PORTAMENTO",

    "LFO_WAVE",
    "LFO_RATE",
    "LFO_DEPTH",
    "LFO_FADE_TIME",

    "LFO_OSC_AMT",
    "LFO_OSC_DST",
    "LFO_FILTER_AMT",
    "AMP_GAIN",

    "AMP_ATTACK",
    "AMP_DECAY",
    "AMP_SUSTAIN",
    "AMP_RELEASE",

    "FILTER_MODE",
    "EG_AMP_MOD",
    "REL_EQ_DECAY",
    "P_BEND_RANGE",

    "BTH_FILTER_AMT",
    "BTH_AMP_MOD",
    "EG_VEL_SENS",
    "AMP_VEL_SENS",

    "CHORUS_MIX",
    "CHORUS_RATE",
    "CHORUS_DEPTH",


    "DELAY_FEEDBACK",
    "DELAY_TIME",
    "DELAY_MODE",
};

#endif
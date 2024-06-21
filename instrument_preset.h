#include "pra32-u-constants.h"
const uint8_t dodepan_program_parameters[] = {
    OSC_1_WAVE     ,
    OSC_1_SHAPE    ,
    OSC_1_MORPH    ,
    MIXER_SUB_OSC  ,

    OSC_2_WAVE     ,
    OSC_2_COARSE   ,
    OSC_2_PITCH    ,
    MIXER_OSC_MIX  ,

    FILTER_CUTOFF  ,
    FILTER_RESO    ,
    FILTER_EG_AMT  ,
    FILTER_KEY_TRK ,

    EG_ATTACK      ,
    EG_DECAY       ,
    EG_SUSTAIN     ,
    EG_RELEASE     ,

    EG_OSC_AMT     ,
    EG_OSC_DST     ,
    VOICE_MODE     ,
    PORTAMENTO     ,

    LFO_WAVE       ,
    LFO_RATE       ,
    LFO_DEPTH      ,
    LFO_FADE_TIME  ,

    LFO_OSC_AMT    ,
    LFO_OSC_DST    ,
    LFO_FILTER_AMT ,
    AMP_GAIN       ,

    AMP_ATTACK     ,
    AMP_DECAY      ,
    AMP_SUSTAIN    ,
    AMP_RELEASE    ,

    FILTER_MODE    ,
    EG_AMP_MOD     ,
    REL_EQ_DECAY   ,
    P_BEND_RANGE   ,

    BTH_FILTER_AMT ,
    BTH_AMP_MOD    ,
    EG_VEL_SENS    ,
    AMP_VEL_SENS   ,

    CHORUS_MIX     ,
    CHORUS_RATE    ,
    CHORUS_DEPTH   ,


    DELAY_FEEDBACK ,
    DELAY_TIME     ,
    DELAY_MODE     ,

};

// See PRA32-U-Parameter-Guide.md for more guidance

const uint8_t dodepan_program[] = {
      5, // OSC_1_WAVE - Pulse wave for a percussive sound
     88, // OSC_1_SHAPE - Mellow waveform shaping
    106, // OSC_1_MORPH - Add some waveform morphing
    127, // MIXER_SUB_OSC - Sub Osc 100%
        
      2, // OSC_2_WAVE - Triangle wave for a bright sound
     52, // OSC_2_COARSE - One octave (12 semitones) lower than OSC_1
     64, // OSC_2_PITCH - Keep tuning sharp
     80, // MIXER_OSC_MIX - Favor OSC_2
        
     90, // FILTER_CUTOFF - Emphasize midrange frequencies
     90, // FILTER_RESO - Add a gentle pinch to the sound
     60, // FILTER_EG_AMT - Swell lightly, like a handpan
     127, // FILTER_KEY_TRK - Max key tracking
        
      8, // EG_ATTACK - Quick attack
     70, // EG_DECAY - Allow the filter to open a bit
      0, // EG_SUSTAIN - No sustain
      0, // EG_RELEASE - Disabled
        
     64, // EG_OSC_AMT - No effect
      0, // EG_OSC_DST - Not in use
      0, // VOICE_MODE - Polyphonic (LFO Single Trigger)
      0, // PORTAMENTO - Omit portamento to preserve sound clarity
        
      1, // LFO_WAVE - Sine wave
     16, // LFO_RATE - A slow rate for subtle wavering effect
     20, // LFO_DEPTH - Just enough to add some variation
      0, // LFO_FADE_TIME - No fade
        
     64, // LFO_OSC_AMT - Add a touch of vibrato
      0, // LFO_OSC_DST - Omit LFO modulation of the oscillator's destination
     88, // LFO_FILTER_AMT - Very light LFO modulation of the filter
    127, // AMP_GAIN - Maximize the amplifier's gain
        
      8, // AMP_ATTACK - Quick attack
     78, // AMP_DECAY - Ring a little
      4, // AMP_SUSTAIN - Minimal sustain as it's a percussive instrument 
      0, // AMP_RELEASE - Disabled
        
      0, // FILTER_MODE - Low Pass
      0, // EG_AMP_MOD - Off, EG ADSR does not overwrite Amp ADSR
      1, // REL_EQ_DECAY - On (Release = Decay)
      2, // P_BEND_RANGE - +/- two semitones
        
     64, // BTH_FILTER_AMT - Breath does not affect the filter
      0, // BTH_AMP_MOD - No breath amp modulation
      0, // EG_VEL_SENS - No EG velocity sensitivity
      0, // AMP_VEL_SENS - No Amp velocity sensitivity
        
    116, // CHORUS_MIX - Mostly wet, like laundry on a balcony on a rainy day
     20, // CHORUS_RATE - 0.134 Hz LFO Frequency
     90, // CHORUS_DEPTH - ~3.95 ms
        
        
    127, // DELAY_FEEDBACK - 49.6%
     12, // DELAY_TIME - 20 ms
      1, // DELAY_MODE - Ping Pong Delay
};

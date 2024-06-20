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
     50, // OSC_1_WAVE - Triangle wave for a mellow sound
      0, // OSC_1_SHAPE - Ignore waveform shaping as Osc 1 Wave is not pulse
     64, // OSC_1_MORPH - Ignore waveform morphing as Osc 1 Wave is not pulse
    127, // MIXER_SUB_OSC - Sub Osc 100%
        
      0, // OSC_2_WAVE - Sawtooth wave for a brighter and piercing sound
     52, // OSC_2_COARSE - One octave (12 semitones) lower than OSC_1
      0, // OSC_2_PITCH - Keep tuning sharp
     40, // MIXER_OSC_MIX - Favor OSC_1 // TODO
        
    100, // FILTER_CUTOFF - Emphasize midrange frequencies
     80, // FILTER_RESO - (Q = 2.8) Add a gentle wobble to the sound
     40, // FILTER_EG_AMT - Moderately influence the filter cutoff frequency
     96, // FILTER_KEY_TRK - Track the key pitch fairly closely
        
     32, // EG_ATTACK - Swell like a handpan
     59, // EG_DECAY - ~150 ms, ringing a little
     20, // EG_SUSTAIN - 
     88, // EG_RELEASE - Allowing the sound to decay slowly
        
     64, // EG_OSC_AMT ? // TODO
      0, // EG_OSC_DST ? // TODO 
      0, // VOICE_MODE - Polyphonic (LFO Single Trigger)
      0, // PORTAMENTO - Omit portamento to preserve sound clarity
        
     25, // LFO_WAVE - Sine wave
     80, // LFO_RATE - (6.9 Hz) A slow rate for subtle wavering effect
     25, // LFO_DEPTH // TODO
      0, // LFO_FADE_TIME - No fade // TODO fade in or fade out?
        
     96, // LFO_OSC_AMT - Add a touch of vibrato // TODO
      0, // LFO_OSC_DST - Omit LFO modulation of the oscillator's destination
     74, // LFO_FILTER_AMT - Very light LFO modulation of the filter
    127, // AMP_GAIN - Maximize the amplifier's gain
        
     93, // AMP_ATTACK ?
     59, // AMP_DECAY ?
     56, // AMP_SUSTAIN ?
     64, // AMP_RELEASE ?
        
      0, // FILTER_MODE ? - Low Pass
      1, // EG_AMP_MOD - On, Amp ADSR = EG ADSR
      0, // REL_EQ_DECAY - Neutral release EQ decay
      2, // P_BEND_RANGE - Two semitones
        
     64, // BTH_FILTER_AMT - Breath does not affect the filter
      0, // BTH_AMP_MOD - No breath amp modulation
    127, // EG_VEL_SENS - ? EG velocity sensitivity // TODO
    127, // AMP_VEL_SENS - ? Amp velocity sensitivity // TODO
        
    127, // CHORUS_MIX ? // TODO
     94, // CHORUS_RATE - ~2 Hz LFO Frequency
     16, // CHORUS_DEPTH - ~1 ms
        
        
     64, // DELAY_FEEDBACK - ~25%
     72, // DELAY_TIME - 200 ms
    127, // DELAY_MODE - Ping Pong Delay
};

// Scale data from https://github.com/Air-Craft/MusicalLib

static const uint8_t scales[16][12] = {
    {0,2,4,5,7,9,11},   // MAJOR; IONIAN
    {0,2,3,5,7,8,10},   // NATURAL_MINOR; AEOLIAN
    {0,2,3,5,7,8,11},   // HARMONIC_MINOR
    {0,2,3,5,7,9,10},   // DORIAN
    {0,2,4,6,7,9,11},   // LYDIAN
    {0,2,4,5,7,9,10},   // MIXOLYDIAN
    {0,1,3,5,6,8,10},   // LOCRIAN
    {0,1,3,5,7,8,10},   // PHRYGIAN
    {0,1,4,5,7,8,10},   // PHRYGIAN DOMINANT
    {0,2,4,7,9},        // PENTATONIC_MAJOR; DIATONIC
    {0,3,5,7,10},       // PENTATONIC_MINOR
    {0,3,5,6,7,10},     // PENTATONIC_BLUES
    {0,2,3,5,6,8,9,11}, // ARABIAN
    {0,1,4,5,6,8,10},   // ORIENTAL
    {0,1,5,7,8},        // JAPANESE
    {0,1,2,3,4,5,6,7,8,9,10,11}, // CHROMATIC
};
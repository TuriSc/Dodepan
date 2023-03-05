#include "note36.h"
#include "note37.h"
#include "note38.h"
#include "note39.h"
#include "note40.h"
#include "note41.h"
#include "note42.h"
#include "note43.h"
#include "note44.h"
#include "note45.h"
#include "note46.h"
#include "note47.h"
#include "note48.h"
#include "note49.h"
#include "note50.h"
#include "note51.h"
#include "note52.h"
#include "note53.h"
#include "note54.h"
#include "note55.h"
#include "note56.h"
#include "note57.h"
#include "note58.h"
#include "note59.h"
#include "note60.h"
#include "note61.h"
#include "note62.h"
#include "note63.h"
#include "note64.h"
#include "note65.h"
#include "note66.h"
#include "note67.h"
#include "note68.h"
#include "note69.h"
#include "note70.h"
#include "note71.h"
#include "note72.h"
#include "note73.h"
#include "note74.h"
#include "note75.h"
#include "note76.h"
#include "note77.h"
#include "note78.h"
#include "note79.h"
#include "note80.h"
#include "note81.h"
#include "note82.h"
#include "note83.h"
#include "note84.h"
#include "note85.h"
#include "note86.h"
#include "note87.h"
#include "note88.h"
#include "note89.h"
#include "note90.h"
#include "note91.h"
#include "note92.h"
#include "note93.h"
#include "note94.h"
#include "note95.h"
#include "note96.h"
#include "note97.h"
#include "note98.h"
#include "note99.h"
#include "note100.h"
#include "note101.h"
#include "note102.h"
#include "note103.h"
#include "note104.h"
#include "note105.h"
#include "note106.h"
#include "note107.h"
#include "note108.h"
#include "note109.h"
#include "note110.h"

#define SAMPLE_LENGTH 11024 // The same for all notes, 500ms.

static const unsigned char *notes[75] = {
    note36_data,
    note37_data,
    note38_data,
    note39_data,
    note40_data,
    note41_data,
    note42_data,
    note43_data,
    note44_data,
    note45_data,
    note46_data,
    note47_data,
    note48_data,
    note49_data,
    note50_data,
    note51_data,
    note52_data,
    note53_data,
    note54_data,
    note55_data,
    note56_data,
    note57_data,
    note58_data,
    note59_data,
    note60_data,
    note61_data,
    note62_data,
    note63_data,
    note64_data,
    note65_data,
    note66_data,
    note67_data,
    note68_data,
    note69_data,
    note70_data,
    note71_data,
    note72_data,
    note73_data,
    note74_data,
    note75_data,
    note76_data,
    note77_data,
    note78_data,
    note79_data,
    note80_data,
    note81_data,
    note82_data,
    note83_data,
    note84_data,
    note85_data,
    note86_data,
    note87_data,
    note88_data,
    note89_data,
    note90_data,
    note91_data,
    note92_data,
    note93_data,
    note94_data,
    note95_data,
    note96_data,
    note97_data,
    note98_data,
    note99_data,
    note100_data,
    note101_data,
    note102_data,
    note103_data,
    note104_data,
    note105_data,
    note106_data,
    note107_data,
    note108_data,
    note109_data,
    note110_data,
};
/* MPR121 functions */

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "mpr121.h"         // https://github.com/antgon/pico-mpr121
#include <config.h>
#include "touch.h"

struct mpr121_sensor mpr121;

void mpr121_i2c_init(){
    i2c_init(MPR121_I2C_PORT, MPR121_I2C_FREQ);
    gpio_set_function(MPR121_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(MPR121_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(MPR121_SDA_PIN);
    gpio_pull_up(MPR121_SCL_PIN);

    mpr121_init(MPR121_I2C_PORT, MPR121_ADDRESS, &mpr121);
    mpr121_set_thresholds(MPR121_TOUCH_THRESHOLD,
                          MPR121_RELEASE_THRESHOLD, &mpr121);
    
    mpr121_enable_electrodes(12, &mpr121);
}

void mpr121_task(){
    bool is_touched;
    static bool was_touched[12];
    for(uint8_t i=0; i<12; i++) {
        mpr121_is_touched(i, &is_touched, &mpr121);
        if (is_touched != was_touched[i]){
            if(time_us_32() < 500000) return;  // Ignore readings for half a second,
                                               // allowing the MPR121 to calibrate.
            if (is_touched){
                trigger_note_on(get_note_by_id(i));
            } else {
                trigger_note_off(get_note_by_id(i));
            }
            was_touched[i] = is_touched;
        }
    }
}
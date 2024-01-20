#include "pico/stdlib.h"
#include <config.h>
#include "demuxer.h"

/* 74HC4067 Multiplexer/Demultiplexer functions */
// The number of LEDs that are simultaneously on is always between 2 and 3.
// leds_on[0] for the scale (LEDs on C0 to C7),
// leds_on[1] for the key (LEDs on C9 to C15),
// leds_on[2] for alterations (LED on C8).

int8_t leds_on[3] =         {-1, -1, -1};
int8_t control_pins[4] =    {DEMUX_S0, DEMUX_S1, DEMUX_S2, DEMUX_S3};

uint8_t enable_pin;
int8_t signal_pin;

void demuxer_init(uint8_t en, uint8_t sig, int8_t s0, int8_t s1, int8_t s2, int8_t s3) {
	enable_pin = en;
    gpio_init(en);
    gpio_set_dir(en, GPIO_OUT);
    gpio_put(en, 1); // 1 = disable

	signal_pin = sig;
	gpio_init(signal_pin);
	gpio_set_dir(signal_pin, GPIO_OUT);

	control_pins[0] = s0;
    control_pins[1] = s1;
    control_pins[2] = s2;
    control_pins[3] = s3;
	for (uint8_t i = 0; i < 4; ++i){
		gpio_init(control_pins[i]);
		gpio_set_dir(control_pins[i], GPIO_OUT);
	}
    gpio_put(en, 0); // 0 = enable
    gpio_put(sig, 1);
}

void demuxer_task(){
    static uint8_t ticker;
    int8_t selected_led = leds_on[ticker++ % 3];

    if (selected_led > -1){
        for (uint8_t i = 0; i < 4; ++i)
        {
            gpio_put(control_pins[i], selected_led & 0x01);
            selected_led >>= 1;
        }
    }
}

void set_led(uint8_t type, uint8_t led){
    leds_on[type] = led;
}
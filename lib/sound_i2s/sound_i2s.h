/* sound_i2s.h */

#ifndef SOUND_I2S_H_FILE
#define SOUND_I2S_H_FILE

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sound_i2s_config {
  uint8_t  pio_num;
  uint8_t  pin_scl;
  uint8_t  pin_sda;
  uint8_t  pin_ws;
  uint16_t sample_rate;
  uint8_t  bits_per_sample;
  bool swap;
  uint16_t samples_per_buffer;
};

int sound_i2s_init(const struct sound_i2s_config *cfg);
int16_t *sound_i2s_get_next_buffer();
int16_t *sound_i2s_get_buffer(int buffer_num);

extern volatile unsigned int sound_i2s_num_buffers_played;

#ifdef __cplusplus
}
#endif

#endif /* SOUND_I2S_H_FILE */

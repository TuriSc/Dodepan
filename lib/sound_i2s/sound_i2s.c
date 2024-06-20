/* sound_i2s.c */

#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "sound_i2s.h"
#include "sound_i2s_16bits.pio.h"

volatile unsigned int sound_i2s_num_buffers_played = 0;

static struct sound_i2s_config config;
static PIO sound_pio;
static uint sound_pio_sm;
static uint sound_dma_chan;

static volatile int sound_cur_buffer_num;
static void *sound_sample_buffers[2];

static void __isr __time_critical_func(dma_handler)(void)
{
  // swap buffers
  uint cur_buf = !sound_cur_buffer_num;
  sound_cur_buffer_num = cur_buf;
  sound_i2s_num_buffers_played++;

  // set dma dest to new buffer and re-trigger dma:
  dma_hw->ch[sound_dma_chan].al3_read_addr_trig = (uintptr_t) sound_sample_buffers[cur_buf];

  // ack dma irq
  dma_hw->ints0 = 1u << sound_dma_chan;
}

int sound_i2s_init(const struct sound_i2s_config *cfg)
{
  config = *cfg;

  // allocate sound buffers
  size_t sound_buffer_size = 4 * SOUND_I2S_BUFFER_NUM_SAMPLES;
  sound_sample_buffers[0] = malloc(sound_buffer_size);
  sound_sample_buffers[1] = malloc(sound_buffer_size);
  if (! sound_sample_buffers[0] || ! sound_sample_buffers[1]) {
    free(sound_sample_buffers[0]);
    free(sound_sample_buffers[1]);
    return -1;
  }
  memset(sound_sample_buffers[0], 0, sound_buffer_size);
  memset(sound_sample_buffers[1], 0, sound_buffer_size);

  // setup pio
  sound_pio = (config.pio_num == 0) ? pio0 : pio1;
  uint offset = pio_add_program(sound_pio, &sound_i2s_16bits_program);
  sound_pio_sm = pio_claim_unused_sm(sound_pio, true);
    sound_i2s_16bits_program_init(sound_pio, sound_pio_sm, offset, config.sample_rate, config.pin_sda, config.pin_scl);

  // allocate dma channel and setup irq
  sound_dma_chan = dma_claim_unused_channel(true);
  dma_channel_set_irq0_enabled(sound_dma_chan, true);
  irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
  irq_set_priority(DMA_IRQ_0, 0xff);
  irq_set_enabled(DMA_IRQ_0, true);
  return 0;
}

void sound_i2s_playback_start(void)
{
  // reset buffer
  sound_i2s_num_buffers_played = 0;
  sound_cur_buffer_num = 0;
  void *buffer = sound_sample_buffers[sound_cur_buffer_num];

  // start pio
  pio_sm_set_enabled(sound_pio, sound_pio_sm, true);

  // setup dma channel
  dma_channel_config dma_cfg = dma_channel_get_default_config(sound_dma_chan);
  channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_32);
  channel_config_set_read_increment(&dma_cfg, true);
  channel_config_set_write_increment(&dma_cfg, false);
  channel_config_set_dreq(&dma_cfg, pio_get_dreq(sound_pio, sound_pio_sm, true));
  dma_channel_configure(sound_dma_chan, &dma_cfg,
                        &sound_pio->txf[sound_pio_sm],  // destination
                        buffer,                         // source
                        SOUND_I2S_BUFFER_NUM_SAMPLES,   // number of dma transfers
                        true                            // start immediatelly (will be blocked by pio)
                        );
}

int16_t *sound_i2s_get_next_buffer()
{
  return sound_sample_buffers[1-sound_cur_buffer_num];
}

int16_t *sound_i2s_get_buffer(int buffer_num)
{
  return sound_sample_buffers[buffer_num];
}

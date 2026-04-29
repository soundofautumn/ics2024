#include <common.h>
#include <device/map.h>
#include <SDL2/SDL.h>

enum {
  reg_freq,
  reg_channels,
  reg_samples,
  reg_sbuf_size,
  reg_init,
  reg_count,
  nr_reg
};

static uint8_t *sbuf = NULL;
static uint32_t *audio_base = NULL;

static int rpos = 0;
static int wpos = 0;
static int init_done = 0;

static void audio_cb(void *userdata, uint8_t *stream, int len) {
  int sbuf_size = CONFIG_SB_SIZE;
  int count = (wpos - rpos + sbuf_size) % sbuf_size;
  int read_len = (len < count) ? len : count;

  if (read_len > 0) {
    int first = sbuf_size - rpos;
    if (first > read_len) first = read_len;
    memcpy(stream, sbuf + rpos, first);
    if (first < read_len) {
      memcpy(stream + first, sbuf, read_len - first);
    }
    rpos = (rpos + read_len) % sbuf_size;
  }

  if (read_len < len) {
    memset(stream + read_len, 0, len - read_len);
  }
}

static void audio_io_handler(uint32_t offset, int len, bool is_write) {
  if (is_write) {
    switch (offset / 4) {
      case reg_freq:
      case reg_channels:
      case reg_samples:
        break;
      case reg_init: {
        uint32_t val = audio_base[reg_init];
        if (init_done) {
          if (val > 0) wpos = (wpos + val) % CONFIG_SB_SIZE;
        } else {
          SDL_AudioSpec s = {};
          s.freq = audio_base[reg_freq];
          s.channels = audio_base[reg_channels];
          s.samples = audio_base[reg_samples];
          s.format = AUDIO_S16SYS;
          s.userdata = NULL;
          s.callback = audio_cb;

          SDL_InitSubSystem(SDL_INIT_AUDIO);
          SDL_OpenAudio(&s, NULL);
          SDL_PauseAudio(0);
          init_done = 1;
        }
        break;
      }
      default: break;
    }
  } else {
    switch (offset / 4) {
      case reg_sbuf_size:
        audio_base[reg_sbuf_size] = CONFIG_SB_SIZE;
        break;
      case reg_count: {
        int cnt = (wpos - rpos + CONFIG_SB_SIZE) % CONFIG_SB_SIZE;
        audio_base[reg_count] = cnt;
        break;
      }
      default: break;
    }
  }
}

void init_audio() {
  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  audio_base = (uint32_t *)new_space(space_size);
  audio_base[reg_sbuf_size] = CONFIG_SB_SIZE;
  audio_base[reg_count] = 0;
  rpos = wpos = 0;
  init_done = 0;

#ifdef CONFIG_HAS_PORT_IO
  add_pio_map("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#endif

  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);
}

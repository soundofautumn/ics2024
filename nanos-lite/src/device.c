#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

size_t serial_write(const void *buf, size_t offset, size_t len) {
  for (size_t i = 0; i < len; i++) {
    putch(((char *)buf)[i]);
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  AM_INPUT_KEYBRD_T ev;
  ioe_read(AM_INPUT_KEYBRD, &ev);
  if (ev.keycode == AM_KEY_NONE) {
    return 0;
  } 
    
  if (ev.keydown == 0) {
    sprintf((char *)buf, "ku %s", keyname[ev.keycode]);
  } else {
    sprintf((char *)buf, "kd %s", keyname[ev.keycode]);
  }
  return strlen((char *)buf);
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  AM_GPU_CONFIG_T cfg;
  ioe_read(AM_GPU_CONFIG, &cfg);
  int w = cfg.width;
  int h = cfg.height;
  sprintf((char *)buf, "WIDTH:%d\nHEIGHT:%d\n", w, h);
  return strlen((char *)buf);
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  int w = io_read(AM_GPU_CONFIG).width;
  int x = offset % w;
  int y = offset / w;
  io_write(AM_GPU_FBDRAW, x, y, (void *)buf, len, 1, true);
  return 0;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}

#include <am.h>
#include <NDL.h>
#include <stdlib.h>
#include <time.h>

static int screen_w = 0, screen_h = 0;
static bool gpu_inited = false;

bool ioe_init() {
  return NDL_Init(0) == 0;
}

void ioe_read(int reg, void *buf) {
  switch (reg) {
    case AM_UART_CONFIG: {
      AM_UART_CONFIG_T *cfg = (AM_UART_CONFIG_T *)buf;
      cfg->present = true;
      break;
    }
    case AM_UART_RX: {
      AM_UART_RX_T *rx = (AM_UART_RX_T *)buf;
      rx->data = 0;
      break;
    }
    case AM_TIMER_CONFIG: {
      AM_TIMER_CONFIG_T *cfg = (AM_TIMER_CONFIG_T *)buf;
      cfg->present = true;
      cfg->has_rtc = true;
      break;
    }
    case AM_TIMER_RTC: {
      AM_TIMER_RTC_T *rtc = (AM_TIMER_RTC_T *)buf;
      time_t t = time(NULL);
      struct tm tm;
      localtime_r(&t, &tm);
      rtc->year = tm.tm_year + 1900;
      rtc->month = tm.tm_mon + 1;
      rtc->day = tm.tm_mday;
      rtc->hour = tm.tm_hour;
      rtc->minute = tm.tm_min;
      rtc->second = tm.tm_sec;
      break;
    }
    case AM_TIMER_UPTIME: {
      AM_TIMER_UPTIME_T *ut = (AM_TIMER_UPTIME_T *)buf;
      ut->us = (uint64_t)NDL_GetTicks() * 1000;
      break;
    }
    case AM_INPUT_CONFIG: {
      AM_INPUT_CONFIG_T *cfg = (AM_INPUT_CONFIG_T *)buf;
      cfg->present = true;
      break;
    }
    case AM_INPUT_KEYBRD: {
      AM_INPUT_KEYBRD_T *kbd = (AM_INPUT_KEYBRD_T *)buf;
      char event[64];
      int len = NDL_PollEvent(event, sizeof(event) - 1);
      if (len > 0) {
        event[len] = '\0';
        if (len >= 4 && event[0] == 'k') {
          kbd->keydown = (event[1] == 'd');
          kbd->keycode = atoi(event + 3);
        } else {
          kbd->keydown = false;
          kbd->keycode = AM_KEY_NONE;
        }
      } else {
        kbd->keydown = false;
        kbd->keycode = AM_KEY_NONE;
      }
      break;
    }
    case AM_GPU_CONFIG: {
      AM_GPU_CONFIG_T *cfg = (AM_GPU_CONFIG_T *)buf;
      if (!gpu_inited) {
        NDL_OpenCanvas(&screen_w, &screen_h);
        gpu_inited = true;
      }
      cfg->present = true;
      cfg->has_accel = false;
      cfg->width = screen_w;
      cfg->height = screen_h;
      cfg->vmemsz = 0;
      break;
    }
    case AM_GPU_STATUS: {
      AM_GPU_STATUS_T *st = (AM_GPU_STATUS_T *)buf;
      st->ready = true;
      break;
    }
    case AM_AUDIO_CONFIG: {
      AM_AUDIO_CONFIG_T *cfg = (AM_AUDIO_CONFIG_T *)buf;
      cfg->present = true;
      cfg->bufsize = 4096;
      break;
    }
    case AM_AUDIO_STATUS: {
      AM_AUDIO_STATUS_T *st = (AM_AUDIO_STATUS_T *)buf;
      st->count = NDL_QueryAudio();
      break;
    }
    default: break;
  }
}

void ioe_write(int reg, void *buf) {
  switch (reg) {
    case AM_UART_TX: {
      AM_UART_TX_T *tx = (AM_UART_TX_T *)buf;
      putch(tx->data);
      break;
    }
    case AM_GPU_FBDRAW: {
      AM_GPU_FBDRAW_T *fb = (AM_GPU_FBDRAW_T *)buf;
      NDL_DrawRect((uint32_t *)fb->pixels, fb->x, fb->y, fb->w, fb->h);
      break;
    }
    case AM_AUDIO_CTRL: {
      AM_AUDIO_CTRL_T *ctrl = (AM_AUDIO_CTRL_T *)buf;
      NDL_OpenAudio(ctrl->freq, ctrl->channels, ctrl->samples);
      break;
    }
    case AM_AUDIO_PLAY: {
      AM_AUDIO_PLAY_T *play = (AM_AUDIO_PLAY_T *)buf;
      int len = (uintptr_t)play->buf.end - (uintptr_t)play->buf.start;
      NDL_PlayAudio(play->buf.start, len);
      break;
    }
    default: break;
  }
}

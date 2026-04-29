#include <NDL.h>
#include <SDL.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static SDL_AudioSpec g_spec;
static int audio_open = 0;
static int audio_paused = 1;
static uint32_t last_cb_time = 0;

void CallbackHelper() {
  if (!audio_open || audio_paused || !g_spec.callback) return;

  uint32_t now = NDL_GetTicks();
  int interval_ms = g_spec.samples * 1000 / g_spec.freq;
  if (interval_ms <= 0) interval_ms = 1;

  if ((uint32_t)(now - last_cb_time) < (uint32_t)interval_ms) return;

  int bytes_per_sample = 2;
  int len = g_spec.samples * g_spec.channels * bytes_per_sample;

  int free_space = NDL_QueryAudio();
  if (free_space < len) return;

  uint8_t *stream = malloc(len);
  g_spec.callback(g_spec.userdata, stream, len);
  NDL_PlayAudio(stream, len);
  free(stream);

  last_cb_time = now;
}

int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) {
  g_spec = *desired;
  NDL_OpenAudio(desired->freq, desired->channels, desired->samples);
  audio_open = 1;
  audio_paused = 1;
  last_cb_time = NDL_GetTicks();
  return 0;
}

void SDL_CloseAudio() {
  NDL_CloseAudio();
  audio_open = 0;
}

void SDL_PauseAudio(int pause_on) {
  audio_paused = pause_on;
  last_cb_time = NDL_GetTicks();
}

void SDL_MixAudio(uint8_t *dst, uint8_t *src, uint32_t len, int volume) {
  int16_t *d = (int16_t *)dst;
  int16_t *s = (int16_t *)src;
  for (uint32_t i = 0; i < len / 2; i++) {
    int32_t mixed = d[i] + (s[i] * volume / SDL_MIX_MAXVOLUME);
    if (mixed > 32767) mixed = 32767;
    if (mixed < -32768) mixed = -32768;
    d[i] = mixed;
  }
}

SDL_AudioSpec *SDL_LoadWAV(const char *file, SDL_AudioSpec *spec, uint8_t **audio_buf, uint32_t *audio_len) {
  FILE *fp = fopen(file, "r");
  if (!fp) return NULL;

  fseek(fp, 0, SEEK_END);
  long fsize = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  uint8_t *buf = malloc(fsize);
  fread(buf, fsize, 1, fp);
  fclose(fp);

  uint32_t data_offset = 12;
  while (data_offset + 8 < (uint32_t)fsize) {
    char chunk_id[5] = {0};
    memcpy(chunk_id, buf + data_offset, 4);
    uint32_t chunk_size;
    memcpy(&chunk_size, buf + data_offset + 4, 4);

    if (strcmp(chunk_id, "fmt ") == 0) {
      uint16_t audio_format, channels, bits_per_sample;
      uint32_t sample_rate;
      memcpy(&audio_format, buf + data_offset + 8, 2);
      memcpy(&channels, buf + data_offset + 10, 2);
      memcpy(&sample_rate, buf + data_offset + 12, 4);
      memcpy(&bits_per_sample, buf + data_offset + 22, 2);

      spec->freq = sample_rate;
      spec->channels = channels;
      spec->format = (bits_per_sample == 16) ? AUDIO_S16 : AUDIO_U8;
    }

    if (strcmp(chunk_id, "data") == 0) {
      *audio_len = chunk_size;
      *audio_buf = malloc(chunk_size);
      memcpy(*audio_buf, buf + data_offset + 8, chunk_size);
      free(buf);
      return spec;
    }

    data_offset += 8 + chunk_size;
    if (chunk_size % 2) data_offset++;
  }

  free(buf);
  return NULL;
}

void SDL_FreeWAV(uint8_t *audio_buf) {
  free(audio_buf);
}

void SDL_LockAudio() {
}

void SDL_UnlockAudio() {
}

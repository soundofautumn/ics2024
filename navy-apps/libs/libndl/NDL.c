#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>

static int evtdev = -1;
static int fbdev = -1;
static int sb_fd = -1;
static int sbctl_fd = -1;
static int screen_w = 0, screen_h = 0;
static struct timeval start_time;

uint32_t NDL_GetTicks() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  static uint64_t start_ticks = 0;
  if (start_ticks == 0) {
    start_ticks = tv.tv_sec * 1000 + tv.tv_usec / 1000;
  }
  uint32_t ticks = tv.tv_sec * 1000 + tv.tv_usec / 1000;
  return ticks - start_ticks;
}

int NDL_PollEvent(char *buf, int len) {
  int fd = open("/dev/events", 0, 0);
  return read(fd, buf, len);
}

void NDL_OpenCanvas(int *w, int *h) {
  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }
  int fd = open("/proc/dispinfo", 0, 0);
  char dispinfo[128];
  int nread = read(fd, dispinfo, sizeof(dispinfo) - 1);
  dispinfo[nread] = '\0';
  sscanf(dispinfo, "WIDTH:%d\nHEIGHT:%d\n", &screen_w, &screen_h);
  if (*w == 0 && *h == 0) {
    *w = screen_w;
    *h = screen_h;
  }
  if (*w > screen_w) *w = screen_w;
  if (*h > screen_h) *h = screen_h;
  close(fd);
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  for (int j = 0; j < h; j++) {
    int offset = ((y + j) * screen_w + x);
    lseek(fbdev, offset, SEEK_SET);
    write(fbdev, pixels + j * w, w);
  }
}

void NDL_OpenAudio(int freq, int channels, int samples) {
  sbctl_fd = open("/dev/sbctl", 0);
  sb_fd = open("/dev/sb", 0);
  int init_data[3] = {freq, channels, samples};
  write(sbctl_fd, init_data, sizeof(init_data));
}

void NDL_CloseAudio() {
  close(sb_fd);
  close(sbctl_fd);
  sb_fd = sbctl_fd = -1;
}

int NDL_PlayAudio(void *buf, int len) {
  return write(sb_fd, buf, len);
}

int NDL_QueryAudio() {
  int free;
  read(sbctl_fd, &free, sizeof(free));
  return free;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  gettimeofday(&start_time, NULL);
  fbdev = open("/dev/fb", 0, 0);
  return 0;
}

void NDL_Quit() {
  close(fbdev);
}

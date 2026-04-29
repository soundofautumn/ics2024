#include <NDL.h>
#include <SDL.h>
#include <string.h>

#define keyname(k) #k,

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

int SDL_PushEvent(SDL_Event *ev) {
  assert(0);
  return 0;
}

int SDL_PollEvent(SDL_Event *ev) {
  char buf[64];
  int len = NDL_PollEvent(buf, sizeof(buf) - 1);
  if (len <= 0) return 0;
  buf[len] = '\0';

  if (len < 4) return 0;
  int keydown = (buf[0] == 'k' && buf[1] == 'd');
  int keyup   = (buf[0] == 'k' && buf[1] == 'u');
  if (!keydown && !keyup) return 0;
  if (buf[2] != ' ') return 0;

  char *key_str = buf + 3;
  char *nl = strchr(key_str, '\n');
  if (nl) *nl = '\0';

  ev->type = keydown ? SDL_KEYDOWN : SDL_KEYUP;

  uint8_t sym = 0;
  int nkeys = sizeof(keyname) / sizeof(keyname[0]);
  for (int i = 1; i < nkeys; i++) {
    if (strcmp(key_str, keyname[i]) == 0) {
      sym = i;
      break;
    }
  }
  ev->key.keysym.sym = sym;

  return 1;
}

int SDL_WaitEvent(SDL_Event *event) {
  while (SDL_PollEvent(event) == 0) ;
  return 1;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  assert(0);
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  assert(0);
  return NULL;
}

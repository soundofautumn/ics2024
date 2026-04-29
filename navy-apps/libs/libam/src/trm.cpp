#include <am.h>
#include <unistd.h>
#include <stdlib.h>

Area heap;

void putch(char ch) {
  write(1, &ch, 1);
}

void halt(int code) {
  exit(code);
  while (1);
}

#include <stdarg.h>
#include "sdb.h"

#define NR_IRINGBUF 16

static IRingBuf sdb_iringbuf;

void init_iringbuf() {
  sdb_iringbuf.capacity = NR_IRINGBUF;
  sdb_iringbuf.size = 0;
  sdb_iringbuf.head = 0;
  sdb_iringbuf.tail = 0;
  sdb_iringbuf.data = malloc(sizeof(char*) * NR_IRINGBUF);
  for (int i = 0; i < NR_IRINGBUF; i++) {
    sdb_iringbuf.data[i] = malloc(sizeof(char) * 128);
  }
}

void write_iringbuf(const char *str) {
  strncpy(sdb_iringbuf.data[sdb_iringbuf.head], str, 128);
  sdb_iringbuf.head = (sdb_iringbuf.head + 1) % sdb_iringbuf.capacity;
  if (sdb_iringbuf.size < sdb_iringbuf.capacity) {
    sdb_iringbuf.size++;
  } else {
    sdb_iringbuf.tail = (sdb_iringbuf.tail + 1) % sdb_iringbuf.capacity;
  }
}

void iringbuf_display() {
  int idx = sdb_iringbuf.tail;
  for (int i = 0; i < sdb_iringbuf.size; i++) {
    printf("%s\n", sdb_iringbuf.data[idx]);
    idx = (idx + 1) % sdb_iringbuf.capacity;
  }
}

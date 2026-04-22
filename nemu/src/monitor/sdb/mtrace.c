#include <memory/paddr.h>
#include "sdb.h"

static word_t mtrace_start = CONFIG_MBASE, mtrace_end = CONFIG_MBASE + CONFIG_MSIZE;

void set_mtrace_range(word_t start, word_t end) {
  mtrace_start = start;
  mtrace_end = end;
}

void mtrace_read(word_t addr, int len, word_t data) {
  if (addr >= mtrace_start && addr < mtrace_end) {
    printf("Memory read: addr = " FMT_PADDR ", len = %d, data = " FMT_WORD "\n", addr, len, data);
  }
}

void mtrace_write(word_t addr, int len, word_t data) {
  if (addr >= mtrace_start && addr < mtrace_end) {
    printf("Memory write: addr = " FMT_PADDR ", len = %d, data = " FMT_WORD "\n", addr, len, data);
  }
}
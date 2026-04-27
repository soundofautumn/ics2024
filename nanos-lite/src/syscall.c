#include <common.h>
#include "syscall.h"

// #define CONFIG_STRACE

#ifdef CONFIG_STRACE
char strace_buf[128];
#define STRACE_LOG(...) do { \
  int len = snprintf(strace_buf, sizeof(strace_buf), __VA_ARGS__); \
  strace_buf[len] = '\0'; \
  for (int i = 0; i < len; i++) { \
    putch(strace_buf[i]); \
  } \
} while (0)
#else // CONFIG_STRACE
#define STRACE_LOG(...) do { } while (0)
#endif

void do_syscall(Context *c) {
  uintptr_t sysnum = c->GPR1;
  uintptr_t a0 = c->GPR2;
  uintptr_t a1 = c->GPR3;
  uintptr_t a2 = c->GPR4;
  uintptr_t ret = 0;

  switch (sysnum) {
    case SYS_yield: {
      yield();
      ret = 0;
      STRACE_LOG("syscall: yield() -> %d", ret);
      break;
    }
    case SYS_exit: {
      STRACE_LOG("syscall: exit(%d)", a0);
      halt(a0);
      break;
    }
    case SYS_write: {
      char *buf = (char *)a1;
      for (int i = 0; i < a2; i++) {
        putch(buf[i]);
      }
      ret = a2;
      STRACE_LOG("syscall: write(%d) -> %d", a2, ret);
      break;
    }
    default: panic("Unhandled syscall ID = %d", sysnum);
  }

  c->GPRx = ret;
}

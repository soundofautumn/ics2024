#include <common.h>
#include <fs.h>
#include <sys/time.h>
#include "syscall.h"

#define CONFIG_STRACE

#ifdef CONFIG_STRACE
char strace_buf[128];
#define STRACE_LOG(...) do { \
  int len = snprintf(strace_buf, sizeof(strace_buf), __VA_ARGS__); \
  strace_buf[len] = '\0'; \
  for (int i = 0; i < len; i++) { \
    putch(strace_buf[i]); \
  } \
  putch('\n'); \
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
      ret = fs_write(a0, (const void *)a1, a2);
      STRACE_LOG("syscall: write(%d, %p, %d) -> %d", a0, (const void *)a1, a2, ret);
      break;
    }
    case SYS_brk: {
      ret = 0;
      STRACE_LOG("syscall: brk(%p) -> %d", (void *)a0, ret);
      break;
    }
    case SYS_open: {
      ret = fs_open((const char *)a0, a1, a2);
      STRACE_LOG("syscall: open('%s') -> %d", (const char *)a0, ret);
      break;
    }
    case SYS_read: {
      ret = fs_read(a0, (void *)a1, a2);
      STRACE_LOG("syscall: read(%d, %p, %d) -> %d", a0, (void *)a1, a2, ret);
      break;
    }
    case SYS_close: {
      ret = fs_close(a0);
      STRACE_LOG("syscall: close(%d) -> %d", a0, ret);
      break;
    }
    case SYS_lseek: {
      ret = fs_lseek(a0, a1, a2);
      STRACE_LOG("syscall: lseek(%d, %d, %d) -> %d", a0, a1, a2, ret);
      break;
    }
    case SYS_gettimeofday: {
      struct timeval *tv = (struct timeval *)a0;
      uint64_t us = io_read(AM_TIMER_UPTIME).us;
      tv->tv_sec = us / 1000000;
      tv->tv_usec = us % 1000000;
      ret = 0;
      STRACE_LOG("syscall: gettimeofday(%p, %p) -> %d", (void *)a0, (void *)a1, ret);
      break;
    }
    default: panic("Unhandled syscall ID = %d", sysnum);
  }

  c->GPRx = ret;
}

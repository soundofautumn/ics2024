#include <common.h>
#include "syscall.h"

#define CONFIG_STRACE

static void sys_yield(uintptr_t *a) {
  yield();
  a[2] = 0;
}

static void sys_exit(uintptr_t *a) {
  halt(a[2]);
}

static void sys_write(uintptr_t *a) {
  char *buf = (char *)a[2];
  for (int i = 0; i < a[3]; i++) {
    putch(buf[i]);
  }
  a[2] = a[3];
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;

#ifdef CONFIG_STRACE
  // sys exit is special since it will not return, so we print the log before executing it.
  if (a[0] == SYS_exit) {
    Log("syscall: exit(%d)", a[2]);
  }
#endif

  switch (a[0]) {
    case SYS_yield: sys_yield(a); break;
    case SYS_exit: sys_exit(a); break;
    case SYS_write: sys_write(a); break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

#ifdef CONFIG_STRACE
  switch (a[0]) {
    case SYS_yield: Log("syscall: yield() -> %d", a[2]); break;
    case SYS_write: Log("syscall: write(%d) -> %d", a[3], a[2]); break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
#endif

  c->GPRx = a[2];
}

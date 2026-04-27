#include <common.h>
#include "syscall.h"

static void sys_yield(uintptr_t *a) {
  yield();
  a[2] = 0;
}

static void sys_exit(uintptr_t *a) {
  halt(a[2]);
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;

  switch (a[0]) {
    case SYS_yield: sys_yield(a); break;
    case SYS_exit: sys_exit(a); break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

#ifdef STRACE
    Log("strace: syscall %d => %d", a[0], a[2]);
#endif
}

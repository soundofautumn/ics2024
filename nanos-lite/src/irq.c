#include <common.h>


intptr_t do_syscall(intptr_t type, intptr_t a0, intptr_t a1, intptr_t a2);

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_SYSCALL: {
      c->GPRx = do_syscall(c->GPR1, c->GPR2, c->GPR3, c->GPR4);
      break;
    }
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}

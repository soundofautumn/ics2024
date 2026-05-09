#include <common.h>

Context* do_syscall(Context *c);
Context* schedule(Context *prev);

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_IRQ_TIMER:
      Log("Timer interrupt triggered");
    case EVENT_YIELD: {
      return schedule(c);
    }
    case EVENT_SYSCALL: {
      return do_syscall(c);
    }
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}

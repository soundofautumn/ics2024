#include <memory.h>
#include <common.h>
#include <proc.h>

static void *pf = NULL;

void* new_page(size_t nr_page) {
  void *ret = pf;
  pf += nr_page * PGSIZE;
  assert(pf <= (void *)heap.end);
  return ret;
}

#ifdef HAS_VME
static void* pg_alloc(int n) {
  int pages = (n + PGSIZE - 1) / PGSIZE;
  void *p = new_page(pages);
  memset(p, 0, pages * PGSIZE);
  return p;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  uintptr_t new_brk = ROUNDUP(brk, PGSIZE);
  if (new_brk > (uintptr_t) heap.end) {
    return -1;
  }
  while (current->max_brk < new_brk) {
    map(&(current->as), (void *)current->max_brk, pg_alloc(PGSIZE), MMAP_READ | MMAP_WRITE);
    current->max_brk += PGSIZE;
  }
  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}

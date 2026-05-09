#include <proc.h>
#include <elf.h>
#include <fs.h>
#include <common.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

#if defined(__ISA_AM_NATIVE__)
# define EXPECT_TYPE EM_X86_64
#elif defined(__ISA_X86)
# define EXPECT_TYPE EM_X86_64
#elif defined(__ISA_RISCV32__)
# define EXPECT_TYPE EM_RISCV
#elif defined(__ISA_RISCV64__)
# define EXPECT_TYPE EM_RISCV
#else 
# error "Unsupported ISA"
#endif


static uintptr_t loader(PCB *pcb, const char *filename) {
  int fd = fs_open(filename, 0, 0);
  assert(fd >= 0);
  Elf_Ehdr ehdr;
  assert(fs_lseek(fd, 0, SEEK_SET) == 0);
  assert(fs_read(fd, &ehdr, sizeof(Elf_Ehdr)) == sizeof(Elf_Ehdr));
  assert(memcmp(ehdr.e_ident, ELFMAG, SELFMAG) == 0);
  assert(ehdr.e_type == ET_EXEC);
  assert(ehdr.e_machine == EXPECT_TYPE);
  assert(ehdr.e_phnum > 0);

  Elf_Phdr phdr;
  for (int i = 0; i < ehdr.e_phnum; i++) {
    assert(fs_lseek(fd, ehdr.e_phoff + i * sizeof(Elf_Phdr), SEEK_SET) == ehdr.e_phoff + i * sizeof(Elf_Phdr));
    assert(fs_read(fd, &phdr, sizeof(Elf_Phdr)) == sizeof(Elf_Phdr));
    if (phdr.p_type == PT_LOAD) {
#ifdef HAS_VME
      uintptr_t seg_start = phdr.p_vaddr;
      uintptr_t seg_end = phdr.p_vaddr + phdr.p_memsz;
      uintptr_t page_start = ROUNDDOWN(seg_start, PGSIZE);

      assert(fs_lseek(fd, phdr.p_offset, SEEK_SET) == phdr.p_offset);

      size_t remain_file = phdr.p_filesz;

      for (uintptr_t va = page_start; va < seg_end; va += PGSIZE) {
        void *pa = new_page(1);
        map(&pcb->as, (void *)va, pa, MMAP_READ | MMAP_WRITE);

        size_t page_off = (va == page_start) ? (seg_start - page_start) : 0;

        size_t ncpy = PGSIZE - page_off;
        if (ncpy > remain_file) ncpy = remain_file;

        if (ncpy > 0) {
          assert(fs_read(fd, pa + page_off, ncpy) == ncpy);
          remain_file -= ncpy;
        }

        size_t page_end = (va + PGSIZE < seg_end) ? PGSIZE : (seg_end - va);
        if (page_end > page_off + ncpy) {
          memset(pa + page_off + ncpy, 0, page_end - page_off - ncpy);
        }
      }
#else
      // [VirtAddr, VirtAddr + MemSiz)
      assert(fs_lseek(fd, phdr.p_offset, SEEK_SET) == phdr.p_offset);
      assert(fs_read(fd, (void *)phdr.p_vaddr, phdr.p_filesz) == phdr.p_filesz);
      if (phdr.p_memsz > phdr.p_filesz) {
        // [VirtAddr + FileSiz, VirtAddr + MemSiz)
        memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, phdr.p_memsz - phdr.p_filesz);
      }
#endif
    }
  }
  fs_close(fd);
  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

#define USER_STACK_PGS (STACK_SIZE / PGSIZE)
#define NO_USER_STACK

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {

  void *user_stack = new_page(USER_STACK_PGS);

#ifdef HAS_VME
  protect(&pcb->as);

  uintptr_t stack_va_start = (uintptr_t)pcb->as.area.end - STACK_SIZE;
  Log("map user stack at va %p", stack_va_start);
  for (int i = 0; i < USER_STACK_PGS; i++) {
    map(&pcb->as, (void *)(stack_va_start + i * PGSIZE), user_stack + i * PGSIZE, MMAP_READ | MMAP_WRITE);
  }
#endif

  int argc = 0;
  while (argv && argv[argc] != NULL) {
    argc ++;
  }

  int envc = 0;
  while (envp && envp[envc] != NULL) {
    envc ++;
  }

  char *argv_copy[argc + 1];
  char *envp_copy[envc + 1];
  envp_copy[envc] = NULL;
  argv_copy[argc] = NULL;

  uint8_t *stack_top = (uint8_t *)user_stack + STACK_SIZE;
#ifndef NO_USER_STACK
  stack_top -= sizeof(Context);
#endif
  stack_top -= sizeof(char *);

  for (int i = envc - 1; i >= 0; i --) {
    stack_top -= strlen(envp[i]) + 1;
    strcpy((char *)stack_top, envp[i]);
    envp_copy[i] = (char *)stack_top;
  }

  for (int i = argc - 1; i >= 0; i --) {
    stack_top -= strlen(argv[i]) + 1;
    strcpy((char *)stack_top, argv[i]);
    argv_copy[i] = (char *)stack_top;
  }

  uintptr_t align = (uintptr_t)stack_top % sizeof(uintptr_t);
  if (align) {
    stack_top -= align;
  }

  // envp
  stack_top -= sizeof(char *) * (envc + 1);
  memcpy(stack_top, envp_copy, sizeof(char *) * (envc + 1));

  // argv
  stack_top -= sizeof(char *) * (argc + 1);
  memcpy(stack_top, argv_copy, sizeof(char *) * (argc + 1));

  // argc
  stack_top -= sizeof(uintptr_t);
  *(uintptr_t *)stack_top = (uintptr_t)argc;

  // entry
  uintptr_t entry = loader(pcb, filename);
#ifdef NO_USER_STACK
  pcb->cp = ucontext(&pcb->as, (Area) { pcb->stack, pcb->stack + STACK_SIZE }, (void *)entry);
# ifdef HAS_VME
  pcb->cp->gpr[2] = (uintptr_t)pcb->as.area.end;
# else
  pcb->cp->gpr[2] = (uintptr_t)user_stack + STACK_SIZE;
# endif
#else
  pcb->cp = ucontext(&pcb->as, (Area) { user_stack, user_stack + STACK_SIZE }, (void *)entry);
  pcb->cp->GPRx = (uintptr_t)stack_top;
#endif
}


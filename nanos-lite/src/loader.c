#include <proc.h>
#include <elf.h>
#include <fs.h>

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
      // [VirtAddr, VirtAddr + MemSiz)
      assert(fs_lseek(fd, phdr.p_offset, SEEK_SET) == phdr.p_offset);
      assert(fs_read(fd, (void *)phdr.p_vaddr, phdr.p_filesz) == phdr.p_filesz);
      if (phdr.p_memsz > phdr.p_filesz) {
        // [VirtAddr + FileSiz, VirtAddr + MemSiz)
        memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, phdr.p_memsz - phdr.p_filesz);
      }
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

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  uintptr_t entry = loader(pcb, filename);
  pcb->cp = ucontext(&pcb->as, (Area) { pcb->stack, pcb->stack + STACK_SIZE }, (void *)entry);

  int argc = 0;
  while (argv[argc] != NULL) {
    argc ++;
  }

  int envc = 0;
  while (envp[envc] != NULL) {
    envc ++;
  }

  char *argv_copy[argc + 1];
  char *envp_copy[envc + 1];
  uint8_t *stack_top = pcb->stack + STACK_SIZE;
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

  stack_top -= sizeof(char *) * (envc + 1);
  memcpy(stack_top, envp_copy, sizeof(char *) * (envc + 1));
  stack_top -= sizeof(char *) * (argc + 1);
  memcpy(stack_top, argv_copy, sizeof(char *) * (argc + 1));
  stack_top -= sizeof(int);
  *(int *)stack_top = argc;
  pcb->cp->GPRx = (uintptr_t)stack_top;
}


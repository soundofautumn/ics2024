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

  int argc = 0, envc = 0;
  size_t str_size = 0;
  if (argv != NULL) {
    for (char *const *p = argv; *p != NULL; p++) {
      argc++;
      str_size += strlen(*p) + 1;
    }
  }
  if (envp != NULL) {
    for (char *const *p = envp; *p != NULL; p++) {
      envc++;
      str_size += strlen(*p) + 1;
    }
  }

  size_t total_words = 1 + (argc + 1) + (envc + 1);
  size_t total_size = total_words * sizeof(uintptr_t) + str_size;

  uintptr_t stack_top;
#if defined(__ISA_AM_NATIVE__)
  stack_top = 0xc0000000;
#elif defined(__ISA_RISCV32__) || defined(__ISA_RISCV64__)
  stack_top = 0x80000000;
#else
#error Unsupported ISA for context_uload
#endif

  uintptr_t args_addr = stack_top - total_size;
  args_addr &= ~(uintptr_t)0xf;
  uintptr_t *args = (uintptr_t *)args_addr;

  char *str_base = (char *)args + total_words * sizeof(uintptr_t);

  size_t offset = 0;
  for (int i = 0; i < argc; i++) {
    size_t len = strlen(argv[i]) + 1;
    memcpy(str_base + offset, argv[i], len);
    args[1 + i] = (uintptr_t)(str_base + offset);
    offset += len;
  }
  args[1 + argc] = 0;

  for (int i = 0; i < envc; i++) {
    size_t len = strlen(envp[i]) + 1;
    memcpy(str_base + offset, envp[i], len);
    args[1 + argc + 1 + i] = (uintptr_t)(str_base + offset);
    offset += len;
  }
  args[1 + argc + 1 + envc] = 0;

  args[0] = (uintptr_t)argc;

#if defined(__ISA_AM_NATIVE__)
  ((Context *)pcb->cp)->uc.uc_mcontext.gregs[REG_RAX] = args_addr;
  ((Context *)pcb->cp)->uc.uc_mcontext.gregs[REG_RSP] = args_addr;
#elif defined(__ISA_RISCV32__) || defined(__ISA_RISCV64__)
  ((Context *)pcb->cp)->gpr[10] = args_addr;
  ((Context *)pcb->cp)->gpr[2] = args_addr;
#endif
}


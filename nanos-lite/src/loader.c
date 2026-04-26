#include <proc.h>
#include <elf.h>

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

size_t get_ramdisk_size();
size_t ramdisk_read(void *buf, size_t offset, size_t len);


static uintptr_t loader(PCB *pcb, const char *filename) {
  Elf_Ehdr *ehdr;
  Log("Loading ELF file '%s' to ramdisk", filename);
  assert(ramdisk_read(&ehdr, 0, sizeof(Elf_Ehdr)) == sizeof(Elf_Ehdr));
  assert(memcmp(ehdr->e_ident, ELFMAG, SELFMAG) == 0);
  assert(ehdr->e_type == ET_EXEC);
  assert(ehdr->e_machine == EXPECT_TYPE);
  assert(ehdr->e_phnum > 0);

  Elf_Phdr phdr;
  for (int i = 0; i < ehdr->e_phnum; i++) {
    Log("Loading program header %d", i);
    Log("  type = %d, offset = %d, vaddr = %p, filesz = %d, memsz = %d",
        phdr.p_type, phdr.p_offset, phdr.p_vaddr, phdr.p_filesz, phdr.p_memsz);
    assert(ramdisk_read(&phdr, ehdr->e_phoff + i * sizeof(Elf_Phdr), sizeof(Elf_Phdr)) == sizeof(Elf_Phdr));
    if (phdr.p_type == PT_LOAD) {
      // [VirtAddr, VirtAddr + MemSiz)
      ramdisk_read((void *)phdr.p_vaddr, phdr.p_offset, phdr.p_filesz);
      if (phdr.p_memsz > phdr.p_filesz) {
        // [VirtAddr + FileSiz, VirtAddr + MemSiz)
        memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, phdr.p_memsz - phdr.p_filesz);
      }
    }
  }
  return ehdr->e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}


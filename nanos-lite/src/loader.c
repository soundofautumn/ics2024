#include <proc.h>
#include <elf.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

size_t get_ramdisk_size();
size_t ramdisk_read(void *buf, size_t offset, size_t len);


static uintptr_t loader(PCB *pcb, const char *filename) {
  size_t elf_size = get_ramdisk_size();
  char *elf_buf = (char *)malloc(elf_size);
  assert(elf_buf != NULL);
  assert(ramdisk_read(elf_buf, 0, elf_size) == elf_size);

  Elf_Ehdr *ehdr = (Elf_Ehdr *)elf_buf;
  assert(memcmp(ehdr->e_ident, ELFMAG, SELFMAG) == 0);
  assert(ehdr->e_type == ET_EXEC);
  assert(ehdr->e_phnum > 0);
  return ehdr->e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}


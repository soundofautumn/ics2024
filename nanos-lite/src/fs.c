#include <fs.h>

size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t ramdisk_read(void *buf, size_t offset, size_t len);

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, invalid_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, invalid_write},
#include "files.h"
};

void init_fs() {
  // TODO: initialize the size of /dev/fb
}

int fs_open(const char *pathname, int flags, int mode) {
  for (size_t i = 0; i < sizeof(file_table) / sizeof(file_table[0]); i++) {
    if (strcmp(pathname, file_table[i].name) == 0) {
      return i;
    }
  }
  panic("cannot find file '%s'", pathname);
  return -1;
}

size_t fs_read(int fd, void *buf, size_t len) {
  assert(fd >= 0 && fd < sizeof(file_table) / sizeof(file_table[0]));
  Finfo *f = &file_table[fd];
  if(f->read == NULL) {
    ramdisk_read(buf, f->disk_offset, len);
    f->disk_offset += len;
    return len;
  }
  return f->read(buf, 0, len);
}

size_t fs_write(int fd, const void *buf, size_t len) {
  assert(fd >= 0 && fd < sizeof(file_table) / sizeof(file_table[0]));
  Finfo *f = &file_table[fd];
  if(f->write == NULL) {
    ramdisk_write(buf, f->disk_offset, len);
    f->disk_offset += len;
    return len;
  }
  return f->write(buf, 0, len);
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  assert(fd >= 0 && fd < sizeof(file_table) / sizeof(file_table[0]));
  Finfo *f = &file_table[fd];
  size_t new_offset = 0;
  switch (whence) {
    case SEEK_SET: new_offset = offset; break;
    case SEEK_CUR: new_offset = f->disk_offset + offset; break;
    case SEEK_END: new_offset = f->size + offset; break;
    default: panic("invalid whence = %d", whence);
  }
  if (new_offset > f->size) {
    panic("invalid offset = %d", new_offset);
  }
  f->disk_offset = new_offset;
  return f->disk_offset;
}

int fs_close(int fd) {
  assert(fd >= 0 && fd < sizeof(file_table) / sizeof(file_table[0]));
  return 0;
}
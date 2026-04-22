#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len = 0;
  while (*s++) len++;
  return len;
}

char *strcpy(char *dst, const char *src) {
  char *p = dst;
  while ((*dst++ = *src++));
  return p;
}

char *strncpy(char *dst, const char *src, size_t n) {
  char *p = dst;
  while (n -- && (*dst++ = *src++));
  return p;
}

char *strcat(char *dst, const char *src) {
  char *p = dst;
  while (*dst) dst++;
  while ((*dst++ = *src++));
  return p;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s2) {
    if (*s1 != *s2) {
      return *s1 - *s2;
    }
    s1++;
    s2++;
  }
  return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  while (n -- && *s1 && *s2) {
    if (*s1 != *s2) {
      return *s1 - *s2;
    }
    s1++;
    s2++;
  }
  return *s1 - *s2;
}

void *memset(void *s, int c, size_t n) {
  char *p = (char *)s;
  while (n--) {
    *p++ = (char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  char *d = (char *)dst;
  const char *s = (const char *)src;
  if (d < s) {
    while (n--) {
      *d++ = *s++;
    }
  } else {
    d += n;
    s += n;
    while (n--) {
      *--d = *--s;
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  char *d = (char *)out;
  const char *s = (const char *)in;
  while (n--) {
    *d++ = *s++;
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  char *d1 = (char *)s1;
  char *d2 = (char *)s2;
  while (n--) {
    if (*d1 != *d2) {
      return *d1 - *d2;
    }
    d1++;
    d2++;
  }
  return 0;
}

#endif

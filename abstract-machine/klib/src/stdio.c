#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  char buf[1024];
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(buf, fmt, ap);
  va_end(ap);
  for (char *p = buf; *p; p++) {
    putch(*p);
  }
  return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  int ret = 0;
  while (*fmt) {
    if (*fmt != '%') {
      *out++ = *fmt++;
      ret ++;
      continue;
    }
    fmt ++;
    switch (*fmt) {
      case 'd': 
        int n = va_arg(ap, int);
        if (n == 0) {
          *out++ = '0';
          ret ++;
        } else {
          if (n < 0) {
            *out++ = '-';
            ret ++;
            n = -n;
          }
          char buf[32];
          int i = 0;
          while (n) {
            buf[i++] = n % 10 + '0';
            n /= 10;
          }
          while (i > 0) {
            *out++ = buf[--i];
            ret ++;
          }
        }

        break;
      case 's': 
        char *str = va_arg(ap, char *);
        while (*str) {
          *out++ = *str++;
          ret ++;
        }
        break;
      case 'c': 
        *out++ = (char)va_arg(ap, int); 
        ret ++;
        break;
      case '%': 
        *out++ = '%'; 
        ret ++;
        break;
      default: panic("unsupported format");
    }
    fmt ++;
  }
  *out = '\0';
  return ret;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(out, fmt, ap);
  va_end(ap);
  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return ret;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  int ret = vsprintf(out, fmt, ap);
  if (ret >= n) {
    out[n - 1] = '\0';
  }
  return ret;
}

#endif

#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  char buf[10240];
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(buf, fmt, ap);
  va_end(ap);
  for (char *p = buf; *p; p++) {
    putch(*p);
  }
  return ret;
}

static int parse_uint(const char **fmt) {
  int val = 0;
  while (**fmt >= '0' && **fmt <= '9') {
    val = val * 10 + (**fmt - '0');
    (*fmt)++;
  }
  return val;
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

    int width = -1;
    int precision = -1;

    if (*fmt == '*') {
      width = va_arg(ap, int);
      if (width < 0) {
        width = -width;
      }
      fmt ++;
    } else if (*fmt >= '0' && *fmt <= '9') {
      width = parse_uint(&fmt);
    }

    if (*fmt == '.') {
      fmt ++;
      if (*fmt == '*') {
        precision = va_arg(ap, int);
        if (precision < 0) {
          precision = -1;
        }
        fmt ++;
      } else {
        precision = parse_uint(&fmt);
      }
    }

    switch (*fmt) {
      case 'd': {
        int n = va_arg(ap, int);
        int is_neg = (n < 0);
        unsigned int u = is_neg ? (unsigned int)(-(n + 1)) + 1 : (unsigned int)n;

        char buf[32];
        int digit_len = 0;
        if (u == 0) {
          if (precision != 0) {
            buf[digit_len++] = '0';
          }
        } else {
          while (u) {
            buf[digit_len++] = u % 10 + '0';
            u /= 10;
          }
        }

        int zero_pad = 0;
        if (precision > digit_len) {
          zero_pad = precision - digit_len;
        }

        int body_len = digit_len + zero_pad + (is_neg ? 1 : 0);
        int space_pad = 0;
        if (width > body_len) {
          space_pad = width - body_len;
        }

        while (space_pad-- > 0) {
          *out++ = ' ';
          ret ++;
        }
        if (is_neg) {
          *out++ = '-';
          ret ++;
        }
        while (zero_pad-- > 0) {
          *out++ = '0';
          ret ++;
        }
        while (digit_len > 0) {
          *out++ = buf[--digit_len];
          ret ++;
        }
        break;
      }
      case 'u': {
        unsigned int u = va_arg(ap, unsigned int);

        char buf[32];
        int digit_len = 0;
        if (u == 0) {
          if (precision != 0) {
            buf[digit_len++] = '0';
          }
        } else {
          while (u) {
            buf[digit_len++] = u % 10 + '0';
            u /= 10;
          }
        }

        int zero_pad = 0;
        if (precision > digit_len) {
          zero_pad = precision - digit_len;
        }

        int body_len = digit_len + zero_pad;
        int space_pad = 0;
        if (width > body_len) {
          space_pad = width - body_len;
        }

        while (space_pad-- > 0) {
          *out++ = ' ';
          ret ++;
        }
        while (zero_pad-- > 0) {
          *out++ = '0';
          ret ++;
        }
        while (digit_len > 0) {
          *out++ = buf[--digit_len];
          ret ++;
        }
        break;
      }
      case 'x': {
        unsigned int u = va_arg(ap, unsigned int);

        char buf[32];
        int digit_len = 0;
        if (u == 0) {
          if (precision != 0) {
            buf[digit_len++] = '0';
          }
        } else {
          while (u) {
            unsigned int digit = u % 16;
            if (digit < 10) {
              buf[digit_len++] = digit + '0';
            } else {
              buf[digit_len++] = digit - 10 + 'a';
            }
            u /= 16;
          }
        }

        int zero_pad = 0;
        if (precision > digit_len) {
          zero_pad = precision - digit_len;
        }

        int body_len = digit_len + zero_pad;
        int space_pad = 0;
        if (width > body_len) {
          space_pad = width - body_len;
        }

        while (space_pad-- > 0) {
          *out++ = ' ';
          ret ++;
        }
        while (zero_pad-- > 0) {
          *out++ = '0';
          ret ++;
        }
        while (digit_len > 0) {
          *out++ = buf[--digit_len];
          ret ++;
        }
        break;
      }
      case 's': {
        const char *str = va_arg(ap, const char *);
        if (str == NULL) {
          str = "(null)";
        }

        int len = 0;
        while (str[len]) {
          len++;
        }

        int out_len = len;
        if (precision >= 0 && precision < out_len) {
          out_len = precision;
        }

        int space_pad = 0;
        if (width > out_len) {
          space_pad = width - out_len;
        }

        while (space_pad-- > 0) {
          *out++ = ' ';
          ret ++;
        }
        for (int i = 0; i < out_len; i++) {
          *out++ = str[i];
          ret ++;
        }
        break;
      }
      case 'c': {
        int ch = va_arg(ap, int);
        int space_pad = (width > 1) ? (width - 1) : 0;
        while (space_pad-- > 0) {
          *out++ = ' ';
          ret ++;
        }
        *out++ = (char)ch;
        ret ++;
        break;
      }
      case '%': {
        int space_pad = (width > 1) ? (width - 1) : 0;
        while (space_pad-- > 0) {
          *out++ = ' ';
          ret ++;
        }
        *out++ = '%';
        ret ++;
        break;
      }
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

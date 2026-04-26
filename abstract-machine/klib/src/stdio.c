#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

typedef void (*emit_func_t)(char ch, void *ctx);

static void emit_char(emit_func_t emit, void *ctx, int *ret, char ch) {
  emit(ch, ctx);
  (*ret)++;
}

static void emit_repeated(emit_func_t emit, void *ctx, int *ret, char ch, int cnt) {
  while (cnt-- > 0) {
    emit_char(emit, ctx, ret, ch);
  }
}

static int parse_uint(const char **fmt) {
  int val = 0;
  while (**fmt >= '0' && **fmt <= '9') {
    val = val * 10 + (**fmt - '0');
    (*fmt)++;
  }
  return val;
}

static int vformat(emit_func_t emit, void *ctx, const char *fmt, va_list ap) {
  int ret = 0;
  while (*fmt) {
    if (*fmt != '%') {
      emit_char(emit, ctx, &ret, *fmt++);
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

        emit_repeated(emit, ctx, &ret, ' ', space_pad);
        if (is_neg) {
          emit_char(emit, ctx, &ret, '-');
        }
        emit_repeated(emit, ctx, &ret, '0', zero_pad);
        while (digit_len > 0) {
          emit_char(emit, ctx, &ret, buf[--digit_len]);
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

        emit_repeated(emit, ctx, &ret, ' ', space_pad);
        emit_repeated(emit, ctx, &ret, '0', zero_pad);
        while (digit_len > 0) {
          emit_char(emit, ctx, &ret, buf[--digit_len]);
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

        emit_repeated(emit, ctx, &ret, ' ', space_pad);
        emit_repeated(emit, ctx, &ret, '0', zero_pad);
        while (digit_len > 0) {
          emit_char(emit, ctx, &ret, buf[--digit_len]);
        }
        break;
      }
      case 'p': {
        uintptr_t u = (uintptr_t)va_arg(ap, void *);

        char buf[2 * sizeof(uintptr_t)];
        int digit_len = 0;
        if (u == 0) {
          buf[digit_len++] = '0';
        } else {
          while (u) {
            uintptr_t digit = u % 16;
            if (digit < 10) {
              buf[digit_len++] = (char)(digit + '0');
            } else {
              buf[digit_len++] = (char)(digit - 10 + 'a');
            }
            u /= 16;
          }
        }

        int zero_pad = 0;
        if (precision > digit_len) {
          zero_pad = precision - digit_len;
        }

        int body_len = 2 + zero_pad + digit_len;
        int space_pad = 0;
        if (width > body_len) {
          space_pad = width - body_len;
        }

        emit_repeated(emit, ctx, &ret, ' ', space_pad);
        emit_char(emit, ctx, &ret, '0');
        emit_char(emit, ctx, &ret, 'x');
        emit_repeated(emit, ctx, &ret, '0', zero_pad);
        while (digit_len > 0) {
          emit_char(emit, ctx, &ret, buf[--digit_len]);
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

        emit_repeated(emit, ctx, &ret, ' ', space_pad);
        for (int i = 0; i < out_len; i++) {
          emit_char(emit, ctx, &ret, str[i]);
        }
        break;
      }
      case 'c': {
        int ch = va_arg(ap, int);
        int space_pad = (width > 1) ? (width - 1) : 0;
        emit_repeated(emit, ctx, &ret, ' ', space_pad);
        emit_char(emit, ctx, &ret, (char)ch);
        break;
      }
      case '%': {
        int space_pad = (width > 1) ? (width - 1) : 0;
        emit_repeated(emit, ctx, &ret, ' ', space_pad);
        emit_char(emit, ctx, &ret, '%');
        break;
      }
      default: panic(strcat("unsupported format: ", &*fmt));
    }

    fmt ++;
  }
  return ret;
}

static void emit_to_putch(char ch, void *ctx) {
  (void)ctx;
  putch(ch);
}

struct sprintf_ctx {
  char *out;
};

static void emit_to_sprintf(char ch, void *ctx) {
  struct sprintf_ctx *s = (struct sprintf_ctx *)ctx;
  *s->out++ = ch;
}

struct snprintf_ctx {
  char *out;
  size_t n;
  size_t pos;
};

static void emit_to_snprintf(char ch, void *ctx) {
  struct snprintf_ctx *s = (struct snprintf_ctx *)ctx;
  if (s->n > 0 && s->pos < s->n - 1) {
    s->out[s->pos] = ch;
  }
  s->pos++;
}

int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vformat(emit_to_putch, NULL, fmt, ap);
  va_end(ap);
  return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  struct sprintf_ctx ctx = { .out = out };
  int ret = vformat(emit_to_sprintf, &ctx, fmt, ap);
  *ctx.out = '\0';
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
  struct snprintf_ctx ctx = { .out = out, .n = n, .pos = 0 };
  int ret = vformat(emit_to_snprintf, &ctx, fmt, ap);
  if (n > 0) {
    size_t end = (ctx.pos < n - 1) ? ctx.pos : (n - 1);
    out[end] = '\0';
  }
  return ret;
}

#endif

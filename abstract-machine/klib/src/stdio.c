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

static char to_digit(unsigned int digit) {
  return (digit < 10) ? (char)('0' + digit) : (char)('a' + digit - 10);
}

static int encode_uint_rev(char *buf, uintptr_t u, int base, int precision, int keep_zero) {
  int digit_len = 0;
  if (u == 0) {
    if (precision != 0 || keep_zero) {
      buf[digit_len++] = '0';
    }
    return digit_len;
  }

  while (u) {
    buf[digit_len++] = to_digit((unsigned int)(u % (uintptr_t)base));
    u /= (uintptr_t)base;
  }
  return digit_len;
}

static void emit_number(emit_func_t emit, void *ctx, int *ret,
    char *digits_rev, int digit_len, int width, int precision,
    int is_neg, const char *prefix) {
  int prefix_len = 0;
  while (prefix != NULL && prefix[prefix_len] != '\0') {
    prefix_len++;
  }

  int zero_pad = 0;
  if (precision > digit_len) {
    zero_pad = precision - digit_len;
  }

  int body_len = digit_len + zero_pad + (is_neg ? 1 : 0) + prefix_len;
  int space_pad = 0;
  if (width > body_len) {
    space_pad = width - body_len;
  }

  emit_repeated(emit, ctx, ret, ' ', space_pad);
  if (is_neg) {
    emit_char(emit, ctx, ret, '-');
  }
  for (int i = 0; i < prefix_len; i++) {
    emit_char(emit, ctx, ret, prefix[i]);
  }
  emit_repeated(emit, ctx, ret, '0', zero_pad);
  while (digit_len > 0) {
    emit_char(emit, ctx, ret, digits_rev[--digit_len]);
  }
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
        uintptr_t u = (uintptr_t)(is_neg ? (unsigned int)(-(n + 1)) + 1 : (unsigned int)n);
        char buf[32];
        int digit_len = encode_uint_rev(buf, u, 10, precision, 0);
        emit_number(emit, ctx, &ret, buf, digit_len, width, precision, is_neg, NULL);
        break;
      }
      case 'u': {
        uintptr_t u = (uintptr_t)va_arg(ap, unsigned int);
        char buf[32];
        int digit_len = encode_uint_rev(buf, u, 10, precision, 0);
        emit_number(emit, ctx, &ret, buf, digit_len, width, precision, 0, NULL);
        break;
      }
      case 'x': {
        uintptr_t u = (uintptr_t)va_arg(ap, unsigned int);
        char buf[32];
        int digit_len = encode_uint_rev(buf, u, 16, precision, 0);
        emit_number(emit, ctx, &ret, buf, digit_len, width, precision, 0, NULL);
        break;
      }
      case 'p': {
        uintptr_t u = (uintptr_t)va_arg(ap, void *);
        char buf[2 * sizeof(uintptr_t)];
        int digit_len = encode_uint_rev(buf, u, 16, precision, 1);
        emit_number(emit, ctx, &ret, buf, digit_len, width, precision, 0, "0x");
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
      default: panic("unsupported format");
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

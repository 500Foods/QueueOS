/* The C library Jansson calls and the firmware does not provide.
 * malloc, free, and realloc live in pool.c. memcpy and memset live in
 * memory.c. This file is the rest of that link.
 */

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int shim_errno;

int *__errno_location(void)
{
    return &shim_errno;
}

void *memmove(void *dst, const void *src, size_t n)
{
    uint8_t *out = dst;
    const uint8_t *in = src;
    size_t i;

    if (out == in || n == 0)
        return dst;
    if (out < in) {
        for (i = 0; i < n; i++)
            out[i] = in[i];
    } else {
        for (i = n; i > 0; i--)
            out[i - 1] = in[i - 1];
    }
    return dst;
}

int memcmp(const void *left, const void *right, size_t n)
{
    const uint8_t *a = left;
    const uint8_t *b = right;
    size_t i;

    for (i = 0; i < n; i++) {
        if (a[i] != b[i])
            return a[i] < b[i] ? -1 : 1;
    }
    return 0;
}

void *memchr(const void *src, int c, size_t n)
{
    const uint8_t *p = src;
    size_t i;

    for (i = 0; i < n; i++) {
        if (p[i] == (uint8_t)c)
            return (void *)(p + i);
    }
    return 0;
}

size_t strlen(const char *s)
{
    size_t n = 0;

    while (s[n] != '\0')
        n++;
    return n;
}

int strcmp(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        left++;
        right++;
    }
    return (unsigned char)*left - (unsigned char)*right;
}

int strncmp(const char *left, const char *right, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++) {
        if (left[i] != right[i] || left[i] == '\0')
            return (unsigned char)left[i] - (unsigned char)right[i];
    }
    return 0;
}

char *strncpy(char *dst, const char *src, size_t n)
{
    size_t i;

    for (i = 0; i < n && src[i] != '\0'; i++)
        dst[i] = src[i];
    for (; i < n; i++)
        dst[i] = '\0';
    return dst;
}

char *strchr(const char *s, int c)
{
    while (*s != '\0') {
        if (*s == (char)c)
            return (char *)s;
        s++;
    }
    if ((char)c == '\0')
        return (char *)s;
    return 0;
}

#undef strtoll
long long strtoll(const char *nptr, char **endptr, int base)
{
    const char *s = nptr;
    int neg = 0;
    int any = 0;
    unsigned long long acc = 0;

    errno = 0;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
        s++;
    if (*s == '+' || *s == '-') {
        neg = *s == '-';
        s++;
    }
    if (base != 10) {
        if (endptr != 0)
            *endptr = (char *)nptr;
        return 0;
    }
    while (*s >= '0' && *s <= '9') {
        unsigned int dig = (unsigned int)(*s - '0');
        if (acc > (ULLONG_MAX - dig) / 10u) {
            errno = ERANGE;
            while (*s >= '0' && *s <= '9')
                s++;
            if (endptr != 0)
                *endptr = (char *)s;
            return neg ? LLONG_MIN : LLONG_MAX;
        }
        acc = acc * 10u + dig;
        any = 1;
        s++;
    }
    if (endptr != 0)
        *endptr = (char *)(any ? s : nptr);
    if (!any)
        return 0;
    if (neg) {
        if (acc > (unsigned long long)LLONG_MAX + 1ull) {
            errno = ERANGE;
            return LLONG_MIN;
        }
        return -(long long)acc;
    }
    if (acc > (unsigned long long)LLONG_MAX) {
        errno = ERANGE;
        return LLONG_MAX;
    }
    return (long long)acc;
}

/* glibc redirects the call to this name. The definition above stays strtoll. */
long long __isoc23_strtoll(const char *nptr, char **endptr, int base)
{
    return strtoll(nptr, endptr, base);
}

double strtod(const char *nptr, char **endptr)
{
    const char *s = nptr;
    int neg = 0;
    int any = 0;
    int exp_sign = 1;
    int exp_digits = 0;
    double value = 0.0;
    double place = 0.1;
    int exponent = 0;

    errno = 0;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
        s++;
    if (*s == '+' || *s == '-') {
        neg = *s == '-';
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        value = value * 10.0 + (double)(*s - '0');
        any = 1;
        s++;
    }
    if (*s == '.') {
        s++;
        while (*s >= '0' && *s <= '9') {
            value += (double)(*s - '0') * place;
            place *= 0.1;
            any = 1;
            s++;
        }
    }
    if (*s == 'e' || *s == 'E') {
        const char *mark = s;
        s++;
        if (*s == '+' || *s == '-') {
            exp_sign = *s == '-' ? -1 : 1;
            s++;
        }
        while (*s >= '0' && *s <= '9') {
            if (exponent < 100000)
                exponent = exponent * 10 + (*s - '0');
            exp_digits = 1;
            s++;
        }
        if (!exp_digits)
            s = mark;
        else
            exponent *= exp_sign;
    }
    if (!any) {
        if (endptr != 0)
            *endptr = (char *)nptr;
        return 0.0;
    }
    if (exponent > 0) {
        while (exponent > 0) {
            value *= 10.0;
            exponent--;
            if (value > 1.0e308) {
                errno = ERANGE;
                value = __builtin_huge_val();
                break;
            }
        }
    } else {
        while (exponent < 0) {
            value /= 10.0;
            exponent++;
        }
    }
    if (neg)
        value = -value;
    if (endptr != 0)
        *endptr = (char *)s;
    return value;
}

static void put_char(char *buf, size_t size, size_t *used, char c)
{
    if (buf != 0 && *used + 1 < size)
        buf[*used] = c;
    *used += 1;
}

static void put_span(char *buf, size_t size, size_t *used, const char *text)
{
    while (*text != '\0') {
        put_char(buf, size, used, *text);
        text++;
    }
}

static void put_uint(char *buf, size_t size, size_t *used,
                     unsigned long long value, int radix, int upper,
                     int width, char pad)
{
    char tmp[64];
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    int n = 0;

    if (width < 0)
        width = 0;
    if (width > (int)sizeof tmp)
        width = (int)sizeof tmp;
    if (value == 0)
        tmp[n++] = '0';
    while (value > 0 && n < (int)sizeof tmp) {
        tmp[n++] = digits[value % (unsigned)radix];
        value /= (unsigned)radix;
    }
    while (n < width && n < (int)sizeof tmp)
        tmp[n++] = pad;
    while (n > 0)
        put_char(buf, size, used, tmp[--n]);
}

static void put_int(char *buf, size_t size, size_t *used, long long value,
                    int width, char pad)
{
    unsigned long long mag;

    if (value < 0) {
        put_char(buf, size, used, '-');
        mag = (unsigned long long)(-(value + 1)) + 1ull;
        if (width > 0)
            width--;
    } else {
        mag = (unsigned long long)value;
    }
    put_uint(buf, size, used, mag, 10, 0, width, pad);
}

/* %#.0f of 1 is the decimal-point probe. Other values are rounded. */
static void put_float(char *buf, size_t size, size_t *used, double value,
                      int precision, int hash)
{
    int i;
    int neg = 0;
    double scale = 1.0;
    double mag;
    long long scaled;
    long long divisor = 1;
    long long whole;
    long long frac;

    if (precision < 0)
        precision = 6;
    if (precision > 9)
        precision = 9;
    if (value < 0.0) {
        neg = 1;
        value = -value;
    }
    for (i = 0; i < precision; i++)
        scale *= 10.0;
    mag = value * scale + 0.5;
    if (!(mag < 9223372036854775808.0)) {
        put_span(buf, size, used, neg ? "-0" : "0");
        return;
    }
    scaled = (long long)mag;
    for (i = 0; i < precision; i++)
        divisor *= 10;
    whole = precision == 0 ? scaled : scaled / divisor;
    frac = precision == 0 ? 0 : scaled % divisor;
    if (neg)
        put_char(buf, size, used, '-');
    put_uint(buf, size, used, (unsigned long long)whole, 10, 0, 0, ' ');
    if (precision > 0 || hash) {
        long long place;

        put_char(buf, size, used, '.');
        place = divisor / 10;
        for (i = 0; i < precision; i++) {
            int digit = place == 0 ? 0 : (int)(frac / place);

            put_char(buf, size, used, (char)('0' + digit));
            if (place != 0)
                frac %= place;
            place /= 10;
        }
    }
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    size_t used = 0;

    while (*fmt != '\0') {
        int longness = 0;
        int hash = 0;
        int zero = 0;
        int width = 0;
        int precision = -1;
        char pad = ' ';

        if (*fmt != '%') {
            put_char(buf, size, &used, *fmt);
            fmt++;
            continue;
        }
        fmt++;
        while (*fmt == '-' || *fmt == '+' || *fmt == ' ' || *fmt == '#' || *fmt == '0') {
            if (*fmt == '#')
                hash = 1;
            if (*fmt == '0')
                zero = 1;
            fmt++;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }
        if (zero)
            pad = '0';
        if (*fmt == '.') {
            fmt++;
            precision = 0;
            while (*fmt >= '0' && *fmt <= '9') {
                precision = precision * 10 + (*fmt - '0');
                fmt++;
            }
        }
        while (*fmt == 'l') {
            longness++;
            fmt++;
        }
        if (*fmt == 'z') {
            longness = 2;
            fmt++;
        }
        if (*fmt == 'd' || *fmt == 'i') {
            long long value;
            if (longness >= 2)
                value = va_arg(ap, long long);
            else if (longness == 1)
                value = va_arg(ap, long);
            else
                value = va_arg(ap, int);
            put_int(buf, size, &used, value, width, pad);
        } else if (*fmt == 'u' || *fmt == 'x' || *fmt == 'X') {
            unsigned long long value;
            int radix = *fmt == 'u' ? 10 : 16;
            if (longness >= 2)
                value = va_arg(ap, unsigned long long);
            else if (longness == 1)
                value = va_arg(ap, unsigned long);
            else
                value = va_arg(ap, unsigned int);
            put_uint(buf, size, &used, value, radix, *fmt == 'X', width, pad);
        } else if (*fmt == 'p') {
            put_span(buf, size, &used, "0x");
            put_uint(buf, size, &used, (unsigned long long)(uintptr_t)va_arg(ap, void *), 16, 0, 0, ' ');
        } else if (*fmt == 's') {
            const char *text = va_arg(ap, const char *);
            if (text == 0)
                text = "(null)";
            while (*text != '\0' && precision != 0) {
                put_char(buf, size, &used, *text);
                text++;
                if (precision > 0)
                    precision--;
            }
        } else if (*fmt == 'c') {
            put_char(buf, size, &used, (char)va_arg(ap, int));
        } else if (*fmt == 'f' || *fmt == 'e' || *fmt == 'E'
                   || *fmt == 'g' || *fmt == 'G') {
            double value = va_arg(ap, double);
            if (hash && precision == 0 && value == 1.0)
                put_span(buf, size, &used, "1.");
            else
                put_float(buf, size, &used, value, precision, hash);
        } else if (*fmt == '%') {
            put_char(buf, size, &used, '%');
        } else if (*fmt == '\0') {
            break;
        } else {
            put_char(buf, size, &used, *fmt);
        }
        if (*fmt != '\0')
            fmt++;
    }
    if (buf != 0 && size > 0) {
        size_t at = used < size ? used : size - 1;
        buf[at] = '\0';
    }
    if (used > (size_t)INT_MAX)
        return -1;
    return (int)used;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}

int sprintf(char *buf, const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, (size_t)-1, fmt, ap);
    va_end(ap);
    return n;
}

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *))
{
    uint8_t *items = base;
    uint8_t stack_tmp[128];
    uint8_t *swap = stack_tmp;
    uint8_t *heap = 0;
    size_t i;

    if (nmemb < 2 || size == 0)
        return;
    if (size > sizeof stack_tmp) {
        heap = malloc(size);
        if (heap == 0)
            return;
        swap = heap;
    }
    for (i = 1; i < nmemb; i++) {
        size_t j = i;
        memcpy(swap, items + i * size, size);
        while (j > 0 && compar(items + (j - 1) * size, swap) > 0) {
            memcpy(items + j * size, items + (j - 1) * size, size);
            j--;
        }
        memcpy(items + j * size, swap, size);
    }
    free(heap);
}

time_t time(time_t *timer)
{
    if (timer != 0)
        *timer = 0;
    return 0;
}

FILE *fopen(const char *path, const char *mode)
{
    (void)path;
    (void)mode;
    return 0;
}

int fclose(FILE *stream)
{
    (void)stream;
    return -1;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    (void)ptr;
    (void)size;
    (void)nmemb;
    (void)stream;
    return 0;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    (void)ptr;
    (void)size;
    (void)nmemb;
    (void)stream;
    return 0;
}

int fgetc(FILE *stream)
{
    (void)stream;
    return EOF;
}

char *strerror(int errnum)
{
    (void)errnum;
    return "error";
}

/* json_loadf compares its FILE* with stdin. Boot does not call it. */
#undef stdin
static FILE stdin_file;
FILE *stdin = &stdin_file;

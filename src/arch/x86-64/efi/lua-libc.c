/* Symbols Lua's own libraries name, beyond c-shim.c.
 * The boot scripts use math.floor, math.max, math.min, and string.format.
 * pcall uses _setjmp in setjmp.c. Trig, logs, and the file calls are here
 * so lmathlib and liolib link. The scripts do not call those.
 */

#include <locale.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define CT_UPPER 0x100u
#define CT_LOWER 0x200u
#define CT_ALPHA 0x400u
#define CT_DIGIT 0x800u
#define CT_XDIGIT 0x1000u
#define CT_SPACE 0x2000u
#define CT_PRINT 0x4000u
#define CT_GRAPH 0x8000u
#define CT_BLANK 0x1u
#define CT_CNTRL 0x2u
#define CT_PUNCT 0x4u
#define CT_ALNUM 0x8u

const unsigned short **__ctype_b_loc(void)
{
    static unsigned short table[384];
    static const unsigned short *page;
    static int ready;
    int c;

    if (ready)
        return (const unsigned short **)&page;
    for (c = 0; c < 128; c++)
        table[c] = 0;
    for (c = 0; c < 256; c++) {
        unsigned short bits = 0;

        if (c < 32 || c == 127)
            bits = CT_CNTRL;
        if (c == ' ' || c == '\t')
            bits |= CT_SPACE | CT_BLANK | CT_PRINT;
        else if (c == '\n' || c == '\r' || c == '\v' || c == '\f')
            bits |= CT_SPACE;
        if (c >= '0' && c <= '9')
            bits |= CT_DIGIT | CT_XDIGIT | CT_PRINT | CT_GRAPH | CT_ALNUM;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            bits |= CT_ALPHA | CT_PRINT | CT_GRAPH | CT_ALNUM;
            bits |= (c <= 'Z') ? CT_UPPER : CT_LOWER;
            if ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
                bits |= CT_XDIGIT;
        }
        if (c > 32 && c < 127 && (bits & (CT_ALNUM | CT_SPACE)) == 0)
            bits |= CT_PUNCT | CT_PRINT | CT_GRAPH;
        if (c == ' ')
            bits |= CT_PRINT;
        table[128 + c] = bits;
    }
    page = table + 128;
    ready = 1;
    return (const unsigned short **)&page;
}

int toupper(int c)
{
    if (c >= 'a' && c <= 'z')
        return c - ('a' - 'A');
    return c;
}

int tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');
    return c;
}

char *strcpy(char *dst, const char *src)
{
    char *out = dst;

    while ((*out++ = *src++) != '\0')
        ;
    return dst;
}

char *strstr(const char *hay, const char *needle)
{
    size_t n;

    if (needle[0] == '\0')
        return (char *)hay;
    n = strlen(needle);
    for (; *hay != '\0'; hay++) {
        size_t i = 0;

        while (i < n && hay[i] == needle[i])
            i++;
        if (i == n)
            return (char *)hay;
    }
    return 0;
}

char *strpbrk(const char *s, const char *accept)
{
    for (; *s != '\0'; s++) {
        if (strchr(accept, *s) != 0)
            return (char *)s;
    }
    return 0;
}

size_t strspn(const char *s, const char *accept)
{
    size_t n = 0;

    while (s[n] != '\0' && strchr(accept, s[n]) != 0)
        n++;
    return n;
}

int strcoll(const char *left, const char *right)
{
    return strcmp(left, right);
}

char *setlocale(int category, const char *locale)
{
    (void)category;
    (void)locale;
    return "C";
}

struct lconv *localeconv(void)
{
    static struct lconv conv;
    static int ready;

    if (!ready) {
        conv.decimal_point = ".";
        conv.thousands_sep = "";
        conv.grouping = "";
        conv.int_curr_symbol = "";
        conv.currency_symbol = "";
        conv.mon_decimal_point = "";
        conv.mon_thousands_sep = "";
        conv.mon_grouping = "";
        conv.positive_sign = "";
        conv.negative_sign = "";
        ready = 1;
    }
    return &conv;
}

static FILE std_out;
static FILE std_err;
FILE *stdout = &std_out;
FILE *stderr = &std_err;

int fflush(FILE *stream)
{
    (void)stream;
    return 0;
}

FILE *fopen64(const char *path, const char *mode)
{
    return fopen(path, mode);
}

FILE *freopen64(const char *path, const char *mode, FILE *stream)
{
    (void)path;
    (void)mode;
    (void)stream;
    return 0;
}

FILE *tmpfile64(void)
{
    return 0;
}

char *tmpnam(char s[L_tmpnam])
{
    (void)s;
    return 0;
}

int getc(FILE *stream)
{
    return fgetc(stream);
}

int ungetc(int c, FILE *stream)
{
    (void)c;
    (void)stream;
    return EOF;
}

char *fgets(char *s, int size, FILE *stream)
{
    (void)s;
    (void)size;
    (void)stream;
    return 0;
}

int feof(FILE *stream)
{
    (void)stream;
    return 1;
}

int ferror(FILE *stream)
{
    (void)stream;
    return 1;
}

void clearerr(FILE *stream)
{
    (void)stream;
}

long ftell(FILE *stream)
{
    (void)stream;
    return -1;
}

int fseek(FILE *stream, long offset, int whence)
{
    (void)stream;
    (void)offset;
    (void)whence;
    return -1;
}

int setvbuf(FILE *stream, char *buf, int mode, size_t size)
{
    (void)stream;
    (void)buf;
    (void)mode;
    (void)size;
    return -1;
}

int fprintf(FILE *stream, const char *fmt, ...)
{
    char buf[128];
    va_list ap;

    (void)stream;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    return 0;
}

char *getenv(const char *name)
{
    (void)name;
    return 0;
}

int system(const char *command)
{
    (void)command;
    return -1;
}

int rename(const char *old, const char *new_name)
{
    (void)old;
    (void)new_name;
    return -1;
}

int remove(const char *path)
{
    (void)path;
    return -1;
}

clock_t clock(void)
{
    return 0;
}

double difftime(time_t end, time_t start)
{
    return (double)end - (double)start;
}

time_t mktime(struct tm *timeptr)
{
    (void)timeptr;
    return (time_t)-1;
}

struct tm *localtime(const time_t *timer)
{
    (void)timer;
    return 0;
}

struct tm *gmtime(const time_t *timer)
{
    (void)timer;
    return 0;
}

size_t strftime(char *s, size_t max, const char *fmt, const struct tm *timeptr)
{
    (void)s;
    (void)max;
    (void)fmt;
    (void)timeptr;
    return 0;
}

void exit(int status)
{
    (void)status;
    for (;;)
        ;
}

void abort(void)
{
    for (;;)
        ;
}

int abs(int n)
{
    return n < 0 ? -n : n;
}

static int finite_in_range(double x)
{
    return x == x && x < 9223372036854775808.0 && x > -9223372036854775808.0;
}

double floor(double x)
{
    long long n;

    if (!finite_in_range(x))
        return x;
    n = (long long)x;
    if ((double)n != x && x < 0)
        n--;
    return (double)n;
}

double ceil(double x)
{
    long long n;

    if (!finite_in_range(x))
        return x;
    n = (long long)x;
    if ((double)n != x && x > 0)
        n++;
    return (double)n;
}

double fabs(double x)
{
    union {
        double d;
        uint64_t u;
    } bits;

    bits.d = x;
    bits.u &= ~0x8000000000000000ull;
    return bits.d;
}

double fmod(double x, double y)
{
    double q;

    if (y == 0)
        return x - x;
    q = x / y;
    if (!finite_in_range(q))
        return x - x;
    q = (double)(long long)q;
    return x - q * y;
}

double sqrt(double x)
{
    double g;
    int i;

    if (x < 0 || x != x)
        return x - x;
    if (x == 0)
        return 0;
    g = x;
    for (i = 0; i < 24; i++)
        g = 0.5 * (g + x / g);
    return g;
}

double frexp(double x, int *exp)
{
    union {
        double d;
        uint64_t u;
    } bits;
    int e;

    if (exp == 0)
        return x;
    if (x == 0 || x != x) {
        *exp = 0;
        return x;
    }
    bits.d = x;
    e = (int)((bits.u >> 52) & 0x7ffu);
    if (e == 0) {
        *exp = 0;
        return x;
    }
    *exp = e - 1022;
    bits.u = (bits.u & ~0x7ff0000000000000ull) | (1022ull << 52);
    return bits.d;
}

double ldexp(double x, int exp)
{
    union {
        double d;
        uint64_t u;
    } bits;
    int e;

    if (x == 0 || x != x)
        return x;
    bits.d = x;
    e = (int)((bits.u >> 52) & 0x7ffu) + exp;
    if (e <= 0)
        return 0;
    if (e >= 0x7ff)
        return x * 1e300 * 1e300;
    bits.u = (bits.u & ~0x7ff0000000000000ull) | ((uint64_t)e << 52);
    return bits.d;
}

double pow(double x, double y)
{
    double result = 1;
    long long n;
    int inverse = 0;

    if (y == 0)
        return 1;
    if (floor(y) != y || y <= -64 || y >= 64)
        return 0;
    n = (long long)y;
    if (n < 0) {
        inverse = 1;
        n = -n;
    }
    while (n > 0) {
        if ((n & 1ll) != 0)
            result *= x;
        x *= x;
        n >>= 1;
    }
    return inverse ? 1.0 / result : result;
}

double log(double x)
{
    (void)x;
    return 0;
}

double log2(double x)
{
    (void)x;
    return 0;
}

double log10(double x)
{
    (void)x;
    return 0;
}

double exp(double x)
{
    (void)x;
    return 0;
}

double sin(double x)
{
    (void)x;
    return 0;
}

double cos(double x)
{
    (void)x;
    return 0;
}

double tan(double x)
{
    (void)x;
    return 0;
}

double asin(double x)
{
    (void)x;
    return 0;
}

double acos(double x)
{
    (void)x;
    return 0;
}

double atan2(double y, double x)
{
    (void)y;
    (void)x;
    return 0;
}

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "util.h"

enum {
	BLACK    = 30,
	RED      = 31,
	GREEN    = 32,
	YELLOW   = 33,
	BLUE     = 34,
	MAGENTA  = 35,
	CYAN     = 36,
	WHITE    = 37,
	DEFAULT  = 39
};

const char *lvl_strings[] = { "log", "warning", "error" };
const int lvl_colors[] = { CYAN, MAGENTA, RED };

char *
nstrchr(const char *s, int c, int n)
{
	if (n == 0)
		return NULL;

	do {
		if (n == 0)
			return (char *)s - 1;
		if (*s == c)
			n--;
	} while (*s++);
	return NULL;
}

size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	const char *p = src;

	if (siz == 0) /* duh */
		return strlen(src);

	while (--siz != 0 && (*dst++ = *p++) != '\0');
	if (siz == 0) {
		*dst = '\0';
		while (*p++);
	}
	return p - src - 1;
}

size_t
strlcat(char *dst, const char *src, size_t siz)
{
	size_t dsiz = strlen(dst);
	if (dsiz >= siz)
		return strlen(src) + siz;
	return dsiz + strlcpy(dst + dsiz, src, siz - dsiz);
}

int
file_exists(const char *filename)
{
	struct stat buf;
	return (stat(filename, &buf) == 0);
}

time_t
ltime(void)
{
	static long tz;

	if (!tz) {
		const time_t t = time(NULL);
		const struct tm *tm = localtime(&t);
		tz = tm->tm_gmtoff;
	}

	return time(NULL) + tz;
}

void
__log(int lvl, char *file, int line, const char *func, char *fmt, ...)
{
	va_list ap;
	time_t t = ltime();

	fprintf(stderr,
	        "\033[1m%02ld:%02ld:%02ld \033[%dm%-7s\033[39m %s:%d: %s: ",
	        (t / (60 * 60)) % 24,
	        (t / 60) % 60,
	        t % 60,
	        lvl_colors[lvl],
	        lvl_strings[lvl],
	        file,
	        line,
	        func);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt == NULL) {
		perror(NULL);
	} else if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}
	fprintf(stderr, "\033[m");
}

void *
emalloc(size_t size)
{
	void *p;

	if ((p = malloc(size)) == NULL)
		DIE(NULL);
	return p;
}

FILE *
efopen(const char *filename, const char *mode)
{
	FILE *fp;

	if ((fp = fopen(filename, mode)) == NULL)
		DIE("failed to open %s (%s):", filename, mode);
	return fp;
}

size_t
uintfmt(char *dst, size_t dsiz, unsigned int num)
{
	char buf[16], *p = buf + sizeof(buf) - 1;
	size_t sizn = 1;

	*p-- = '\0';
	while (sizn < sizeof(buf)) {
		*p-- = (num % 10) + '0';
		if ((num /= 10) == 0)
			break;
		if ((sizn % 3) == 0)
			*p-- = ',';
		sizn++;
	}

	return strlcpy(dst, ++p, dsiz);
}

size_t
longfmt(char *dst, size_t dsiz, const char *opt, long num)
{
	char buf[32], *p = buf + sizeof(buf) - 1;
	size_t sizn = 1;
	int sign = 0, fmt = 0;

	do {
		switch (*opt) {
		case '\'':
			fmt = 1;
			break;
		case '+':
			sign = '+';
			break;
		case ' ':
			sign = ' ';
			break;
		}
	} while (*opt++);

	*p-- = '\0';
	if (num < 0) {
		sign = '-';
		num *= -1;
	}

	while (sizn < sizeof(buf)) {
		*p-- = (num % 10) + '0';
		if ((num /= 10) == 0)
			break;
		if (fmt && (sizn % 3) == 0)
			*p-- = ',';
		sizn++;
	}

	if (sign && p - buf > 0)
		*p-- = sign;

	return strlcpy(dst, ++p, dsiz);
}

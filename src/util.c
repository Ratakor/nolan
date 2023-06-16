/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <stdarg.h>
#include <stdint.h>
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
nstrchr(const char *s, int c, size_t n)
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
	const char *osrc = src;

	if (siz == 0) /* duh */
		return strlen(src);

	while (--siz != 0 && (*dst++ = *src++) != '\0');
	if (siz == 0) {
		*dst = '\0';
		while (*src++);
	}
	return src - osrc - 1;
}

size_t
strlcat(char *dst, const char *src, size_t siz)
{
	size_t dsiz = strlen(dst);
	if (dsiz >= siz)
		return strlen(src) + siz;
	return dsiz + strlcpy(dst + dsiz, src, siz - dsiz);
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

	if (!fmt[0]) {
		perror(NULL);
	} else if (fmt[strlen(fmt) - 1] == ':') {
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
		DIE("");
	return p;
}

FILE *
efopen(const char *filename, const char *mode)
{
	FILE *fp;

	if ((fp = fopen(filename, mode)) == NULL)
		DIE("%s [%s]:", filename, mode);
	return fp;
}

size_t
ufmt(char *dst, size_t dsiz, uint64_t n)
{
	char buf[32], *p = buf + sizeof(buf) - 1;
	size_t sizn = 1;

	*p-- = '\0';
	while (sizn < sizeof(buf)) {
		*p-- = (n % 10) + '0';
		if ((n /= 10) == 0)
			break;
		if ((sizn % 3) == 0)
			*p-- = ',';
		sizn++;
	}

	return strlcpy(dst, ++p, dsiz);
}

size_t
ifmt(char *dst, size_t dsiz, int64_t n)
{
	if (dsiz == 0)
		return 0;

	if (n < 0) {
		*dst = '-';
		n = -n;
	} else {
		*dst = '+';
	}

	return ufmt(dst + 1, dsiz - 1, n);

}

int
file_exists(const char *filename)
{
	struct stat buf;
	return (stat(filename, &buf) == 0);
}

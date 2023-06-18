/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <sys/stat.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "util.h"

static void evprintf(const char *fmt, va_list ap);

char *argv0;

void
evprintf(const char *fmt, va_list ap)
{
	if (!fmt) {
		fputc('\n', stderr);
		return;
	}

	if (argv0 && strncmp(fmt, "usage", STRLEN("usage")))
		fprintf(stderr, "%s: ", argv0);

	vfprintf(stderr, fmt, ap);

	if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}
}

void
warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	evprintf(fmt, ap);
	va_end(ap);
}

void
die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	evprintf(fmt, ap);
	va_end(ap);

	exit(EXIT_FAILURE);
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
wstrlcpy(char *dst, const char *src, size_t siz)
{
	size_t ret;

	if ((ret = strlcpy(dst, src, siz)) >= siz)
		warn("strlcpy: string truncation");

	return ret;
}

size_t
estrlcpy(char *dst, const char *src, size_t siz)
{
	size_t ret;

	if ((ret = strlcpy(dst, src, siz)) >= siz)
		die("strlcpy: string truncation");

	return ret;
}

size_t
strlcat(char *dst, const char *src, size_t siz)
{
	size_t dsiz = strlen(dst);

	if (dsiz >= siz)
		return strlen(src) + siz;

	return dsiz + strlcpy(dst + dsiz, src, siz - dsiz);
}

size_t
wstrlcat(char *dst, const char *src, size_t siz)
{
	size_t ret;

	if ((ret = strlcat(dst, src, siz)) >= siz)
		warn("strlcat: string truncation");

	return ret;
}

size_t
estrlcat(char *dst, const char *src, size_t siz)
{
	size_t ret;

	if ((ret = strlcat(dst, src, siz)) >= siz)
		die("strlcat: string truncation");

	return ret;
}

void *
emalloc(size_t size)
{
	void *p;

	if ((p = malloc(size)) == NULL)
		die("malloc:");

	return p;
}

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if ((p = calloc(nmemb, size)) == NULL)
		die("calloc:");

	return p;
}

void *
erealloc(void *p, size_t size)
{
	if ((p = realloc(p, size)) == NULL)
		die("realloc:");

	return p;
}

void *
estrdup(const char *s)
{
	char *p;

	if ((p = strdup(s)) == NULL)
		die("strdup:");

	return p;
}

void *
estrndup(const char *s, size_t n)
{
	char *p;

	if ((p = strndup(s, n)) == NULL)
		die("strndup:");

	return p;
}

FILE *
efopen(const char *filename, const char *mode)
{
	FILE *fp;

	if ((fp = fopen(filename, mode)) == NULL)
		die("fopen %s [%s]:", filename, mode);

	return fp;
}

FILE *
efreopen(const char *filename, const char *mode, FILE *stream)
{
	FILE *fp;

	if ((fp = freopen(filename, mode, stream)) == NULL)
		die("freopen %s [%s]:", filename, mode);

	return fp;
}

size_t
ufmt(char *dst, size_t dsiz, unsigned long long n)
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
ifmt(char *dst, size_t dsiz, long long n)
{
	if (dsiz == 0)
		return 0;

	if (n < 0) {
		*dst = '-';
		n = -n;
	} else {
		*dst = '+';
	}

	return ufmt(dst + 1, dsiz - 1, n) + 1;

}

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

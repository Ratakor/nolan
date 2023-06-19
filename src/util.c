/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <sys/stat.h>

#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "util.h"

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
		warnx("strlcpy: String truncation");

	return ret;
}

size_t
estrlcpy(char *dst, const char *src, size_t siz)
{
	size_t ret;

	if ((ret = strlcpy(dst, src, siz)) >= siz)
		errx(1, "strlcpy: String truncation");

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
		warnx("strlcat: String truncation");

	return ret;
}

size_t
estrlcat(char *dst, const char *src, size_t siz)
{
	size_t ret;

	if ((ret = strlcat(dst, src, siz)) >= siz)
		errx(1, "strlcat: String truncation");

	return ret;
}

void *
emalloc(size_t siz)
{
	void *p;

	if ((p = malloc(siz)) == NULL)
		err(1, "malloc");

	return p;
}

void *
ecalloc(size_t nmemb, size_t siz)
{
	void *p;

	if ((p = calloc(nmemb, siz)) == NULL)
		err(1, "calloc");

	return p;
}

void *
erealloc(void *p, size_t siz)
{
	if ((p = realloc(p, siz)) == NULL)
		err(1, "realloc");

	return p;
}

void *
estrdup(const char *s)
{
	char *p;

	if ((p = strdup(s)) == NULL)
		err(1, "strdup");

	return p;
}

void *
estrndup(const char *s, size_t n)
{
	char *p;

	if ((p = strndup(s, n)) == NULL)
		err(1, "strndup");

	return p;
}

FILE *
efopen(const char *filename, const char *mode)
{
	FILE *fp;

	if ((fp = fopen(filename, mode)) == NULL)
		err(1, "fopen: '%s' [%s]", filename, mode);

	return fp;
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

	return ufmt(dst + 1, dsiz - 1, n) + 1;

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
	static time_t tz;

	if (!tz) {
		const time_t t = time(NULL);
		const struct tm *tm = localtime(&t);
		tz = tm->tm_gmtoff;
	}

	return time(NULL) + tz;
}

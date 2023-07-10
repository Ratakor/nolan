/* Copyright © 2023 Ratakor. See LICENSE file for license details. */

#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ubik.h"

size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	size_t slen, len;

	slen = strlen(src);
	if (siz != 0) {
		len = MIN(slen, siz - 1);
		memcpy(dst, src, len);
		dst[len] = '\0';
	}

	return slen;
}

size_t
strlcat(char *dst, const char *src, size_t siz)
{
	size_t dlen;

	dlen = strlen(dst);
	if (dlen >= siz)
		return strlen(src) + siz;

	return dlen + strlcpy(dst + dlen, src, siz - dlen);
}

char *
nstrchr(const char *s, int c, size_t n)
{
	char *p;

	if (n == 0 || s == NULL)
		return NULL;

	p = strchr(s, c);
	while (--n != 0 && (p = strchr(p + 1, c)) != NULL);

	return p;
}

size_t
ufmt(char *dst, size_t dsiz, uintmax_t n)
{
	char buf[32], *p;
	size_t sizn = 1;

	p = buf + sizeof(buf) - 1;
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
ifmt(char *dst, size_t dsiz, intmax_t n)
{
	if (dsiz == 0)
		return ufmt(dst, dsiz, n);

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
	static int once;

	if (!once) {
		const time_t t = time(NULL);
		const struct tm *tm = localtime(&t);
		time_t tz_hour, tz_min;

		tz_hour = tm->tm_hour - HOUR(t);
		tz_min = tm->tm_min - MINUTE(t);
		tz = ((tz_hour * 60) + tz_min) * 60;
		once = 1;
	}

	return time(NULL) + tz;
}

size_t
xstrlcpy(char *dst, const char *src, size_t siz)
{
	size_t rv;

	if ((rv = strlcpy(dst, src, siz)) >= siz)
		errx(1, "strlcpy: String truncation");

	return rv;
}

size_t
xstrlcat(char *dst, const char *src, size_t siz)
{
	size_t rv;

	if ((rv = strlcat(dst, src, siz)) >= siz)
		errx(1, "strlcat: String truncation");

	return rv;
}

FILE *
xfopen(const char *filename, const char *mode)
{
	FILE *fp;

	if ((fp = fopen(filename, mode)) == NULL)
		err(1, "fopen: '%s' [%s]", filename, mode);

	return fp;
}

void *
xmalloc(size_t siz)
{
	void *p;

	if ((p = malloc(siz)) == NULL)
		err(1, "malloc");

	return p;
}

void *
xcalloc(size_t nmemb, size_t siz)
{
	void *p;

	if ((p = calloc(nmemb, siz)) == NULL)
		err(1, "calloc");

	return p;
}

void *
xrealloc(void *p, size_t siz)
{
	/* siz == 0 can mean free */
	if ((p = realloc(p, siz)) == NULL && siz != 0)
		err(1, "realloc");

	return p;
}

void *
xreallocarray(void *p, size_t nmemb, size_t siz)
{
	if (siz && nmemb > -1 / siz)
		errx(1, "reallocarray: %s", strerror(ENOMEM));

	return xrealloc(p, nmemb * siz);
}

void *
xstrdup(const char *s)
{
	char *p;

	if ((p = strdup(s)) == NULL)
		err(1, "strdup");

	return p;
}

void *
xstrndup(const char *s, size_t n)
{
	char *p;

	if ((p = strndup(s, n)) == NULL)
		err(1, "strndup");

	return p;
}

int
xvasprintf(char **strp, const char *fmt, va_list ap)
{
	va_list ap2;
	size_t siz;
	int rv;

	va_copy(ap2, ap);
	rv = vsnprintf(NULL, 0, fmt, ap2);
	va_end(ap2);
	if (rv < 0)
		err(1, "snprintf");
	siz = rv + 1;
	*strp = xmalloc(siz);
	rv = vsnprintf(*strp, siz, fmt, ap);
	if (rv < 0)
		err(1, "snprintf");

	return rv;
}

int
xasprintf(char **strp, const char *fmt, ...)
{
	va_list ap;
	int rv;

	va_start(ap, fmt);
	rv = xvasprintf(strp, fmt, ap);
	va_end(ap);

	return rv;
}

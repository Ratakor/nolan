/* Copyright © 2023 Ratakor. See LICENSE file for license details. */

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ubik.h"

extern char *__progname;

static void
vwarn(const char *fmt, va_list ap)
{
	if (__progname)
		fprintf(stderr, "%s: ", __progname);

	if (fmt == NULL) {
		perror(NULL);
		return;
	}

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
	vwarn(fmt, ap);
	va_end(ap);
}

void
die(int status, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vwarn(fmt, ap);
	va_end(ap);

	exit(status);
}

void *
memdup(const void *src, size_t siz)
{
	void *p;

	if ((p = malloc(siz)) == NULL)
		return NULL;

	return memcpy(p, src, siz);
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
	char buf[sizeof(n) * 4], *p;
	size_t sizn = 1;

	p = buf + sizeof(buf) - 1;
	*p-- = '\0';
	for (;;) {
		*p-- = (n % 10) + '0';
		if ((n /= 10) == 0)
			break;
		if ((sizn % 3) == 0)
			*p-- = ',';
		sizn++;
	}

	return strlcpy(dst, p + 1, dsiz);
}

size_t
ifmt(char *dst, size_t dsiz, intmax_t n)
{
	if (dsiz == 0) {
		if (n < 0)
			n = -n;
		return ufmt(NULL, 0, (uintmax_t)n) + 1;
	}

	if (n < 0) {
		*dst = '-';
		n = -n;
	} else {
		*dst = '+';
	}

	return ufmt(dst + 1, dsiz - 1, (uintmax_t)n) + 1;
}

time_t
ltime(void)
{
	static time_t tz;
	static bool once;

	if (!once) {
		const time_t t = time(NULL);
		const struct tm *tm = localtime(&t);
		time_t tz_hour, tz_min;

		tz_hour = tm->tm_hour - HOUR(t);
		tz_min = tm->tm_min - MINUTE(t);
		tz = ((tz_hour * 60) + tz_min) * 60;
		once = true;
	}

	return time(NULL) + tz;
}

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

void *
reallocarray(void *p, size_t nmemb, size_t siz)
{
	if (siz && nmemb > SIZE_MAX / siz) {
		errno = ENOMEM;
		return NULL;
	}

	return realloc(p, nmemb * siz);
}

char *
strdup(const char *s)
{
	return memdup(s, strlen(s) + 1);
}

char *
strndup(const char *s, size_t n)
{
	char *p;
	size_t siz;

	siz = MIN(strlen(s), n);
	if ((p = malloc(siz + 1)) == NULL)
		return NULL;
	memcpy(p, s, siz);
	p[siz] = '\0';

	return p;
}

int
vasprintf(char **strp, const char *fmt, va_list ap)
{
	va_list ap2;
	int rv;

	va_copy(ap2, ap);
	rv = vsnprintf(NULL, 0, fmt, ap2);
	va_end(ap2);

	if (rv < 0 || (*strp = malloc((size_t)rv + 1)) == NULL)
		return -1;

	return vsnprintf(*strp, (size_t)rv + 1, fmt, ap);
}

int
asprintf(char **strp, const char *fmt, ...)
{
	va_list ap;
	int rv;

	va_start(ap, fmt);
	rv = vasprintf(strp, fmt, ap);
	va_end(ap);

	return rv;
}

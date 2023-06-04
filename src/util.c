#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "util.h"

void
warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void
die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	exit(EXIT_FAILURE);
}

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
file_exists(char *filename)
{
	struct stat buf;
	return (stat(filename, &buf) == 0);
}

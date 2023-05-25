#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "util.h"

void
die(const char *errstr, ...)
{
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

char *
nstrchr(const char *s, int c, int n)
{
	if (n == 0)
		return (char *)s;

	do {
		if (n == 0)
			return (char *)s - 1;
		if (*s == c)
			n--;
	} while (*s++);
	return NULL;
}

#include <string.h>
#include <sys/stat.h>

#include "util.h"

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

void *
emalloc(size_t size)
{
	void *p;

	if ((p = malloc(size)) == NULL)
		DIE("malloc failed");
	return p;
}

FILE *
efopen(const char *filename, const char *mode)
{
	FILE *fp;

	if ((fp = fopen(filename, mode)) == NULL)
		DIE("failed to open %s (%s)", filename, mode);
	return fp;
}

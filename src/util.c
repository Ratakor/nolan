#include <string.h>
#include <sys/stat.h>
#include <time.h>

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

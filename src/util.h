#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LENGTH(X)      (sizeof(X) / sizeof(X[0]))
#define MAX(X, Y)      ((X) > (Y) ? (X) : (Y))
#define MIN(X, Y)      ((X) < (Y) ? (X) : (Y))
#define UNUSED(X)      (void)(X)
#define TIMEZONE        2
#define __EPRINTF(...) ((fprintf(stderr,\
				"\033[1m%s:%d: %s: %02ldh%02ld ",\
				__FILE_NAME__,\
				__LINE__,\
				__func__,\
				(time(NULL) / (60 * 60)) % 24 + TIMEZONE,\
				(time(NULL) / 60) % 60)),\
				(fprintf(stderr, __VA_ARGS__)),\
				(fprintf(stderr, "\033[m\n")))
#define LOG(...)       (__EPRINTF("\033[36;1mLOG:\033[39;1m " __VA_ARGS__))
#define WARN(...)      (__EPRINTF("\033[35;1mwarning:\033[39;1m " __VA_ARGS__))
#define DIE(...)       (__EPRINTF("\033[31;1merror:\033[39;1m " __VA_ARGS__),\
			exit(EXIT_FAILURE))

char *nstrchr(const char *s, int c, int n);
size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);
int file_exists(const char *filename);
void *emalloc(size_t size);
FILE *efopen(const char *filename, const char *mode);

#endif /* UTIL_H */

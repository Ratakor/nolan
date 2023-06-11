#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>

#define LENGTH(X)      (sizeof(X) / sizeof(X[0]))
#define MAX(X, Y)      ((X) > (Y) ? (X) : (Y))
#define MIN(X, Y)      ((X) < (Y) ? (X) : (Y))
#define __EPRINTF(...) ((fprintf(stderr,\
				"\033[1m%s:%d: %s: ",\
				__FILE__,\
				__LINE__,\
				__func__)),\
				(fprintf(stderr, __VA_ARGS__)),\
				(fprintf(stderr, "\033[m\n")))
#define WARN(...)      (__EPRINTF("\033[35;1mwarning:\033[39;1m " __VA_ARGS__))
#define DIE(...)       (__EPRINTF("\033[31;1merror:\033[39;1m " __VA_ARGS__),\
			exit(EXIT_FAILURE))

char *nstrchr(const char *s, int c, int n);
size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);
int file_exists(const char *filename);
void *emalloc(size_t size);
FILE *efopen(const char *filename, const char *modes);

#endif /* UTIL_H */

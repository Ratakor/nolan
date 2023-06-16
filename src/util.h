#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>

enum { LOG__, WARN__, DIE__ };

#define LENGTH(X)       (sizeof(X) / sizeof(X[0]))
#define MAX(X, Y)       ((X) > (Y) ? (X) : (Y))
#define MIN(X, Y)       ((X) < (Y) ? (X) : (Y))
#define UNUSED(X)       ((void)(X))
#define __LOG(LVL, ...)                                                       \
	(__log(LVL, __FILE_NAME__, __LINE__, __func__, __VA_ARGS__))
#define LOG(...)        (__LOG(LOG__, __VA_ARGS__))
#define WARN(...)       (__LOG(WARN__, __VA_ARGS__))
#define DIE(...)        (__LOG(DIE__, __VA_ARGS__), exit(EXIT_FAILURE))

char *nstrchr(const char *s, int c, int n);
size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);
int file_exists(const char *filename);
time_t ltime(void);
void __log(int lvl, char *file, int line, const char *func, char *fmt, ...);
void *emalloc(size_t size);
FILE *efopen(const char *filename, const char *mode);
size_t uintfmt(char *dst, size_t dsiz, unsigned int num);
size_t longfmt(char *dst, size_t dsiz, const char *opt, long num);

#endif /* UTIL_H */

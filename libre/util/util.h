/* Copyright © 2023 Ratakor. See LICENSE file for license details. */

#ifndef _UTIL_H
#define _UTIL_H

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>

#define LENGTH(X)   (sizeof(X) / sizeof(X[0]))
#define STRLEN(X)   (sizeof(X) - 1)
#define MAX(X, Y)   ((X) > (Y) ? (X) : (Y))
#define MIN(X, Y)   ((X) < (Y) ? (X) : (Y))
#define UNUSED(X)   ((void)(X))
#define LIKELY(X)   __builtin_expect(!!(X), 1)
#define UNLIKELY(X) __builtin_expect(!!(X), 0)

size_t strlcpy(char *restrict dst, const char *restrict src, size_t siz);
size_t wstrlcpy(char *dst, const char *src, size_t siz);
size_t estrlcpy(char *dst, const char *src, size_t siz);

size_t strlcat(char *restrict dst, const char *restrict src, size_t siz);
size_t wstrlcat(char *dst, const char *src, size_t siz);
size_t estrlcat(char *dst, const char *src, size_t siz);

void *emalloc(size_t siz);
void *ecalloc(size_t nmemb, size_t siz);
void *erealloc(void *p, size_t siz);
void *ereallocarray(void *p, size_t nmemb, size_t siz);
void *estrdup(const char *s);
void *estrndup(const char *s, size_t n);
FILE *efopen(const char *filename, const char *mode);

char *nstrchr(const char *s, int c, size_t n);
size_t ufmt(char *dst, size_t dsiz, uint64_t n);
size_t ifmt(char *dst, size_t dsiz, int64_t n);
int file_exists(const char *filename);
time_t ltime(void);

#endif /* _UTIL_H */

/* Copyright © 2023 Ratakor. See LICENSE file for license details. */

#ifndef LIBRE_UBIK_H
#define LIBRE_UBIK_H

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>

#define LENGTH(X)   (sizeof(X) / sizeof(X[0]))
#define STRLEN(X)   (sizeof(X) - 1)
#define MAX(X, Y)   ((X) > (Y) ? (X) : (Y))
#define MIN(X, Y)   ((X) < (Y) ? (X) : (Y))
#define UNUSED(X)   ((void)(X))
#define HOUR(t)     ((t / 3600) % 24)
#define MINUTE(t)   ((t / 60) % 60)
#define SECONDE(t)  (t % 60)

size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);
char *nstrchr(const char *s, int c, size_t n);
size_t ufmt(char *dst, size_t dsiz, uintmax_t n);
size_t ifmt(char *dst, size_t dsiz, intmax_t n);
int file_exists(const char *filename);
time_t ltime(void);

size_t xstrlcpy(char *dst, const char *src, size_t siz);
size_t xstrlcat(char *dst, const char *src, size_t siz);
FILE *xfopen(const char *filename, const char *mode);
void *xmalloc(size_t siz);
void *xcalloc(size_t nmemb, size_t siz);
void *xrealloc(void *p, size_t siz);
void *xreallocarray(void *p, size_t nmemb, size_t siz);
void *xstrdup(const char *s);
void *xstrndup(const char *s, size_t n);
int xvasprintf(char **strp, const char *fmt, va_list ap);
int xasprintf(char **strp, const char *fmt, ...);

#endif /* LIBRE_UBIK_H */

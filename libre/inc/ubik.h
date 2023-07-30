/* Copyright © 2023 Ratakor. See LICENSE file for license details. */

#ifndef LIBRE_UBIK_H
#define LIBRE_UBIK_H

#include <sys/types.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define LENGTH(X)   (sizeof(X) / sizeof(X[0]))
#define STRLEN(X)   (sizeof(X) - 1)
#define MAX(X, Y)   ((X) > (Y) ? (X) : (Y))
#define MIN(X, Y)   ((X) < (Y) ? (X) : (Y))
#define UNUSED(X)   ((void)(X))
#define HOUR(t)     ((t / 3600) % 24)
#define MINUTE(t)   ((t / 60) % 60)
#define SECONDE(t)  (t % 60)
#define btoa(X)     (!!(X) ? "true" : "false")

void warn(const char *fmt, ...);
void die(int status, const char *fmt, ...);
void *memdup(const void *src, size_t siz);
char *nstrchr(const char *s, int c, size_t n);
size_t ufmt(char *dst, size_t dsiz, uintmax_t n);
size_t ifmt(char *dst, size_t dsiz, intmax_t n);
time_t ltime(void);

size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);
void *reallocarray(void *p, size_t nmemb, size_t siz);
char *strdup(const char *s);
char *strndup(const char *s, size_t n);
int vasprintf(char **strp, const char *fmt, va_list ap);
int asprintf(char **strp, const char *fmt, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LIBRE_UBIK_H */

/*
 * Copyright © 2023, Ratakor <ratakor@disroot.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DALLOC
#define NO_DALLOC     "dalloc: Define `DALLOC` to enable dalloc"
#else
void dalloc_check_all(void) __attribute__((destructor));
#undef DALLOC
#endif /* DALLOC */
#include "../libre.h"

#define OVER_ALLOC    64
#define MAGIC_NUMBER  0x99
#define EXIT_STATUS   9
#define MAX_POINTERS  1024
#define COMMENT_MAX   128
#ifndef PATH_MAX
#define PATH_MAX      256
#endif /* PATH_MAX */

struct Pointer {
	void *p;
	size_t siz;
	char comment[COMMENT_MAX];
	int ignored;
	char file[PATH_MAX];
	int line;
};

static int overflow(unsigned char *p, size_t siz);
static size_t find_pointer_index(void *p, char *file, int line);

static cthread_mutex_t dalloc_mutex = CTHREAD_MUTEX_INITIALIZER;
static struct Pointer pointers[MAX_POINTERS];
static size_t npointers;

static int
overflow(unsigned char *p, size_t siz)
{
	size_t i = 0;

	while (p[siz + i] == MAGIC_NUMBER && ++i < OVER_ALLOC);

	return i < OVER_ALLOC;
}

static size_t
find_pointer_index(void *p, char *file, int line)
{
	size_t i = npointers;

	while (i-- > 0 && p != pointers[i].p);

	if (i == (size_t) -1) {
		fprintf(stderr, "%s:%d: dalloc: Unknown pointer %p\n",
		        file, line, p);
		cthread_mutex_unlock(&dalloc_mutex);
		exit(EXIT_STATUS);
	}

	return i;
}

size_t
dalloc_check_overflow(void)
{
	size_t i, sum = 0;

#ifdef NO_DALLOC
	fprintf(stderr, "%s\n", NO_DALLOC);
	return 0;
#endif /* NO_DALLOC */

	cthread_mutex_lock(&dalloc_mutex);
	fprintf(stderr, "Memory overflow:");
	for (i = 0; i < npointers; i++) {
		if (!overflow(pointers[i].p, pointers[i].siz))
			continue;

		sum++;
		fprintf(stderr, "\n%s:%d: %p, total: %zu bytes",
		        pointers[i].file, pointers[i].line,
		        pointers[i].p, pointers[i].siz);
		if (pointers[i].comment[0])
			fprintf(stderr, " /* %s */", pointers[i].comment);
	}
	cthread_mutex_unlock(&dalloc_mutex);

	if (sum == 0)
		fprintf(stderr, " 0 overflow :)\n");
	else
		fprintf(stderr, "\nTotal overflow: %zu\n", sum);

	return sum;
}

void
dalloc_check_free(void)
{
	size_t i, sum = 0;

#ifdef NO_DALLOC
	fprintf(stderr, "%s\n", NO_DALLOC);
	return;
#endif /* NO_DALLOC */

	cthread_mutex_lock(&dalloc_mutex);
	fprintf(stderr, "Memory allocated and not freed:");
	for (i = 0; i < npointers; i++) {
		if (pointers[i].ignored)
			continue;

		sum += pointers[i].siz;
		fprintf(stderr, "\n%s:%d: %p, %zu bytes",
		        pointers[i].file, pointers[i].line,
		        pointers[i].p, pointers[i].siz);
		if (pointers[i].comment[0])
			fprintf(stderr, " /* %s */", pointers[i].comment);
	}
	cthread_mutex_unlock(&dalloc_mutex);

	if (sum == 0)
		fprintf(stderr, " 0 byte :)\n");
	else
		fprintf(stderr, "\nTotal: %zu bytes, %zu pointers\n", sum, i);
}

void
dalloc_check_all(void)
{
#ifdef NO_DALLOC
	fprintf(stderr, "%s\n", NO_DALLOC);
	return;
#endif /* NO_DALLOC */

	dalloc_check_overflow();
	dalloc_check_free();
}

void
dalloc_sighandler(int sig)
{
#ifdef _WIN32
	fprintf(stderr, "dalloc: signal %d\n", sig);
#else
	fprintf(stderr, "dalloc: %s\n", strsignal(sig));
#endif /* _WIN32 */

	exit(EXIT_STATUS);
}

void
___dalloc_ignore(void *p, char *file, int line)
{
	size_t i;

#ifdef NO_DALLOC
	return;
#endif /* NO_DALLOC */

	cthread_mutex_lock(&dalloc_mutex);
	i = find_pointer_index(p, file, line);
	pointers[i].ignored = 1;
	cthread_mutex_unlock(&dalloc_mutex);
}

void
___dalloc_comment(void *p, char *comment, char *file, int line)
{
	size_t i, j;

#ifdef NO_DALLOC
	return;
#endif /* NO_DALLOC */

	cthread_mutex_lock(&dalloc_mutex);
	i = find_pointer_index(p, file, line);
	for (j = 0; j < COMMENT_MAX && comment[j] != '\0'; j++)
		pointers[i].comment[j] = comment[j];
	pointers[i].comment[j] = '\0';
	cthread_mutex_unlock(&dalloc_mutex);
}

void
___free(void *p, char *file, int line)
{
	size_t i;

	if (p == NULL)
		return;

	cthread_mutex_lock(&dalloc_mutex);
	i = find_pointer_index(p, file, line);
	if (overflow(pointers[i].p, pointers[i].siz)) {
		fprintf(stderr, "%s:%d: dalloc: "
		        "Memory overflow on %p, total: %zu bytes\n",
		        file, line, pointers[i].p, pointers[i].siz);
		fprintf(stderr, "The pointer ");
		if (pointers[i].comment[0])
			fprintf(stderr, "'%s' ", pointers[i].comment);
		fprintf(stderr, "was allocated in '%s' on line %d.\n",
		        pointers[i].file, pointers[i].line);
		cthread_mutex_unlock(&dalloc_mutex);
		exit(EXIT_STATUS);
	}

	for (i++; i < npointers; i++)
		pointers[i - 1] = pointers[i];
	npointers--;
	free(p);
	cthread_mutex_unlock(&dalloc_mutex);
}


void *
___malloc(size_t siz, char *file, int line)
{
	void *p = NULL;
	size_t i;

	if (siz == 0)
		return NULL;

	if (npointers == MAX_POINTERS) {
		fprintf(stderr, "dalloc: Too much pointers (max:%d)\n",
		        MAX_POINTERS);
		exit(EXIT_STATUS);
	}

	if (siz + OVER_ALLOC < OVER_ALLOC)
		errno = ENOMEM;
	else
		p = malloc(siz + OVER_ALLOC);

	if (p == NULL) {
		fprintf(stderr, "%s:%d: dalloc: %s\n",
		        file, line, strerror(errno));
		exit(EXIT_STATUS);
	}

	memset((unsigned char *)p + siz, MAGIC_NUMBER, OVER_ALLOC);
	cthread_mutex_lock(&dalloc_mutex);
	pointers[npointers].p = p;
	pointers[npointers].siz = siz;
	for (i = 0; i < PATH_MAX - 1 && file[i] != '\0'; i++)
		pointers[npointers].file[i] = file[i];
	pointers[npointers].file[i] = '\0';
	pointers[npointers].line = line;
	npointers++;
	cthread_mutex_unlock(&dalloc_mutex);

	return p;
}

void *
___calloc(size_t nmemb, size_t siz, char *file, int line)
{
	void *p;

	if (nmemb == 0 || siz == 0)
		return NULL;

	if (nmemb > -1 / siz) {
		fprintf(stderr, "%s:%d: dalloc: calloc: %s\n",
		        file, line, strerror(ENOMEM));
		exit(EXIT_STATUS);
	}

	siz *= nmemb;
	p = ___malloc(siz, file, line);
	memset(p, 0, siz);

	return p;
}

void *
___realloc(void *p, size_t siz, char *file, int line)
{
	size_t i, j;

	if (p == NULL)
		return ___malloc(siz, file, line);

	if (siz == 0) {
		___free(p, file, line);
		return NULL;
	}

	cthread_mutex_lock(&dalloc_mutex);
	i = find_pointer_index(p, file, line);

	if (overflow(pointers[i].p, pointers[i].siz)) {
		fprintf(stderr, "%s:%d: dalloc: "
		        "Memory overflow on %p, total: %zu bytes\n",
		        file, line, pointers[i].p, pointers[i].siz);
		fprintf(stderr, "The pointer ");
		if (pointers[i].comment[0])
			fprintf(stderr, "'%s' ", pointers[i].comment);
		fprintf(stderr, "was allocated in '%s' on line %d.\n",
		        pointers[i].file, pointers[i].line);
		cthread_mutex_unlock(&dalloc_mutex);
		exit(EXIT_STATUS);
	}

	if (siz + OVER_ALLOC < OVER_ALLOC) {
		p = NULL;
		errno = ENOMEM;
	} else {
		p = realloc(p, siz + OVER_ALLOC);
	}

	if (p == NULL) {
		fprintf(stderr, "%s:%d: dalloc: %s\n",
		        file, line, strerror(errno));
		cthread_mutex_unlock(&dalloc_mutex);
		exit(EXIT_STATUS);
	}

	memset((unsigned char *)p + siz, MAGIC_NUMBER, OVER_ALLOC);
	pointers[i].p = p;
	pointers[i].siz = siz;
	for (j = 0; j < PATH_MAX - 1 && file[j] != '\0'; j++)
		pointers[i].file[j] = file[j];
	pointers[i].file[j] = '\0';
	pointers[i].line = line;
	cthread_mutex_unlock(&dalloc_mutex);

	return p;
}

void *
___reallocarray(void *p, size_t nmemb, size_t siz, char *file, int line)
{
	if (siz != 0 && nmemb > -1 / siz) {
		fprintf(stderr, "%s:%d: dalloc: reallocarray: %s\n",
		        file, line, strerror(ENOMEM));
		exit(EXIT_STATUS);
	}

	return ___realloc(p, nmemb * siz, file, line);
}

char *
___strdup(const char *s, char *file, int line)
{
	char *p;
	size_t siz;

	siz = strlen(s) + 1;
	p = ___malloc(siz, file, line);
	memcpy(p, s, siz);

	return p;
}

char *
___strndup(const char *s, size_t n, char *file, int line)
{
	const char *end;
	char *p;
	size_t siz;

	end = memchr(s, '\0', n);
	siz = (end ? (size_t)(end - s) : n) + 1;
	p = ___malloc(siz, file, line);
	memcpy(p, s, siz - 1);
	p[siz - 1] = '\0';

	return p;
}

void
exitsegv(int dummy)
{
	int *x = NULL;

	dalloc_check_all();
	*x = dummy;
}

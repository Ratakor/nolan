/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "nolan.h"

char *progname;

static void
vwarn(const char *fmt, va_list ap)
{
	fprintf(stderr, "%s: ", progname);

	if (fmt == NULL) {
		perror(NULL);
		return;
	}

	vfprintf(stderr, fmt, ap);
	if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}
}

void
warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vwarn(fmt, ap);
	va_end(ap);
}

void
die(int status, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vwarn(fmt, ap);
	va_end(ap);

	exit(status);
}

char *
nstrchr(const char *s, int c, size_t n)
{
	char *p;

	if (n == 0 || s == NULL)
		return NULL;

	p = strchr(s, c);
	while (--n != 0 && (p = strchr(p + 1, c)) != NULL);

	return p;
}

size_t
ufmt(char *dst, size_t dsiz, uintmax_t n)
{
	char buf[sizeof(n) * 4], *p;
	size_t sizn = 1;

	p = buf + sizeof(buf) - 1;
	*p-- = '\0';
	for (;;) {
		*p-- = (n % 10) + '0';
		if ((n /= 10) == 0)
			break;
		if ((sizn % 3) == 0)
			*p-- = ',';
		sizn++;
	}

	return strlcpy(dst, p + 1, dsiz);
}

size_t
ifmt(char *dst, size_t dsiz, intmax_t n)
{
	if (dsiz == 0) {
		if (n < 0)
			n = -n;
		return ufmt(NULL, 0, (uintmax_t)n) + 1;
	}

	if (n < 0) {
		*dst = '-';
		n = -n;
	} else {
		*dst = '+';
	}

	return ufmt(dst + 1, dsiz - 1, (uintmax_t)n) + 1;
}

size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	size_t slen, len;

	slen = strlen(src);
	if (siz != 0) {
		len = MIN(slen, siz - 1);
		memcpy(dst, src, len);
		dst[len] = '\0';
	}

	return slen;
}

size_t
strlcat(char *dst, const char *src, size_t siz)
{
	size_t dlen;

	dlen = strlen(dst);
	if (dlen >= siz)
		return strlen(src) + siz;

	return dlen + strlcpy(dst + dlen, src, siz - dlen);
}

#ifndef DALLOC
void *
xmalloc(size_t siz)
{
	void *p;

	if ((p = malloc(siz)) == NULL)
		die(1, "malloc:");

	return p;
}

void *
xcalloc(size_t nmemb, size_t siz)
{
	void *p;

	if ((p = calloc(nmemb, siz)) == NULL)
		die(1, "calloc:");

	return p;
}

void *
xrealloc(void *p, size_t siz)
{
	if ((p = realloc(p, siz)) == NULL)
		die(1, "realloc:");

	return p;
}

char *
xstrdup(const char *s)
{
	char *p;

	if ((p = strdup(s)) == NULL)
		die(1, "strdup:");

	return p;
}
#endif /* !DALLOC */

FILE *
xfopen(const char *filename, const char *mode)
{
	FILE *fp;

	if ((fp = fopen(filename, mode)) == NULL)
		die(1, "fopen: '%s' [%s]:", filename, mode);

	return fp;
}

int
file_exists(const char *filename)
{
	struct stat buf;

	return (stat(filename, &buf) == 0);
}

Player *
find_player(u64snowflake userid)
{
	Player *pp;

	for (pp = player_head; pp && pp->userid != userid; pp = pp->next);

	return pp;
}

void
discord_send_message(struct discord *client, u64snowflake channel_id,
                     const char *fmt, ...)
{
	struct discord_create_message msg = {0};
	char buf[MAX_MESSAGE_LEN] = "";
	va_list ap;
	int rv;

	va_start(ap, fmt);
	rv = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (rv < 0) {
		log_error("%s: %s", __func__, strerror(errno));
		return;
	}

	if ((size_t)rv >= sizeof(buf))
		log_warn("%s: string truncation", __func__);

	msg.content = buf;
	discord_create_message(client, channel_id, &msg, NULL);
}

void
discord_send_interaction_message(struct discord *client, u64snowflake id,
                                 const char *token, const char *fmt, ...)
{
	struct discord_interaction_response msg = {0};
	char buf[MAX_MESSAGE_LEN] = "";
	va_list ap;
	int rv;

	va_start(ap, fmt);
	rv = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (rv < 0) {
		log_error("%s: %s", __func__, strerror(errno));
		return;
	}

	if ((size_t)rv >= sizeof(buf))
		log_warn("%s: string truncation", __func__);

	msg.type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE;
	msg.data = &(struct discord_interaction_callback_data) {
		.content = buf
	};
	discord_create_interaction_response(client, id, token, &msg, NULL);
}

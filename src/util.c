/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <libre/ubik.h>

#include "nolan.h"

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
	char buf[MAX_MESSAGE_LEN];
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

	struct discord_create_message msg = { .content = buf };

	discord_create_message(client, channel_id, &msg, NULL);
}

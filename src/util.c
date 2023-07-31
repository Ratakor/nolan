/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "nolan.h"

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

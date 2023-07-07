/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nolan.h"

static void write_invalid(char *buf, size_t siz);
static int compare(const void *s1, const void *s2);
static void write_lbraid(char *buf, size_t siz, Slayer slayers[], size_t nslayers);
static void lbraid(char *buf, size_t siz);

void
create_slash_lbraid(struct discord *client)
{
	struct discord_create_guild_application_command cmd = {
		.name = "lbraid",
		.description = "Shows the raid leaderboard for the last 7 days",
	};
	discord_create_guild_application_command(client, APP_ID, RAID_GUILD_ID,
	                &cmd, NULL);
}

void
write_invalid(char *buf, size_t siz)
{
	strlcpy(buf, "There is no data for the last 7 days.", siz);
}

int
compare(const void *s1, const void *s2)
{
	const unsigned int dmg1 = ((const Slayer *)s1)->damage;
	const unsigned int dmg2 = ((const Slayer *)s2)->damage;

	return dmg2 - dmg1;
}

void
write_lbraid(char *buf, size_t siz, Slayer slayers[], size_t nslayers)
{
	unsigned int i, lb_max;
	size_t s = 0;

	lb_max = MIN(nslayers, LB_MAX);
	s += snprintf(buf + s, siz - s, "Raid stats for the last 7 days:\n");
	for (i = 0; i < lb_max; i++) {
		if (s >= siz) {
			log_warn("%s: string truncation", __func__);
			return;
		}

		s += snprintf(buf + s, siz - s, "%u. %s: ", i, slayers[i].name);
		s += ufmt(buf + s, siz - s, slayers[i].damage);
		s += strlcpy(buf + s, " damage\n", siz - s);
	}
}

void
lbraid(char *buf, size_t siz)
{
	Slayer *slayers;
	size_t nslayers = 0, i;

	slayers = xcalloc(MAX_SLAYERS, sizeof(*slayers));
	load_files(slayers, &nslayers);
	if (nslayers == 0) {
		write_invalid(buf, siz);
		free(slayers);
		return;
	}
	qsort(slayers, nslayers, sizeof(slayers[0]), compare);
	write_lbraid(buf, siz, slayers, nslayers);
	for (i = 0; i < nslayers; i++)
		free(slayers[i].name);
	free(slayers);
}

void
on_lbraid(struct discord *client, const struct discord_message *event)
{
	char buf[MAX_MESSAGE_LEN];

	if (event->author->bot)
		return;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#else
	if (event->guild_id != RAID_GUILD_ID)
		return;
#endif /* DEVEL */

	log_info("%s", __func__);
	lbraid(buf, sizeof(buf));
	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

void
on_lbraid_interaction(struct discord *client,
                      const struct discord_interaction *event)
{
	char buf[MAX_MESSAGE_LEN];

	log_info("%s", __func__);
	lbraid(buf, sizeof(buf));
	struct discord_interaction_response params = {
		.type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
		.data = &(struct discord_interaction_callback_data)
		{
			.content = buf,
		}
	};
	discord_create_interaction_response(client, event->id, event->token,
	                                    &params, NULL);
}

/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <string.h>

#include "nolan.h"

static void help(char *buf, size_t siz, u64snowflake guild_id);

void
create_slash_help(struct discord *client)
{
	struct discord_create_global_application_command cmd = {
		.name = "help",
		.description = "Shows help",
	};
	discord_create_global_application_command(client, APP_ID, &cmd, NULL);
}

void
help(char *buf, size_t siz, u64snowflake guild_id)
{
	unsigned int i, len = LENGTH(stats_ids);
	size_t s;

	s = strlcpy(buf, "Post a screenshot of your stats to ", siz);
	for (i = 0; i < len - 1; i++) {
		s += snprintf(buf + s, siz - s, "<#%lu> or ", stats_ids[i]);
		if (s >= siz) {
			WARN("string truncation\n\
\033[33mhint:\033[39m this is probably because stats_ids is too big");
			return;
		}
	}
	snprintf(buf + s, siz - s, "<#%lu> ", stats_ids[i]);
	strlcat(buf, "to enter the database.\n", siz);
	strlcat(buf, "Commands:\n", siz);
	strlcat(buf, "\t/stats *screenshot*\n", siz);
	strlcat(buf, "\t/info *[[@]user]*\n", siz);
	strlcat(buf, "\t/leaderboard *category*\n", siz);
	strlcat(buf, "\t/correct *category* *value*\n", siz);
	strlcat(buf, "\t/source *[kingdom]*\n", siz);
	if (guild_id == RAID_GUILD_ID) {
		strlcat(buf, "\t/uraid *username*\n", siz);
		strlcat(buf, "\t/lbraid\n", siz);
	}
	strlcat(buf, "\t/help\n", siz);
	strlcat(buf, "\n[...] means optional.\n", siz);
	strlcat(buf, "Also works with ", siz);
	strlcat(buf, PREFIX, siz);
	s = strlcat(buf, " instead of /.", siz);
	if (s >= siz)
		WARN("string truncation\n\
\033[33mhint:\033[39m this is probably because stats_ids is too big");
}

void
on_help(struct discord * client, const struct discord_message * event)
{
	char buf[MAX_MESSAGE_LEN];

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	LOG("start");
	help(buf, sizeof(buf), event->guild_id);
	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
	LOG("end");
}

void
on_help_interaction(struct discord *client,
                    const struct discord_interaction *event)
{
	char buf[MAX_MESSAGE_LEN];

	LOG("start");
	help(buf, sizeof(buf), event->guild_id);
	struct discord_interaction_response params = {
		.type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
		.data = &(struct discord_interaction_callback_data)
		{
			.content = buf,
		}
	};
	discord_create_interaction_response(client, event->id, event->token,
	                                    &params, NULL);
	LOG("end");
}

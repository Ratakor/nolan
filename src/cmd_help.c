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
			log_warn("%s: string truncation\n\
\033[33mhint:\033[39m this is probably because stats_ids is too big", __func__);
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
	strlcat(buf, "\t/time\n", siz);
	strlcat(buf, "\t/help\n", siz);
	strlcat(buf, "\n[...] means optional.\n", siz);
	strlcat(buf, "Also works with ", siz);
	strlcat(buf, PREFIX, siz);
	s = strlcat(buf, " instead of /.", siz);
	if (s >= siz)
		log_warn("%s: string truncation\n\
\033[33mhint:\033[39m this is probably because stats_ids is too big", __func__);
}

void
on_help(struct discord *client, const struct discord_message *ev)
{
	char buf[MAX_MESSAGE_LEN];

	if (ev->author->bot)
		return;

#ifdef DEVEL
	if (ev->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	log_info("%s", __func__);
	help(buf, sizeof(buf), ev->guild_id);
	discord_send_message(client, ev->channel_id, "%s", buf);
}

void
on_help_interaction(struct discord *client,
                    const struct discord_interaction *ev)
{
	char buf[MAX_MESSAGE_LEN];

	log_info("%s", __func__);
	help(buf, sizeof(buf), ev->guild_id);
	discord_send_interaction_message(client, ev->id, ev->token, "%s", buf);
}

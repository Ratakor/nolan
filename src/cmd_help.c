#include <string.h>

#include "nolan.h"

static void help(char *buf, size_t siz);

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
help(char *buf, size_t siz)
{
	char *p;
	size_t rsiz;
	unsigned int i, len = LENGTH(stats_ids);

	strlcpy(buf, "Post a screenshot of your stats to ", siz);
	for (i = 0; i < len; i++) {
		p = strchr(buf, '\0');;
		rsiz = snprintf(p, siz, "<#%lu> ", stats_ids[i]);
		if (rsiz >= siz) {
			warn("nolan: truncation happened while writing help, \
probably way too much channels for stats\n");
		}
		if (i < len - 1)
			strlcat(buf, "or ", siz);
	}
	strlcat(buf, "to enter the database.\n", siz);
	strlcat(buf, "Commands:\n", siz);
	strlcat(buf, "\t/stats *screenshot*\n", siz);
	strlcat(buf, "\t/info *[[@]user]*\n", siz);
	strlcat(buf, "\t/leaderboard *category*\n", siz);
	/* catstr(buf, "\t/correct [category] [corrected value]\n", siz); */
	strlcat(buf, "\t/source *[kingdom]*\n", siz);
	strlcat(buf, "\t/uraid *username* (only for Scream of Terra)\n", siz);
	strlcat(buf, "\t/lbraid (only for Scream of Terra)\n", siz);
	strlcat(buf, "\t/help\n", siz);
	strlcat(buf, "\n[...] means optional.\n", siz);
	strlcat(buf, "Also works with ", siz);
	strlcat(buf, PREFIX, siz);
	rsiz = strlcat(buf, " instead of /.", siz);
	if (rsiz >= siz)
		warn("nolan: truncation happened while writing help\n");
}

void
on_help(struct discord * client, const struct discord_message * event)
{
	char buf[MAX_MESSAGE_LEN];

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	help(buf, sizeof(buf));
	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

void
on_help_interaction(struct discord *client,
                    const struct discord_interaction *event)
{
	char buf[MAX_MESSAGE_LEN];

	help(buf, sizeof(buf));
	struct discord_interaction_response params = {
		.type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
		.data = &(struct discord_interaction_callback_data)
		{
			.content = buf,
			/* .flags = DISCORD_MESSAGE_EPHEMERAL */
		}
	};
	discord_create_interaction_response(client, event->id, event->token,
	                                    &params, NULL);
}

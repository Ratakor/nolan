#include <string.h>
#include "nolan.h"

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
	unsigned long i, len = LENGTH(stats_ids);

	cpstr(buf, "Post a screenshot of your stats to ", siz);
	for (i = 0; i < len; i++) {
		p = strchr(buf, '\0');;
		snprintf(p, siz, "<#%lu> ", stats_ids[i]);
		if (i < len - 1)
			catstr(buf, "or ", siz);
	}
	catstr(buf, "to enter the database.\n", siz);
	catstr(buf, "Commands:\n", siz);
	catstr(buf, "\t?info *[[@]user]*\n", siz);
	catstr(buf, "\t?leaderboard or ?lb *category*\n", siz);
	/* catstr(buf, "\t?correct [category] [corrected value]\n", siz); */
	catstr(buf, "\t?source or ?src *[kingdom]*\n", siz);
	catstr(buf, "\t/help\n\n", siz);
	catstr(buf, "[...] means optional.\n", siz);
	catstr(buf, "Also works with /", siz);
}

void
on_help(struct discord * client, const struct discord_message * event)
{
	size_t siz = DISCORD_MAX_MESSAGE_LEN;
	char buf[DISCORD_MAX_MESSAGE_LEN];

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif

	help(buf, siz);
	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

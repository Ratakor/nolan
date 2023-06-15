#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nolan.h"

static void write_invalid(char *buf, size_t siz);
static void load_files(char *username, unsigned int *dmgs);
static void write_uraid(char *buf, size_t siz, char *username,
                        unsigned int *dmgs);
static void uraid(char *buf, size_t siz, char *username);

void
create_slash_uraid(struct discord *client)
{
	struct discord_application_command_option options[] = {
		{
			.type = DISCORD_APPLICATION_OPTION_STRING,
			.name = "username",
			.description = "username in Orna",
			.required = true
		},
	};
	struct discord_create_guild_application_command cmd = {
		.name = "uraid",
		.description = "Shows the user raid damage for the last 7 days",
		.options = &(struct discord_application_command_options)
		{
			.size = LENGTH(options),
			.array = options
		},
	};
	discord_create_guild_application_command(client, APP_ID, RAID_GUILD_ID,
	                &cmd, NULL);
}

void
write_invalid(char *buf, size_t siz)
{
	strlcpy(buf, "NO WRONG YOU MUST USE AN ARGUMENT.\n", siz);
	strlcat(buf, "Valid argument is an Orna username.\n", siz);
}

void
load_files(char *username, unsigned int *dmgs)
{
	unsigned int i;
	long day = time(NULL) / 86400;
	char fname[128];
	size_t namelen = strlen(username);

	for (i = 0; i < 6; i++) {
		snprintf(fname, sizeof(fname), "%s%ld.csv",
		         RAIDS_FOLDER, day - i);
		if (file_exists(fname))
			dmgs[i] = parse_file(fname, username, namelen);
	}
}

void
write_uraid(char *buf, size_t siz, char *username, unsigned int *dmgs)
{
	unsigned int i = 0, total = 0;
	size_t s = 0;
	time_t t = time(NULL);
	struct tm *tm = gmtime(&t);
	int d = tm->tm_wday;
	const char *week[] = {
		"Sun",
		"Mon",
		"Tue",
		"Wed",
		"Thu",
		"Fri",
		"Sat"
	};

	s += snprintf(buf + s, siz - s,
	              "%s's raids stats for the last 7 days:\n", username);
	for (; i < 7; i++, d--) {
		if (d < 0) d = 6;
		total += dmgs[i];
		s += snprintf(buf + s, siz - s, "%s: ", week[d]);
		s += uintfmt(buf + s, siz - s, dmgs[i]);
		s += strlcpy(buf + s, " damage\n", siz - s);
		if (s >= siz) {
			WARN("string truncation");
			return;
		}
	}
	s += strlcpy(buf + s, "\nTotal: ", siz - s);
	s += uintfmt(buf + s, siz - s, total);
	s += strlcpy(buf + s, " damage", siz - s);
	if (s >= siz)
		WARN("string truncation");
}

void
uraid(char *buf, size_t siz, char *username)
{
	unsigned int dmgs[7];

	memset(dmgs, 0, sizeof(dmgs));
	load_files(username, dmgs);
	write_uraid(buf, siz, username, dmgs);
}

void
on_uraid(struct discord *client, const struct discord_message *event)
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

	LOG("start");
	if (strlen(event->content) == 0)
		write_invalid(buf, sizeof(buf));
	else
		uraid(buf, sizeof(buf), event->content);

	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
	LOG("end");
}

void
on_uraid_interaction(struct discord *client,
                     const struct discord_interaction *event)
{
	char buf[MAX_MESSAGE_LEN];

	LOG("start");
	if (!event->data->options)
		write_invalid(buf, sizeof(buf));
	else
		uraid(buf, sizeof(buf), event->data->options->array[0].value);

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

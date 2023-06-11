#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nolan.h"

static void write_invalid(char *buf, size_t siz);
static unsigned long parse_file(char *fname, char *username);
static unsigned long *load_files(char *username, unsigned long *dmgs);
static void write_uraid(char *buf, int siz, char *username,
                        unsigned long *dmgs);
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

unsigned long
parse_file(char *fname, char *username)
{
	FILE *fp;
	unsigned long dmg;
	char line[LINE_SIZE], *endname;

	fp = efopen(fname, "r");
	while (fgets(line, LINE_SIZE, fp)) {
		endname = strchr(line, DELIM);
		dmg = strtoul(endname + 1, NULL, 10);
		if (endname)
			*endname = '\0';
		if (strcmp(username, line) == 0) {
			fclose(fp);
			return dmg;
		}
	}

	fclose(fp);
	return 0;
}

unsigned long *
load_files(char *username, unsigned long *dmgs)
{
	unsigned int i;
	long day = time(NULL) / 86400;
	char fname[128];

	for (i = 0; i < 6; i++) {
		snprintf(fname, sizeof(fname), "%s%ld.csv",
		         RAIDS_FOLDER, day - i);
		if (file_exists(fname))
			dmgs[i] = parse_file(fname, username);
	}

	return dmgs;
}

void
write_uraid(char *buf, int siz, char *username, unsigned long *dmgs)
{
	char *p;
	unsigned long total = 0;
	time_t t = time(NULL);
	struct tm *tm = gmtime(&t);
	int i = 0, d = tm->tm_wday;
	const char *week[] = {
		"Sun",
		"Mon",
		"Tue",
		"Wed",
		"Thu",
		"Fri",
		"Sat"
	};

	siz -= snprintf(buf, siz, "%s's raids stats for the last 7 days:\n",
	                username);
	p = strchr(buf, '\0');
	for (; i < 7; i++, d--) {
		if (d < 0) d = 6;
		siz -= snprintf(p, siz, "%s: %'lu damage\n", week[d], dmgs[i]);
		total += dmgs[i];
		p = strchr(buf, '\0');
	}
	siz -= snprintf(p, siz, "\nTotal: %'lu damage\n", total);
	if (siz <= 0)
		WARN("string truncation");
}

void
uraid(char *buf, size_t siz, char *username)
{
	unsigned long dmgs[7];

	memset(dmgs, 0, sizeof(dmgs));
	load_files(username, dmgs);
	write_uraid(buf, (int)siz, username, dmgs);
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

	if (strlen(event->content) == 0)
		write_invalid(buf, sizeof(buf));
	else
		uraid(buf, sizeof(buf), event->content);

	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

void
on_uraid_interaction(struct discord *client,
                     const struct discord_interaction *event)
{
	char buf[MAX_MESSAGE_LEN];

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
}

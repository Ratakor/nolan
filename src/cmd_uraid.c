/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nolan.h"

/* see raids.c */
#define DIFF        3 /* reduce n size in strncmp to reduce tesseract errors */

static uint32_t parse_file_uraid(char *fname, char *username, size_t namelen);
static void write_invalid(char *buf, size_t siz);
static void load_files_uraid(char *username, uint32_t *dmgs);
static void write_uraid(char *buf, size_t siz, char *username, uint32_t *dmgs);
static void uraid(char *buf, size_t siz, char *username);

static const char *week[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

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

uint32_t
parse_file_uraid(char *fname, char *username, size_t namelen)
{
	FILE *fp;
	char line[LINE_SIZE];
	uint32_t dmg;

	fp = xfopen(fname, "r");
	while (fgets(line, LINE_SIZE, fp)) {
		dmg = strtoul(strchr(line, DELIM) + 1, NULL, 10);
		if (strncasecmp(username, line, namelen - DIFF) == 0) {
			fclose(fp);
			return dmg;
		}
	}

	fclose(fp);
	return 0;
}

void
load_files_uraid(char *username, uint32_t *dmgs)
{
	char fname[128];
	size_t namelen, i;
	time_t day;

	namelen = strlen(username);
	day = time(NULL) / 86400;
	for (i = 0; i < 6; i++) {
		snprintf(fname, sizeof(fname), "%s%ld.csv",
		         RAIDS_FOLDER, day - i);
		if (file_exists(fname))
			dmgs[i] = parse_file_uraid(fname, username, namelen);
	}
}

void
write_uraid(char *buf, size_t siz, char *username, uint32_t *dmgs)
{
	size_t i = 0, s = 0;
	uint32_t total = 0;
	int day;

	day = ((time(NULL) / 86400) + 4) % 7;
	s += snprintf(buf + s, siz - s,
	              "%s's raids stats for the last 7 days:\n", username);
	for (; i < 7; i++, day--) {
		if (day < 0) day = 6;
		total += dmgs[i];
		s += snprintf(buf + s, siz - s, "%s: ", week[day]);
		s += ufmt(buf + s, siz - s, dmgs[i]);
		s += strlcpy(buf + s, " damage\n", siz - s);
		if (s >= siz) {
			log_warn("%s: string truncation", __func__);
			return;
		}
	}
	s += strlcpy(buf + s, "\nTotal: ", siz - s);
	s += ufmt(buf + s, siz - s, total);
	s += strlcpy(buf + s, " damage", siz - s);
	if (s >= siz)
		log_warn("%s: string truncation", __func__);
}

void
uraid(char *buf, size_t siz, char *username)
{
	uint32_t dmgs[7];

	memset(dmgs, 0, sizeof(dmgs));
	load_files_uraid(username, dmgs);
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

	log_info("%s", __func__);
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

	log_info("%s", __func__);
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

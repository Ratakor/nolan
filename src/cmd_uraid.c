/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nolan.h"

static uint32_t parse_file_uraid(char *fname, char *username);
static void write_invalid(char *buf, size_t siz);
static void load_files_uraid(char *username, uint32_t *dmgs);
static void write_uraid(char *buf, size_t siz, char *username, uint32_t *dmgs);
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
	strlcpy(buf, "NO WRONG THIS IS NOT A VALID ARGUMENT.\n", siz);
	strlcat(buf, "Valid argument is an Orna username.\n", siz);
}

uint32_t
parse_file_uraid(char *fname, char *username)
{
	FILE *fp;
	char line[LINE_SIZE];
	uint32_t dmg;

	fp = xfopen(fname, "r");
	while (fgets(line, LINE_SIZE, fp)) {
		dmg = strtoul(strchr(line, DELIM) + 1, NULL, 10);
		if (strncasecmp(username, line, strlen(username)) == 0) {
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
	time_t day;
	int n, i;

	day = time(NULL) / 86400;
	i = (day + 3) % 7; /* current day, used as index */
	for (n = 0; n < 7; n++, i--) {
		if (i == -1) i = 6;
		snprintf(fname, sizeof(fname), "%s%ld.csv",
		         RAIDS_FOLDER, day - n);
		if (file_exists(fname))
			dmgs[i] = parse_file_uraid(fname, username);
	}
}

void
write_uraid(char *buf, size_t siz, char *username, uint32_t *dmgs)
{
	static const char *wday[7] = {
		"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
	};
	int day, i, s = 0;
	uint32_t total = 0;

	day = (time(NULL) / 86400 + 3) % 7 - 6;
	s += snprintf(buf + s, siz - s,
	              "%s's raids stats for the last 7 days:\n```\n", username);
	for (i = 0; i < 6; i++, day++) {
		total += dmgs[i];
		if (day < 0) {
			s += snprintf(buf + s, siz - s, "%s: ", wday[7 + day]);
			s += ufmt(buf + s, siz - s, dmgs[7 + day]);
		} else {
			s += snprintf(buf + s, siz - s, "%s: ", wday[day]);
			s += ufmt(buf + s, siz - s, dmgs[day]);
		}
		s += strlcpy(buf + s, " damage\n", siz - s);
		if ((size_t)s >= siz) {
			log_warn("%s: string truncation", __func__);
			return;
		}
	}
	total += dmgs[i];
	if (total == 0) {
		snprintf(buf, siz, "There is no data for %s.", username);
		return;
	}

	s += snprintf(buf + s, siz - s, "Today: ");
	if (day < 0)
		s += ufmt(buf + s, siz - s, dmgs[7 + day]);
	else
		s += ufmt(buf + s, siz - s, dmgs[day]);
	s += strlcpy(buf + s, " damage\n```\n\nTotal: ", siz - s);
	s += ufmt(buf + s, siz - s, total);
	s += strlcpy(buf + s, " damage\n", siz - s);
	s += strlcpy(buf + s,
	             "\nOccasional errors may have impacted scores\n"
	             "Please check screenshot data manually and report what's wrong to Ratakor",
	             siz - s);
	if ((size_t)s >= siz)
		log_warn("%s: string truncation", __func__);
}

void
uraid(char *buf, size_t siz, char *username)
{
	uint32_t dmgs[7];
	const char *r = username;
	char *w = username;

	memset(dmgs, 0, sizeof(dmgs));
	do {
		if ((*r >= 'A' && *r <= 'Z') || (*r >= 'a' && *r <= 'z'))
			*w++ = *r;
	} while (*r++);
	*w = '\0';

	if (username[0] == '\0') {
		write_invalid(buf, siz);
		return;
	}
	load_files_uraid(username, dmgs);
	write_uraid(buf, siz, username, dmgs);
}

void
on_uraid(struct discord *client, const struct discord_message *ev)
{
	char buf[MAX_MESSAGE_LEN];

	if (ev->author->bot)
		return;

#ifdef DEVEL
	if (ev->channel_id != DEVEL)
		return;
#else
	if (ev->guild_id != RAID_GUILD_ID)
		return;
#endif /* DEVEL */

	log_info("%s", __func__);
	if (strlen(ev->content) == 0)
		write_invalid(buf, sizeof(buf));
	else
		uraid(buf, sizeof(buf), ev->content);

	discord_send_message(client, ev->channel_id, "%s", buf);
}

void
on_uraid_interaction(struct discord *client,
                     const struct discord_interaction *ev)
{
	char buf[MAX_MESSAGE_LEN];

	log_info("%s", __func__);
	if (!ev->data->options)
		write_invalid(buf, sizeof(buf));
	else
		uraid(buf, sizeof(buf), ev->data->options->array[0].value);

	discord_send_interaction_message(client, ev->id, ev->token, "%s", buf);
}

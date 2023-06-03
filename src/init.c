#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "nolan.h"

static Player load_player(unsigned int line);

void
create_folders(void)
{
	if (!file_exists(SAVE_FOLDER)) {
		if (mkdir(SAVE_FOLDER, 0755) == -1)
			die("nolan: Failed to create %s\n", SAVE_FOLDER);
	}
	if (!file_exists(IMAGE_FOLDER)) {
		if (mkdir(IMAGE_FOLDER, 0755) == -1)
			die("nolan: Failed to create %s\n", IMAGE_FOLDER);
	}
	if (!file_exists(RAIDS_FOLDER)) {
		if (mkdir(RAIDS_FOLDER, 0755) == -1)
			die("nolan: Failed to create %s\n", RAIDS_FOLDER);
	}
}

void
create_stats_file(void)
{
	FILE *fp;
	unsigned int i;
	long size = 0;

	fp = fopen(STATS_FILE, "r");

	if (fp != NULL) {
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
	}

	if (size == 0 && (fp = fopen(STATS_FILE, "w")) != NULL) {
		for (i = 0; i < LENGTH(fields) - 1; i++)
			fprintf(fp, "%s%c", fields[i], DELIM);
		fprintf(fp, "%s\n", fields[LENGTH(fields) - 1]);
	}
	fclose(fp);
}

void
create_slash_commands(struct discord *client)
{
	create_slash_help(client);
	create_slash_info(client);
	create_slash_leaderboard(client);
	create_slash_source(client);
	create_slash_lbraid(client);
	create_slash_uraid(client);
}

Player
load_player(unsigned int line)
{
	FILE *fp;
	Player player;
	char buf[LINE_SIZE], *p = NULL, *end;
	unsigned int i = 0;

	if (line <= 1)
		die("nolan: Tried to load the description line as a player\n");
	if ((fp = fopen(STATS_FILE, "r")) == NULL)
		die("nolan: Failed to open %s (read)\n", STATS_FILE);

	while (i++ < line && (p = fgets(buf, LINE_SIZE, fp)) != NULL);
	fclose(fp);
	if (p == NULL)
		die("nolan: Line %d is not present in %s\n", line, STATS_FILE);

	player.name = malloc(DISCORD_MAX_USERNAME_LEN);
	player.kingdom = malloc(32 + 1);
	i = 0;
	end = p;

	/* -1 because the last field in the file finish with a '\n' */
	while (i < LENGTH(fields) - 1 && *++end != '\0') {
		if (*end != DELIM)
			continue;
		*end = '\0';
		if (i <= 1) /* name and kingdom */
			strlcpy(((char **)&player)[i], p, 32 + 1);
		else
			((long *)&player)[i] = atol(p);
		p = end + 1;
		i++;
	}
	if (i != LENGTH(fields) - 1)
		die("nolan: Player on line %d is missing a field\n", line);
	player.userid = strtoul(p, NULL, 10);

	return player;
}

void
init_players(void)
{
	FILE *fp;
	char buf[LINE_SIZE];
	unsigned int i;

	if ((fp = fopen(STATS_FILE, "r")) == NULL)
		die("nolan: Failed to open %s (read)\n", STATS_FILE);

	while (fgets(buf, LINE_SIZE, fp))
		nplayers++;
	nplayers--; /* first line is not a player */

	if (nplayers > MAX_PLAYERS)
		die("nolan: There is too much players to load (max:%d)\n",
		    MAX_PLAYERS);

	for (i = 0; i < nplayers; i++)
		players[i] = load_player(i + 2);
}

void
on_interaction(struct discord *client, const struct discord_interaction *event)
{
	if (event->type != DISCORD_INTERACTION_APPLICATION_COMMAND)
		return;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	if (strcmp(event->data->name, "help") == 0)
		on_help_interaction(client, event);
	else if (strcmp(event->data->name, "info") == 0)
		on_info_interaction(client, event);
	else if (strcmp(event->data->name, "leaderboard") == 0)
		on_leaderboard_interaction(client, event);
	else if (strcmp(event->data->name, "source") == 0)
		on_source_interaction(client, event);
	else if (strcmp(event->data->name, "lbraid") == 0)
		on_lbraid_interaction(client, event);
	else if (strcmp(event->data->name, "uraid") == 0)
		on_uraid_interaction(client, event);
}

void
on_ready(struct discord *client, const struct discord_ready *event)
{
	struct discord_activity activities[] = {
		{
			.name = "/help",
			.type = DISCORD_ACTIVITY_LISTENING,
		},
	};

	struct discord_presence_update status = {
		.activities =
		        &(struct discord_activities)
		{
			.size = LENGTH(activities),
			.array = activities
		},
		.status = "online",
		.afk = false,
		.since = discord_timestamp(client),
	};

	discord_update_presence(client, &status);
}

void
on_message(struct discord *client, const struct discord_message *event)
{
	unsigned int i;

	if (event->author->bot)
		return;
	if (event->attachments->size == 0)
		return;
	if (strchr(event->attachments->array->filename, '.') == NULL)
		return;
	if (strcmp(event->attachments->array->content_type, "image/jpeg") != 0
	                && strcmp(event->attachments->array->content_type,
	                          "image/png") != 0)
		return;

#ifdef DEVEL
	if (event->channel_id == DEVEL)
		on_stats(client, event);
	return;
#endif /* DEVEL */

	for (i = 0; i < LENGTH(stats_ids); i++) {
		if (event->channel_id == stats_ids[i]) {
			on_stats(client, event);
			break;
		}
	}

	for (i = 0; i < LENGTH(raids_ids); i++) {
		if (event->channel_id == raids_ids[i]) {
			on_raids(client, event);
			break;
		}
	}
}

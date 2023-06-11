#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "nolan.h"

static Player load_player(unsigned int line);

void
create_folders(void)
{
	unsigned int i;
	const char *folders[] = {
		SAVE_FOLDER,
		IMAGES_FOLDER,
		RAIDS_FOLDER,
	};

	for (i = 0; i < LENGTH(folders); i++) {
		if (file_exists(folders[i]))
			continue;
		if (mkdir(folders[i], 0755) == -1)
			DIE("failed to create %s\n", folders[i]);
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

	if (size == 0) {
		if (fp) fclose(fp);
		fp = efopen(STATS_FILE, "w");
		for (i = 0; i < LENGTH(fields) - 1; i++)
			fprintf(fp, "%s%c", fields[i], DELIM);
		fprintf(fp, "%s\n", fields[LENGTH(fields) - 1]);
	}
	fclose(fp);
}

void
create_slash_commands(struct discord *client)
{
#ifndef DEVEL
	create_slash_help(client);
	create_slash_stats(client);
	create_slash_stats_admin(client);
	create_slash_info(client);
	create_slash_leaderboard(client);
	create_slash_source(client);
	create_slash_lbraid(client);
	create_slash_uraid(client);
#endif /* DEVEL */
}

Player
load_player(unsigned int line)
{
	FILE *fp;
	Player player;
	char buf[LINE_SIZE], *p = NULL, *delim;
	unsigned int i = 0;

	if (line <= 1)
		DIE("tried to load the description line as a player");
	fp = efopen(STATS_FILE, "r");

	while (i++ < line && (p = fgets(buf, LINE_SIZE, fp)) != NULL);
	fclose(fp);
	if (p == NULL)
		DIE("line %d is not present in %s", line, STATS_FILE);

	i = 0;
	delim = p;

	/* -1 because the last field in the file finish with a '\n' */
	while (i < LENGTH(fields) - 1 && *++delim != '\0') {
		if (*delim != DELIM)
			continue;

		*delim = '\0';
		if (i == 0)
			player.name = strndup(p, MAX_USERNAME_LEN);
		else if (i == 1)
			player.kingdom = strndup(p, MAX_KINGDOM_LEN);
		else
			((long *)&player)[i] = atol(p);
		p = delim + 1;
		i++;
	}
	if (i != LENGTH(fields) - 1) {
		DIE("player in %s on line %d is missing a field", STATS_FILE,
		    line);
	}
	player.userid = strtoul(p, NULL, 10);

	return player;
}

void
init_players(void)
{
	FILE *fp;
	char buf[LINE_SIZE];
	unsigned int i;

	fp = efopen(STATS_FILE, "r");
	while (fgets(buf, LINE_SIZE, fp))
		nplayers++;
	nplayers--; /* first line is not a player */

	if (nplayers > MAX_PLAYERS)
		DIE("there is too much players to load (max:%lu)", MAX_PLAYERS);

	for (i = 0; i < nplayers; i++)
		players[i] = load_player(i + 2);
}

void
on_interaction(struct discord *client, const struct discord_interaction *event)
{
	if (event->type != DISCORD_INTERACTION_APPLICATION_COMMAND)
		return;
	if (!event->data)
		return;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	if (strcmp(event->data->name, "help") == 0)
		on_help_interaction(client, event);
	else if (strcmp(event->data->name, "stats") == 0)
		on_stats_interaction(client, event);
	else if (strcmp(event->data->name, "stats_admin") == 0)
		on_stats_admin_interaction(client, event);
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

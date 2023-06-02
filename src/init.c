#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "nolan.h"

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
	unsigned long i;
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
	if (enable_raids) {
		create_slash_lbraid(client);
		create_slash_uraid(client);
	}
}

void
init_players(void)
{
	FILE *fp;
	char buf[LINE_SIZE];
	unsigned long i;

	if ((fp = fopen(STATS_FILE, "r")) == NULL)
		die("nolan: Failed to open %s (read)\n", STATS_FILE);

	while (fgets(buf, LINE_SIZE, fp))
		nplayers++;
	nplayers--; /* first line is not a player */

	if (nplayers > MAX_PLAYERS)
		die("nolan: There is too much players to load (max:%d)\n",
		    MAX_PLAYERS);

	for (i = 0; i < nplayers; i++)
		players[i] = create_player(i + 2);
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
	else if (enable_raids) {
		if (strcmp(event->data->name, "lbraid") == 0)
			on_lbraid_interaction(client, event);
		else if (strcmp(event->data->name, "uraid") == 0)
			on_uraid_interaction(client, event);
	}
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
	unsigned long i;

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
	if (enable_raids) {
		for (i = 0; i < LENGTH(raids_ids); i++) {
			if (event->channel_id == raids_ids[i]) {
				on_raids(client, event);
				break;
			}
		}
	}
}

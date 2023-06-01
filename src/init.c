#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "nolan.h"

static int file_exists(char *filename);

static int
file_exists(char *filename)
{
	struct stat buf;
	return (stat(filename, &buf) == 0);
}

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
	/* TODO */
	/* create_slash_source(client); */
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

/* TODO: handle files and embed (and no opt/opt better), ephemeral... */
void
on_interaction(struct discord *client, const struct discord_interaction *event)
{
	size_t siz = DISCORD_MAX_MESSAGE_LEN;
	char buf[DISCORD_MAX_MESSAGE_LEN] = "";

	if (event->type != DISCORD_INTERACTION_APPLICATION_COMMAND)
		return;
	/* if (!event->data || !event->data->options) */
	/* 	return; */

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif

	if (strcmp(event->data->name, "help") == 0) {
		help(buf, siz);
	} else if (strcmp(event->data->name, "info") == 0) {
		if (!event->data || !event->data->options) {
			info_from_uid(buf, siz, event->member->user->id);
		} else {
			info_from_txt(buf, siz,
			              event->data->options->array[0].value);
		}
	} else if (strcmp(event->data->name, "leaderboard") == 0) {
		if (event->data && event->data->options) {
			leaderboard(buf, siz,
			            event->data->options->array[0].value,
			            event->member->user->id);
		}
	} /*else if (strcmp(event->data->name, "source") == 0) {
		if (!event->data || !event->data->options) {
			source(buf, siz);
		} else {
			source_sorted(buf, siz,
			             event->data->options->array[0].value);
		}
	}*/

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
		on_raids(client, event);
	return;
#endif

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

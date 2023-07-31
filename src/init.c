/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <sys/stat.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nolan.h"

void
create_folders(void)
{
	size_t i;
	const char *folders[] = {
		SAVE_FOLDER,
		IMAGES_FOLDER,
		RAIDS_FOLDER,
	};

	for (i = 0; i < LENGTH(folders); i++) {
		if (file_exists(folders[i]))
			continue;

		if (mkdir(folders[i], 0755) == -1)
			die(1, "mkdir: %s:", folders[i]);
		else
			log_info("Created %s", folders[i]);
	}
}

void
create_stats_file(void)
{
	FILE *fp;
	size_t i, fsiz = 0;

	fp = fopen(STATS_FILE, "r");

	if (fp != NULL) {
		fseek(fp, 0, SEEK_END);
		fsiz = ftell(fp);
		fclose(fp);
	}

	if (fsiz == 0) {
		fp = xfopen(STATS_FILE, "w");
		for (i = 0; i < LENGTH(fields) - 1; i++)
			fprintf(fp, "%s%c", fields[i], DELIM);
		fprintf(fp, "%s\n", fields[LENGTH(fields) - 1]);
		fclose(fp);
		log_info("Created %s", STATS_FILE);
	}
}

void
create_slash_commands(struct discord *client)
{
#ifdef DEVEL
	UNUSED(client);
#else
	create_slash_help(client);
	create_slash_stats(client);
	create_slash_info(client);
	create_slash_leaderboard(client);
	create_slash_correct(client);
	create_slash_source(client);
	create_slash_lbraid(client);
	create_slash_uraid(client);
	create_slash_time(client);
#endif /* DEVEL */
}

void
init_players(void)
{
	FILE *fp;
	Player *player;
	char line[LINE_SIZE], *p, *delim;
	size_t i, line_counter = 1;

	fp = xfopen(STATS_FILE, "r");
	fgets(line, LINE_SIZE, fp); /* discard description line */
	while ((p = fgets(line, LINE_SIZE, fp)) != NULL) {
		line_counter++;
		player = xcalloc(1, sizeof(*player));
		i = 0;
		delim = p;
		/* -1 because the last field in the file finish with a '\n' */
		while (i < LENGTH(fields) - 1 && *++delim != '\0') {
			if (*delim != DELIM)
				continue;

			*delim = '\0';
			switch (i) {
			case NAME:
				strlcpy(player->name, p, MAX_USERNAME_SIZ);
				break;
			case KINGDOM:
				strlcpy(player->kingdom, p, MAX_KINGDOM_SIZ);
				break;
			case UPDATE:
				player->update = strtol(p, NULL, 10);
				break;
			default:
				U32CAST(player)[i] = strtoul(p, NULL, 10);
				break;
			}
			p = delim + 1;
			i++;
		}
		if (i != LENGTH(fields) - 1) {
			die(1, "Player in %s on line %zu is missing a field",
			    STATS_FILE, line_counter);
		}
		player->userid = strtoull(p, NULL, 10);
		pthread_mutex_lock(&player_mutex);
		player->next = player_head;
		player_head = player;
		pthread_mutex_unlock(&player_mutex);
	}
	fclose(fp);
}

void
on_ready(struct discord *client, const struct discord_ready *event)
{
	log_info("Logged in as %s", event->user->username);
	if (event->guilds->size <= 1)
		log_info("Connected to %d server", event->guilds->size);
	else
		log_info("Connected to %d servers", event->guilds->size);

	struct discord_activity activities[] = {
		{
			.name = "/help",
			.type = DISCORD_ACTIVITY_LISTENING,
		},
	};
	struct discord_presence_update status = {
		.activities = &(struct discord_activities)
		{
			.size = LENGTH(activities),
			.array = activities
		},
		.status = "online",
		.since = discord_timestamp(client),
	};
	discord_update_presence(client, &status);
}

#include <string.h>
#include <sys/stat.h>

#include "nolan.h"

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
#endif /* DEVEL */
}

void
init_players(void)
{
	FILE *fp;
	char line[LINE_SIZE], *p, *delim;
	unsigned int i;

	fp = efopen(STATS_FILE, "r");
	fgets(line, LINE_SIZE, fp); /* discard description line */
	while ((p = fgets(line, LINE_SIZE, fp)) != NULL) {
		i = 0;
		delim = p;
		/* -1 because the last field in the file finish with a '\n' */
		while (i < LENGTH(fields) - 1 && *++delim != '\0') {
			if (*delim != DELIM)
				continue;

			*delim = '\0';
			if (i == NAME)
				players[nplayers].name = strndup(p, MAX_USERNAME_LEN);
			else if (i == KINGDOM)
				players[nplayers].kingdom = strndup(p, MAX_KINGDOM_LEN);
			else
				((long *)&players[nplayers])[i] = atol(p);
			p = delim + 1;
			i++;
		}
		if (i != LENGTH(fields) - 1) {
			DIE("player in %s on line %lu is missing a field",
			    STATS_FILE, nplayers + 1);
		}
		players[nplayers].userid = strtoul(p, NULL, 10);

		if (++nplayers > MAX_PLAYERS)
			DIE("there is too much players to load (max:%d)",
			    MAX_PLAYERS);
	}
	fclose(fp);
}

void
on_ready(struct discord *client, const struct discord_ready *event)
{
	UNUSED(event);
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

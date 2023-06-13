#include <stdlib.h>
#include <string.h>

#include "nolan.h"

#define STRLEN(STR) (sizeof STR - 1)

static long playtime_to_long(char *playtime, char *str);
static long trim_stat(char *str);
static void parse_line(Player *player, char *line);
static void for_line(Player *player, char *txt);
static void create_player(Player *player, unsigned int i);
static char *update_player(char *buf, int siz, Player *player, unsigned int i);
static unsigned int update_players(char *buf, size_t siz, Player *player);
static void stats(char *buf, size_t siz, char *url, char *username,
                  u64snowflake userid, u64snowflake guild_id,
                  struct discord *client);

void
create_slash_stats(struct discord *client)
{
	struct discord_application_command_option options[] = {
		{
			.type = DISCORD_APPLICATION_OPTION_ATTACHMENT,
			.name = "stats",
			.description = "stats screenshot",
			.required = true
		},
	};
	struct discord_create_global_application_command cmd = {
		.name = "stats",
		.description = "Update your Orna stats",
		.options = &(struct discord_application_command_options)
		{
			.size = LENGTH(options),
			.array = options
		}
	};
	discord_create_global_application_command(client, APP_ID, &cmd, NULL);
}

long
playtime_to_long(char *playtime, char str[])
{
	char *p;
	long days, hours;

	days = atol(playtime);
	if ((p = strchr(playtime, str[0])) == 0)
		return days; /* less than a day of playtime */
	while (*str && (*p++ == *str++));
	hours = atol(p);

	return days * 24 + hours;
}

char *
playtime_to_str(long playtime)
{
	long days = playtime / 24;
	long hours = playtime % 24;
	size_t siz = 36;
	char *buf = emalloc(siz);

	switch (hours) {
	case 0:
		if (days <= 1)
			snprintf(buf, siz, "%ld day", days);
		else
			snprintf(buf, siz, "%ld days", days);
		break;
	case 1:
		if (days == 0)
			snprintf(buf, siz, "%ld hour", hours);
		else if (days == 1)
			snprintf(buf, siz, "%ld day, %ld hour", days, hours);
		else
			snprintf(buf, siz, "%ld days, %ld hour", days, hours);
		break;
	default:
		if (days == 0)
			snprintf(buf, siz, "%ld hours", hours);
		else if (days == 1)
			snprintf(buf, siz, "%ld day, %ld hours", days, hours);
		else
			snprintf(buf, siz, "%ld days, %ld hours", days, hours);
		break;
	}

	return buf;
}

/* trim everything that is not a number and replace | with 1 */
long
trim_stat(char *str)
{
	const char *p = str;
	long stat = 0;

	do {
		if (*p >= '0' && *p <= '9')
			stat = (stat * 10) + (*p - '0');
		else if (*p == '|')
			stat = (stat * 10) + 1;
	} while (*p++ && *p != '(');

	return stat;
}

void
parse_line(Player *player, char *line)
{
	char *str;
	long stat;

	if (strncmp(line, "KINGDOM", STRLEN("KINGDOM")) == 0) {
		str = "KINGDOM ";
		while (*str && (*line++ == *str++));
		player->kingdom = line;
		return;
	}
	if (strncmp(line, "ROYAUME", STRLEN("ROYAUME")) == 0) {
		str = "ROYAUME ";
		while (*str && (*line++ == *str++));
		player->kingdom = line;
		return;
	}
	/* tesseract madness */
	if (strncmp(line, "\\ele]]", STRLEN("\\ele]]")) == 0) {
		str = "\\ele]] ";
		while (*str && (*line++ == *str++));
		player->kingdom = line;
		return;
	}

	if (strncmp(line, "PLAYTIME", STRLEN("PLAYTIME")) == 0) {
		str = "PLAYTIME ";
		while (*str && (*line++ == *str++));
		player->playtime = playtime_to_long(line, "days, ");
		return;
	}
	if (strncmp(line, "TEMPS DE JEU", STRLEN("TEMPS DE JEU")) == 0) {
		str = "TEMPS DE JEU ";
		while (*str && (*line++ == *str++));
		player->playtime = playtime_to_long(line, "jours, ");
		return;
	}

	if (strncmp(line, "ASCENSION LEVEL", STRLEN("ASCENSION LEVEL")) == 0 ||
	                strncmp(line, "NIVEAU D'ELEVATION", STRLEN("NIVEAU D'ELEVATION")) == 0) {
		player->ascension = trim_stat(line);
	} else if (strncmp(line, "LEVEL", STRLEN("LEVEL")) == 0 ||
	                strncmp(line, "NIVEAU", STRLEN("NIVEAU")) == 0) {
		if ((stat = trim_stat(line)) <= 250)
			player->level = stat;
	} else if (strncmp(line, "GLOBAL RANK", STRLEN("GLOBAL RANK")) == 0 ||
	                strncmp(line, "RANG GLOBAL", STRLEN("RANG GLOBAL")) == 0) {
		player->global = trim_stat(line);
	} else if (strncmp(line, "REGIONAL RANK", STRLEN("REGIONAL RANK")) == 0 ||
	                strncmp(line, "RANG REGIONAL", STRLEN("RANG REGIONAL")) == 0) {
		player->regional = trim_stat(line);
	} else if (strncmp(line, "COMPETITIVE RANK", STRLEN("COMPETITIVE RANK")) == 0 ||
	                strncmp(line, "RANG COMPETITIF", STRLEN("RANG COMPETITIF")) == 0) {
		player->competitive = trim_stat(line);
	} else if (strncmp(line, "MONSTERS SLAIN", STRLEN("MONSTERS SLAIN")) == 0 ||
	                strncmp(line, "MONSTRES TUES", STRLEN("MONSTRES TUES")) == 0) {
		player->monsters = trim_stat(line);
	} else if (strncmp(line, "BOSSES SLAIN", STRLEN("BOSSES SLAIN")) == 0 ||
	                strncmp(line, "BOSS TUES", STRLEN("BOSS TUES")) == 0) {
		player->bosses = trim_stat(line);
	} else if (strncmp(line, "PLAYERS DEFEATED", STRLEN("PLAYERS DEFEATED")) == 0 ||
	                strncmp(line, "JOUEURS VAINCUS", STRLEN("JOUEURS VAINCUS")) == 0) {
		player->players = trim_stat(line);
	} else if (strncmp(line, "QUESTS COMPLETED", STRLEN("QUESTS COMPLETED")) == 0 ||
	                strncmp(line, "QUETES TERMINEES", STRLEN("QUETES TERMINEES")) == 0) {
		player->quests = trim_stat(line);
	} else if (strncmp(line, "AREAS EXPLORED", STRLEN("AREAS EXPLORED")) == 0 ||
	                strncmp(line, "TERRES EXPLOREES", STRLEN("TERRES EXPLOREES")) == 0) {
		player->explored = trim_stat(line);
	} else if (strncmp(line, "AREAS TAKEN", STRLEN("AREAS TAKEN")) == 0 ||
	                strncmp(line, "TERRES PRISES", STRLEN("TERRES PRISES")) == 0) {
		player->taken = trim_stat(line);
	} else if (strncmp(line, "DUNGEONS CLEARED", STRLEN("DUNGEONS CLEARED")) == 0 ||
	                strncmp(line, "DONJONS TERMINES", STRLEN("DONJONS TERMINES")) == 0) {
		player->dungeons = trim_stat(line);
	} else if (strncmp(line, "COLISEUM WINS", STRLEN("COLISEUM WINS")) == 0 ||
	                strncmp(line, "VICTOIRES DANS LE", STRLEN("VICTOIRES DANS LE")) == 0 ||
	                strncmp(line, "VICTOIRES DANS LE COLISEE", STRLEN("VICTOIRES DANS LE COLISEE")) == 0) {
		player->coliseum = trim_stat(line);
	} else if (strncmp(line, "ITEMS UPGRADED", STRLEN("ITEMS UPGRADED")) == 0 ||
	                strncmp(line, "OBJETS AMELIORES", STRLEN("OBJETS AMELIORES")) == 0) {
		player->items = trim_stat(line);
	} else if (strncmp(line, "FISH CAUGHT", STRLEN("FISH CAUGHT")) == 0 ||
	                strncmp(line, "POISSONS ATTRAPES", STRLEN("POISSONS ATTRAPES")) == 0) {
		player->fish = trim_stat(line);
	} else if (strncmp(line, "DISTANCE TRAVELLED", STRLEN("DISTANCE TRAVELLED")) == 0 ||
	                strncmp(line, "DISTANCE VOYAGEE", STRLEN("DISTANCE VOYAGEE")) == 0) {
		player->distance = trim_stat(line);
	} else if (strncmp(line, "REPUTATION", STRLEN("REPUTATION")) == 0) {
		player->reputation = trim_stat(line);
	} else if (strncmp(line, "ENDLESS RECORD", STRLEN("ENDLESS RECORD")) == 0 ||
	                strncmp(line, "RECORD DU MODE", STRLEN("RECORD DU MODE")) == 0 ||
	                strncmp(line, "RECORD DU MODE SANS-FIN", STRLEN("RECORD DU MODE SANS-FIN")) == 0) {
		player->endless = trim_stat(line);
	} else if (strncmp(line, "ENTRIES COMPLETED", STRLEN("ENTRIES COMPLETED")) == 0 ||
	                strncmp(line, "RECHERCHES TERMINEES", STRLEN("RECHERCHES TERMINEES")) == 0) {
		player->codex = trim_stat(line);
	}
}

void
for_line(Player *player, char *txt)
{
	char *line = txt, *endline;

	while (line) {
		endline = strchr(line, '\n');
		if (endline)
			*endline = '\0';
		parse_line(player, line);
		line = endline ? (endline + 1) : 0;
	}
}


void
create_player(Player *player, unsigned int i)
{
	unsigned int j;

	players[i].name = strndup(player->name, MAX_USERNAME_LEN);
	players[i].kingdom = strndup(player->kingdom, MAX_KINGDOM_LEN);
	for (j = 2; j < LENGTH(fields); j++)
		((long *)&players[i])[j] = ((long *)player)[j];
}

char *
update_player(char *buf, int siz, Player *player, unsigned int i)
{
	char *p, *plto, *pltn, *pltd;
	unsigned int j;
	long old, new, diff;
	struct tm *tm = gmtime(&players[i].update);

	/* keep this commented to not update name and keep corrected change */
	/* strlcpy(players[i].name, player->name, MAX_USERNAME_LEN); */

	siz -= snprintf(buf, siz, "**%s**'s profile has been updated.\n\n",
	                players[i].name);
	p = strchr(buf, '\0');

	if (strcmp(players[i].kingdom, player->kingdom) != 0) {
		siz -= snprintf(p, siz, "%s: %s -> %s\n", fields[1],
		                players[i].kingdom, player->kingdom);
		p = strchr(buf, '\0');

		/* update player */
		strlcpy(players[i].kingdom, player->kingdom, MAX_KINGDOM_LEN);
	}

	/* -2 to not include update and userid */
	for (j = 2; j < LENGTH(fields) - 2; j++) {
		old = ((long *)&players[i])[j];
		new = ((long *)player)[j];
		diff = new - old;
		/*
		 * this is to prevent random tesseract error but block the user
		 * from correcting the error by sending a new screenshot
		 */
		if (new == 0 || diff == 0)
			continue;
		/* don't update if stat decreases except for ranks */
		if (diff < 0 && j != 4 && j != 5 && j != 6)
			continue;

		if (j == 7) { /* playtime */
			plto = playtime_to_str(old);
			pltn = playtime_to_str(new);
			pltd = playtime_to_str(diff);
			siz -= snprintf(p, siz, "%s: %s -> %s (+ %s)\n",
			                fields[7], plto, pltn, pltd);
			free(plto);
			free(pltn);
			free(pltd);
		} else {
			siz -= snprintf(p, siz, "%s: %'ld -> %'ld (%'+ld)\n",
			                fields[j], old, new, diff);
		}
		p = strchr(buf, '\0');

		/* update player */
		((long *)&players[i])[j] = new;
	}
	if (siz <= 0)
		WARN("string truncation");
	if (!strftime(p, siz, "\nLast update was on %d %b %Y at %R UTC\n", tm))
		WARN("strftime: string truncation");
	players[i].update = player->update;

	/*
	 * TODO
	 * New roles: ...
	 */

	return buf;
}

void
update_file(Player *player)
{
	FILE *w, *r;
	char line[LINE_SIZE], *startuid, tmpfname[128];
	u64snowflake userid;
	unsigned int i;
	int found = 0;

	strlcpy(tmpfname, SAVE_FOLDER, sizeof(tmpfname));
	strlcat(tmpfname, "tmpfile", sizeof(tmpfname));
	r = efopen(STATS_FILE, "r");
	w = efopen(tmpfname, "w");

	while (fgets(line, LINE_SIZE, r)) {
		if ((startuid = strrchr(line, DELIM)) == NULL)
			DIE("line \"%s\" in %s is wrong", line, STATS_FILE);
		userid = strtoul(startuid + 1, NULL, 10);
		if (userid == player->userid) {
			found = 1;
			fprintf(w, "%s%c", player->name, DELIM);
			fprintf(w, "%s%c", player->kingdom, DELIM);
			for (i = 2; i < LENGTH(fields) - 1; i++)
				fprintf(w, "%ld%c", ((long *)player)[i], DELIM);
			fprintf(w, "%lu\n", player->userid);
		} else {
			fprintf(w, "%s", line);
		}
	}
	if (!found) {
		fprintf(w, "%s%c", player->name, DELIM);
		fprintf(w, "%s%c", player->kingdom, DELIM);
		for (i = 2; i < LENGTH(fields) - 1; i++)
			fprintf(w, "%ld%c", ((long *)player)[i], DELIM);
		fprintf(w, "%lu\n", player->userid);
	}

	fclose(r);
	fclose(w);
	remove(STATS_FILE);
	rename(tmpfname, STATS_FILE);
}

/* update players, write the update msg and update file, returns player's index */
unsigned int
update_players(char *buf, size_t siz, Player *player)
{
	unsigned int i = 0;
	int r;

	while (i < nplayers && players[i].userid != player->userid)
		i++;

	if (i == nplayers) { /* new player */
		nplayers++;
		if (nplayers > MAX_PLAYERS)
			DIE("there is too much players (max:%lu)", MAX_PLAYERS);
		create_player(player, i);
		r = snprintf(buf, siz,
		             "**%s** has been registrated in the database.\n\n",
		             player->name);
		write_info(buf + r, siz - r, &players[i]);
	} else {
		update_player(buf, siz, player, i);
	}

	update_file(&players[i]);

	return i;
}

void
stats(char *buf, size_t siz, char *url, char *username, u64snowflake userid,
      u64snowflake guild_id, struct discord *client)
{
	unsigned int i, ret;
	char *txt, fname[128];
	Player player;

	/* not always a jpg but idc */
	snprintf(fname, sizeof(fname), "%s/%lu.jpg", IMAGES_FOLDER, userid);
	if ((ret = curl(url, fname)) != 0) {
		WARN("curl failed CURLcode:%u", ret);
		strlcpy(buf, "Error: Failed to download image", siz);
		return;
	}
	if ((txt = ocr(fname, "eng")) == NULL) {
		WARN("failed to read image");
		strlcpy(buf, "Error: Failed to read image", siz);
		return;
	}

	memset(&player, 0, sizeof(player));
	player.name = username;
	player.userid = userid;
	player.update = time(NULL);
	for_line(&player, txt);
	free(txt);

	/* detect wrong images */
	i = 2;
	while (((long *)&player)[i] == 0 && i++ < LENGTH(fields) - 2);
	if (i == LENGTH(fields) - 2)
		return;

	if (player.kingdom == NULL)
		player.kingdom = "(null)";

	if (kingdom_verification) {
		i = LENGTH(kingdoms);

		while (i > 0 && strcmp(player.kingdom, kingdoms[i++]) != 0);

		if (i == 0) {
			strlcpy(buf, "Sorry you're not part of the kingdom :/",
			        siz);
			return;
		}
	}

	i = update_players(buf, siz, &player);

#ifndef DEVEL
	if (guild_id == ROLE_GUILD_ID)
		update_roles(client, &players[i]);
#endif /* DEVEL */
}

void
on_stats(struct discord *client, const struct discord_message *event)
{
	char buf[MAX_MESSAGE_LEN] = "";

	stats(buf,
	      sizeof(buf),
	      event->attachments->array[0].url,
	      event->author->username,
	      event->author->id,
	      event->guild_id,
	      client);

	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

void
on_stats_interaction(struct discord *client,
                     const struct discord_interaction *event)
{
	char buf[MAX_MESSAGE_LEN] = "", *url, *endurl;
	json_char *attachment = strdup(event->data->resolved->attachments);

	url = strstr(attachment, "\"url\":");
	if (!url) {
		free(attachment);
		return;
	}
	url += STRLEN("\"url\":\"");
	endurl = strchr(url, '"');
	if (!endurl) {
		free(attachment);
		return;
	}
	*endurl = '\0';

	stats(buf,
	      sizeof(buf),
	      url,
	      event->member->user->username,
	      event->member->user->id,
	      event->guild_id,
	      client);
	free(attachment);

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

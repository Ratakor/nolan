#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nolan.h"

#define LEN(X)        (sizeof X - 1)

static long playtime_to_long(char *playtime, char *str);
static char *trim_all(char *str);
static void parse_line(Player *player, char *line);
static void for_line(Player *player, char *txt);
static char *update_msg(char *buf, int siz, Player *player, unsigned int i);
static void update_player(Player *player, unsigned int i);
static void save_player_to_file(Player *player);
static void update_players(Player *player, struct discord *client,
                           const struct discord_message *event);

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
	size_t siz = 32;
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

/* trim everything that is not a number or a left parenthesis */
char *
trim_all(char *str)
{
	const char *r = str;
	char *w = str;

	do {
		if ((*r >= 48 && *r <= 57) || *r == '(')
			*w++ = *r;
	} while (*r++);
	*w = '\0';

	return str;
}

void
parse_line(Player *player, char *line)
{
	char *str;

	if (strncmp(line, "KINGDOM", LEN("KINGDOM")) == 0) {
		str = "KINGDOM ";
		while (*str && (*line++ == *str++));
		player->kingdom = line;
		return;
	}
	if (strncmp(line, "ROYAUME", LEN("ROYAUME")) == 0) {
		str = "ROYAUME ";
		while (*str && (*line++ == *str++));
		player->kingdom = line;
		return;
	}

	if (strncmp(line, "PLAYTIME", LEN("PLAYTIME")) == 0) {
		str = "PLAYTIME ";
		while (*str && (*line++ == *str++));
		player->playtime = playtime_to_long(line, "days, ");
		return;
	}
	if (strncmp(line, "TEMPS DE JEU", LEN("TEMPS DE JEU")) == 0) {
		str = "TEMPS DE JEU ";
		while (*str && (*line++ == *str++));
		player->playtime = playtime_to_long(line, "jours, ");
		return;
	}

	if (strncmp(line, "ASCENSION LEVEL", LEN("ASCENSION LEVEL")) == 0 ||
	                strncmp(line, "NIVEAU D'ELEVATION", LEN("NIVEAU D'ELEVATION")) == 0) {
		player->ascension = atol(trim_all(line));
	} else if (strncmp(line, "LEVEL", LEN("LEVEL")) == 0 ||
	                strncmp(line, "NIVEAU", LEN("NIVEAU")) == 0) {
		player->level = atol(trim_all(line));
	} else if (strncmp(line, "GLOBAL RANK", LEN("GLOBAL RANK")) == 0 ||
	                strncmp(line, "RANG GLOBAL", LEN("RANG GLOBAL")) == 0) {
		player->global = atol(trim_all(line));
	} else if (strncmp(line, "REGIONAL RANK", LEN("REGIONAL RANK")) == 0 ||
	                strncmp(line, "RANG REGIONAL", LEN("RANG REGIONAL")) == 0) {
		player->regional = atol(trim_all(line));
	} else if (strncmp(line, "COMPETITIVE RANK", LEN("COMPETITIVE RANK")) == 0 ||
	                strncmp(line, "RANG COMPETITIF", LEN("RANG COMPETITIF")) == 0) {
		player->competitive = atol(trim_all(line));
	} else if (strncmp(line, "MONSTERS SLAIN", LEN("MONSTERS SLAIN")) == 0 ||
	                strncmp(line, "MONSTRES TUES", LEN("MONSTRES TUES")) == 0) {
		player->monsters = atol(trim_all(line));
	} else if (strncmp(line, "BOSSES SLAIN", LEN("BOSSES SLAIN")) == 0 ||
	                strncmp(line, "BOSS TUES", LEN("BOSS TUES")) == 0) {
		player->bosses = atol(trim_all(line));
	} else if (strncmp(line, "PLAYERS DEFEATED", LEN("PLAYERS DEFEATED")) == 0 ||
	                strncmp(line, "JOUEURS VAINCUS", LEN("JOUEURS VAINCUS")) == 0) {
		player->players = atol(trim_all(line));
	} else if (strncmp(line, "QUESTS COMPLETED", LEN("QUESTS COMPLETED")) == 0 ||
	                strncmp(line, "QUETES TERMINEES", LEN("QUETES TERMINEES")) == 0) {
		player->quests = atol(trim_all(line));
	} else if (strncmp(line, "AREAS EXPLORED", LEN("AREAS EXPLORED")) == 0 ||
	                strncmp(line, "TERRES EXPLOREES", LEN("TERRES EXPLOREES")) == 0) {
		player->explored = atol(trim_all(line));
	} else if (strncmp(line, "AREAS TAKEN", LEN("AREAS TAKEN")) == 0 ||
	                strncmp(line, "TERRES PRISES", LEN("TERRES PRISES")) == 0) {
		player->taken = atol(trim_all(line));
	} else if (strncmp(line, "DUNGEONS CLEARED", LEN("DUNGEONS CLEARED")) == 0 ||
	                strncmp(line, "DONJONS TERMINES", LEN("DONJONS TERMINES")) == 0) {
		player->dungeons = atol(trim_all(line));
	} else if (strncmp(line, "COLISEUM WINS", LEN("COLISEUM WINS")) == 0 ||
	                strncmp(line, "VICTOIRES DANS LE COLISEE", LEN("VICTOIRES DANS LE COLISEE")) == 0) {
		player->coliseum = atol(trim_all(line));
	} else if (strncmp(line, "ITEMS UPGRADED", LEN("ITEMS UPGRADED")) == 0 ||
	                strncmp(line, "OBJETS AMELIORES", LEN("OBJETS AMELIORES")) == 0) {
		player->items = atol(trim_all(line));
	} else if (strncmp(line, "FISH CAUGHT", LEN("FISH CAUGHT")) == 0 ||
	                strncmp(line, "POISSONS ATTRAPES", LEN("POISSONS ATTRAPES")) == 0) {
		player->fish = atol(trim_all(line));
	} else if (strncmp(line, "DISTANCE TRAVELLED", LEN("DISTANCE TRAVELLED")) == 0 ||
	                strncmp(line, "DISTANCE VOYAGEE", LEN("DISTANCE VOYAGEE")) == 0) {
		player->distance = atol(trim_all(line));
	} else if (strncmp(line, "REPUTATION", LEN("REPUTATION")) == 0) {
		player->reputation = atol(trim_all(line));
	} else if (strncmp(line, "ENDLESS RECORD", LEN("ENDLESS RECORD")) == 0 ||
	                strncmp(line, "RECORD DU MODE", LEN("RECORD DU MODE")) == 0 ||
	                strncmp(line, "RECORD DU MODE SANS-FIN", LEN("RECORD DU MODE SANS-FIN")) == 0) {
		player->endless = atol(trim_all(line));
	} else if (strncmp(line, "ENTRIES COMPLETED", LEN("ENTRIES COMPLETED")) == 0 ||
	                strncmp(line, "RECHERCHES TERMINEES", LEN("RECHERCHES TERMINEES")) == 0) {
		player->codex = atol(trim_all(line));
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

char *
update_msg(char *buf, int siz, Player *player, unsigned int i)
{
	char *p, *plto, *pltn, *pltd;
	unsigned int j;
	long old, new, diff;
	struct tm *tm = gmtime(&players[i].update);

	siz -= snprintf(buf, siz, "%s's profile has been updated.\n\n",
	                player->name);
	p = strchr(buf, '\0');

	if (strcmp(players[i].kingdom, player->kingdom) != 0) {
		siz -= snprintf(p, siz, "%s: %s -> %s\n", fields[1],
		                players[i].kingdom, player->kingdom);
		p = strchr(buf, '\0');
	}

	/* -2 to not include update and userid */
	for (j = 2; j < LENGTH(fields) - 2; j++) {
		if (siz <= 0)
			warn("nolan: truncation in updatemsg\n");
		old = ((long *)&players[i])[j];
		new = ((long *)player)[j];
		diff = new - old;
		if (new == 0 || diff == 0)
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
	}

	if (!strftime(p, siz, "\nLast update was on %d %b %Y at %R UTC\n", tm))
		warn("nolan: strftime: truncation in updatemsg\n");

	/*
	 * TODO
	 * New roles: ...
	 */

	return buf;
}

void
update_player(Player *player, unsigned int i)
{
	unsigned int j;

	free(players[i].name);
	free(players[i].kingdom);
	players[i].name = strndup(player->name, MAX_USERNAME_LEN);
	players[i].kingdom = strndup(player->kingdom, MAX_KINGDOM_LEN);
	for (j = 2; j < LENGTH(fields); j++) {
		if (((long *)player)[j] != 0)
			((long *)&players[i])[j] = ((long *)player)[j];
	}
}

/* Save player to file and return player's index in file if it was found */
void
save_player_to_file(Player *player)
{
	FILE *w, *r;
	char line[LINE_SIZE], *endname, tmpfname[128];
	unsigned int i;
	int found = 0;

	strlcpy(tmpfname, SAVE_FOLDER, sizeof(tmpfname));
	strlcat(tmpfname, "tmpfile", sizeof(tmpfname));
	if ((r = fopen(STATS_FILE, "r")) == NULL)
		die("nolan: Failed to open %s\n", STATS_FILE);
	if ((w = fopen(tmpfname, "w")) == NULL)
		die("nolan: Failed to open %s\n", tmpfname);

	while (fgets(line, LINE_SIZE, r)) {
		endname = strchr(line, DELIM);
		if (endname)
			*endname = 0;
		if (strcmp(player->name, line) == 0) {
			found = 1;
			fprintf(w, "%s%c", player->name, DELIM);
			fprintf(w, "%s%c", player->kingdom, DELIM);
			for (i = 2; i < LENGTH(fields) - 1; i++)
				fprintf(w, "%ld%c", ((long *)player)[i], DELIM);
			fprintf(w, "%lu\n", player->userid);
		} else {
			if (endname)
				*endname = DELIM;
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

/* update players, source file, roles if Orna FR and post an update msg */
void
update_players(Player *player, struct discord *client,
               const struct discord_message *event)
{
	unsigned int i = 0;
	int r;
	char buf[MAX_MESSAGE_LEN];

	/* while (i < nplayers && players[i].userid != player->userid) */
	while (i < nplayers && strcmp(players[i].name, player->name) != 0)
		i++;

	if (i == nplayers) { /* new player */
		nplayers++;
		if (nplayers > MAX_PLAYERS)
			die("nolan: There is too much players (max:%d)\n",
			    MAX_PLAYERS);
		r = snprintf(buf, sizeof(buf),
		             "**%s** has been registrated in the database.\n\n",
		             player->name);
		write_info(buf + r, sizeof(buf) - r, player);
	} else {
		update_msg(buf, (int)sizeof(buf), player, i);
	}
	update_player(player, i);
	save_player_to_file(&players[i]);
#ifndef DEVEL
	if (event->guild_id == ROLE_GUILD_ID)
		update_roles(client, event->author->id, &players[i]);
#endif /* DEVEL */

	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

void
on_stats(struct discord *client, const struct discord_message *event)
{
	unsigned int i;
	int ham = event->author->id == HAM && strlen(event->content) > 0;
	char *txt, fname[128];
	Player player;

	/* not always a jpg but idc */
	if (ham) {
		snprintf(fname, sizeof(fname), "%s/%s.jpg", IMAGES_FOLDER,
		         event->content);
	} else {
		snprintf(fname, sizeof(fname), "%s/%s.jpg", IMAGES_FOLDER,
		         event->author->username);
	}
	curl(event->attachments->array->url, fname);
	txt = ocr(fname, "eng");
	if (txt == NULL) {
		warn("nolan: Failed to read stats image\n");
		struct discord_create_message msg = {
			.content = "Error: Failed to read image"
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		return;
	}

	memset(&player, 0, sizeof(player));
	if (ham) {
		player.name = event->content;
	} else {
		player.name = event->author->username;
		player.userid = event->author->id;
	}
	player.update = time(NULL);
	for_line(&player, txt);
	free(txt);

	if (player.kingdom == NULL)
		player.kingdom = "(null)";

	if (kingdom_verification) {
		i = LENGTH(kingdoms);

		while (i > 0 && strcmp(player.kingdom, kingdoms[i++]) != 0);

		if (i == 0) {
			struct discord_create_message msg = {
				.content = "Sorry you're not part of the kingdom :/"
			};
			discord_create_message(client, event->channel_id, &msg,
			                       NULL);
			return;
		}
	}

	update_players(&player, client, event);
}

#include <stdlib.h>
#include <string.h>
#include "nolan.h"

#define LEN(X) (sizeof X - 1)

static void update_players(Player *player);
static long playtime_to_long(char *playtime, char *str);
static char *trim_all(char *str);
static void parse_line(Player *player, char *line);
static void for_line(Player *player, char *txt);
static int save_player_to_file(Player *player);
static char *update_msg(Player *player, int iplayer);

Player
create_player(unsigned int line)
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
			cpstr(((char **)&player)[i], p, 32 + 1);
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

static void
update_players(Player *player)
{
	unsigned long i = 0, j;

	/* while (i < nplayers && strcmp(players[i].name, player->name) != 0) */
	while (i < nplayers && players[i].userid != player->userid)
		i++;

	if (i == nplayers) { /* new player */
		if (nplayers > MAX_PLAYERS)
			die("nolan: There is too much players (max:%d)\n",
			    MAX_PLAYERS);
		players[nplayers] = create_player(nplayers + 2);
		nplayers++;
	} else {
		if (player->name) {
			cpstr(players[i].name, player->name,
			      DISCORD_MAX_USERNAME_LEN);
		}
		if (player->kingdom)
			cpstr(players[i].kingdom, player->kingdom, 32 + 1);
		/* keep original userid */
		for (j = 2; j < LENGTH(fields) - 1; j++)
			((long *)&players[i])[j] = ((long *)player)[j];
	}
}

static long
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
	char *buf = malloc(siz);

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
static char *
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

static void
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
	                strncmp(line, "RECORD DU MODE SANS-FIN", LEN("RECORD DU MODE SANS-FIN")) == 0) {
		player->endless = atol(trim_all(line));
	} else if (strncmp(line, "ENTRIES COMPLETED", LEN("ENTRIES COMPLETED")) == 0 ||
	                strncmp(line, "RECHERCHES TERMINEES", LEN("RECHERCHES TERMINEES")) == 0) {
		player->codex = atol(trim_all(line));
	}
}

static void
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

/* Save player to file and return player's index in file if it was found */
static int
save_player_to_file(Player *player)
{
	FILE *w, *r;
	char buf[LINE_SIZE], *p, *endname;
	unsigned long iplayer = 0, cpt = 1, i;

	if ((r = fopen(STATS_FILE, "r")) == NULL)
		die("nolan: Failed to open %s (read)\n", STATS_FILE);
	if ((w = fopen("tmpfile", "w")) == NULL)
		die("nolan: Failed to open %s (write)\n", STATS_FILE);

	while ((p = fgets(buf, LINE_SIZE, r)) != NULL) {
		endname = strchr(p, DELIM);
		if (endname)
			*endname = 0;
		if (strcmp(player->name, p) == 0) {
			iplayer = cpt;
			fprintf(w, "%s%c", player->name, DELIM);
			fprintf(w, "%s%c", player->kingdom, DELIM);
			for (i = 2; i < LENGTH(fields) - 1; i++)
				fprintf(w, "%ld%c", ((long *)player)[i], DELIM);
			fprintf(w, "%lu\n", player->userid);
		} else {
			if (endname)
				*endname = DELIM;
			fprintf(w, "%s", p);
		}
		cpt++;
	}
	if (!iplayer) {
		fprintf(w, "%s%c", player->name, DELIM);
		fprintf(w, "%s%c", player->kingdom, DELIM);
		for (i = 2; i < LENGTH(fields) - 1; i++)
			fprintf(w, "%ld%c", ((long *)player)[i], DELIM);
		fprintf(w, "%lu\n", player->userid);
	}

	fclose(r);
	fclose(w);
	remove(STATS_FILE);
	rename("tmpfile", STATS_FILE);

	return iplayer;
}

char *
update_msg(Player *player, int iplayer)
{
	size_t sz = 1024;
	char *buf = malloc(sz + 1), *p, *plto, *pltn, *pltd;
	unsigned long i;
	long old, new, diff;

	sz -= snprintf(buf, sz, "%s's profile has been updated.\n\n",
	               player->name);
	p = strchr(buf, '\0');

	if (strcmp(players[iplayer].kingdom, player->kingdom) != 0) {
		sz -= snprintf(p, sz, "%s: %s -> %s\n", fields[1],
		               players[iplayer].kingdom, player->kingdom);
		p = strchr(buf, '\0');
	}

	for (i = 2; i < LENGTH(fields) - 1; i++) {
		if (sz <= 0)
			die("nolan: truncation in updatemsg\n");
		old = ((long *)&players[iplayer])[i];
		new = ((long *)player)[i];
		diff = new - old;
		if (diff == 0)
			continue;

		if (i == 7) { /* playtime */
			plto = playtime_to_str(old);
			pltn = playtime_to_str(new);
			pltd = playtime_to_str(diff);
			sz -= snprintf(p, sz, "%s: %s -> %s (+ %s)\n",
			               fields[7], plto, pltn, pltd);
			free(plto);
			free(pltn);
			free(pltd);
		} else {
			sz -= snprintf(p, sz, "%s: %'ld -> %'ld (%'+ld)\n",
			               fields[i], old, new, diff);
		}
		p = strchr(buf, '\0');
	}

	/*
	 * TODO
	 * Last update was xxx ago
	 */

	return buf;
}

void
on_stats(struct discord *client, const struct discord_message *event)
{
	int i, iplayer;
	char *txt, fname[64];
	Player player;

	snprintf(fname, 64, "%s/%s.jpg", IMAGE_FOLDER, event->author->username);
	curl(event->attachments->array->url, fname);
	txt = ocr(fname);
	if (txt == NULL) {
		struct discord_create_message msg = {
			.content = "Error: Failed to read image"
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		return;
	}

	memset(&player, 0, sizeof(player));
	player.name = event->author->username;
	player.userid = event->author->id;
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
			discord_create_message(client, event->channel_id, &msg, NULL);
			return;
		}
	}

	if ((iplayer = save_player_to_file(&player))) {
		txt = update_msg(&player, iplayer - 2);
	} else {
		txt = malloc(128);
		snprintf(txt, 128, "**%s** has been registrated in the database.",
		         player.name);
	}
	struct discord_create_message msg = {
		.content = txt
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
	update_players(&player);
	free(txt);
}

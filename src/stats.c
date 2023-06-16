#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nolan.h"

#define STRLEN(STR) (sizeof STR - 1)
#define URL         "https://api.oss117quotes.xyz/v1/random"

struct Field {
	const char *key;
	unsigned int val;
	unsigned int keylen;
};

enum lang {
	ENGLISH,
	FRENCH
};

static long playtime_to_long(char *playtime, int lang);
static long trim_stat(char *str);
static void parse_line(Player *player, char *line);
static void for_line(Player *player, char *txt);
static size_t write_quote(char *buf, size_t siz);
static void create_player(Player *player, unsigned int i);
static void update_player(char *buf, size_t siz, Player *player, unsigned int i);
static unsigned int update_players(char *buf, size_t siz, Player *player);
static void stats(char *buf, size_t siz, char *url, char *username,
                  u64snowflake userid, u64snowflake guild_id,
                  struct discord *client);

const struct Field english[] = {
	{ "KINGDOM",        KINGDOM,          STRLEN("KINGDOM")        },
	{ "\\ele]]",        KINGDOM,          STRLEN("\\ele]]")        },
	{ "PLAYTIME",       PLAYTIME,         STRLEN("PLAYTIME")       },
	{ "ASCENSION",      ASCENSION,        STRLEN("ASCENSION")      },
	{ "LEVEL",          LEVEL,            STRLEN("LEVEL")          },
	{ "GLOBAL",         GLOBAL_RANK,      STRLEN("GLOBAL")         },
	{ "REGIONAL",       REGIONAL_RANK,    STRLEN("REGIONAL")       },
	{ "COMPETITIVE",    COMPETITIVE_RANK, STRLEN("COMPETITIVE")    },
	{ "MONSTERS",       MONSTERS,         STRLEN("MONSTERS")       },
	{ "BOSSES",         BOSSES,           STRLEN("BOSSES")         },
	{ "PLAYERS",        PLAYERS_DEFEATED, STRLEN("PLAYERS")        },
	{ "QUESTS",         QUESTS,           STRLEN("QUESTS")         },
	{ "AREAS EXPLORED", AREAS_EXPLORED,   STRLEN("AREAS EXPLORED") },
	{ "AREAS TAKEN",    AREAS_TAKEN,      STRLEN("AREAS TAKEN")    },
	{ "DUNGEONS",       DUNGEONS,         STRLEN("DUNGEONS")       },
	{ "COLISEUM",       COLISEUM,         STRLEN("COLISEUM")       },
	{ "ITEMS",          ITEMS,            STRLEN("ITEMS")          },
	{ "FISH",           FISH,             STRLEN("FISH")           },
	{ "DISTANCE",       DISTANCE,         STRLEN("DISTANCE")       },
	{ "REPUTATION",     REPUTATION,       STRLEN("REPUTATION")     },
	{ "ENDLESS",        ENDLESS,          STRLEN("ENDLESS")        },
	{ "ENTRIES",        CODEX,            STRLEN("ENTRIES")        },

};

const struct Field french[] = {
	{ "ROYAUME",          KINGDOM,          STRLEN("ROYAUME")          },
	{ "TEMPS DE JEU",     PLAYTIME,         STRLEN("TEMPS DE JEU")     },
	{ "NIVEAU D'ELEVAT",  ASCENSION,        STRLEN("NIVEAU D'ELEVAT")  },
	{ "NIVEAU",           LEVEL,            STRLEN("NIVEAU")           },
	{ "RANG GLOBAL",      GLOBAL_RANK,      STRLEN("RANG GLOBAL")      },
	{ "RANG REGIONAL",    REGIONAL_RANK,    STRLEN("RANG REGIONAL")    },
	{ "RANG COMPETITIF",  COMPETITIVE_RANK, STRLEN("RANG COMPETITIF")  },
	{ "MONSTRES",         MONSTERS,         STRLEN("MONSTRES")         },
	{ "BOSS TUES",        BOSSES,           STRLEN("BOSS TUES")        },
	{ "JOUEURS",          PLAYERS_DEFEATED, STRLEN("JOUEURS")          },
	{ "QUETES",           QUESTS,           STRLEN("QUETES")           },
	{ "TERRES EXPLOREES", AREAS_EXPLORED,   STRLEN("TERRES EXPLOREES") },
	{ "TERRES PRISES",    AREAS_TAKEN,      STRLEN("TERRES PRISES")    },
	{ "DONJONS",          DUNGEONS,         STRLEN("DONJONS")          },
	{ "VICTOIRE DANS",    COLISEUM,         STRLEN("VICTOIRE DANS")    },
	{ "OBJETS",           ITEMS,            STRLEN("OBJETS")           },
	{ "POISSONS",         FISH,             STRLEN("POISSONS")         },
	{ "RECORD DU MODE",   ENDLESS,          STRLEN("RECORD DU MODE")   },
	{ "RECHERCHES",       CODEX,            STRLEN("RECHERCHES")       },

};

const struct Field *languages[] = {
	english,
	french,
};

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
playtime_to_long(char *playtime, int lang)
{
	char *p, dayname[32], *pdn = dayname;
	long days, hours;

	if (lang == ENGLISH)
		strlcpy(dayname, "days, ", sizeof(dayname));
	else if (lang == FRENCH)
		strlcpy(dayname, "jours, ", sizeof(dayname));
	else
		DIE("new language not correctly added");

	days = strtol(playtime, NULL, 10);
	if ((p = strchr(playtime, dayname[0])) == 0)
		return days; /* less than a day of playtime */
	while (*pdn && (*p++ == *pdn++));
	hours = strtol(p, NULL, 10);

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
	const struct Field *field;
	unsigned int lang, i;
	size_t langlen;
	long stat;

	for (lang = 0; lang < LENGTH(languages); lang++) {
		if (lang == ENGLISH)
			langlen = LENGTH(english);
		else if (lang == FRENCH)
			langlen = LENGTH(french);
		else
			DIE("new language not correctly added");

		for (i = 0; i < langlen; i++) {
			field = &languages[lang][i];
			if (strncmp(line, field->key, field->keylen) == 0)
				break;
		}
		if (i < langlen)
			break;
	}

	if (lang == LENGTH(languages))
		return;

	switch (field->val) {
	case KINGDOM:
		player->kingdom = line + field->keylen + 1;
		break;
	case PLAYTIME:
		player->playtime = playtime_to_long(line + field->keylen, lang);
		break;
	case LEVEL:
		stat = trim_stat(line);
		if (stat && stat <= 250)
			player->level = stat;
		break;
	default:
		stat = trim_stat(line);
		if (stat)
			((long *)player)[field->val] = stat;
		break;
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

size_t
write_quote(char *buf, size_t siz)
{
	char *quote = curl(URL), *start, *end;
	size_t ret;

	start = nstrchr(quote, '"', 3) + 1;
	if (!start) {
		free(quote);
		return 0;
	}
	end = strchr(start, '"');
	if (!end) {
		free(quote);
		return 0;
	}
	*end = '\0';

	ret = snprintf(buf, siz, "%s\n\n", start);
	free(quote);
	return ret;
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

void
update_player(char *buf, size_t siz, Player *player, unsigned int i)
{
	char *plto, *pltn, *pltd;
	unsigned int j;
	long old, new, diff;
	size_t s = 0;

	/* keep this commented to not update name and keep corrected change */
	/* strlcpy(players[i].name, player->name, MAX_USERNAME_LEN); */

	s += write_quote(buf + s, siz - s);
	s += snprintf(buf + s, siz - s, "**%s**'s profile has been updated.\n",
	              players[i].name);

	if (strcmp(players[i].kingdom, player->kingdom) != 0) {
		s += snprintf(buf + s, siz - s, "%s: %s -> %s\n", fields[1],
		              players[i].kingdom, player->kingdom);
		strlcpy(players[i].kingdom, player->kingdom, MAX_KINGDOM_LEN);
	}

	/* -2 to not include update and userid */
	for (j = 2; j < LENGTH(fields) - 2; j++) {
		if (s >= siz) {
			WARN("string truncation");
			return;
		}

		old = ((long *)&players[i])[j];
		new = ((long *)player)[j];
		diff = new - old;
		/*
		 * this is to prevent random tesseract error but block the user
		 * from correcting the error by sending a new screenshot
		 */
		if (new == 0 || diff == 0)
			continue;
		/* don't update if stat decreases except for special cases */
		if (diff < 0 && j != ASCENSION     && j != GLOBAL_RANK &&
		                j != REGIONAL_RANK && j != COMPETITIVE_RANK)
			continue;

		if (j == PLAYTIME) {
			plto = playtime_to_str(old);
			pltn = playtime_to_str(new);
			pltd = playtime_to_str(diff);
			s += snprintf(buf + s, siz - s, "%s: %s -> %s (+ %s)\n",
			              fields[7], plto, pltn, pltd);
			free(plto);
			free(pltn);
			free(pltd);
		} else {
			s += snprintf(buf + s, siz - s, "%s: ", fields[j]);
			s += longfmt(buf + s, siz - s, "'", old);
			if (j == DISTANCE) s += strlcpy(buf + s, "m", siz - s);
			s += strlcpy(buf + s, " -> ", siz - s);
			s += longfmt(buf + s, siz - s, "'", new);
			if (j == DISTANCE) s += strlcpy(buf + s, "m", siz - s);
			s += strlcpy(buf + s, " (", siz - s);
			s += longfmt(buf + s, siz - s, "'+", diff);
			if (j == DISTANCE) s += strlcpy(buf + s, "m", siz - s);
			s += strlcpy(buf + s, ")\n", siz - s);
		}

		/* update player */
		((long *)&players[i])[j] = new;
	}

	s += snprintf(buf + s, siz - s,
	              "\nLast update was <t:%ld:R> on <t:%ld:f>\n",
	              players[i].update, players[i].update);
	players[i].update = player->update;
	if (s >= siz) {
		WARN("string truncation");
		return;
	}

	s += snprintf(buf + s, siz - s, "Correct your stats with %scorrect ;)",
	              PREFIX);
	if (s >= siz)
		WARN("string truncation");

	/* TODO: New roles: ... */
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
	size_t s = 0;

	while (i < nplayers && players[i].userid != player->userid)
		i++;

	if (i == nplayers) { /* new player */
		nplayers++;
		if (nplayers > MAX_PLAYERS)
			DIE("there is too much players (max:%d)", MAX_PLAYERS);
		create_player(player, i);
		s += write_quote(buf + s, siz - s);
		s += snprintf(buf + s, siz - s,
		              "**%s** has been registrated in the database.\n",
		              player->name);
		write_info(buf + s, siz - s, &players[i]);
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
	if ((ret = curl_file(url, fname)) != 0) {
		WARN("curl failed CURLcode: %u", ret);
		strlcpy(buf, "Error: Failed to download image", siz);
		return;
	}
	if ((txt = ocr(fname, "eng")) == NULL) {
		WARN("failed to read image from %s", username);
		strlcpy(buf, "Error: Failed to read image", siz);
		return;
	}

	memset(&player, 0, sizeof(player));
	player.name = username;
	player.userid = userid;
	player.update = time(NULL);
	for_line(&player, txt);
	/* txt contains player.kingdom */

	/* detect wrong images */
	i = 2;
	while (((long *)&player)[i] == 0 && i++ < LENGTH(fields) - 2);
	if (i == LENGTH(fields) - 2) {
		free(txt);
		return;
	}

	if (player.kingdom == NULL)
		player.kingdom = "(null)";

	i = update_players(buf, siz, &player);
	free(txt);

#ifdef DEVEL
	UNUSED(guild_id);
	UNUSED(client);
#else
	if (guild_id == ROLE_GUILD_ID)
		update_roles(client, &players[i]);
#endif /* DEVEL */
}

void
on_stats(struct discord *client, const struct discord_message *event)
{
	char buf[MAX_MESSAGE_LEN] = "";

	LOG("start");
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
	LOG("end");
}

void
on_stats_interaction(struct discord *client,
                     const struct discord_interaction *event)
{
	char buf[MAX_MESSAGE_LEN] = "", *url, *endurl;
	json_char *attachment = strdup(event->data->resolved->attachments);

	LOG("start");
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
	LOG("end");
}

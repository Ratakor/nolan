/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nolan.h"

#define URL "https://api.oss117quotes.xyz/v1/random"

struct Field {
	const char *const key;
	const unsigned int val;
	const unsigned int keylen;
};

enum { ENGLISH, FRENCH };

static uint32_t playtime_to_u32(char *playtime, int lang);
static void parse_line(Player *player, char *line);
static void for_line(Player *player, char *txt);
static char *get_quote(void);
static size_t write_quote(char *buf, size_t siz);
static void update_player(char *buf, size_t siz, Player *player,
                          Player *new_player);
static Player *update_players(char *buf, size_t siz, Player *new_player);
static void stats(char *buf, size_t siz, char *url, char *username,
                  u64snowflake userid, u64snowflake guild_id,
                  struct discord *client);

static const struct Field english[] = {
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

static const struct Field french[] = {
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

static const struct Field *languages[] = {
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

uint32_t
playtime_to_u32(char *playtime, int lang)
{
	const char *dayname = NULL, *pdn;
	char *p;
	uint32_t days, hours;

	if (lang == ENGLISH)
		dayname = "days, ";
	else if (lang == FRENCH)
		dayname = "jours, ";
	else
		die(1, "%s:%d %s: New language not correctly added",
		    __FILE__, __LINE__, __func__);

	days = strtoul(playtime, NULL, 10);
	if ((p = strchr(playtime, dayname[0])) == 0)
		return days; /* less than a day of playtime */
	pdn = dayname;
	while (*pdn && (*p++ == *pdn++));
	hours = strtoul(p, NULL, 10);

	return days * 24 + hours;
}

char *
playtime_to_str(uint32_t playtime)
{
	uint32_t days, hours;
	const size_t siz = 36;
	char *buf;

	days = playtime / 24;
	hours = playtime % 24;
	buf = xmalloc(siz);
	dalloc_comment(buf, "playtime_to_str buf");
	switch (hours) {
	case 0:
		if (days <= 1)
			snprintf(buf, siz, "%"PRIu32" day", days);
		else
			snprintf(buf, siz, "%"PRIu32" days", days);
		break;
	case 1:
		if (days == 0) {
			snprintf(buf, siz, "%"PRIu32" hour", hours);
		} else if (days == 1) {
			snprintf(buf, siz, "%"PRIu32" day, %"PRIu32" hour",
			         days, hours);
		} else {
			snprintf(buf, siz, "%"PRIu32" days, %"PRIu32" hour",
			         days, hours);
		}
		break;
	default:
		if (days == 0) {
			snprintf(buf, siz, "%"PRIu32" hours", hours);
		} else if (days == 1) {
			snprintf(buf, siz, "%"PRIu32" day, %"PRIu32" hours",
			         days, hours);
		} else {
			snprintf(buf, siz, "%"PRIu32" days, %"PRIu32" hours",
			         days, hours);
		}
		break;
	}

	return buf;
}

/* trim everything that is not a number and replace | with 1 */
uint32_t
trim_stat(const char *str)
{
	uint32_t stat = 0;

	do {
		if (*str >= '0' && *str <= '9')
			stat = (stat * 10) + (*str - '0');
		else if (*str == '|')
			stat = (stat * 10) + 1;
	} while (*str++ && *str != '(');

	return stat;
}

void
parse_line(Player *player, char *line)
{
	const struct Field *field;
	unsigned int lang, i;
	size_t langlen;
	uint32_t stat;

	for (lang = 0; lang < LENGTH(languages); lang++) {
		if (lang == ENGLISH)
			langlen = LENGTH(english);
		else if (lang == FRENCH)
			langlen = LENGTH(french);
		else
			die(1, "%s:%d %s: New language not correctly added",
			    __FILE__, __LINE__, __func__);

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
		if (VALID_STATS(line + field->keylen + 1))
			strlcpy(player->kingdom, line + field->keylen + 1,
			        MAX_KINGDOM_SIZ);
		break;
	case PLAYTIME:
		player->playtime = playtime_to_u32(line + field->keylen, lang);
		break;
	case LEVEL:
		stat = trim_stat(line);
		if (stat && stat <= 250)
			player->level = stat;
		break;
	default:
		stat = trim_stat(line);
		if (stat)
			U32CAST(player)[field->val] = stat;
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

char *
get_quote(void)
{
	char *quote, *p, *sentence, *name, *tmp, *op, *otmp;

	quote = curl(URL);
	if ((sentence = nstrchr(quote, '"', 3)) == NULL) {
		free(quote);
		return NULL;
	}
	sentence++;
	if ((name = nstrchr(quote, '"', 8)) == NULL) {
		free(quote);
		return NULL;
	}
	*name++ = '\n';
	*name++ = '>';
	*name = ' ';
	name -= 2;

	if ((p = strchr(sentence, '"')) == NULL) {
		free(quote);
		return NULL;
	}
	*p = '\0';
	if ((p = strchr(name, '"')) == NULL) {
		free(quote);
		return NULL;
	}
	*p = '\0';

	p = quote;
	while ((*p++ = *sentence++));
	p--;
	while ((*p++ = *name++));

	/* change '*' to "\*", must need a large enough buffer */
	p = quote;
	while ((p = strchr(p, '*')) != NULL) {
		tmp = xstrdup(p);
		op = p;
		otmp = tmp;
		*p++ = '\\';
		while ((*p++ = *tmp++));
		p = op + 2;
		free(otmp);
	}

	return quote;
}

size_t
write_quote(char *buf, size_t siz)
{
	char *quote;
	size_t ret;

	quote = get_quote();
	if (quote == NULL)
		return 0;

	ret = snprintf(buf, siz, "%s\n\n", quote);
	free(quote);

	return ret;
}

void
update_player(char *buf, size_t siz, Player *player, Player *new_player)
{
	char *plto, *pltn, *pltd;
	size_t i, s = 0;
	uint32_t old, new;
	int32_t diff;

	s += snprintf(buf + s, siz - s, "**%s**'s profile has been updated.\n",
	              player->name);

	if (strcmp(player->kingdom, new_player->kingdom) != 0) {
		s += snprintf(buf + s, siz - s, "%s: %s -> %s\n", fields[1],
		              player->kingdom, new_player->kingdom);
		strlcpy(player->kingdom, new_player->kingdom, MAX_KINGDOM_SIZ);
	}

	/* -2 to not include update and userid */
	for (i = 2; i < LENGTH(fields) - 2; i++) {
		if (s >= siz) {
			log_warn("%s: string truncation", __func__);
			return;
		}

		old = U32CAST(player)[i];
		new = U32CAST(new_player)[i];
		diff = new - old;
		/*
		 * this is to prevent random tesseract error but block the user
		 * from correcting the error by sending a new screenshot
		 */
		if (new == 0 || diff == 0)
			continue;
		/* don't update if stat decreases except for special cases */
		if (diff < 0 && i != ASCENSION     && i != GLOBAL_RANK &&
		                i != REGIONAL_RANK && i != COMPETITIVE_RANK)
			continue;

		if (i == PLAYTIME) {
			plto = playtime_to_str(old);
			pltn = playtime_to_str(new);
			pltd = playtime_to_str(diff);
			s += snprintf(buf + s, siz - s, "%s: %s -> %s (+%s)\n",
			              fields[i], plto, pltn, pltd);
			free(plto);
			free(pltn);
			free(pltd);
		} else {
			s += snprintf(buf + s, siz - s, "%s: ", fields[i]);
			s += ufmt(buf + s, siz - s, old);
			if (i == DISTANCE) s += strlcpy(buf + s, "m", siz - s);
			s += strlcpy(buf + s, " -> ", siz - s);
			s += ufmt(buf + s, siz - s, new);
			if (i == DISTANCE) s += strlcpy(buf + s, "m", siz - s);
			s += strlcpy(buf + s, " (", siz - s);
			s += ifmt(buf + s, siz - s, diff);
			if (i == DISTANCE) s += strlcpy(buf + s, "m", siz - s);
			s += strlcpy(buf + s, ")\n", siz - s);
		}

		/* update player */
		U32CAST(player)[i] = new;
	}

	s += snprintf(buf + s, siz - s,
	              "\nLast update was <t:%ld:R> on <t:%ld:f>\n",
	              player->update, player->update);
	player->update = new_player->update;
	if (s >= siz) {
		log_warn("string truncation", __func__);
		return;
	}

	s += snprintf(buf + s, siz - s, "Correct your stats with %scorrect ;)",
	              PREFIX);
	if (s >= siz) {
		log_warn("string truncation", __func__);
		return;
	}

	/* TODO: New roles: ... */
}

void
update_file(Player *player)
{
	FILE *w, *r;
	char line[LINE_SIZE], *startuid;
	u64snowflake userid;
	unsigned int i;
	bool found = false;

	r = xfopen(STATS_FILE, "r");
	w = xfopen(SAVE_FOLDER "tmpfile", "w");

	while (fgets(line, LINE_SIZE, r)) {
		if ((startuid = strrchr(line, DELIM)) == NULL)
			die(1, "Line \"%s\" in %s is wrong", line, STATS_FILE);
		userid = strtoull(startuid + 1, NULL, 10);
		if (userid == player->userid) {
			found = true;
			fprintf(w, "%s%c", player->name, DELIM);
			fprintf(w, "%s%c", player->kingdom, DELIM);
			for (i = 2; i < LENGTH(fields) - 2; i++)
				fprintf(w, "%"PRIu32"%c", U32CAST(player)[i],
				        DELIM);
			fprintf(w, "%ld%c", player->update, DELIM);
			fprintf(w, "%"PRIu64"\n", player->userid);
		} else {
			fprintf(w, "%s", line);
		}
	}
	if (!found) {
		fprintf(w, "%s%c", player->name, DELIM);
		fprintf(w, "%s%c", player->kingdom, DELIM);
		for (i = 2; i < LENGTH(fields) - 2; i++)
			fprintf(w, "%"PRIu32"%c", U32CAST(player)[i], DELIM);
		fprintf(w, "%ld%c", player->update, DELIM);
		fprintf(w, "%"PRIu64"\n", player->userid);
	}

	fclose(r);
	fclose(w);
	remove(STATS_FILE);
	rename(SAVE_FOLDER "tmpfile", STATS_FILE);
}

/* update players, write the update msg and update file, returns updated plyr */
Player *
update_players(char *buf, size_t siz, Player *new_player)
{
	Player *player;
	size_t s = 0;

	player = find_player(new_player->userid);
	s += write_quote(buf + s, siz - s);
	if (player == NULL) { /* new player */
		player = memdup(new_player, sizeof(*new_player));
		if (player == NULL)
			die(1, "memdup:");
		player->next = player_head;
		player_head = player;
		s += snprintf(buf + s, siz - s,
		              "**%s** has been registrated in the database.\n",
		              player->name);
		write_info(buf + s, siz - s, player);
	} else {
		update_player(buf + s, siz - s, player, new_player);
	}

	update_file(player);

	return player;
}

void
stats(char *buf, size_t siz, char *url, char *username, u64snowflake userid,
      u64snowflake guild_id, struct discord *client)
{
	CURLcode code;
	unsigned int i;
	char *txt, fname[128];
	Player local_player, *player;

	/* not always a jpg but idc */
	snprintf(fname, sizeof(fname), "%s/%lu.jpg", IMAGES_FOLDER, userid);
	if ((code = curl_file(url, fname)) != 0) {
		log_error("curl failed CURLcode: %s [%u]",
		          curl_easy_strerror(code), code);
		strlcpy(buf, "Error: Failed to download image", siz);
		return;
	}
	if ((txt = ocr(fname, "eng")) == NULL) {
		log_error("failed to read image from %s", username);
		strlcpy(buf, "Error: Failed to read image", siz);
		return;
	}

	memset(&local_player, 0, sizeof(local_player));
	strlcpy(local_player.name, username, MAX_USERNAME_SIZ);
	local_player.userid = userid;
	local_player.update = time(NULL);
	for_line(&local_player, txt);
	free(txt);

	/* detect wrong images */
	i = 2;
	while (U32CAST(&local_player)[i] == 0 && i++ < LENGTH(fields) - 2);
	if (i == LENGTH(fields) - 2)
		return;

	if (local_player.kingdom[0] == 0)
		strlcpy(local_player.kingdom, "(null)", MAX_KINGDOM_SIZ);

	pthread_mutex_lock(&player_mutex);
	player = update_players(buf, siz, &local_player);
#ifdef DEVEL
	UNUSED(guild_id);
	UNUSED(client);
	UNUSED(player);
#else
	if (guild_id == ROLE_GUILD_ID)
		update_roles(client, player);
#endif /* DEVEL */
	pthread_mutex_unlock(&player_mutex);
}

void
on_stats(struct discord *client, const struct discord_message *ev)
{
	char buf[MAX_MESSAGE_LEN] = "";

	log_info("%s", __func__);
	stats(buf,
	      sizeof(buf),
	      ev->attachments->array[0].url,
	      ev->author->username,
	      ev->author->id,
	      ev->guild_id,
	      client);

	discord_send_message(client, ev->channel_id, "%s", buf);
}

void
on_stats_interaction(struct discord *client,
                     const struct discord_interaction *ev)
{
	char buf[MAX_MESSAGE_LEN] = "", *url, *endurl;
	json_char *attachment;

	log_info("%s", __func__);
	attachment = xstrdup(ev->data->resolved->attachments);
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
	      ev->member->user->username,
	      ev->member->user->id,
	      ev->guild_id,
	      client);
	free(attachment);

	discord_send_interaction_message(client, ev->id, ev->token, "%s", buf);
}

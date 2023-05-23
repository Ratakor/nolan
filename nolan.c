#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <leptonica/allheaders.h>
#include <tesseract/capi.h>
#include <curl/curl.h>
#include <concord/discord.h>

#include "config.h"

#define MAX       300
#define NKINGDOM  sizeof(kingdoms) / sizeof(kingdoms[0])
#define NFIELDS   22

typedef struct {
	char *name;
	char *level;
	char *ascension;
	char *kingdom;
	char *global;
	char *regional;
	char *competitive;
	char *playtime;
	char *monsters;
	char *bosses;
	char *players;
	char *quests;
	char *explored;
	char *taken;
	char *dungeons;
	char *coliseum;
	char *items;
	char *fish;
	char *distance;
	char *reputation;
	char *endless;
	char *codex;
	char *bufptr;
} Player;

void die(const char *errstr);
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
void dlimg(char *url);
char *extract_txt_from_img(void);
void initplayer(Player *player);
Player createplayerfromtsv(int line);
void initplayers(void);
void updateplayers(Player *player);
char *trim(char *start, char *end, char *line);
void parseline(Player *player, char *line);
void forline(Player *player, char *src);
void createtsv(void);
int playerinfile(Player *player, int col);
void savetotsv(Player *player);
void loadtsv(char *src);
void on_ready(struct discord *client, const struct discord_ready *event);
void on_stats(struct discord *client, const struct discord_message *event);
void on_raids(struct discord *client, const struct discord_message *event);
void on_message(struct discord *client, const struct discord_message *event);
void on_sourcetxt(struct discord *client, const struct discord_message *event);
void on_source(struct discord *client, const struct discord_message *event);
void on_leaderboard(struct discord *client, const struct discord_message *event);
void on_info(struct discord *client, const struct discord_message *event);
void on_help(struct discord *client, const struct discord_message *event);

static Player players[NKINGDOM * 50];
static int nplayers;
static const char *fields[] = {
	"Name",
	"Level",
	"Ascension",
	"Kingdom",
	"Global Rank",
	"Regional Rank",
	"Competitive Rank",
	"Playtime",
	"Monsters Slain",
	"Bosses Slain",
	"Players Defeated",
	"Quests Completed",
	"Areas Explored",
	"Areas Taken",
	"Dungeons Cleared",
	"Coliseum Wins",
	"Items Upgraded",
	"Fish Caught",
	"Distance Travelled",
	"Reputation",
	"Endless Record",
	"Entries Completed",
};

void
die(const char *errstr)
{
	fputs(errstr, stderr);
	exit(EXIT_FAILURE);
}

static size_t
write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}

void
dlimg(char *url)
{
	CURL *handle;
	FILE *f;

	curl_global_init(CURL_GLOBAL_ALL);

	handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);

	f = fopen("img", "wb");
	if (f) {

		curl_easy_setopt(handle, CURLOPT_WRITEDATA, f);
		curl_easy_perform(handle);
		fclose(f);
	}

	curl_easy_cleanup(handle);
	curl_global_cleanup();
}

char *
extract_txt_from_img(void)
{
	TessBaseAPI *handle;
	PIX *img;
	char *txt;

	if ((img = pixRead("img")) == NULL)
		die("Error reading image\n");

	handle = TessBaseAPICreate();
	if (TessBaseAPIInit3(handle, NULL, "eng") != 0)
		die("Error initialising tesseract\n");

	TessBaseAPISetImage2(handle, img);
	if (TessBaseAPIRecognize(handle, NULL) != 0)
		die("Error in tesseract recognition\n");

	if ((txt = TessBaseAPIGetUTF8Text(handle)) == NULL)
		die("Error getting text\n");

	TessBaseAPIEnd(handle);
	TessBaseAPIDelete(handle);
	pixDestroy(&img);

	return txt;
}

void
initplayer(Player *player)
{
	for (int i = 0; i < NFIELDS + 1; i++)
		*((char **)player + i) = NULL;
}

Player
createplayerfromtsv(int line)
{
	FILE *f;
	Player player;
	char *buf = malloc(MAX), *p, *val;
	int i = 0;

	if (line < 1)
		die("createplayerfromtsv: line < 1\n");

	if ((f = fopen("players.tsv", "r")) == NULL)
		die("createplayerfromtsv: cannot open players.tsv\n");

	while (i++ <= line && (p = fgets(buf, MAX, f)) != NULL);
	fclose(f);
	i = 0;
	val = p;
	player.bufptr = buf;

	while (i < NFIELDS - 1 && *p != '\0') {
		if (*p == '\t') {
			*p = '\0';
			*((char **)&player + i) = val;
			val = p + 1;
			i++;
		}
		p++;
	}

	if (i != NFIELDS - 1)
		die("createplayerfromtsv: i != NFIELDS - 1\n");

	*((char **)&player + i) = val;

	return player;
}

void
initplayers(void)
{
	FILE *f;
	char buf[MAX];

	if ((f = fopen("players.tsv", "r")) == NULL)
		die("initplayers: cannot open players.tsv\n");

	while (fgets(buf, MAX, f)) {
		nplayers++;
	}
	nplayers--;

	for (int i = 0; i < nplayers; i++)
		players[i] = createplayerfromtsv(i + 1);
}

void
updateplayers(Player *player)
{
	int i = 0;

	while (i < nplayers && strcmp(players[i].name, player->name) != 0)
		i++;
	if (i == nplayers)
		nplayers++;
	for (int j = 1; j < NFIELDS; j++)
		*((char **)&players[i] + j) = *((char **)player + j);
}

char *
trim(char *start, char *end, char *line)
{
	char *trimmed = line;

	if (start != NULL) {
		while (*start && (*trimmed++ == *start++));

		/* if (*start != 0) */
		/* 	die("trim: start was not found in line\n"); */
	}

	if (end != NULL) {
		char *endtrimmed = trimmed;
		char *endofend = end;

		while (*endtrimmed++);
		endtrimmed--;
		while (*endofend++);
		endofend--;

		while ((*endtrimmed-- == *endofend--) && (endofend != end));
		if ((endofend == end) && (*endtrimmed == *endofend ))
			*endtrimmed = 0;
		/* else */
		/* 	die("trim: end was not found in line\n"); */
	}

	return trimmed;
}

void
parseline(Player *player, char *line)
{
	if (strncmp(line, "LEVEL", 5) == 0) {
		player->level = trim("LEVEL ", NULL, line);
	} else if (strncmp(line, "ASCENSION LEVEL", 15) == 0) {
		player->ascension = trim("ASCENSION LEVEL ", NULL, line);
	} else if (strncmp(line, "KINGDOM", 7) == 0) {
		player->kingdom = trim("KINGDOM ", NULL, line);
	} else if (strncmp(line, "GLOBAL RANK", 11) == 0) {
		player->global = trim("GLOBAL RANK #", " @", line);
	} else if (strncmp(line, "REGIONAL RANK", 12) == 0) {
		player->regional = trim("REGIONAL RANK #", " @", line);
	} else if (strncmp(line, "COMPETITIVE RANK", 16) == 0) {
		player->competitive = trim("COMPETITIVE RANK #", " @", line);
	} else if (strncmp(line, "PLAYTIME", 8) == 0) {
		player->playtime = trim("PLAYTIME ", NULL, line);
	} else if (strncmp(line, "MONSTERS SLAIN", 14) == 0) {
		player->monsters = trim("MONSTERS SLAIN ", " @", line);
	} else if (strncmp(line, "BOSSES SLAIN", 12) == 0) {
		player->bosses = trim("BOSSES SLAIN ", " @", line);
	} else if (strncmp(line, "PLAYERS DEFEATED", 16) == 0) {
		player->players = trim("PLAYERS DEFEATED ", " @", line);
	} else if (strncmp(line, "QUESTS COMPLETED", 16) == 0) {
		player->quests = trim("QUESTS COMPLETED ", " @", line);
	} else if (strncmp(line, "AREAS EXPLORED", 14) == 0) {
		player->explored = trim("AREAS EXPLORED ", " @", line);
	} else if (strncmp(line, "AREAS TAKEN", 11) == 0) {
		player->taken = trim("AREAS TAKEN ", " @", line);
	} else if (strncmp(line, "DUNGEONS CLEARED", 16) == 0) {
		player->dungeons = trim("DUNGEONS CLEARED ", " @", line);
	} else if (strncmp(line, "COLISEUM WINS", 13) == 0) {
		player->coliseum = trim("COLISEUM WINS ", " @", line);
	} else if (strncmp(line, "ITEMS UPGRADED", 14) == 0) {
		player->items = trim("ITEMS UPGRADED ", " @", line);
	} else if (strncmp(line, "FISH CAUGHT", 11) == 0) {
		player->fish = trim("FISH CAUGHT ", " @", line);
	} else if (strncmp(line, "DISTANCE TRAVELLED", 18) == 0) {
		player->distance = trim("DISTANCE TRAVELLED ", NULL, line);
	} else if (strncmp(line, "REPUTATION", 10) == 0) {
		player->reputation = trim("REPUTATION ", " @", line);
	} else if (strncmp(line, "ENDLESS RECORD", 14) == 0) {
		player->endless = trim("ENDLESS RECORD Floor ", " @", line);
	} else if (strncmp(line, "ENTRIES COMPLETED", 17) == 0) {
		player->codex = trim("ENTRIES COMPLETED ", " @", line);
	}
}

void
forline(Player *player, char *src)
{
	char *endline, *p = src;

	while (p) {
		endline = strchr(p, '\n');
		if (endline)
			*endline = 0;
		parseline(player, p);
		/* if (endline) */
		/* 	*endline = '\n'; */
		p = endline ? (endline + 1) : NULL;
	}
}

/*
void
forlineraid(char *src)
{
	char *endline, *p = src;

	while (p && strcmp(p, "+ Raid options") != 0) {
		endline = strchr(p, '\n');
		if (endline)
			*endline = 0;
		p = endline ? (endline + 1) : NULL;
	}

	src = p; // endline + 1;
}
*/

void
createtsv(void)
{
	FILE *f;
	int size = 0;

	f = fopen("players.tsv", "r");

	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		size = ftell(f);
	}

	if (size == 0 && (f = fopen("players.tsv", "w")) != NULL) {
		for (int i = 0; i < NFIELDS - 1; i++)
			fprintf(f, "%s\t", fields[i]);
		fprintf(f, "%s\n", fields[NFIELDS - 1]);
	}

	fclose(f);
}

int
playerinfile(Player *player, int col)
{
	FILE *f;
	char buf[MAX], *p, *endname;
	int line = 1;

	if ((f = fopen("players.tsv", "r")) == NULL)
		die("saveplayer: cannot open players.tsv\n");

	while ((p = fgets(buf, MAX, f)) != NULL) {
		endname = strchr(p, '\t');
		if (--endname)
			*endname = 0;
		if (strcmp(player->name, p) == 0) {
			if (endname)
				*endname = '\t';
			break;
		}
		if (endname)
			*endname = '\t';
		line++;
	}

	fclose(f);
	return line;
}

void
savetotsv(Player *player)
{
	FILE *fw, *fr;
	int pos, c, cpt = 0;
	int edited = 0;

	pos = playerinfile(player, 1);

	if ((fr = fopen("players.tsv", "r")) == NULL)
		die("savetotsv: cannot open players.tsv (read)\n");
	if ((fw = fopen("tmpfile", "w")) == NULL)
		die("savetotsv: cannot open players.tsv (write)\n");

	while ((c = fgetc(fr)) != EOF) {
		if (c == '\n')
			cpt++;
		if (!edited && cpt == pos - 1) {
			if (cpt == 0)
				fprintf(fw, "%s\t", player->name);
			else
				fprintf(fw, "\n%s\t", player->name);

			fprintf(fw, "%s\t", player->level);
			fprintf(fw, "%s\t", player->ascension);
			fprintf(fw, "%s\t", player->kingdom);
			fprintf(fw, "%s\t", player->global);
			fprintf(fw, "%s\t", player->regional);
			fprintf(fw, "%s\t", player->competitive);
			fprintf(fw, "%s\t", player->playtime);
			fprintf(fw, "%s\t", player->monsters);
			fprintf(fw, "%s\t", player->bosses);
			fprintf(fw, "%s\t", player->players);
			fprintf(fw, "%s\t", player->quests);
			fprintf(fw, "%s\t", player->explored);
			fprintf(fw, "%s\t", player->taken);
			fprintf(fw, "%s\t", player->dungeons);
			fprintf(fw, "%s\t", player->coliseum);
			fprintf(fw, "%s\t", player->items);
			fprintf(fw, "%s\t", player->fish);
			fprintf(fw, "%s\t", player->distance);
			fprintf(fw, "%s\t", player->reputation);
			fprintf(fw, "%s\t", player->endless);
			fprintf(fw, "%s\n", player->codex);

			edited = 1;

			while ((c = fgetc(fr)) != EOF) {
				if (c == '\n')
					break;
			}
		} else
			fprintf(fw, "%c", c);
	}

	fclose(fr);
	fclose(fw);

	remove("players.tsv");
	rename("tmpfile", "players.tsv");
}

void
loadtsv(char *src)
{
	FILE *f;
	char *p = src, c;

	if ((f = fopen("players.tsv", "r")) == NULL)
		die("loadtsv: cannot open players.tsv\n");

	while ((c = fgetc(f)) != EOF && (*p++ = c));
	*p = 0;
	fclose(f);
}

void
on_ready(struct discord *client, const struct discord_ready *event)
{
	struct discord_activity activities[] = {
		{
			.name = "?help",
			.type = DISCORD_ACTIVITY_LISTENING,
		},
	};

	struct discord_presence_update status = {
		.activities =
		        &(struct discord_activities)
		{
			.size = sizeof(activities) / sizeof * activities,
			.array = activities,
		},
		.status = "online",
		.afk = false,
		.since = discord_timestamp(client),
	};

	discord_update_presence(client, &status);
}

void
on_stats(struct discord *client, const struct discord_message *event)
{
	if (event->attachments->size == 0)
		return;
	if (strchr(event->attachments->array->filename, '.') == NULL)
		return;
	if (strncmp(event->attachments->array->content_type, "image", 5) != 0)
		return;

	dlimg(event->attachments->array->url);
	char *txt = extract_txt_from_img();

	if (txt == NULL)
		return;

	Player player;

	initplayer(&player);
	player.name = event->author->username;
	forline(&player, txt);

	if (kingdom_verification) {
		int i = NKINGDOM;

		while (i > 0 && strcmp(player.kingdom, kingdoms[i++]) != 0);

		if (i == 0) {
			struct discord_create_message msg = {
				.content = "Sorry you're not part of the kingdom :/"
			};
			discord_create_message(client, event->channel_id, &msg, NULL);
		} else {
			savetotsv(&player);
			updateplayers(&player);
		}
	} else {
		savetotsv(&player);
		updateplayers(&player);
	}

	free(player.bufptr); /* bufptr should always be NULL */
}

void
on_raids(struct discord *client, const struct discord_message *event)
{
	if (event->attachments->size == 0)
		return;
	if (strchr(event->attachments->array->filename, '.') == NULL)
		return;
	if (strncmp(event->attachments->array->content_type, "image", 5) != 0)
		return;

	dlimg(event->attachments->array->url);
	char *txt = extract_txt_from_img();

	if (txt == NULL)
		return;

	/* TODO */
	return;
	/* forlineraid(txt); */

	struct discord_create_message msg = {
		.content = txt
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}


void
on_message(struct discord *client, const struct discord_message *event)
{
	if (event->channel_id == STATS_ID) {
		on_stats(client, event);
	} else if (event->channel_id == RAIDS_ID) {
		on_raids(client, event);
	} else if (event->channel_id == TEST_ID) {
		on_stats(client, event);
	}
}

void
on_sourcetxt(struct discord *client, const struct discord_message *event)
{
	char *src = malloc(16384);
	loadtsv(src);
	struct discord_create_message msg = {
		.content = src
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
	free(src);
}

void
on_source(struct discord *client, const struct discord_message *event)
{
	char *src = malloc(16384);
	loadtsv(src);
	struct discord_attachment attachment = {
		.content = src,
		.filename = "players.tsv"
	};
	struct discord_attachments attachments = {
		.size = 1,
		.array = &attachment
	};
	struct discord_create_message msg = {
		.attachments = &attachments
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
	free(src);
}


void
on_leaderboard(struct discord *client, const struct discord_message *event)
{
	if (strlen(event->content) == 0) {
		struct discord_create_message msg = {
			.content = "NO WRONG, YOU MUST USE AN ARGUMENT"
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		return;
	}

	int i = 0;
	char *src;

	while (i < NFIELDS && strcmp(fields[i], event->content) != 0)
		i++;
	if (i == NFIELDS) {
		struct discord_create_message msg = {
			.content = "This is not a valid category"
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		return;
	}

	src = malloc(1024);
	*src = 0;

	/* TODO */

	struct discord_create_message msg = {
		.content = "DOESN'T WORK"
	};
	discord_create_message(client, event->channel_id, &msg, NULL);

	free(src);
}

void
on_info(struct discord *client, const struct discord_message *event)
{
	if (strlen(event->content) == 0) {
		struct discord_create_message msg = {
			.content = "NO WRONG, YOU MUST USE AN ARGUMENT"
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		return;
	}

	int i = 0;
	char *src;

	while (i < nplayers && strcmp(players[i].name, event->content) != 0)
		i++;
	if (i == nplayers) {
		struct discord_create_message msg = {
			.content = "This player does not exist in the database"
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		return;
	}

	src = malloc(1024);
	*src = 0;

	for (int j = 0; j < NFIELDS; j++) {
		strcat(src, fields[j]);
		strcat(src, ": ");
		strcat(src, *((char **)&players[i] + j));
		strcat(src, "\n");
	}

	struct discord_create_message msg = {
		.content = src
	};
	discord_create_message(client, event->channel_id, &msg, NULL);

	free(src);
}

void
on_help(struct discord *client, const struct discord_message *event)
{
	struct discord_create_message msg = {
		.content = "Post image to stat-bot to enter the database.\n\
Commands:\n\
	?sourcetxt or ?srctxt\n\
	?source or ?src\n\
	?info\n\
	?leaderboard or ?lb (not implemented yet)\n\
	?help\n\n\
Ask Ratakor to know what they do."
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

int
main(void)
{
	ccord_global_init();
	struct discord *client = discord_config_init("config.json");

	discord_add_intents(client, DISCORD_GATEWAY_MESSAGE_CONTENT);
	discord_set_on_ready(client, &on_ready);
	discord_set_on_message_create(client, &on_message);
	discord_set_on_command(client, "?srctxt", &on_sourcetxt);
	discord_set_on_command(client, "?sourcetxt", &on_sourcetxt);
	discord_set_on_command(client, "?src", &on_source);
	discord_set_on_command(client, "?source", &on_source);
	/* discord_set_on_command(client, "?leaderboard", &on_leaderboard); */
	/* discord_set_on_command(client, "?lb", &on_leaderboard); */
	discord_set_on_command(client, "?info", &on_info);
	discord_set_on_command(client, "?help", &on_help);

	createtsv();
	initplayers();

	discord_run(client);

	discord_cleanup(client);
	ccord_global_cleanup();

	return EXIT_SUCCESS;
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <leptonica/allheaders.h>
#include <tesseract/capi.h>
#include <curl/curl.h>
#include <concord/discord.h>

#include "config.h"

#define MAX         300
#define NKINGDOM    sizeof(kingdoms) / sizeof(kingdoms[0])
#define MAX_PLAYERS NKINGDOM * 50
#define NFIELDS     24

/* ALL FIELDS MUST HAVE THE SAME SIZE */
typedef struct {
	char *name;
	char *kingdom;
	long level;
	long ascension;
	long global;
	long regional;
	long competitive;
	long playtime;
	long monsters;
	long bosses;
	long players;
	long quests;
	long explored;
	long taken;
	long dungeons;
	long coliseum;
	long items;
	long fish;
	long distance;
	long reputation;
	long endless;
	long codex;
	u64snowflake userid;
	char *bufptr;
} Player;

void die(const char *errstr);
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
void dlimg(char *url);
char *extract_txt_from_img(void);
Player loadplayerfromfile(int line);
void initplayers(void);
void updateplayers(Player *player);
long playtimetolong(char *playtime);
char *playtimetostr(long playtime);
void trimall(char *str);
void parseline(Player *player, char *line);
void forline(Player *player, char *src);
void createtsv(void);
int playerinfile(Player *player);
void savetotsv(Player *player);
void loadtsv(char *src);
u64snowflake useridtoul(char *id);
void on_ready(struct discord *client, const struct discord_ready *event);
void on_stats(struct discord *client, const struct discord_message *event);
void on_raids(struct discord *client, const struct discord_message *event);
void on_message(struct discord *client, const struct discord_message *event);
void on_sourcetxt(struct discord *client, const struct discord_message *event);
void on_source(struct discord *client, const struct discord_message *event);
void on_leaderboard(struct discord *client, const struct discord_message *event);
void on_info(struct discord *client, const struct discord_message *event);
void on_help(struct discord *client, const struct discord_message *event);

static Player players[MAX_PLAYERS];
static int nplayers = 0;
static const char *fields[] = {
	"Name",
	"Kingdom",
	"Level",
	"Ascension",
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
	"User ID",
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

Player
loadplayerfromfile(int line)
{
	FILE *f;
	Player player;
	char *buf = malloc(MAX), *p, *val;
	int i = 0;

	if (line < 1)
		die("loadplayerfromfile: line < 1\n"); /* description line */

	if ((f = fopen("players.tsv", "r")) == NULL)
		die("loadplayerfromfile: cannot open players.tsv\n");

	while (i++ <= line && (p = fgets(buf, MAX, f)) != NULL);
	fclose(f);
	i = 0;
	val = p;
	player.bufptr = buf;

	/* -2 because the last field in tsv finish with a '\n'
	   and bufptr is not in the tsv */
	while (i < NFIELDS - 2 && *p != '\0') {
		if (*p == '\t') {
			*p = '\0';
			if (i <= 1) /* name and kingdom */
				((char **)&player)[i] = val;
			else
				((long *)&player)[i] = atol(val);
			val = p + 1;
			i++;
		}
		p++;
	}
	if (i != NFIELDS - 2)
		die("loadplayerfromfile: a player is missing a field\n");
	player.userid = strtoul(val, NULL, 10);

	return player;
}

void
initplayers(void)
{
	FILE *f;
	char buf[MAX];

	if ((f = fopen("players.tsv", "r")) == NULL)
		die("initplayers: cannot open players.tsv\n");

	while (fgets(buf, MAX, f))
		nplayers++;
	nplayers--; /* first line is not a player */

	for (int i = 0; i < nplayers; i++)
		players[i] = loadplayerfromfile(i + 1);
}

void
updateplayers(Player *player)
{
	int i = 0, j;

	while (i < nplayers && strcmp(players[i].name, player->name) != 0)
		i++;
	if (i == nplayers) {
		nplayers++;
		players[nplayers - 1] = loadplayerfromfile(nplayers);
	} else {
		/*
		 * j = 1 because we want to keep the original username and
		 * NFIELDS - 2 because we want to keep the original userid and
		 * bufptr
		 */
		for (j = 1; j < NFIELDS - 2; j++)
			((void **)&players[i])[j] = ((void **)player)[j];
	}
}

long
playtimetolong(char *playtime)
{
	char *str = "days, ";
	int days, hours;

	/* we assume that each players have played for at least 1 day */
	days = atol(playtime);
	playtime = strchr(playtime, 'd');
	while (*str && (*playtime++ == *str++));
	hours = atol(playtime);

	return days * 24 + hours;
}

char *
playtimetostr(long playtime)
{
	int days, hours;
	char *buf;

	days = playtime / 24;
	hours = playtime % 24;
	buf = malloc(20);
	/* we assume that each players have played for at least 1 day */
	if (hours == 0)
		sprintf(buf, "%'d days", days);
	else
		sprintf(buf, "%'d days, %'d hours", days, hours);

	return buf;
}

/* trim everything that is not a number or a left parenthesis */
void
trimall(char *str)
{
	const char *r = str;
	char *w = str;

	do {
		if ((*r >= 48 && *r <= 57) || *r == '(')
			*w++ = *r;
	} while (*r++);

	*w = 0;
}

void
parseline(Player *player, char *line)
{
	char *str;

	if (strncmp(line, "KINGDOM", 7) == 0) {
		str = "KINGDOM ";
		while (*str && (*line++ == *str++));
		player->kingdom = line;
		return;
	}

	if (strncmp(line, "PLAYTIME", 8) == 0) {
		str = "PLAYTIME ";
		while (*str && (*line++ == *str++));
		player->playtime = playtimetolong(line);
		return;
	}

	if (strncmp(line, "LEVEL", 5) == 0) {
		trimall(line);
		player->level = atol(line);
	} else if (strncmp(line, "ASCENSION LEVEL", 15) == 0) {
		trimall(line);
		player->ascension = atol(line);
	} else if (strncmp(line, "GLOBAL RANK", 11) == 0) {
		trimall(line);
		player->global = atol(line);
	} else if (strncmp(line, "REGIONAL RANK", 12) == 0) {
		trimall(line);
		player->regional = atol(line);
	} else if (strncmp(line, "COMPETITIVE RANK", 16) == 0) {
		trimall(line);
		player->competitive = atol(line);
	} else if (strncmp(line, "MONSTERS SLAIN", 14) == 0) {
		trimall(line);
		player->monsters = atol(line);
	} else if (strncmp(line, "BOSSES SLAIN", 12) == 0) {
		trimall(line);
		player->bosses = atol(line);
	} else if (strncmp(line, "PLAYERS DEFEATED", 16) == 0) {
		trimall(line);
		player->players = atol(line);
	} else if (strncmp(line, "QUESTS COMPLETED", 16) == 0) {
		trimall(line);
		player->quests = atol(line);
	} else if (strncmp(line, "AREAS EXPLORED", 14) == 0) {
		trimall(line);
		player->explored = atol(line);
	} else if (strncmp(line, "AREAS TAKEN", 11) == 0) {
		trimall(line);
		player->taken = atol(line);
	} else if (strncmp(line, "DUNGEONS CLEARED", 16) == 0) {
		trimall(line);
		player->dungeons = atol(line);
	} else if (strncmp(line, "COLISEUM WINS", 13) == 0) {
		trimall(line);
		player->coliseum = atol(line);
	} else if (strncmp(line, "ITEMS UPGRADED", 14) == 0) {
		trimall(line);
		player->items = atol(line);
	} else if (strncmp(line, "FISH CAUGHT", 11) == 0) {
		trimall(line);
		player->fish = atol(line);
	} else if (strncmp(line, "DISTANCE TRAVELLED", 18) == 0) {
		trimall(line);
		player->distance = atol(line);
	} else if (strncmp(line, "REPUTATION", 10) == 0) {
		trimall(line);
		player->reputation = atol(line);
	} else if (strncmp(line, "ENDLESS RECORD", 14) == 0) {
		trimall(line);
		player->endless = atol(line);
	} else if (strncmp(line, "ENTRIES COMPLETED", 17) == 0) {
		trimall(line);
		player->codex = atol(line);
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
		p = endline ? (endline + 1) : 0;
	}
}

void
createtsv(void)
{
	FILE *f;
	int i, size = 0;

	f = fopen("players.tsv", "r");

	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		size = ftell(f);
	}

	/* -2 because bufptr is not in the tsv */
	if (size == 0 && (f = fopen("players.tsv", "w")) != NULL) {
		for (i = 0; i < NFIELDS - 2; i++)
			fprintf(f, "%s\t", fields[i]);
		fprintf(f, "%s\n", fields[NFIELDS - 2]);
	}

	fclose(f);
}

int
playerinfile(Player *player)
{
	FILE *f;
	char buf[MAX], *p, *endname;
	int line = 1;

	if ((f = fopen("players.tsv", "r")) == NULL)
		die("playerinfile: cannot open players.tsv\n");

	while ((p = fgets(buf, MAX, f)) != NULL) {
		endname = strchr(p, '\t');
		if (endname)
			*endname = 0;
		if (strcmp(player->name, p) == 0)
			break;
		line++;
	}

	fclose(f);
	return line;
}

void
savetotsv(Player *player)
{
	FILE *w, *r;
	int pos, c, cpt = 0;
	int edited = 0;

	pos = playerinfile(player);

	if ((r = fopen("players.tsv", "r")) == NULL)
		die("savetotsv: cannot open players.tsv (read)\n");
	if ((w = fopen("tmpfile", "w")) == NULL)
		die("savetotsv: cannot open players.tsv (write)\n");

	while ((c = fgetc(r)) != EOF) {
		if (c == '\n')
			cpt++;
		if (!edited && cpt == pos - 1) {
			if (cpt == 0)
				fprintf(w, "%s\t", player->name);
			else
				fprintf(w, "\n%s\t", player->name);

			fprintf(w, "%s\t", player->kingdom);
			fprintf(w, "%ld\t", player->level);
			fprintf(w, "%ld\t", player->ascension);
			fprintf(w, "%ld\t", player->global);
			fprintf(w, "%ld\t", player->regional);
			fprintf(w, "%ld\t", player->competitive);
			fprintf(w, "%ld\t", player->playtime);
			fprintf(w, "%ld\t", player->monsters);
			fprintf(w, "%ld\t", player->bosses);
			fprintf(w, "%ld\t", player->players);
			fprintf(w, "%ld\t", player->quests);
			fprintf(w, "%ld\t", player->explored);
			fprintf(w, "%ld\t", player->taken);
			fprintf(w, "%ld\t", player->dungeons);
			fprintf(w, "%ld\t", player->coliseum);
			fprintf(w, "%ld\t", player->items);
			fprintf(w, "%ld\t", player->fish);
			fprintf(w, "%ld\t", player->distance);
			fprintf(w, "%ld\t", player->reputation);
			fprintf(w, "%ld\t", player->endless);
			fprintf(w, "%ld\t", player->codex);
			fprintf(w, "%lu\n", player->userid);

			edited = 1;

			while ((c = fgetc(r)) != EOF) {
				if (c == '\n')
					break;
			}
		} else
			fprintf(w, "%c", c);
	}

	fclose(r);
	fclose(w);

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

u64snowflake
useridtoul(char *id)
{
	char *start = id, *end = strchr(id, 0);

	if (strncmp(start, "<@", 2) == 0 && strcmp(--end, ">") == 0) {
		start += 2;
		end = 0;

		return strtoul(start, NULL, 10);
	}

	return 0;
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
	int i;
	char *txt;
	Player player;

	if (event->attachments->size == 0)
		return;
	if (strchr(event->attachments->array->filename, '.') == NULL)
		return;
	if (strncmp(event->attachments->array->content_type, "image", 5) != 0)
		return;

	dlimg(event->attachments->array->url);
	txt = extract_txt_from_img();

	if (txt == NULL)
		return;

	memset(&player, 0, sizeof(player));
	player.name = event->author->username;
	player.userid = event->author->id;
	forline(&player, txt);

	if (kingdom_verification) {
		i = NKINGDOM;

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
	char *txt;

	if (event->attachments->size == 0)
		return;
	if (strchr(event->attachments->array->filename, '.') == NULL)
		return;
	if (strncmp(event->attachments->array->content_type, "image", 5) != 0)
		return;

	dlimg(event->attachments->array->url);
	txt = extract_txt_from_img();

	if (txt == NULL)
		return;

	/* TODO */
	/* "+ Raid options" */
	return;

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
on_source(struct discord *client, const struct discord_message *event)
{
	char *txt = malloc(MAX + MAX * MAX_PLAYERS);
	loadtsv(txt);
	struct discord_attachment attachment = {
		.content = txt,
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
	free(txt);
}


void
on_leaderboard(struct discord *client, const struct discord_message *event)
{
	if (strlen(event->content) == 0) {
		struct discord_create_message msg = {
			.content = "NO WRONG, YOU MUST USE AN ARGUMENT!"
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		return;
	}

	int i = 0;

	while (i < NFIELDS && strcmp(fields[i], event->content) != 0)
		i++;
	if (i == NFIELDS) {
		struct discord_create_message msg = {
			.content = "This is not a valid category."
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		return;
	}

	/* TODO */

	struct discord_create_message msg = {
		.content = "DOESN'T WORK!"
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

void
on_info(struct discord *client, const struct discord_message *event)
{
	if (strlen(event->content) == 0) {
		struct discord_create_message msg = {
			.content = "NO WRONG, YOU MUST USE AN ARGUMENT!"
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		return;
	}

	int i = 0, j;
	char *txt, *p;
	u64snowflake userid = useridtoul(event->content);

	if (userid > 0) {
		while (i < nplayers && players[i].userid != userid)
			i++;
	} else {
		while (i < nplayers && strcmp(players[i].name, event->content) != 0)
			i++;
	}

	if (i == nplayers) {
		struct discord_create_message msg = {
			.content = "This player does not exist in the database."
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		return;
	}

	txt = malloc(512);
	*txt = 0;

	for (j = 0; j < NFIELDS - 2; j++) {
		strcat(txt, fields[j]);
		strcat(txt, ": ");
		if (j <= 1) { /* name and kingdom */
			strcat(txt, ((char **)&players[i])[j]);
		} else if (j == 7) { /* playtime */
			p = playtimetostr(((long *)&players[i])[j]);
			strcat(txt, p);
			free(p);
		} else {
			p = strrchr(txt, ' ');
			sprintf(++p, "%'ld", ((long *)&players[i])[j]);

			if (j == 18) /* distance */
				strcat(txt, "m");
		}
		strcat(txt, "\n");
	}

	struct discord_create_message msg = {
		.content = txt
	};
	discord_create_message(client, event->channel_id, &msg, NULL);

	free(txt);
}

void
on_help(struct discord * client, const struct discord_message * event)
{
	char *txt = malloc(2001), *p;

	strcpy(txt, "Post image to <#");
	p = strchr(txt, '#');;
	sprintf(++p, "%lu", STATS_ID);
	strcat(txt, "> to enter the database.\n");
	strcat(txt, "Commands: \n");
	strcat(txt, "\t?source or ?src\n");
	strcat(txt, "\t?info\n");
	strcat(txt, "\t?leaderboard or ?lb (not implemented yet)\n");
	strcat(txt, "\t?help\n\n");
	strcat(txt, "Try them or ask Ratakor to know what they do.");

	struct discord_create_message msg = {
		.content = txt
	};
	discord_create_message(client, event->channel_id, &msg, NULL);

	free(txt);
}

int
main(void)
{
	setlocale(LC_NUMERIC, "");
	ccord_global_init();
	struct discord *client = discord_config_init("config.json");

	discord_add_intents(client, DISCORD_GATEWAY_MESSAGE_CONTENT);
	discord_set_on_ready(client, &on_ready);
	discord_set_on_message_create(client, &on_message);
	discord_set_on_command(client, "?src", &on_source);
	discord_set_on_command(client, "?source", &on_source);
	discord_set_on_command(client, "?leaderboard", &on_leaderboard);
	discord_set_on_command(client, "?lb", &on_leaderboard);
	discord_set_on_command(client, "?info", &on_info);
	discord_set_on_command(client, "?help", &on_help);

	createtsv();
	initplayers();

	discord_run(client);

	discord_cleanup(client);
	ccord_global_cleanup();

	return EXIT_SUCCESS;
}

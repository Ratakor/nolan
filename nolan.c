#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <sys/stat.h>
#include <leptonica/allheaders.h>
#include <tesseract/capi.h>
#include <curl/curl.h>
#include <concord/discord.h>

#include "util.h"
#include "config.h"

#define LINE_SIZE   300 + 1
#define LEN(X)      (sizeof X - 1)
#define MAX_PLAYERS LENGTH(kingdoms) * 50
#define ICON_URL    "https://orna.guide/static/orna/img/npcs/master_gnome.png"

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
} Player;

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
static void dlimg(char *url, char *fname);
static char *ocr(char *fname);
static Player loadplayer(unsigned int line);
static void initplayers(void);
static void updateplayers(Player *player);
static long playtimetolong(char *playtime, char *str);
static char *playtimetostr(long playtime);
static void trimall(char *str);
static void parseline(Player *player, char *line);
static void forline(Player *player, char *src);
static void createfile(void);
static int savetofile(Player *player);
static char *updatemsg(Player *player, int iplayer);
static char *loadfilekd(char *kingdom, size_t *fszp);
static int compare(const void *e1, const void *e2);
static char *playerinlb(int i);
static char *leaderboard(u64snowflake userid);
static char *invalidlb(void);
static u64snowflake useridtoul(char *id);
static void on_ready(struct discord *client, const struct discord_ready *event);
static void stats(struct discord *client, const struct discord_message *event);
static void raids(struct discord *client, const struct discord_message *event);
static void on_message(struct discord *client, const struct discord_message *event);
static void on_source(struct discord *client, const struct discord_message *event);
static void on_lb(struct discord *client, const struct discord_message *event);
static void on_info(struct discord *client, const struct discord_message *event);
static void on_help(struct discord *client, const struct discord_message *event);

static Player players[MAX_PLAYERS];
static int nplayers = 0;
static int category = 0;
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
	"Codex",
	"User ID",
};

static size_t
write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}

void
dlimg(char *url, char *fname)
{
	CURL *handle;
	FILE *fp;

	curl_global_init(CURL_GLOBAL_ALL);

	handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);

	if ((fp = fopen(fname, "wb")) == NULL)
		die("nolan: Failed to open %s\n", fname);

	curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);
	curl_easy_perform(handle);

	fclose(fp);
	curl_easy_cleanup(handle);
	curl_global_cleanup();
}

char *
ocr(char *fname)
{
	TessBaseAPI *handle;
	PIX *img;
	char *txt_ocr, *txt_out;

	if ((img = pixRead(fname)) == NULL)
		die("nolan: Error reading image\n");

	handle = TessBaseAPICreate();
	if (TessBaseAPIInit3(handle, NULL, "eng") != 0)
		die("nolan: Error initialising tesseract\n");

	TessBaseAPISetImage2(handle, img);
	if (TessBaseAPIRecognize(handle, NULL) != 0)
		die("nolan: Error in tesseract recognition\n");

	txt_ocr = TessBaseAPIGetUTF8Text(handle);
	txt_out = strdup(txt_ocr);

	TessDeleteText(txt_ocr);
	TessBaseAPIEnd(handle);
	TessBaseAPIDelete(handle);
	pixDestroy(&img);

	return txt_out;
}

Player
loadplayer(unsigned int line)
{
	FILE *fp;
	Player player;
	char buf[LINE_SIZE], *p = NULL, *end;
	unsigned int i = 0;

	if (line <= 1)
		die("nolan: Tried to load the description line as a player\n");
	if ((fp = fopen(FILENAME, "r")) == NULL)
		die("nolan: Failed to open %s (read)\n", FILENAME);

	while (i++ < line && (p = fgets(buf, LINE_SIZE, fp)) != NULL);
	fclose(fp);
	if (p == NULL)
		die("nolan: Line %d is not present in %s\n", line, FILENAME);

	player.name = malloc(32 + 1);
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

void
initplayers(void)
{
	FILE *fp;
	char buf[LINE_SIZE];
	int i;

	if ((fp = fopen(FILENAME, "r")) == NULL)
		die("nolan: Failed to open %s (read)\n", FILENAME);

	while (fgets(buf, LINE_SIZE, fp))
		nplayers++;
	nplayers--; /* first line is not a player */

	if (nplayers > MAX_PLAYERS)
		die("nolan: There is too much players to load (max:%d)\n",
		    MAX_PLAYERS);

	for (i = 0; i < nplayers; i++)
		players[i] = loadplayer(i + 2);
}

void
updateplayers(Player *player)
{
	int i = 0, j;

	/* while (i < nplayers && strcmp(players[i].name, player->name) != 0) */
	while (i < nplayers && players[i].userid != player->userid)
		i++;

	if (i == nplayers) { /* new player */
		if (nplayers > MAX_PLAYERS)
			die("nolan: There is too much players (max:%d)\n",
			    MAX_PLAYERS);
		players[nplayers] = loadplayer(nplayers + 2);
		nplayers++;
	} else {
		if (player->name)
			cpstr(players[i].name, player->name, 32 + 1);
		if (player->kingdom)
			cpstr(players[i].kingdom, player->kingdom, 32 + 1);
		/* keep original userid */
		for (j = 2; j < LENGTH(fields) - 1; j++)
			((long *)&players[i])[j] = ((long *)player)[j];
	}
}

long
playtimetolong(char *playtime, char str[])
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
playtimetostr(long playtime)
{
	long days, hours;
	char *buf;

	days = playtime / 24;
	hours = playtime % 24;
	buf = malloc(32);

	switch (hours) {
	case 0:
		if (days <= 1)
			snprintf(buf, 32, "%ld day", days);
		else
			snprintf(buf, 32, "%ld days", days);
		break;
	case 1:
		if (days == 0)
			snprintf(buf, 32, "%ld hour", hours);
		else if (days == 1)
			snprintf(buf, 32, "%ld day, %ld hour", days, hours);
		else
			snprintf(buf, 32, "%ld days, %ld hour", days, hours);
		break;
	default:
		if (days == 0)
			snprintf(buf, 32, "%ld hours", hours);
		else if (days == 1)
			snprintf(buf, 32, "%ld day, %ld hours", days, hours);
		else
			snprintf(buf, 32, "%ld days, %ld hours", days, hours);
		break;
	}

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

	*w = '\0';
}

void
parseline(Player *player, char *line)
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
		player->playtime = playtimetolong(line, "days, ");
		return;
	}
	if (strncmp(line, "TEMPS DE JEU", LEN("TEMPS DE JEU")) == 0) {
		str = "TEMPS DE JEU ";
		while (*str && (*line++ == *str++));
		player->playtime = playtimetolong(line, "jours, ");
		return;
	}

	if (strncmp(line, "ASCENSION LEVEL", LEN("ASCENSION LEVEL")) == 0 ||
	                strncmp(line, "NIVEAU D'ELEVATION", LEN("NIVEAU D'ELEVATION")) == 0) {
		trimall(line);
		player->ascension = atol(line);
	} else if (strncmp(line, "LEVEL", LEN("LEVEL")) == 0 ||
	                strncmp(line, "NIVEAU", LEN("NIVEAU")) == 0) {
		trimall(line);
		player->level = atol(line);
	} else if (strncmp(line, "GLOBAL RANK", LEN("GLOBAL RANK")) == 0 ||
	                strncmp(line, "RANG GLOBAL", LEN("RANG GLOBAL")) == 0) {
		trimall(line);
		player->global = atol(line);
	} else if (strncmp(line, "REGIONAL RANK", LEN("REGIONAL RANK")) == 0 ||
	                strncmp(line, "RANG REGIONAL", LEN("RANG REGIONAL")) == 0) {
		trimall(line);
		player->regional = atol(line);
	} else if (strncmp(line, "COMPETITIVE RANK", LEN("COMPETITIVE RANK")) == 0 ||
	                strncmp(line, "RANG COMPETITIF", LEN("RANG COMPETITIF")) == 0) {
		trimall(line);
		player->competitive = atol(line);
	} else if (strncmp(line, "MONSTERS SLAIN", LEN("MONSTERS SLAIN")) == 0 ||
	                strncmp(line, "MONSTRES TUES", LEN("MONSTRES TUES")) == 0) {
		trimall(line);
		player->monsters = atol(line);
	} else if (strncmp(line, "BOSSES SLAIN", LEN("BOSSES SLAIN")) == 0 ||
	                strncmp(line, "BOSS TUES", LEN("BOSS TUES")) == 0) {
		trimall(line);
		player->bosses = atol(line);
	} else if (strncmp(line, "PLAYERS DEFEATED", LEN("PLAYERS DEFEATED")) == 0 ||
	                strncmp(line, "JOUEURS VAINCUS", LEN("JOUEURS VAINCUS")) == 0) {
		trimall(line);
		player->players = atol(line);
	} else if (strncmp(line, "QUESTS COMPLETED", LEN("QUESTS COMPLETED")) == 0 ||
	                strncmp(line, "QUETES TERMINEES", LEN("QUETES TERMINEES")) == 0) {
		trimall(line);
		player->quests = atol(line);
	} else if (strncmp(line, "AREAS EXPLORED", LEN("AREAS EXPLORED")) == 0 ||
	                strncmp(line, "TERRES EXPLOREES", LEN("TERRES EXPLOREES")) == 0) {
		trimall(line);
		player->explored = atol(line);
	} else if (strncmp(line, "AREAS TAKEN", LEN("AREAS TAKEN")) == 0 ||
	                strncmp(line, "TERRES PRISES", LEN("TERRES PRISES")) == 0) {
		trimall(line);
		player->taken = atol(line);
	} else if (strncmp(line, "DUNGEONS CLEARED", LEN("DUNGEONS CLEARED")) == 0 ||
	                strncmp(line, "DONJONS TERMINES", LEN("DONJONS TERMINES")) == 0) {
		trimall(line);
		player->dungeons = atol(line);
	} else if (strncmp(line, "COLISEUM WINS", LEN("COLISEUM WINS")) == 0 ||
	                strncmp(line, "VICTOIRES DANS LE COLISEE", LEN("VICTOIRES DANS LE COLISEE")) == 0) {
		trimall(line);
		player->coliseum = atol(line);
	} else if (strncmp(line, "ITEMS UPGRADED", LEN("ITEMS UPGRADED")) == 0 ||
	                strncmp(line, "OBJETS AMELIORES", LEN("OBJETS AMELIORES")) == 0) {
		trimall(line);
		player->items = atol(line);
	} else if (strncmp(line, "FISH CAUGHT", LEN("FISH CAUGHT")) == 0 ||
	                strncmp(line, "POISSONS ATTRAPES", LEN("POISSONS ATTRAPES")) == 0) {
		trimall(line);
		player->fish = atol(line);
	} else if (strncmp(line, "DISTANCE TRAVELLED", LEN("DISTANCE TRAVELLED")) == 0 ||
	                strncmp(line, "DISTANCE VOYAGEE", LEN("DISTANCE VOYAGEE")) == 0) {
		trimall(line);
		player->distance = atol(line);
	} else if (strncmp(line, "REPUTATION", LEN("REPUTATION")) == 0) {
		trimall(line);
		player->reputation = atol(line);
	} else if (strncmp(line, "ENDLESS RECORD", LEN("ENDLESS RECORD")) == 0 ||
	                strncmp(line, "RECORD DU MODE SANS-FIN", LEN("RECORD DU MODE SANS-FIN")) == 0) {
		trimall(line);
		player->endless = atol(line);
	} else if (strncmp(line, "ENTRIES COMPLETED", LEN("ENTRIES COMPLETED")) == 0 ||
	                strncmp(line, "RECHERCHES TERMINEES", LEN("RECHERCHES TERMINEES")) == 0) {
		trimall(line);
		player->codex = atol(line);
	}
}

void
forline(Player *player, char *txt)
{
	char *line = txt, *endline;

	while (line) {
		endline = strchr(line, '\n');
		if (endline)
			*endline = '\0';
		parseline(player, line);
		line = endline ? (endline + 1) : 0;
	}
}

void
createfile(void)
{
	FILE *fp;
	int i, size = 0;

	fp = fopen(FILENAME, "r");

	if (fp != NULL) {
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
	}

	if (size == 0 && (fp = fopen(FILENAME, "w")) != NULL) {
		for (i = 0; i < LENGTH(fields) - 1; i++)
			fprintf(fp, "%s%c", fields[i], DELIM);
		fprintf(fp, "%s\n", fields[LENGTH(fields) - 1]);
	}

	fclose(fp);
}

/* Save player to file and return player's index in file if it was found */
int
savetofile(Player *player)
{
	FILE *w, *r;
	char buf[LINE_SIZE], *p, *endname;
	int iplayer = 0, cpt = 1, i;

	if ((r = fopen(FILENAME, "r")) == NULL)
		die("nolan: Failed to open %s (read)\n", FILENAME);
	if ((w = fopen("tmpfile", "w")) == NULL)
		die("nolan: Failed to open %s (write)\n", FILENAME);

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
	remove(FILENAME);
	rename("tmpfile", FILENAME);

	return iplayer;
}

char *
updatemsg(Player *player, int iplayer)
{
	int i;
	char *buf = malloc(2000 + 1), *p, *plto, *pltn, *pltd;
	long old, new, diff;

	sprintf(buf, "%s's profile has been updated.\n\n", player->name);
	return buf; /* FIXME: bug with iplayer or i + unsafe */
	p = strchr(buf, '\0');

	if (strcmp(players[iplayer].kingdom, player->kingdom) != 0) {
		sprintf(p, "%s: %s -> %s\n", fields[1],
		        players[iplayer].kingdom, player->kingdom);
		p = strchr(buf, '\0');
	}

	for (i = 2; i < LENGTH(fields) - 1; i++) {
		old = ((long *)&players[iplayer])[i];
		new = ((long *)player)[i];
		diff = new - old;
		if (diff == 0)
			continue;

		if (i == 7) { /* playtime */
			plto = playtimetostr(old);
			pltn = playtimetostr(new);
			pltd = playtimetostr(diff);
			sprintf(p, "%s: %s -> %s (+ %s)\n",
			        fields[7], plto, pltn, pltd);
			free(plto);
			free(pltn);
			free(pltd);
		} else {
			sprintf(p, "%s: %'ld -> %'ld (%'+ld)\n",
			        fields[i], old, new, diff);
		}
		p = strchr(buf, '\0');
	}

	/* TODO */
	/* sprintf(p, "Last update was xxx ago\n"); */

	return buf;
}

char *
loadfilekd(char *kingdom, size_t *fszp)
{
	FILE *fp;
	char *res, line[LINE_SIZE], *kd, *endkd;
	size_t mfsz = LINE_SIZE + nplayers * LINE_SIZE;

	if ((fp = fopen(FILENAME, "r")) == NULL)
		die("nolan: Failed to open %s (read)\n", FILENAME);

	res = malloc(mfsz);
	fgets(line, LINE_SIZE, fp); /* fields name */
	cpstr(res, line, mfsz);
	while (fgets(line, LINE_SIZE, fp) != NULL) {
		kd = strchr(line, DELIM) + 1;
		endkd = nstrchr(line, DELIM, 2);
		if (endkd)
			*endkd = '\0';
		if (strcmp(kd, kingdom) == 0) {
			*endkd = DELIM;
			catstr(res, line, mfsz);
		}
	}

	fclose(fp);
	*fszp = strlen(res);
	return res;
}

int
compare(const void *e1, const void *e2)
{
	const long l1 = ((long *)(Player *)e1)[category];
	const long l2 = ((long *)(Player *)e2)[category];

	/* ranks */
	if (category == 4 || category == 6) {
		if (l1 == 0)
			return 1;
		if (l2 == 0)
			return -1;
		return l1 - l2;
	}

	return l2 - l1;
}

char *
playerinlb(int i)
{
	size_t siz = 64, ssiz = 16;
	char *buf = malloc(siz), *plt, stat[ssiz];

	snprintf(buf, siz, "%d. %s: ", i + 1, players[i].name);
	if (category == 7) { /* playtime */
		plt = playtimetostr(((long *)&players[i])[category]);
		catstr(buf, plt, siz);
		free(plt);
	} else {
		snprintf(stat, ssiz, "%'ld", ((long *)&players[i])[category]);
		catstr(buf, stat, siz);
		if (i == 18) /* distance */
			catstr(buf, "m", siz);
	}
	catstr(buf, "\n", siz);

	return buf;
}

char *
leaderboard(u64snowflake userid)
{
	int i, lb_max, in_lb = 0;
	size_t siz;
	char *buf, *player;

	qsort(players, nplayers, sizeof(players[0]), compare);
	lb_max = (nplayers < LB_MAX) ? nplayers : LB_MAX;
	siz = (lb_max + 2) * 64;
	buf = malloc(siz);

	cpstr(buf, fields[category], siz);
	catstr(buf, ":\n", siz);
	for (i = 0; i < lb_max ; i++) {
		if (userid == players[i].userid)
			in_lb = 1;
		player = playerinlb(i);
		catstr(buf, player, siz);
		free(player);
	}

	if (!in_lb) {
		catstr(buf, "...\n", siz);
		i = lb_max;
		while (i < nplayers && players[i].userid != userid)
			i++;
		player = playerinlb(i);
		catstr(buf, player, siz);
		free(player);
	}

	return buf;
}

char *
invalidlb(void)
{
	int i;
	size_t siz = 512;
	char *buf = malloc(siz);

	cpstr(buf, "NO WRONG, this is not a valid category.\n", siz);
	catstr(buf, "Valid categories are:\n", siz);
	for (i = 2; i < LENGTH(fields) - 1; i++) {
		if (i == 5) /* regional rank */
			continue;
		catstr(buf, fields[i], siz);
		catstr(buf, "\n", siz);
	}

	return buf;
}

u64snowflake
useridtoul(char *id)
{
	char *start = id, *end = strchr(id, '\0') - 1;

	if (strncmp(start, "<@", 2) == 0 && strncmp(end, ">", 1) == 0)
		return strtoul(start + 2, NULL, 10);
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
stats(struct discord *client, const struct discord_message *event)
{
	int i, iplayer;
	char *txt, *fname = malloc(64);
	Player player;

	snprintf(fname, 64, "./images/%s.jpg", event->author->username);
	dlimg(event->attachments->array->url, fname);
	txt = ocr(fname);
	free(fname);

	if (txt == NULL) {
		struct discord_create_message msg = {
			.content = "Error: Failed to read image"
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		free(txt);
		return;
	}

	memset(&player, 0, sizeof(player));
	player.name = event->author->username;
	player.userid = event->author->id;
	forline(&player, txt);
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

	if ((iplayer = savetofile(&player))) {
		txt = updatemsg(&player, iplayer - 2);
	} else {
		txt = malloc(128);
		snprintf(txt, 128, "**%s** has been registrated in the database.",
		         player.name);
	}
	struct discord_create_message msg = {
		.content = txt
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
	updateplayers(&player);
	free(txt);
}

void
raids(struct discord *client, const struct discord_message *event)
{
	char *txt, *line, *endline;

	dlimg(event->attachments->array->url, "./images/raids.jpg");
	txt = ocr("./images/raids.jpg");

	/* TODO */
	line = txt;
	while (line) {
		endline = strchr(line, '\n');
		if (endline)
			*endline = '\0';
		if (strncmp(line, "+ Raid options", 14) == 0) {
			line = endline + 1;
			break;
		}
		line = endline ? (endline + 1) : 0;
	}

	struct discord_create_message msg = {
		.content = line
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
	free(txt);
}


void
on_message(struct discord *client, const struct discord_message *event)
{
	int i;

	if (event->author->bot)
		return;
	if (event->attachments->size == 0)
		return;
	if (strchr(event->attachments->array->filename, '.') == NULL)
		return;
	if (strncmp(event->attachments->array->content_type, "image", 5) != 0)
		return;

#ifdef DEVEL
	if (event->channel_id == DEVEL)
		raids(client, event);
	return;
#endif

	for (i = 0; i < (int)LENGTH(stats_ids); i++) {
		if (event->channel_id == stats_ids[i]) {
			stats(client, event);
			break;
		}
	}

	if (event->channel_id == RAIDS_ID)
		raids(client, event);
}

void
on_source(struct discord *client, const struct discord_message *event)
{
	size_t fsize = 0;
	char *fbuf = NULL;

	if (event->author->bot)
		return;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif

	if (strlen(event->content) == 0)
		fbuf = cog_load_whole_file(FILENAME, &fsize);
	else
		fbuf = loadfilekd(event->content, &fsize);

	struct discord_attachment attachment = {
		.filename = FILENAME,
		.content = fbuf,
		.size = fsize
	};
	struct discord_attachments attachments = {
		.size = 1,
		.array = &attachment
	};
	struct discord_create_message msg = {
		.attachments = &attachments
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
	free(fbuf);
}


void
on_lb(struct discord *client, const struct discord_message *event)
{
	int i = 2; /* ignore name and kingdom */
	char *txt;

	if (event->author->bot)
		return;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif

	if (strlen(event->content) == 0) {
		txt = invalidlb();
		struct discord_create_message msg = {
			.content = txt
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		free(txt);
		return;
	}

	while (i < LENGTH(fields) - 1 && strcasecmp(fields[i], event->content) != 0)
		i++;

	if (i == LENGTH(fields) - 1 || i == 5) {
		txt = invalidlb();
		struct discord_create_message msg = {
			.content = txt
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		free(txt);
		return;
	}

	category = i;
	txt = leaderboard(event->author->id);
	struct discord_create_message msg = {
		.content = txt
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
	free(txt);
}

void
on_info(struct discord *client, const struct discord_message *event)
{
	int i = 0, j;
	size_t sz = 512;
	char buf[sz], *p;
	u64snowflake userid;

	if (event->author->bot)
		return;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif

	if (strlen(event->content) == 0)
		userid = event->author->id;
	else
		userid = useridtoul(event->content);

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
			           "\nTo check a player's info you can do "
			           "?info @username or ?info username.\n"
			           "To check your info you can just type ?info."
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		return;
	}

	if (use_embed) {
		struct discord_embed embed = {
			.color = 0x3498DB,
			.timestamp = discord_timestamp(client),
			.title = players[i].name,
		};
		discord_embed_set_footer(&embed, "Nolan", ICON_URL, NULL);
		discord_embed_add_field(
		        &embed, (char *)fields[1], players[i].kingdom, true);
		for (j = 2; j < LENGTH(fields) - 1; j++) {
			if (j == 7) { /* playtime */
				p = playtimetostr(((long *)&players[i])[j]);
				discord_embed_add_field(
				        &embed, (char *)fields[j], p, true);
				free(p);
			} else {
				sprintf(buf, "%'ld", ((long *)&players[i])[j]);
				if (j == 18) /* distance */
					strcat(buf, "m");
				discord_embed_add_field(
				        &embed, (char *)fields[j], buf, true);
			}
		}
		struct discord_create_message msg = {
			.embeds = &(struct discord_embeds)
			{
				.size = 1,
				.array = &embed,
			}
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		discord_embed_cleanup(&embed);
	} else {
		buf[0] = '\0';
		for (j = 0; j < LENGTH(fields) - 1; j++) {
			catstr(buf, fields[j], sz);
			catstr(buf, ": ", sz);
			if (j <= 1) { /* name and kingdom */
				catstr(buf, ((char **)&players[i])[j], sz);
			} else if (j == 7) { /* playtime */
				p = playtimetostr(((long *)&players[i])[j]);
				catstr(buf, p, sz);
				free(p);
			} else {
				p = strchr(buf, '\0');
				snprintf(p, sz, "%'ld", ((long *)&players[i])[j]);
				if (j == 18) /* distance */
					catstr(buf, "m", sz);
			}
			catstr(buf, "\n", sz);
		}
		struct discord_create_message msg = {
			.content = buf
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
	}
}

void
on_help(struct discord * client, const struct discord_message * event)
{
	char *txt, *p;
	int i, len = (int)LENGTH(stats_ids);
	size_t sz = 512;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif

	txt = malloc(sz);
	cpstr(txt, "Post a screenshot of your stats to ", sz);
	for (i = 0; i < len; i++) {
		p = strchr(txt, '\0');;
		snprintf(p, sz, "<#%lu> ", stats_ids[i]);
		if (i < len - 1)
			catstr(txt, "or ", sz);
	}
	catstr(txt, "to enter the database.\n", sz);
	/* TODO: add PREFIX */
	catstr(txt, "Commands:\n", sz);
	catstr(txt, "\t?info *[[@]user]*\n", sz);
	catstr(txt, "\t?leaderboard or ?lb *category*\n", sz);
	/* catstr(txt, "\t?correct [category] [corrected value]\n", sz); */
	catstr(txt, "\t?source or ?src [kingdom]\n", sz);
	catstr(txt, "\t?help\n\n", sz);
	catstr(txt, "[...] means optional.\n", sz);
	catstr(txt, "Try them or ask Ratakor to know what they do.", sz);

	struct discord_create_message msg = {
		.content = txt
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
	free(txt);
}

int
main(void)
{
	char *src[] = { "src", "source" };
	char *lb[] = { "lb", "leaderboard" };
	struct discord *client;

	setlocale(LC_NUMERIC, "");
	ccord_global_init();
	client = discord_init(TOKEN);

	discord_add_intents(client, DISCORD_GATEWAY_MESSAGE_CONTENT);
	discord_set_prefix(client, PREFIX);
	/* create_slash_commands(client); */
	discord_set_on_ready(client, on_ready);
	/* TODO: create and interaction for each command */
	/* discord_set_on_interaction_create(client, &on_interaction_create); */
	discord_set_on_message_create(client, on_message);
	discord_set_on_commands(client, lb, LENGTH(lb), on_lb);
	discord_set_on_command(client, "info", on_info);
	discord_set_on_commands(client, src, LENGTH(src), on_source);
	discord_set_on_command(client, "help", on_help);

	if (mkdir("./images/", 0755) == 1)
		die("nolan: Failed to create the images folder\n");
	createfile();
	initplayers();

	discord_run(client);

	discord_cleanup(client);
	ccord_global_cleanup();

	return EXIT_SUCCESS;
}

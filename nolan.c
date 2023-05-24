#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <sys/stat.h>
#include <leptonica/allheaders.h>
#include <tesseract/capi.h>
#include <curl/curl.h>
#include <concord/discord.h>

#include "config.h"

#define MAX         300
#define LENGTH(X)   (sizeof X / sizeof X[0])
#define LEN(X)      (sizeof X - 1)
#define MAX_PLAYERS LENGTH(kingdoms) * 50
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
void dlimg(char *url, char *fname);
char *extract_txt_from_img(char *fname);
Player loadplayerfromfile(int line);
void initplayers(void);
void updateplayers(Player *player);
long playtimetolong(char *playtime, char *str);
char *playtimetostr(long playtime);
void trimall(char *str);
void parseline(Player *player, char *line);
void forline(Player *player, char *src);
void createfile(void);
int savetofile(Player *player);
int compare(const void *e1, const void *e2);
void addplayertolb(char *buf, int i);
void leaderboard(char *buf, u64snowflake userid);
u64snowflake useridtoul(char *id);
void on_ready(struct discord *client, const struct discord_ready *event);
void stats(struct discord *client, const struct discord_message *event);
void raids(struct discord *client, const struct discord_message *event);
void on_message(struct discord *client, const struct discord_message *event);
void on_sourcetxt(struct discord *client, const struct discord_message *event);
void on_source(struct discord *client, const struct discord_message *event);
void on_leaderboard(struct discord *client, const struct discord_message *event);
void on_info(struct discord *client, const struct discord_message *event);
void on_help(struct discord *client, const struct discord_message *event);

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
dlimg(char *url, char *fname)
{
	CURL *handle;
	FILE *f;

	curl_global_init(CURL_GLOBAL_ALL);

	handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);

	f = fopen(fname, "wb");
	if (f) {

		curl_easy_setopt(handle, CURLOPT_WRITEDATA, f);
		curl_easy_perform(handle);
		fclose(f);
	}

	curl_easy_cleanup(handle);
	curl_global_cleanup();
}

char *
extract_txt_from_img(char *fname)
{
	TessBaseAPI *handle;
	PIX *img;
	char *txt;

	if ((img = pixRead(fname)) == NULL)
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
	char *buf = malloc(MAX), *p = NULL, *val;
	int i = 0;

	if (line < 1)
		die("loadplayerfromfile: line < 1\n"); /* description line */

	if ((f = fopen(FILENAME, "r")) == NULL)
		die("loadplayerfromfile: cannot open the save file\n");

	while (i++ <= line && (p = fgets(buf, MAX, f)) != NULL);
	fclose(f);

	if (p == NULL)
		die("loadplayerfromfile: player is not in the save file\n");

	i = 0;
	val = p;
	player.bufptr = buf;

	/*
	 * -2 because the last field in the file finish with a '\n'
	 * and bufptr is not in the file
	 */
	while (i < NFIELDS - 2 && *p != '\0') {
		if (*p == DELIM) {
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
	int i;

	if ((f = fopen(FILENAME, "r")) == NULL)
		die("initplayers: cannot open the save file\n");

	while (fgets(buf, MAX, f))
		nplayers++;
	nplayers--; /* first line is not a player */

	for (i = 0; i < nplayers; i++)
		players[i] = loadplayerfromfile(i + 1);
}

void
updateplayers(Player *player)
{
	int i = 0, j;

	while (i < nplayers && strcmp(players[i].name, player->name) != 0)
		i++;
	if (i == nplayers) { /* new player */
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
playtimetolong(char *playtime, char str[])
{
	int days, hours;

	days = atol(playtime);
	if ((playtime = strchr(playtime, str[0])) == 0)
		return days; /* less than a day of playtime */
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
createfile(void)
{
	FILE *f;
	int i, size = 0;

	f = fopen(FILENAME, "r");

	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		size = ftell(f);
	}

	if (size == 0 && (f = fopen(FILENAME, "w")) != NULL) {
		/* -2 because bufptr is not in the file */
		for (i = 0; i < NFIELDS - 2; i++)
			fprintf(f, "%s%c", fields[i], DELIM);
		fprintf(f, "%s\n", fields[NFIELDS - 2]);
	}

	fclose(f);
}

/* Save player to file and return 1 if player was already present */
int
savetofile(Player *player)
{
	FILE *w, *r;
	char buf[MAX], *p, *endname;
	int infile = 0, i;

	if ((r = fopen(FILENAME, "r")) == NULL)
		die("savetofile: cannot open the save file (read)\n");
	if ((w = fopen("tmpfile", "w")) == NULL)
		die("savetofile: cannot open the save file (write)\n");

	while ((p = fgets(buf, MAX, r)) != NULL) {
		endname = strchr(p, DELIM);
		if (endname)
			*endname = 0;
		if (strcmp(player->name, p) == 0) {
			infile = 1;
			fprintf(w, "%s%c", player->name, DELIM);
			fprintf(w, "%s%c", player->kingdom, DELIM);
			for (i = 2; i < NFIELDS - 2; i++)
				fprintf(w, "%ld%c", ((long *)player)[i], DELIM);
			fprintf(w, "%lu\n", player->userid);
		} else {
			if (endname)
				*endname = DELIM;
			fprintf(w, "%s", p);
		}
	}
	if (!infile) {
		fprintf(w, "%s%c", player->name, DELIM);
		fprintf(w, "%s%c", player->kingdom, DELIM);
		for (i = 2; i < NFIELDS - 2; i++)
			fprintf(w, "%ld%c", ((long *)player)[i], DELIM);
		fprintf(w, "%lu\n", player->userid);
	}

	fclose(r);
	fclose(w);
	remove(FILENAME);
	rename("tmpfile", FILENAME);

	return infile;
}

int
compare(const void *e1, const void *e2)
{
	const Player p1 = *((const Player *)e1);
	const Player p2 = *((const Player *)e2);
	if (category == 4 || category == 6) /* ranks */
		return ((long *)&p1)[category] - ((long *)&p2)[category];
	return ((long *)&p2)[category] - ((long *)&p1)[category];
}

void
addplayertolb(char *buf, int i)
{
	char *p = strchr(buf, '\0');
	sprintf(p, "%d. %s: ", i + 1, players[i].name);
	if (category == 7) { /* playtime */
		p = playtimetostr(((long *)&players[i])[category]);
		strcat(buf, p);
		free(p);
	} else {
		p = strchr(buf, '\0');
		sprintf(p, "%'ld", ((long *)&players[i])[category]);
		if (i == 18) /* distance */
			strcat(buf, "m");
	}
	strcat(buf, "\n");
}

void
leaderboard(char *buf, u64snowflake userid)
{
	int i, lb_max, in_lb = 0;

	qsort(players, nplayers, sizeof(*players), compare);
	lb_max = (nplayers < LB_MAX) ? nplayers : LB_MAX;

	sprintf(buf, "%s:\n", fields[category]);
	for (i = 0; i < lb_max ; i++) {
		if (userid == players[i].userid)
			in_lb = 1;
		addplayertolb(buf, i);
	}

	if (!in_lb) {
		strcat(buf, "...\n");
		i = lb_max;
		while (i < nplayers && players[i].userid != userid)
			i++;
		addplayertolb(buf, i);
	}
}

u64snowflake
useridtoul(char *id)
{
	char *start = id, *end = strchr(id, 0) - 1;

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
	int i;
	char *txt, *fname;
	Player player;

	if (event->attachments->size == 0)
		return;
	if (strchr(event->attachments->array->filename, '.') == NULL)
		return;
	if (strncmp(event->attachments->array->content_type, "image", 5) != 0)
		return;

	fname = malloc(64 + sizeof(char));
	snprintf(fname, 64, "./images/%s.jpg", event->author->username);
	dlimg(event->attachments->array->url, fname);
	txt = extract_txt_from_img(fname);
	free(fname);

	memset(&player, 0, sizeof(player));
	player.name = event->author->username;
	player.userid = event->author->id;
	forline(&player, txt);

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
			free(player.bufptr);
			return;
		}
	}

	if (savetofile(&player)) {
		struct discord_create_message msg = {
			.content = "Your profile has been updated."
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
	} else {
		struct discord_create_message msg = {
			.content = "You have been registrated in the database."
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
	}
	updateplayers(&player);

	free(player.bufptr); /* bufptr should always be NULL */
}

void
raids(struct discord *client, const struct discord_message *event)
{
	char *txt;

	if (event->attachments->size == 0)
		return;
	if (strchr(event->attachments->array->filename, '.') == NULL)
		return;
	if (strncmp(event->attachments->array->content_type, "image", 5) != 0)
		return;

	dlimg(event->attachments->array->url, "./images/raids.jpg");
	txt = extract_txt_from_img("./images/raids.jpg");

	/*
	 * TODO
	 * "+ Raid options"
	 */

	return;

	struct discord_create_message msg = {
		.content = txt
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}


void
on_message(struct discord *client, const struct discord_message *event)
{
	int i;

#ifdef DEVEL
	if (event->channel_id == DEVEL)
		stats(client, event);
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
	size_t fsz = 0;
	char *fbuf = cog_load_whole_file(FILENAME, &fsz);

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif

	struct discord_attachment attachment = {
		.filename = FILENAME,
		.content = fbuf,
		.size = fsz
	};
	struct discord_attachments attachments = {
		.size = 1,
		.array = &attachment
	};
	struct discord_create_message msg = {
		.attachments = &attachments
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}


void
on_leaderboard(struct discord *client, const struct discord_message *event)
{
	int i = 2;
	char *txt;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif

	if (strlen(event->content) == 0) {
		txt = malloc(512);
		strcpy(txt, "NO WRONG, YOU MUST USE AN ARGUMENT!\n");
		strcat(txt, "Valid categories are:\n");
		for (i = 2; i < NFIELDS - 2; i++) {
			if (i == 5) /* regional rank */
				continue;
			strcat(txt, fields[i]);
			strcat(txt, "\n");
		}
		strcat(txt, "\nThis is case sensitive.");
		struct discord_create_message msg = {
			.content = txt
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		free(txt);
		return;
	}

	while (i < NFIELDS - 2 && strcmp(fields[i], event->content) != 0)
		i++;

	if (i == NFIELDS - 2 || i == 5) {
		txt = malloc(512);
		strcpy(txt, "This is not a valid category.\n");
		strcat(txt, "Valid categories are:\n");
		for (i = 2; i < NFIELDS - 2; i++) {
			if (i == 5) /* regional rank */
				continue;
			strcat(txt, fields[i]);
			strcat(txt, "\n");
		}
		strcat(txt, "\nThis is case sensitive.");
		struct discord_create_message msg = {
			.content = txt
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		free(txt);
		return;
	}

	category = i;
	txt = malloc((LB_MAX + 2) * 32);
	leaderboard(txt, event->author->id);

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
	char *txt, *p;
	u64snowflake userid;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif

	if (strlen(event->content) == 0) {
		struct discord_create_message msg = {
			.content = "NO WRONG, YOU MUST USE AN ARGUMENT!"
		};
		discord_create_message(client, event->channel_id, &msg, NULL);
		return;
	}

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
			p = strchr(txt, 0);
			sprintf(p, "%'ld", ((long *)&players[i])[j]);
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
	char *txt = malloc(512), *p;
	int i, len = (int)LENGTH(stats_ids);

#ifdef DEVEL
	if (event->channel_id != DEVEL) {
		free(txt);
		return;
	}
#endif

	strcpy(txt, "Post a screenshot of your stats to ");
	for (i = 0; i < len; i++) {
		p = strchr(txt, 0);;
		sprintf(p, "<#%lu> ", stats_ids[i]);
		if (i < len - 1)
			strcat(txt, "or ");
	}
	strcat(txt, "to enter the database.\n");
	/* TODO: add PREFIX */
	strcat(txt, "Commands: \n");
	strcat(txt, "\t?info [[@]user]\n");
	strcat(txt, "\t?leaderboard or ?lb [category]\n");
	/* strcat(txt, "\t?correct [category] [corrected value]\n"); */
	strcat(txt, "\t?source or ?src\n");
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
	char *src[] = { "src", "source" };
	char *lb[] = { "lb", "leaderboard" };
	struct discord *client;

	setlocale(LC_NUMERIC, "");
	ccord_global_init();
	client = discord_init(TOKEN);

	discord_add_intents(client, DISCORD_GATEWAY_MESSAGE_CONTENT);
	discord_set_prefix(client, PREFIX);
	discord_set_on_ready(client, on_ready);
	discord_set_on_message_create(client, on_message);
	discord_set_on_commands(client, lb, LENGTH(lb), on_leaderboard);
	discord_set_on_command(client, "info", on_info);
	discord_set_on_commands(client, src, LENGTH(src), on_source);
	discord_set_on_command(client, "help", on_help);

	if (mkdir("./images/", 0755) == 1)
		die("main: cannot create the images folder\n");
	createfile();
	initplayers();

	discord_run(client);

	discord_cleanup(client);
	ccord_global_cleanup();

	return EXIT_SUCCESS;
}

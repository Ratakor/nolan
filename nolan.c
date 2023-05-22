#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <leptonica/allheaders.h>
#include <tesseract/capi.h>
#include <curl/curl.h>
#include <concord/discord.h>
#include <concord/log.h>

#define RAIDS_ID 1109604402102276216
#define STATS_ID 1109604402102276216
#define MAX      300
#define KINGDOM  "Scream of Terra"

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
} Player;

void
die(const char *errstr)
{
	fputs(errstr, stderr);
	exit(1);
}

char *
nstrchr(const char *p, char c, int n)
{
	do {
		if (n == 0)
			return (char *)p;
		if (*p == c)
			n--;
	} while (*p++);

	return 0;
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
	player->name = NULL;
	player->level = NULL;
	player->ascension = NULL;
	player->kingdom = NULL;
	player->global = NULL;
	player->regional = NULL;
	player->competitive = NULL;
	player->playtime = NULL;
	player->monsters = NULL;
	player->bosses = NULL;
	player->players = NULL;
	player->quests = NULL;
	player->explored = NULL;
	player->taken = NULL;
	player->dungeons = NULL;
	player->coliseum = NULL;
	player->items = NULL;
	player->fish = NULL;
	player->distance = NULL;
	player->reputation = NULL;
	player->endless = NULL;
	player->codex = NULL;
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

void
createcsv(void)
{
	FILE *f;
	int size;

	f = fopen("players.csv", "r");

	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		size = ftell(f);
	}

	if ((f == NULL || size == 0) && ((f = fopen("players.csv", "w")) != NULL))
		fprintf(f, "Name,Level,Ascension,Kingdom,Global rank,Regional rank,Competitive rank,Playtime,Monsters slain,Bosses slain,Players defeated,Quests completed,Areas explored,Areas taken,Dungeons cleared,Coliseum wins,Items upgraded,Fish caught,Distance travelled,Reputation,Endless record,Entries completed\n");

	fclose(f);
}

/* TODO */
/*
void
getvaluefromcsv(int col)
{
	FILE *f;
	char line[MAX], *p, *end;
	char res[50][MAX];
	int i = 0;

	if ((f = fopen("players.csv", "r")) == NULL)
		die("getvaluefromcsv: cannot open players.csv\n");

	while ((p = fgets(line, MAX, f)) != NULL) {
		end = nstrchr(p, ',', col);
		if (end)
			*end = 0;
		res[i] = p;
	}
}
*/

int
playerinfile(Player *player, int col)
{
	FILE *f;
	char line[MAX], *p, *endname;
	int res = -1, i = 0;

	if ((f = fopen("players.csv", "r")) == NULL)
		die("saveplayer: cannot open players.csv\n");

	while ((p = fgets(line, MAX, f)) != NULL) {
		i++;
		endname = nstrchr(p, ',', col);
		if (--endname)
			*endname = 0;
		if (strcmp(player->name, p) == 0) {
			res = i;
			if (endname)
				*endname = ',';
			break;
		}
		if (endname)
			*endname = ',';
	}

	fclose(f);
	return res;
}

void
savetocsv(Player *player)
{
	FILE *fw, *fr;
	int pos, cpt = 0, ch, edited = 0, t, lineNum;
	char c;
	edited = 0;

	createcsv();
	pos = playerinfile(player, 1);

	if (pos == -1) {
		if ((fw = fopen("players.csv", "a")) == NULL)
			die("savetocsv: cannot open players.csv\n");

		fprintf(fw, "%s,", player->name);
		fprintf(fw, "%s,", player->level);
		fprintf(fw, "%s,", player->ascension);
		fprintf(fw, "%s,", player->kingdom);
		fprintf(fw, "%s,", player->global);
		fprintf(fw, "%s,", player->regional);
		fprintf(fw, "%s,", player->competitive);
		fprintf(fw, "%s,", player->playtime);
		fprintf(fw, "%s,", player->monsters);
		fprintf(fw, "%s,", player->bosses);
		fprintf(fw, "%s,", player->players);
		fprintf(fw, "%s,", player->quests);
		fprintf(fw, "%s,", player->explored);
		fprintf(fw, "%s,", player->taken);
		fprintf(fw, "%s,", player->dungeons);
		fprintf(fw, "%s,", player->coliseum);
		fprintf(fw, "%s,", player->items);
		fprintf(fw, "%s,", player->fish);
		fprintf(fw, "%s,", player->distance);
		fprintf(fw, "%s,", player->reputation);
		fprintf(fw, "%s,", player->endless);
		fprintf(fw, "%s\n", player->codex);
		fclose(fw);
	} else {
		if ((fr = fopen("players.csv", "r")) == NULL)
			die("savetocsv: cannot open players.csv (fr)\n");
		if ((fw = fopen("tmpfile", "w")) == NULL)
			die("savetocsv: cannot open players.csv (fw)\n");

		while ((c = fgetc(fr)) != EOF) {
			if (c == '\n')
				cpt++;
			if (!edited && cpt == pos - 1) {
				if (cpt == 0)
					fprintf(fw, "%s,", player->name);
				else
					fprintf(fw, "\n%s,", player->name);
				fprintf(fw, "%s,", player->level);
				fprintf(fw, "%s,", player->ascension);
				fprintf(fw, "%s,", player->kingdom);
				fprintf(fw, "%s,", player->global);
				fprintf(fw, "%s,", player->regional);
				fprintf(fw, "%s,", player->competitive);
				fprintf(fw, "%s,", player->playtime);
				fprintf(fw, "%s,", player->monsters);
				fprintf(fw, "%s,", player->bosses);
				fprintf(fw, "%s,", player->players);
				fprintf(fw, "%s,", player->quests);
				fprintf(fw, "%s,", player->explored);
				fprintf(fw, "%s,", player->taken);
				fprintf(fw, "%s,", player->dungeons);
				fprintf(fw, "%s,", player->coliseum);
				fprintf(fw, "%s,", player->items);
				fprintf(fw, "%s,", player->fish);
				fprintf(fw, "%s,", player->distance);
				fprintf(fw, "%s,", player->reputation);
				fprintf(fw, "%s,", player->endless);
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

		remove("players.csv");
		rename("tmpfile", "players.csv");
	}
}

void
on_ready(struct discord *client, const struct discord_ready *event)
{
	log_info("Logged in as %s!", event->user->username);

	struct discord_activity activities[] = {
		{
			.name = "nothing",
			.type = DISCORD_ACTIVITY_LISTENING,
			/* .details = "DETAILS", */
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
on_message(struct discord *client, const struct discord_message *event)
{
	if (event->channel_id == STATS_ID) {
		if (event->attachments->size == 0)
			return;
		if (strchr(event->attachments->array->filename, '.') == NULL)
			return;
		if (strncmp(event->attachments->array->content_type, "image", 5) != 0)
			return;

		Player player;

		dlimg(event->attachments->array->url);
		char *txt = extract_txt_from_img();

		if (txt == NULL)
			return;

		initplayer(&player);
		player.name = event->author->username;
		forline(&player, txt);

		if (strcmp(player.kingdom, KINGDOM) != 0) {
			struct discord_create_message params = { .content = "Sorry you're not part of the kingdom :/" };
			discord_create_message(client, event->channel_id, &params, NULL);
		} else {
			savetocsv(&player);
		}

#ifdef DEBUG
		fprintf(stderr, "name: %s\n", player.name);
		fprintf(stderr, "level: %s\n", player.level);
		fprintf(stderr, "ascension: %s\n", player.ascension);
		fprintf(stderr, "kingdom: %s\n", player.kingdom);
		fprintf(stderr, "global rank: %s\n", player.global);
		fprintf(stderr, "regional rank: %s\n", player.regional);
		fprintf(stderr, "competitive rank: %s\n", player.competitive);
		fprintf(stderr, "playtime: %s\n", player.playtime);
		fprintf(stderr, "monsters slain: %s\n", player.monsters);
		fprintf(stderr, "bosses slain: %s\n", player.bosses);
		fprintf(stderr, "players defeated: %s\n", player.players);
		fprintf(stderr, "quests completed: %s\n", player.quests);
		fprintf(stderr, "areas explored: %s\n", player.explored);
		fprintf(stderr, "areas taken: %s\n", player.taken);
		fprintf(stderr, "dungeons cleared: %s\n", player.dungeons);
		fprintf(stderr, "coliseum wins: %s\n", player.coliseum);
		fprintf(stderr, "items upgraded: %s\n", player.items);
		fprintf(stderr, "fish caught: %s\n", player.fish);
		fprintf(stderr, "distance travelled: %s\n", player.distance);
		fprintf(stderr, "reputation: %s\n", player.reputation);
		fprintf(stderr, "endless record: %s\n", player.endless);
		fprintf(stderr, "entries completed: %s\n", player.codex);
#endif

	} else if (event->channel_id == RAIDS_ID) {
		/* TODO */
	}
}

int
main(int argc, char *argv[])
{
	ccord_global_init();
	struct discord *client = discord_config_init("config.json");

	discord_add_intents(client, DISCORD_GATEWAY_MESSAGE_CONTENT);
	discord_set_on_ready(client, &on_ready);
	discord_set_on_message_create(client, &on_message);

	discord_run(client);

	discord_cleanup(client);
	ccord_global_cleanup();

	return 0;
}

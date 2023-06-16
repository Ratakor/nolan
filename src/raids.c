/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nolan.h"

#define DIFF        3 /* reduce n size in strncmp to reduce tesseract errors */
#define DAMAGE_CAP  300000000 /* weekly */

struct Slayers {
	const char *name;
	u64snowflake userid;
};

static void discord_send_message(struct discord *client,
                                 const u64snowflake channel_id,
                                 const char *fmt, ...);
static char *skip_to_slayers(char *txt);
static char *trim_name(char *name);
static unsigned int trim_dmg(char *str);
static size_t get_slayers(Slayer slayers[], char *txt);
static size_t parse(Slayer slayers[], char *txt);
static unsigned int adjust(unsigned int dmg, char *raid);
static void save_to_new_file(Slayer slayers[], size_t nslayers, char *fname,
                             char *raid);
static unsigned int get_weekly_damage(char *username, size_t namelen);
static void overcap_msg(char *name, unsigned int dmg, struct discord *client,
                        const u64snowflake channel_id);
static void save_to_file(Slayer slayers[], size_t nslayers, char *raid,
                         struct discord *client,
                         const u64snowflake channel_id);

const char *delims[] = {
	"+ Raid options",
	"4+ Raid options",
	"4+ Opciones de Asalto",
	"4+ Opzioni Raid",
	"十 “ 王 国 副 本 选 项",
	"十 レ イ ド オ プ シ ョ ン"
};
const char *garbageslayer[] = {
	"Slayer",
	"NEVE",
	"Asesino",
	"Uccissore",
	"击 杀 者",
	"討 伐 者"
};

const struct Slayers kingdom_slayers[] = {
	{ "Davethegray",                618842282564648976 },
	{ "SmittyWerbenJaeger",         372349296772907008 },
	{ "Damaquandey",                237305375211257866 },
	{ "Fhullegans",                 401213200688742412 },
	{ "PhilipXIVTheGintonicKnight", 653570978068299777 },
	{ "Heatnick",                   352065341901504512 },
	{ "Tazziekat",                  669809258153639937 },
	{ "Basic",                      274260433521737729 },
	{ "KovikFrunobulax",            589110637267779584 },
	{ "GoDLiKeKiLL",                425706840060592130 },
	{ "Shazaaamm",                  516711375330869367 },
	{ "Yiri",                       179396886254452736 },
	{ "MilesandroIlgnorante",       163351655478329344 },
	{ "HurricaneHam",               245010787817488384 },
	{ "SneakPeek",                  559664517198381057 },
	{ "Ensseric",                   659545497144655903 },
	{ "Mijikai",                    163048434478088192 },
	{ "BigYoshi",                   247796779804917770 },
	{ "Ratakor",                    277534384175841280 },
	{ "oxDje",                      452860420034789396 },
	{ "LordDroopy",                 704457235467730986 },
	{ "discosoutmurdersin",         609770024944664657 },
	{ "EchinChanfromHELL",          819244261760565280 },
	{ "ANIMAL",                     222464347568472064 },
	{ "Soreloser",                  345516161838088192 },
	{ "KyzeMythos",                 189463147571052544 },
	{ "ElucidatorS",                636975696186441739 },
	{ "BrewmasterAalst",            155893692740141056 },
	{ "Tiroc",                      519282218057596929 },
	{ "Kyzee",                      685267547377106990 },
	{ "Shadowssin",                 265837021400924162 },
	{ "Burtonlol",                  353969780702576641 },
	{ "Wingeren",                   75695406381543424  },
	{ "Schmiss",                    460552631778279457 },
	{ "Bloodshade",                 329400042324623371 },
	{ "Randylittle",                555760640329777153 },
	{ "Sunveela",                   269171964105457665 },
	{ "SaiSenpai",                  958922552284172349 },
	{ "Jakealope",                  164209517926678528 },
	{ "CleverCaitlin",              297650487585406976 },
	{ "Demmo",                      807849505105248287 }, /* Demm974 */
};

/* this should be in util.c but it's only used here*/
void
discord_send_message(struct discord *client, const u64snowflake channel_id,
                     const char *fmt, ...)
{
	char buf[MAX_MESSAGE_LEN];
	size_t rsiz;
	va_list ap;

	va_start(ap, fmt);
	rsiz = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (rsiz >= sizeof(buf))
		WARN("string truncation");

	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, channel_id, &msg, NULL);
}

char *
skip_to_slayers(char *txt)
{
	char *line = txt, *endline;
	unsigned int i;
	int found = 0;

	while (line && !found) {
		endline = strchr(line, '\n');
		if (endline)
			*endline = '\0';
		for (i = 0; i < LENGTH(delims); i++) {
			if (strcmp(line, delims[i]) == 0) {
				found = 1;
				break;
			}
		}
		line = endline ? (endline + 1) : 0;
	}
	if (line) {
		endline = strchr(line, '\n');
		if (endline) {
			*endline = '\0';
			if (strlen(line) == 0) {
				line = endline + 1;
				endline = strchr(line, '\n');
				if (endline)
					return endline + 1;
			} else {
				return endline + 1;
			}
		}
	}
	return NULL;
}

/* trim everything that is not a letter ~~or a space~~ */
char *
trim_name(char *name)
{
	const char *r;
	char *w;

	if (*(name + 1) == ' ')
		name = name + 2;
	r = name;
	w = name;
	do {
		if ((*r >= 'A' && *r <= 'Z') || (*r >= 'a' && *r <= 'z'))
			*w++ = *r;
	} while (*r++);
	*w = '\0';
	/* while (*name == ' ') */
	/* 	name++; */

	/* FIXME: this is really ugly, some tesseract madness again */
	if (strncmp(name, "DEINELENSY", sizeof("DEINELENSY") - 1) == 0)
		strcpy(name, "Damaquandey");

	return name;
}

/* trim everything that is not a number, could segfault in very rare case */
unsigned int
trim_dmg(char *str)
{
	const char *p = str;
	unsigned int dmg = 0;

	do {
		if (*p >= '0' && *p <= '9')
			dmg = (dmg * 10) + (*p - '0');
		else if (*p >= "①"[2] && *p <= "⑳"[2] && *(p - 1) == "①"[1])
			dmg = (dmg * 10) + (*p - "①"[2] + 1);
	} while (*p++);

	return dmg;
}

/* TODO: refactor skipping */
size_t
get_slayers(Slayer slayers[], char *txt)
{
	char *line = txt, *endline;
	size_t nslayers = 0;
	unsigned int i;
	int found = 0;

	if (txt == NULL)
		return 0;

	while (1) {
		endline = strchr(line, '\n');
		if (endline == 0)
			break;
		*endline = '\0';
		if (strlen(line) == 0) { /* skip empty line */
			line = endline + 1;
			endline = strchr(line, '\n');
			if (endline == 0)
				break;
			*endline = '\0';

		}
		if (!found) { /* skip Slayer */
			for (i = 0; i < LENGTH(garbageslayer); i++) {
				if (strcmp(line, garbageslayer[i]) == 0) {
					found = 1;
					break;
				}
			}
			if (found) {
				line = endline + 1;
				continue;
			}

		}
		slayers[nslayers].name = strdup(trim_name(line));
		line = endline + 1;

		endline = strchr(line, '\n');
		if (endline == 0) {
			free(slayers[nslayers].name);
			break;
		}
		*endline = '\0';
		/* skip empty line and chinese garbage */
		if (strlen(line) == 0 || strcmp(line, "阮 万") == 0) {
			line = endline + 1;
			endline = strchr(line, '\n');
			if (endline == 0) {
				free(slayers[nslayers].name);
				break;
			}
			*endline = '\0';
		}
		slayers[nslayers].damage = trim_dmg(line);
		if (slayers[nslayers].damage == 0) {
			free(slayers[nslayers].name);
			break;
		}
		line = endline + 1;
		slayers[nslayers].found_in_file = 0;
		nslayers++;
	}

	return nslayers;
}

size_t
parse(Slayer slayers[], char *txt)
{
	return get_slayers(slayers, skip_to_slayers(txt));
}

unsigned int
adjust(unsigned int dmg, char *raid)
{
	if (strcmp(raid, "starlord") == 0)
		return dmg * 2;
	if (strcmp(raid, "titan") == 0)
		return dmg * 2;
	if (strcmp(raid, "maelor") == 0)
		return dmg * 2;
	/* if (strcmp(raid, "arisen-morrigan") == 0) */
	/* 	return dmg / 2; */
	return dmg;
}

void
save_to_new_file(Slayer slayers[], size_t nslayers, char *fname, char *raid)
{
	FILE *fp;
	unsigned int i;

	fp = efopen(fname, "w");
	for (i = 0; i < nslayers; i++) {
		fprintf(fp, "%s%c%u\n", slayers[i].name, DELIM,
		        adjust(slayers[i].damage, raid));
		free(slayers[i].name);
	}
	fclose(fp);
}

/* also used in uraid.c */
unsigned int
parse_file(char *fname, char *username, size_t namelen)
{
	FILE *fp;
	unsigned int dmg;
	char line[LINE_SIZE];

	fp = efopen(fname, "r");
	while (fgets(line, LINE_SIZE, fp)) {
		dmg = strtoul(strchr(line, DELIM) + 1, NULL, 10);
		if (strncasecmp(username, line, namelen - DIFF) == 0) {
			fclose(fp);
			return dmg;
		}
	}

	fclose(fp);
	return 0;
}

unsigned int
get_weekly_damage(char *username, size_t namelen)
{
	unsigned int i, dmgs = 0;
	long day = (time(NULL) / 86400) - 1; /* today is already included */
	char fname[128];

	for (i = 0; i < 6; i++) {
		snprintf(fname, sizeof(fname), "%s%ld.csv",
		         RAIDS_FOLDER, day - i);
		if (file_exists(fname))
			dmgs += parse_file(fname, username, namelen);
	}

	return dmgs;
}

void
overcap_msg(char *name, unsigned int dmg, struct discord *client,
            const u64snowflake channel_id)
{
	unsigned int i = 0, dmgs = dmg;
	size_t len = strlen(name);

	dmgs += get_weekly_damage(name, len);
	if (dmgs < DAMAGE_CAP)
		return;

	while (i < LENGTH(kingdom_slayers) &&
	                strncasecmp(name, kingdom_slayers[i].name,
	                            len - DIFF) != 0)
		i++;

	if (i == MAX_SLAYERS) {
		WARN("%s is not added to slayers", name);
		discord_send_message(client, channel_id,
		                     "%s has overcapped the limit by %'lu \
damage, he is now at %'lu damage. <@%lu> add this user to the list btw",
		                     name, dmgs - DAMAGE_CAP, dmgs, ADMIN);
	} else {
		discord_send_message(client, channel_id,
		                     "<@%lu> has overcapped the limit by %'lu \
damage, he is now at %'lu damage.", kingdom_slayers[i].userid,
		                     dmgs - DAMAGE_CAP, dmgs);
	}
}

void
save_to_file(Slayer slayers[], size_t nslayers, char *raid,
             struct discord *client, const u64snowflake channel_id)
{
	FILE *w, *r;
	char line[LINE_SIZE], *endname, fname[128], tmpfname[128];
	unsigned int i, olddmg, newdmg;
	long day = time(NULL) / 86400;

	snprintf(fname, sizeof(fname), "%s%ld.csv", RAIDS_FOLDER, day);
	strlcpy(tmpfname, SAVE_FOLDER, sizeof(tmpfname));
	strlcat(tmpfname, "tmpfile2", sizeof(tmpfname));
	if ((r = fopen(fname, "r")) == NULL) {
		save_to_new_file(slayers, nslayers, fname, raid);
		return;
	}
	w = efopen(tmpfname, "w");

	while (fgets(line, LINE_SIZE, r)) {
		endname = strchr(line, DELIM);
		if (endname)
			*endname = '\0';
		for (i = 0; i < nslayers; i++) {
			if (slayers[i].found_in_file)
				continue;
			/* should be strcmp but for common mistakes */
			if (strncasecmp(slayers[i].name, line,
			                strlen(slayers[i].name) - DIFF) == 0) {
				slayers[i].found_in_file = 1;
				break;
			}
		}
		if (i < nslayers) {
			olddmg = strtoul(endname + 1, NULL, 10);
			newdmg = olddmg + adjust(slayers[i].damage, raid);
			overcap_msg(slayers[i].name, newdmg, client, channel_id);
			fprintf(w, "%s%c%u\n", slayers[i].name, DELIM, newdmg);
		} else {
			if (endname)
				*endname = DELIM;
			fprintf(w, "%s", line);
		}
	}
	for (i = 0; i < nslayers; i++) {
		if (!slayers[i].found_in_file) {
			fprintf(w, "%s%c%u\n", slayers[i].name, DELIM,
			        adjust(slayers[i].damage, raid));
		}
		free(slayers[i].name);
	}

	fclose(r);
	fclose(w);
	remove(fname);
	rename(tmpfname, fname);
}

void
on_raids(struct discord *client, const struct discord_message *event)
{
	int is_png;
	unsigned int i, ret;
	char *txt = NULL, fname[128];
	size_t nslayers;
	Slayer slayers[MAX_SLAYERS];
	struct discord_channel chan;
	struct discord_ret_channel rchan = {
		.keep = event,
		.sync = &chan,
	};

	LOG("start");
	is_png = (strcmp(event->attachments->array->content_type,
	                 "image/png") == 0) ? 1 : 0;
	if (is_png)
		snprintf(fname, sizeof(fname), "%s/raids.png", IMAGES_FOLDER);
	else
		snprintf(fname, sizeof(fname), "%s/raids.jpg", IMAGES_FOLDER);
	if ((ret = curl_file(event->attachments->array->url, fname)) != 0) {
		WARN("curl failed CURLcode: %u", ret);
		discord_send_message(client, event->channel_id, "Error: \
Failed to download image <@%lu>.\nFix me <@%lu>", event->author->id, ADMIN);
		return;
	}
	crop(fname, is_png);

	for (i = 0; i < LENGTH(cn_slayer_ids); i++) {
		if (event->author->id == cn_slayer_ids[i]) {
			txt = ocr(fname, "chi_sim");
			break;
		}
	}
	if (txt == NULL) {
		for (i = 0; i < LENGTH(jp_slayer_ids); i++) {
			if (event->author->id == jp_slayer_ids[i]) {
				txt = ocr(fname, "jpn");
				break;
			}
		}
	}
	if (txt == NULL)
		txt = ocr(fname, "eng");

	if (txt == NULL) {
		WARN("failed to read image from %s", event->author->username);
		discord_send_message(client, event->channel_id, "Error: \
Failed to read image <@%lu>.\nFix me <@%lu>", event->author->id, ADMIN);
		return;
	}

	nslayers = parse(slayers, txt);
	if (nslayers == 0) {
		discord_send_message(client, event->channel_id, "This is not \
a correct screenshot sir <@%lu>.", event->author->id);
		free(txt);
		return;
	}
	free(txt);
	discord_get_channel(client, event->channel_id, &rchan);
	save_to_file(slayers, nslayers, chan.name, client, event->channel_id);
	/* ^ will free slayers[].name */
	LOG("end");
}

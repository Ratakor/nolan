/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nolan.h"

#define DAMAGE_CAP  300000000 /* weekly */

enum {
	SUCCESS,
	DOWNLOAD_FAILED,
	OCR_FAILED,
	PARSING_FAILED

};

struct Slayers {
	const char *name;
	u64snowflake userid;
};

static void discord_send_message(struct discord *client,
                                 const u64snowflake channel_id,
                                 const char *fmt, ...);
static char *skip_to_slayers(char *txt);
static char *trim_name(char *name);
static uint32_t trim_dmg(char *str);
static size_t get_slayers(Slayer slayers[], char *txt);
static void save_to_new_file(Slayer slayers[], size_t nslayers, char *fname);
static void save_to_file(Slayer slayers[], size_t nslayers);
static int raids(struct discord_attachment *attachment, const char *lang,
                 Slayer slayers[]);
static void overcap_msg(struct discord *client, u64snowflake channel_id,
                        Slayer slayers[]);

static const struct Slayers kingdom_slayers[] = {
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

static const char *delims[] = {
	"+ Raid options",
	"4+ Raid options",
	"4+ Opciones de Asalto",
	"4+ Opzioni Raid",
	"十 “ 王 国 副 本 选 项",
	"十 レ イ ド オ プ シ ョ ン"
};
static const char *garbageslayer[] = {
	"Slayer",
	"NEVE",
	"Asesino",
	"Uccissore",
	"击 杀 者",
	"討 伐 者"
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
		log_warn("%s: string truncation", __func__);

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
uint32_t
trim_dmg(char *str)
{
	const char *p = str;
	uint32_t dmg = 0;

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
		slayers[nslayers].name = xstrdup(trim_name(line));
		dalloc_comment(slayers[nslayers].name, "slayer name");
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

void
save_to_new_file(Slayer slayers[], size_t nslayers, char *fname)
{
	FILE *fp;
	size_t i;

	fp = xfopen(fname, "w");
	for (i = 0; i < nslayers; i++) {
		fprintf(fp, "%s%c%u\n", slayers[i].name, DELIM,
		        slayers[i].damage);
		free(slayers[i].name);
	}
	fclose(fp);
}

/* also used in uraid.c */

void
save_to_file(Slayer slayers[], size_t nslayers)
{
	FILE *w, *r;
	char line[LINE_SIZE], *endname, fname[128], tmpfname[128];
	uint32_t i, olddmg, newdmg;
	time_t day = time(NULL) / 86400;

	snprintf(fname, sizeof(fname), "%s%ld.csv", RAIDS_FOLDER, day);
	strlcpy(tmpfname, SAVE_FOLDER, sizeof(tmpfname));
	strlcat(tmpfname, "tmpfile2", sizeof(tmpfname));
	if ((r = fopen(fname, "r")) == NULL) {
		save_to_new_file(slayers, nslayers, fname);
		return;
	}
	w = xfopen(tmpfname, "w");

	while (fgets(line, LINE_SIZE, r)) {
		endname = strchr(line, DELIM);
		if (endname)
			*endname = '\0';
		for (i = 0; i < nslayers; i++) {
			if (slayers[i].found_in_file)
				continue;
			if (strncasecmp(slayers[i].name, line,
			                strlen(slayers[i].name)) == 0) {
				slayers[i].found_in_file = 1;
				break;
			}
		}
		if (i < nslayers) {
			olddmg = strtoul(endname + 1, NULL, 10);
			newdmg = olddmg + slayers[i].damage;
			/* overcap_msg(slayers[i].name, newdmg, client, channel_id); */
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
			        slayers[i].damage);
		}
		free(slayers[i].name);
	}

	fclose(r);
	fclose(w);
	remove(fname);
	rename(tmpfname, fname);
}

int
raids(struct discord_attachment *attachment, const char *lang, Slayer slayers[])
{
	char *txt = NULL, fname[PATH_MAX];
	size_t nslayers;
	CURLcode ret;
	int is_png;

	log_info("%s", __func__);
	is_png = (strcmp(attachment->content_type, "image/png") == 0);
	if (is_png)
		snprintf(fname, sizeof(fname), "%s/%s.png",
		         IMAGES_FOLDER, attachment->filename);
	else
		snprintf(fname, sizeof(fname), "%s/%s.jpg",
		         IMAGES_FOLDER, attachment->filename);

	if ((ret = curl_file(attachment->url, fname)) != 0) {
		log_error("curl failed CURLcode: %u", ret);
		return DOWNLOAD_FAILED;
	}

	crop(fname, is_png);
	txt = ocr(fname, lang);
	if (txt == NULL)
		return OCR_FAILED;

	nslayers = get_slayers(slayers, skip_to_slayers(txt));
	if (nslayers > 0)
		save_to_file(slayers, nslayers);
	free(txt);

	return (nslayers > 0) ? SUCCESS : PARSING_FAILED;
}

void
parse_file(char *fname, Slayer slayers[], size_t *nslayers)
{
	FILE *fp;
	char line[LINE_SIZE], *endname;
	size_t i;
	uint32_t dmg;

	fp = xfopen(fname, "r");
	while (fgets(line, LINE_SIZE, fp)) {
		endname = strchr(line, DELIM);
		dmg = strtoul(endname + 1, NULL, 10);
		if (endname)
			*endname = '\0';
		for (i = 0; i < *nslayers; i++) {
			if (strcmp(slayers[i].name, line) == 0)
				break;
		}
		if (i < *nslayers) {
			slayers[i].damage += dmg;
		} else {
			slayers[i].name = xstrdup(line);
			dalloc_comment(slayers[i].name,
			               "parse_file slayers name");
			slayers[i].damage = dmg;
			*nslayers += 1;
		}
	}

	fclose(fp);
}

void
load_files(Slayer slayers[], size_t *nslayers)
{
	char fname[PATH_MAX];
	time_t day;
	unsigned int i;

	day = time(NULL) / 86400;
	for (i = 0; i < 6; i++) {
		snprintf(fname, sizeof(fname), "%s%ld.csv",
		         RAIDS_FOLDER, day - i);
		if (file_exists(fname))
			parse_file(fname, slayers, nslayers);
	}
}

void
overcap_msg(struct discord *client, u64snowflake channel_id, Slayer slayers[])
{
	size_t nslayers = 0, i, j;

	load_files(slayers, &nslayers);
	for (i = 0, j = 0; i < nslayers; i++, j = 0) {
		if (slayers[i].damage < DAMAGE_CAP) {
			free(slayers[i].name);
			continue;
		}

		for (j = 0; j <  LENGTH(kingdom_slayers); j++) {
			if (strncasecmp(slayers[i].name,
			                kingdom_slayers[j].name,
			                strlen(slayers[i].name)) == 0)
				break;
		}

		if (i == MAX_SLAYERS) {
			log_warn("%s: %s is not added to slayers",
			         __func__, slayers[i].name);
			/* FIXME: ' flag */
			discord_send_message(client, channel_id,
			                     "%s has overcapped the limit by %'"PRIu32
			                     " damage, he is now at %'"PRIu32" damage. "
			                     "<@%"PRIu64"> add this user to the list btw",
			                     slayers[i].name,
			                     slayers[i].damage - DAMAGE_CAP,
			                     slayers[i].damage, ADMIN);
		} else {
			discord_send_message(client, channel_id,
			                     "<@%"PRIu64"> has overcapped the limit by "
			                     "%'"PRIu32" damage, he is now at %'"PRIu32
			                     " damage.",
			                     kingdom_slayers[i].userid,
			                     slayers[i].damage - DAMAGE_CAP,
			                     slayers[i].damage);
		}

		free(slayers[i].name);
	}
}


void
on_raids(struct discord *client, const struct discord_message *event)
{
	Slayer *slayers;
	char *lang;
	size_t i;
	int twice = 0;

	for (i = 0; i < LENGTH(cn_slayer_ids); i++) {
		if (event->author->id == cn_slayer_ids[i]) {
			lang = "chi_sim";
			goto lang_found;
		}
	}
	for (i = 0; i < LENGTH(jp_slayer_ids); i++) {
		if (event->author->id == jp_slayer_ids[i]) {
			lang = "jpn";
			goto lang_found;
		}
	}
	lang = "eng";

lang_found:

	slayers = xcalloc(MAX_SLAYERS, sizeof(*slayers));
	for (i = 0; i < (size_t)event->attachments->size; i++) {
run_again:
		switch (raids(event->attachments->array + i, lang, slayers)) {
		case SUCCESS:
			discord_send_message(client, event->channel_id, "Success");
			break;
		case DOWNLOAD_FAILED:
			log_error("Download failed with URL:%s from %s",
			          event->attachments->array[i].url,
			          event->author->username);
			discord_send_message(client, event->channel_id,
			                     "Error: Failed to download image <@%"PRIu64">.\n"
			                     "Fix me <@%"PRIu64">", event->author->id, ADMIN);
			break;
		case OCR_FAILED:
			log_error("OCR failed with lang:%s from %s",
			          lang, event->author->username);
			discord_send_message(client, event->channel_id,
			                     "Error: Failed to read image <@%"PRIu64">.\n"
			                     "Fix me <@%"PRIu64">", event->author->id, ADMIN);
			break;
		case PARSING_FAILED:
			/* TODO: this is ugly */
			if (!twice) {
				twice = 1;
				goto run_again;
			}
			log_error("parsing failed twice with lang:%s from %s",
			          lang, event->author->username);
			discord_send_message(client, event->channel_id,
			                     "Error: Failed to parse image <@%"PRIu64">.\n"
			                     "Fix me <@%"PRIu64">", event->author->id, ADMIN);
			break;
		}
		twice = 0;
		memset(slayers, 0, MAX_SLAYERS * sizeof(*slayers));
	}

	overcap_msg(client, event->channel_id, slayers);
	free(slayers);
}

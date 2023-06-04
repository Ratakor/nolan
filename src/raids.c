#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nolan.h"

#define DAMAGE_CAP  300000000 / 7 /* daily */

static void emsg(struct discord *client, const struct discord_message *event);
static char *skip_to_slayers(char *txt);
static char *trim_name(char *name);
static unsigned long trim_dmg(char *str);
static size_t get_slayers(Slayer slayers[], char *txt);
static size_t parse(Slayer slayers[], char *txt);
static unsigned long adjust(unsigned long dmg, char *raid);
static void save_to_new_file(Slayer slayers[], size_t nslayers, char *fname,
                             char *raid);
static void overcap_msg(char *name, unsigned long dmg, struct discord *client,
                        const struct discord_message *event);
static void save_to_file(Slayer slayers[], size_t nslayers, char *raid,
                         struct discord *client,
                         const struct discord_message *event);

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

void
emsg(struct discord *client, const struct discord_message *event)
{
	char buf[MAX_MESSAGE_LEN];

	warn("nolan: Failed to read raid image\n");
	snprintf(buf, sizeof(buf),
	         "Error: Failed to read image <@%lu>.\nFix me <@%lu>",
	         event->author->id, ADMIN);
	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
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

/* trim everything that is not a letter or a space */
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
		if (*r == ' ' || (*r >= 'A' && *r <= 'Z') || (*r >= 'a' && *r <= 'z'))
			*w++ = *r;
	} while (*r++);
	*w = '\0';
	while (*name == ' ')
		name++;

	return name;
}

/* trim everything that is not a number, could segfault in very rare case */
unsigned long
trim_dmg(char *str)
{
	const char *p = str;
	unsigned long dmg = 0;

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

unsigned long
adjust(unsigned long dmg, char *raid)
{
	if (strcmp(raid, "starlord") == 0)
		return dmg * 2;
	if (strcmp(raid, "titan") == 0)
		return dmg * 2;
	if (strcmp(raid, "maelor") == 0)
		return dmg * 2;
	/* if (strcmp(raid, "arisen-morrigan") == 0) */
	/* 	return *dmg / 2; */
	return dmg;
}

void
save_to_new_file(Slayer slayers[], size_t nslayers, char *fname, char *raid)
{
	FILE *fp;
	unsigned int i;
	if ((fp = fopen(fname, "w")) == NULL)
		die("nolan: Failed to open %s\n", fname);
	for (i = 0; i < nslayers; i++) {
		fprintf(fp, "%s%c%lu\n", slayers[i].name, DELIM,
		        adjust(slayers[i].damage, raid));
		free(slayers[i].name);
	}
	fclose(fp);
}

void
overcap_msg(char *name, unsigned long dmg, struct discord *client,
            const struct discord_message *event)
{
	char buf[MAX_MESSAGE_LEN];

	if (dmg < DAMAGE_CAP)
		return;

	snprintf(buf, sizeof(buf), "%s has overcapped the limit by %'lu \
damage he is now at %'lu damage.", name, dmg - DAMAGE_CAP, dmg);

	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

void
save_to_file(Slayer slayers[], size_t nslayers, char *raid,
             struct discord *client, const struct discord_message *event)
{
	FILE *w, *r;
	char line[LINE_SIZE], *endname, fname[128], tmpfname[128];
	unsigned int i;
	unsigned long olddmg, newdmg;
	long day = time(NULL) / 86400;

	/* assert with rsiz >= siz ? */
	snprintf(fname, sizeof(fname), "%s%ld.csv", RAIDS_FOLDER, day);
	strlcpy(tmpfname, SAVE_FOLDER, sizeof(tmpfname));
	strlcat(tmpfname, "tmpfile2", sizeof(tmpfname));
	if ((r = fopen(fname, "r")) == NULL) {
		save_to_new_file(slayers, nslayers, fname, raid);
		return;
	}
	if ((w = fopen(tmpfname, "w")) == NULL)
		die("nolan: Failed to open %s\n", tmpfname);

	while (fgets(line, LINE_SIZE, r)) {
		endname = strchr(line, DELIM);
		if (endname)
			*endname = '\0';
		for (i = 0; i < nslayers; i++) {
			if (!slayers[i].found_in_file) {
				/* should be strcmp but for common mistakes */
				if (strncmp(slayers[i].name, line,
				                strlen(slayers[i].name) - 3) == 0) {
					slayers[i].found_in_file = 1;
					break;
				}
			}
		}
		if (i < nslayers) {
			olddmg = strtoul(endname + 1, NULL, 10);
			newdmg = olddmg + adjust(slayers[i].damage, raid);
			overcap_msg(slayers[i].name, newdmg, client, event);
			fprintf(w, "%s%c%lu\n", slayers[i].name, DELIM, newdmg);
		} else {
			if (endname)
				*endname = DELIM;
			fprintf(w, "%s", line);
		}
	}
	for (i = 0; i < nslayers; i++) {
		if (!slayers[i].found_in_file) {
			fprintf(w, "%s%c%lu\n", slayers[i].name, DELIM,
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
	unsigned int i;
	char *txt = NULL, fname[128];
	size_t nslayers;
	Slayer slayers[MAX_SLAYERS];
	struct discord_channel chan;
	struct discord_ret_channel rchan = {
		.keep = event,
		.sync = &chan,
	};

	if (strcmp(event->attachments->array->content_type, "image/png") == 0) {
		snprintf(fname, sizeof(fname), "%s/raids.png", IMAGES_FOLDER);
		curl(event->attachments->array->url, fname);
		crop(fname, 1);
	} else { /* always a jpg, check on_message() */
		snprintf(fname, sizeof(fname), "%s/raids.jpg", IMAGES_FOLDER);
		curl(event->attachments->array->url, fname);
		crop(fname, 0);
	}

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

	if (txt == NULL || (nslayers = parse(slayers, txt)) == 0) {
		emsg(client, event);
		free(txt);
		return;
	}
	free(txt);
	discord_get_channel(client, event->channel_id, &rchan);
	save_to_file(slayers, nslayers, chan.name, client, event);
	/* ^ will free slayers[].name */
}

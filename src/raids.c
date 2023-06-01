#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "nolan.h"

#define MAX_SLAYERS 50
#define DAMAGE_CAP  10000000 /* daily */

static void emsg(struct discord *client, const struct discord_message *event);
static char *skip_to_slayers(char *txt);
static char *trim_dmg(char *str);
static int get_slayers(Slayer slayers[], char *txt);
static int parse(Slayer slayers[], char *txt);
static unsigned long adjust(unsigned long dmg, char *raid);
static void save_to_new_file(Slayer slayers[], int nslayers, char *fname,
                             char *raid);
static void overcap_msg(char *name, unsigned long dmg, struct discord *client,
                        const struct discord_message *event);
static void save_to_file(Slayer slayers[], int nslayers, char *raid,
                         struct discord *client,
                         const struct discord_message *event);

static const char *delims[] = {
	"+ Raid options",
	"4+ Raid options",
	"4+ Opciones de Asalto",
	"4+ Opzioni Raid",
};
static const char *garbageslayer[] = {
	"Slayer",
	"NEVE",
	"Asesino",
	"Uccissore",
};

static void
emsg(struct discord *client, const struct discord_message *event)
{
	char buf[128];
	snprintf(buf, 128, "Error: Failed to read image <@%lu>.\nFix me <@%lu>",
	         event->author->id, ADMIN);
	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

static char *
skip_to_slayers(char *txt)
{
	char *line = txt, *endline;
	int i, found = 0;

	while (line && !found) {
		endline = strchr(line, '\n');
		if (endline)
			*endline = '\0';
		for (i = 0; i < (int)LENGTH(delims); i++) {
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

/* trim everything that is not in the ascii table (not all of it) */
static char *
trim_name(char *name)
{
	const char *r;
	char *w;

	if (*(name + 1) == ' ')
		name = name + 2;
	r = name;
	w = name;
	do {
		if (*r >= 32 && *r <= 126)
			*w++ = *r;
	} while (*r++);
	*w = '\0';
	while (*name == ' ')
		name++;

	return name;
}

/* trim everything that is not a number, returns a pointer to the trimmed str*/
static char *
trim_dmg(char *dmg)
{
	const char *r = dmg;
	char *w = dmg;

	do {
		if (*r >= 48 && *r <= 57)
			*w++ = *r;
	} while (*r++);
	*w = '\0';

	return dmg;
}

static int
get_slayers(Slayer slayers[], char *txt)
{
	char *line = txt, *endline;
	int n = 0, i, found = 0;

	if (txt == NULL)
		return 0;

	while (1) {
		endline = strchr(line, '\n');
		if (endline == 0)
			break;
		*endline = '\0';
		if (!found) { /* skip Slayer */
			for (i = 0; i < (int)LENGTH(garbageslayer); i++) {
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
		slayers[n].name = strdup(trim_name(line));
		line = endline + 1;

		endline = strchr(line, '\n');
		if (endline == 0)
			break;
		*endline = '\0';
		slayers[n].damage = strtoul(trim_dmg(line), NULL, 10);
		line = endline + 1;
		slayers[n].found_in_file = 0;
		n++;
	}
	return n;
}

static int
parse(Slayer slayers[], char *txt)
{
	return get_slayers(slayers, skip_to_slayers(txt));
}

static unsigned long
adjust(unsigned long dmg, char *raid)
{
	if (strcmp(raid, "starlord") == 0)
		return dmg * 2;
	else if (strcmp(raid, "titan") == 0)
		return dmg * 2;
	else if (strcmp(raid, "maelor") == 0)
		return dmg * 2;
	/* if (strcmp(raid, "arisen-morrigan") == 0) */
	/* 	return *dmg / 2; */
	return dmg;
}

static void
save_to_new_file(Slayer slayers[], int nslayers, char *fname, char *raid)
{
	FILE *fp;
	int i;
	if ((fp = fopen(fname, "w")) == NULL)
		die("nolan: Failed to open %s\n", fname);
	for (i = 0; i < nslayers; i++) {
		fprintf(fp, "%s%c%lu\n", slayers[i].name, DELIM,
		        adjust(slayers[i].damage, raid));
		free(slayers[i].name);
	}
	fclose(fp);
}

static void
overcap_msg(char *name, unsigned long dmg, struct discord *client,
            const struct discord_message *event)
{
	char buf[DISCORD_MAX_MESSAGE_LEN];

	if (dmg < DAMAGE_CAP)
		return;

	snprintf(buf, sizeof(buf), "%s has overcapped the limit by %'lu \
damage he is now at %'lu damage.", name, dmg - DAMAGE_CAP,
	         dmg);

	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

static void
save_to_file(Slayer slayers[], int nslayers, char *raid,
             struct discord *client, const struct discord_message *event)
{
	FILE *w, *r;
	char line[LINE_SIZE], *endname, fname[64], tmpfname[64];
	int i;
	unsigned long olddmg, newdmg;
	long day = time(NULL) / 86400;

	snprintf(fname, sizeof(fname), "%s%ld.csv", RAIDS_FOLDER, day);
	cpstr(tmpfname, SAVE_FOLDER, sizeof(tmpfname));
	catstr(tmpfname, "tmpfile2", sizeof(tmpfname));
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
				if (strcmp(slayers[i].name, line) == 0) {
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
	char *txt, fname[64];
	int nslayers;
	Slayer slayers[MAX_SLAYERS];
	struct discord_channel chan;
	struct discord_ret_channel rchan = {
		.keep = event,
		.sync = &chan,
	};

	if (strcmp(event->attachments->array->content_type, "image/png") == 0) {
		snprintf(fname, 64, "%s/raids.png", IMAGE_FOLDER);
		curl(event->attachments->array->url, fname);
		crop(fname, 1);
		txt = ocr(fname);
	} else { /* always a jpg, check on_message() */
		snprintf(fname, 64, "%s/raids.jpg", IMAGE_FOLDER);
		curl(event->attachments->array->url, fname);
		crop(fname, 0);
		txt = ocr(fname);
	}

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

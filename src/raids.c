#include <stdlib.h>
#include <string.h>
#include "nolan.h"

/* ?raid username */
/* and it shows their damage for each raid + a the sum according to ratio (like
 * *0.5 for amorri) */

/* if somebody changed their name, it can just state that the recorded activity
 * only starts on X date */

/* ?list */
/* to list members */

#define MAX_SLAYERS 50

char *raid = NULL;
struct Slayer {
	char *name;
	unsigned long damage;
};

static void emsg(struct discord *client, const struct discord_message *event);
static char *skip_to_slayers(char *txt);
static char *trim_dmg(char *str);
static int get_slayers(struct Slayer slayers[], char *txt);
static int parse(struct Slayer slayers[], char *txt);
/* static int save_to_file(struct Slayer *slayers, int nslayers); */


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

void
get_chan_name(struct discord *client, struct discord_response *rep,
              const struct discord_channel *ret)
{
	free(raid);
	raid = strdup(ret->name);
}

/* stock raid index to slayers->raid and return a pointer after raid name */
/* TIED TO CHANNEL NAME NOW */
/*
static char *
get_raid(int *raid, char *txt)
{
	char *line = txt, *endline;
	int i;

	while (line) {
		endline = strchr(line, '\n');
		if (endline)
			*endline = '\0';
		for (i = 0; i < (int)LENGTH(raids); i++) {
			if (strcmp(line, raids[i]) == 0) {
				*raid = i;
				return endline + 1;
			}
		}
		line = endline ? (endline + 1) : 0;
	}
	return line;
}
*/

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
get_slayers(struct Slayer slayers[], char *txt)
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
		n++;
	}
	return n;
}

static int
parse(struct Slayer slayers[], char *txt)
{
	return get_slayers(slayers, skip_to_slayers(txt));
}

static void
save_to_file(struct Slayer slayers[], int nslayers)
{
	FILE *w, *r;
	char buf[LINE_SIZE];
}

void
on_raids(struct discord *client, const struct discord_message *event)
{
	char *txt, fname[64];
	int i, nslayers;
	struct Slayer slayers[MAX_SLAYERS];

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

	struct discord_ret_channel chan = {
		.keep = event,
		.done = get_chan_name /* get_raid_name */
	};
	discord_get_channel(client, event->channel_id, &chan);
	save_to_file(slayers, nslayers);


	/* FIXME: devel */
	char buf[DISCORD_MAX_MESSAGE_LEN], *p = buf;
	for (i = 0; i < nslayers; i++) {
		sprintf(p, "%s %lu\n", slayers[i].name, slayers[i].damage);
		/* fprintf(stderr, "%s ", slayers[i].name); */
		/* fprintf(stderr, "%lu\n", slayers[i].damage); */
		free(slayers[i].name);
		p = strchr(buf, '\0');
	}
	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

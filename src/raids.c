/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nolan.h"

#define DAMAGE_CAP        300000000 /* weekly */

enum {
	SUCCESS,
	DOWNLOAD_FAILED,
	OCR_FAILED,
	PARSING_FAILED

};

static char *skip_to_slayers(char *txt);
static char *trim_name(char *name);
static uint32_t trim_dmg(char *str);
static size_t get_slayers(Slayer slayers[], char *txt);
static void save_to_new_file(Slayer slayers[], size_t nslayers, char *fname);
static void save_to_file(Slayer slayers[], size_t nslayers);
static int raids(struct discord_attachment *attachment, const char *lang,
                 Slayer slayers[], u64snowflake channel_id);
static void parse_file(char *fname, Slayer slayers[], size_t *nslayers);
static void overcap_msg(struct discord *client, u64snowflake channel_id,
                        Slayer slayers[]);

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

char *
skip_to_slayers(char *txt)
{
	char *line = txt, *endline;
	unsigned int i;
	bool found = false;

	while (line && !found) {
		endline = strchr(line, '\n');
		if (endline)
			*endline = '\0';
		for (i = 0; i < LENGTH(delims); i++) {
			if (strcmp(line, delims[i]) == 0) {
				found = true;
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

	// FIXME: some other stuff are x10ed
	if (dmg > 100000000)
		dmg /= 10;

	return dmg;
}

/* TODO: refactor skipping */
size_t
get_slayers(Slayer slayers[], char *txt)
{
	char *line = txt, *endline;
	size_t nslayers = 0;
	unsigned int i;
	bool found = false;

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
					found = true;
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
		slayers[nslayers].found_in_file = false;
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
				slayers[i].found_in_file = true;
				break;
			}
		}
		if (i < nslayers) {
			olddmg = strtoul(endname + 1, NULL, 10);
			newdmg = olddmg + slayers[i].damage;
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
raids(struct discord_attachment *attachment, const char *lang, Slayer slayers[],
      u64snowflake channel_id)
{
	char *txt = NULL, fname[PATH_MAX];
	size_t i, nslayers;
	CURLcode code;
	bool is_png;

	is_png = (strcmp(attachment->content_type, "image/png") == 0);
	if (is_png)
		snprintf(fname, sizeof(fname), "%s/%s.png",
		         IMAGES_FOLDER, attachment->filename);
	else
		snprintf(fname, sizeof(fname), "%s/%s.jpg",
		         IMAGES_FOLDER, attachment->filename);

	if ((code = curl_file(attachment->url, fname)) != 0) {
		log_error("curl failed CURLcode: %s [%u]",
		          curl_easy_strerror(code), code);
		return DOWNLOAD_FAILED;
	}

	crop(fname, is_png);
	txt = ocr(fname, lang);
	if (txt == NULL)
		return OCR_FAILED;
	nslayers = get_slayers(slayers, skip_to_slayers(txt));
	free(txt);

	if (nslayers == 0)
		return PARSING_FAILED;

	for (i = 0; i < nslayers; i++) {
		if (channel_id == CHANNEL_ID_DOUBLE)
			slayers[i].damage *= 2;
		else if (channel_id == CHANNEL_ID_HALF)
			slayers[i].damage /= 2;
	}
	save_to_file(slayers, nslayers);

	return SUCCESS;
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

/* also used in cmd_lbraid.c, retuns nslayers */
size_t
load_files(Slayer slayers[])
{
	char fname[PATH_MAX];
	size_t nslayers = 0, i;
	time_t day;

	day = time(NULL) / 86400;
	for (i = 0; i < 7; i++) {
		snprintf(fname, sizeof(fname), "%s%ld.csv",
		         RAIDS_FOLDER, day - i);
		if (file_exists(fname))
			parse_file(fname, slayers, &nslayers);
	}

	return nslayers;
}

void
overcap_msg(struct discord *client, u64snowflake channel_id, Slayer slayers[])
{
	size_t nslayers, i;

	nslayers = load_files(slayers);
	for (i = 0; i < nslayers; i++) {
		if (slayers[i].damage < DAMAGE_CAP) {
			free(slayers[i].name);
			continue;
		}

		/* FIXME: ' flag */
		discord_send_message(client, channel_id,
		                     "%s has overcapped the limit by %'"PRIu32
		                     " damage, he is now at %'"PRIu32" damage.",
		                     slayers[i].name,
		                     slayers[i].damage - DAMAGE_CAP,
		                     slayers[i].damage);

		free(slayers[i].name);
	}
}

void
on_raids(struct discord *client, const struct discord_message *event)
{
	Slayer *slayers;
	char *lang;
	size_t i;
	unsigned int time_run = 0;

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
		switch (raids(event->attachments->array + i, lang, slayers, event->channel_id)) {
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
			if (time_run < 5) {
				time_run++;
				goto run_again;
			}
			log_error("parsing failed with lang:%s from %s",
			          lang, event->author->username);
			/* TODO: just me being lazy */
			if (strcmp(lang, "jpn") == 0) {
				discord_send_message(client, event->channel_id,
				                     "Error: Sorry japanese doesn't work well");
			} else {
				discord_send_message(client, event->channel_id,
				                     "Error: Failed to parse image <@%"PRIu64">.\n"
				                     "Fix me <@%"PRIu64">", event->author->id, ADMIN);
			}
			break;
		}
		time_run = 0;
		memset(slayers, 0, MAX_SLAYERS * sizeof(*slayers));
	}

	log_info("%s %d", __func__, event->attachments->size);
	overcap_msg(client, event->channel_id, slayers);
	free(slayers);
}

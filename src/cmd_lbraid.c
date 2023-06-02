#include <stdlib.h>
#include <string.h>
#include "nolan.h"

#define MAX_SLAYERS 60 /* different from raids.c in case a lot joined */

static void parse_file(char *fname, Slayer slayers[], int *nslayers);
static void load_files(Slayer slayers[], int *nslayers);
static int compare(const void *s1, const void *s2);
static void write_lbraid(char *buf, size_t siz, Slayer slayers[], int nslayers);
static void lbraid(char *buf, size_t siz);

/* FIXME rsiz >= siz with strncpy + clean */

void
create_slash_lbraid(struct discord *client)
{
	struct discord_create_global_application_command cmd = {
		.name = "lbraid",
		.description = "Shows the weekly raid leaderboard",
	};
	discord_create_global_application_command(client, APP_ID, &cmd, NULL);
}

static void
parse_file(char *fname, Slayer slayers[], int *nslayers)
{
	FILE *fp;
	int i;
	char line[LINE_SIZE], *endname;
	unsigned long dmg;

	if ((fp = fopen(fname, "r")) == NULL)
		return;

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
			slayers[i].name = strdup(line);
			slayers[i].damage = dmg;
			*nslayers += 1;
		}
	}

	fclose(fp);
}

static void
load_files(Slayer slayers[], int *nslayers)
{
	int i;
	long day = time(NULL) / 86400;
	char fname[64];

	for (i = 0; i < 6; i++) {
		snprintf(fname, sizeof(fname), "%s%ld.csv",
		         RAIDS_FOLDER, day - i);
		if (file_exists(fname)) {
			parse_file(fname, slayers, nslayers);
		}
	}
}

static int
compare(const void *s1, const void *s2)
{
	const long dmg1 = ((unsigned long *)(Slayer *)s1)[1];
	const long dmg2 = ((unsigned long *)(Slayer *)s2)[1];

	return dmg2 - dmg1;
}

/* FIXME: unsafe */
static void
write_lbraid(char *buf, size_t siz, Slayer slayers[], int nslayers)
{
	int i, lb_max = MIN(nslayers, LB_MAX);
	char *p = buf;
	for (i = 0; i < lb_max; i++) {
		sprintf(p, "%d. %s: %'lu damage\n", i, slayers[i].name,
		        slayers[i].damage);
		p = strchr(buf, '\0');
	}
}

static void
lbraid(char *buf, size_t siz)
{
	int i, nslayers = 0;
	Slayer slayers[MAX_SLAYERS];

	load_files(slayers, &nslayers);
	qsort(slayers, nslayers, sizeof(slayers[0]), compare);
	write_lbraid(buf, siz, slayers, nslayers);

	if (nslayers > 0) {
		for (i = 0; i < nslayers; i++) {
			free(slayers[i].name);
		}
	}
}

void
on_lbraid(struct discord *client, const struct discord_message *event)
{
	size_t siz = DISCORD_MAX_MESSAGE_LEN;
	char buf[siz];

	if (event->author->bot)
		return;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	lbraid(buf, siz);
	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

void
on_lbraid_interaction(struct discord *client,
                      const struct discord_interaction *event)
{
	size_t siz = DISCORD_MAX_MESSAGE_LEN;
	char buf[DISCORD_MAX_MESSAGE_LEN];

	lbraid(buf, siz);
	struct discord_interaction_response params = {
		.type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
		.data = &(struct discord_interaction_callback_data)
		{
			.content = buf,
		}
	};
	discord_create_interaction_response(client, event->id, event->token,
	                                    &params, NULL);
}

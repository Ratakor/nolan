#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nolan.h"

static void parse_file(char *fname, Slayer slayers[], size_t *nslayers);
static void load_files(Slayer slayers[], size_t *nslayers);
static void write_invalid(char *buf, size_t siz);
static int compare(const void *s1, const void *s2);
static void write_lbraid(char *buf, int siz, Slayer slayers[], size_t nslayers);
static void lbraid(char *buf, size_t siz);

void
create_slash_lbraid(struct discord *client)
{
	struct discord_create_guild_application_command cmd = {
		.name = "lbraid",
		.description = "Shows the raid leaderboard for the last 7 days",
	};
	discord_create_guild_application_command(client, APP_ID, RAID_GUILD_ID,
	                &cmd, NULL);
}

void
parse_file(char *fname, Slayer slayers[], size_t *nslayers)
{
	FILE *fp;
	char line[LINE_SIZE], *endname;
	unsigned int i;
	unsigned long dmg;

	fp = efopen(fname, "r");
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

void
load_files(Slayer slayers[], size_t *nslayers)
{
	unsigned int i;
	long day = time(NULL) / 86400;
	char fname[128];

	for (i = 0; i < 6; i++) {
		snprintf(fname, sizeof(fname), "%s%ld.csv",
		         RAIDS_FOLDER, day - i);
		if (file_exists(fname))
			parse_file(fname, slayers, nslayers);
	}
}

void
write_invalid(char *buf, size_t siz)
{
	strlcpy(buf, "There is no data for the last 7 days.", siz);
}

int
compare(const void *s1, const void *s2)
{
	const unsigned long dmg1 = ((Slayer *)s1)->damage;
	const unsigned long dmg2 = ((Slayer *)s2)->damage;

	return dmg2 - dmg1;
}

void
write_lbraid(char *buf, int siz, Slayer slayers[], size_t nslayers)
{
	unsigned int i, lb_max = MIN(nslayers, LB_MAX);
	char *p = buf;

	for (i = 0; i < lb_max; i++) {
		siz -= snprintf(p, siz, "%d. %s: %'lu damage\n", i,
		                slayers[i].name, slayers[i].damage);
		p = strchr(buf, '\0');
	}
	if (siz <= 0)
		WARN("string truncation");
}

void
lbraid(char *buf, size_t siz)
{
	unsigned int i;
	size_t nslayers = 0;
	Slayer slayers[MAX_SLAYERS];

	load_files(slayers, &nslayers);
	if (nslayers == 0) {
		write_invalid(buf, siz);
		return;
	}
	qsort(slayers, nslayers, sizeof(slayers[0]), compare);
	write_lbraid(buf, (int)siz, slayers, nslayers);
	for (i = 0; i < nslayers; i++) {
		free(slayers[i].name);
	}
}

void
on_lbraid(struct discord *client, const struct discord_message *event)
{
	char buf[MAX_MESSAGE_LEN];

	if (event->author->bot)
		return;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#else
	if (event->guild_id != RAID_GUILD_ID)
		return;
#endif /* DEVEL */

	lbraid(buf, sizeof(buf));
	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

void
on_lbraid_interaction(struct discord *client,
                      const struct discord_interaction *event)
{
	char buf[MAX_MESSAGE_LEN];

	lbraid(buf, sizeof(buf));
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

/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nolan.h"

static void write_invalid(char *buf, size_t siz);
static int compare(const void *s1, const void *s2);
static void write_lbraid(char *buf, size_t siz, Slayer slayers[], size_t nslayers);
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
write_invalid(char *buf, size_t siz)
{
	strlcpy(buf, "There is no data for the last 7 days.", siz);
}

int
compare(const void *s1, const void *s2)
{
	return ((const Slayer *)s2)->damage - ((const Slayer *)s1)->damage;
}

void
write_lbraid(char *buf, size_t siz, Slayer slayers[], size_t nslayers)
{
	unsigned int i, lb_max;
	size_t s = 0;

	lb_max = MIN(nslayers, LB_MAX);
	s += snprintf(buf + s, siz - s, "Raid stats for the last 7 days:\n");
	for (i = 0; i < lb_max; i++) {
		if (s >= siz) {
			log_warn("%s: string truncation", __func__);
			return;
		}

		s += snprintf(buf + s, siz - s, "%u. %s: ", i, slayers[i].name);
		s += ufmt(buf + s, siz - s, slayers[i].damage);
		s += strlcpy(buf + s, " damage\n", siz - s);
	}
	s += strlcpy(buf + s,
	             "\nOccasional errors may have impacted scores\n"
	             "Please check screenshot data manually and report what's wrong to Ratakor",
	             siz - s);
}

void
lbraid(char *buf, size_t siz)
{
	Slayer *slayers;
	size_t nslayers, i;

	slayers = xcalloc(MAX_SLAYERS, sizeof(*slayers));
	nslayers = load_files(slayers);
	if (nslayers == 0) {
		write_invalid(buf, siz);
		free(slayers);
		return;
	}
	qsort(slayers, nslayers, sizeof(slayers[0]), compare);
	write_lbraid(buf, siz, slayers, nslayers);
	for (i = 0; i < nslayers; i++)
		free(slayers[i].name);
	free(slayers);
}

void
on_lbraid(struct discord *client, const struct discord_message *ev)
{
	char buf[MAX_MESSAGE_LEN];

	if (ev->author->bot)
		return;

#ifdef DEVEL
	if (ev->channel_id != DEVEL)
		return;
#else
	if (ev->guild_id != RAID_GUILD_ID)
		return;
#endif /* DEVEL */

	log_info("%s", __func__);
	lbraid(buf, sizeof(buf));
	discord_send_message(client, ev->channel_id, "%s", buf);
}

void
on_lbraid_interaction(struct discord *client,
                      const struct discord_interaction *ev)
{
	char buf[MAX_MESSAGE_LEN];

	log_info("%s", __func__);
	lbraid(buf, sizeof(buf));
	discord_send_interaction_message(client, ev->id, ev->token, "%s", buf);
}

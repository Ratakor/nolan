#include <stdlib.h>
#include <string.h>

#include "nolan.h"

static void write_invalid(char *buf, size_t siz);
static int compare(const void *p1, const void *p2);
static void write_player(char *buf, size_t siz, unsigned int i, int author);
static void write_leaderboard(char *buf, size_t siz, u64snowflake userid);
static void leaderboard(char *buf, size_t siz, char *txt, u64snowflake userid);

static int category = 0;

void
create_slash_leaderboard(struct discord *client)
{
	struct discord_application_command_option_choice category_choices[] = {
		{
			.name = "Level",
			.value = "\"Level\"",
		},
		{
			.name = "Ascension",
			.value = "\"Ascension\"",
		},
		{
			.name = "Global Rank",
			.value = "\"Global Rank\"",
		},
		{
			.name = "Competitive Rank",
			.value = "\"Competitive Rank\"",
		},
		{
			.name = "Playtime",
			.value = "\"Playtime\"",
		},
		{
			.name = "Monsters Slain",
			.value = "\"Monsters Slain\"",
		},
		{
			.name = "Bosses Slain",
			.value = "\"Bosses Slain\"",
		},
		{
			.name = "Players Defeated",
			.value = "\"Players Defeated\"",
		},
		{
			.name = "Quests Completed",
			.value = "\"Quests Completed\"",
		},
		{
			.name = "Areas Explored",
			.value = "\"Areas Explored\"",
		},
		{
			.name = "Areas Taken",
			.value = "\"Areas Taken\"",
		},
		{
			.name = "Dungeons Cleared",
			.value = "\"Dungeons Cleared\"",
		},
		{
			.name = "Coliseum Wins",
			.value = "\"Coliseum Wins\"",
		},
		{
			.name = "Items Upgraded",
			.value = "\"Items Upgraded\"",
		},
		{
			.name = "Fish Caught",
			.value = "\"Fish Caught\"",
		},
		{
			.name = "Distance Travelled",
			.value = "\"Distance Travelled\"",
		},
		{
			.name = "Reputation",
			.value = "\"Reputation\"",
		},
		{
			.name = "Endless Record",
			.value = "\"Endless Record\"",
		},
		{
			.name = "Codex",
			.value = "\"Codex\"",
		},
	};

	struct discord_application_command_option options[] = {
		{
			.type = DISCORD_APPLICATION_OPTION_STRING,
			.name = "category",
			.description = "category name",
			.choices = &(struct discord_application_command_option_choices)
			{
				.size = LENGTH(category_choices),
				.array = category_choices,
			},
			.required = true
		},
	};
	struct discord_create_global_application_command cmd = {
		.name = "leaderboard",
		.description = "Shows the leaderboard based on a category",
		.options = &(struct discord_application_command_options)
		{
			.size = LENGTH(options),
			.array = options
		},
	};
	discord_create_global_application_command(client, APP_ID, &cmd, NULL);
}

void
write_invalid(char *buf, size_t siz)
{
	unsigned int i;

	strlcpy(buf, "NO WRONG, this is not a valid category.\n", siz);
	strlcat(buf, "Valid categories are:\n", siz);
	/* 2 to not include name and kd, -2 to not include userid and update */
	for (i = 2; i < LENGTH(fields) - 2; i++) {
		if (i == 5) /* regional rank */
			continue;
		strlcat(buf, fields[i], siz);
		strlcat(buf, "\n", siz);
	}
}

int
compare(const void *p1, const void *p2)
{
	const long l1 = ((long *)(Player *)p1)[category];
	const long l2 = ((long *)(Player *)p2)[category];

	/* ranks */
	if (category == 4 || category == 6) {
		if (l1 == 0)
			return 1;
		if (l2 == 0)
			return -1;
		return l1 - l2;
	}

	return l2 - l1;
}

void
write_player(char *buf, size_t siz, unsigned int i, int author)
{
	size_t ssiz = 32;
	char *plt, stat[ssiz];

	if (author)
		snprintf(buf, siz, "%d. **%s**: ", i + 1, players[i].name);
	else
		snprintf(buf, siz, "%d. %s: ", i + 1, players[i].name);
	if (category == 7) { /* playtime */
		plt = playtime_to_str(((long *)&players[i])[category]);
		strlcat(buf, plt, siz);
		free(plt);
	} else {
		snprintf(stat, ssiz, "%'ld", ((long *)&players[i])[category]);
		strlcat(buf, stat, siz);
		if (i == 18) /* distance */
			strlcat(buf, "m", siz);
	}
	strlcat(buf, "\n", siz);
}

void
write_leaderboard(char *buf, size_t siz, u64snowflake userid)
{
	int in_lb = 0;
	unsigned int i, lb_max = MIN(nplayers, LB_MAX);
	size_t psiz = 256, rsiz;
	char player[psiz];
	/* siz = (lb_max + 2) * 64; */

	strlcpy(buf, fields[category], siz);
	strlcat(buf, ":\n", siz);
	for (i = 0; i < lb_max; i++) {
		if (userid == players[i].userid) {
			in_lb = 1;
			write_player(player, psiz, i, 1);
		} else {
			write_player(player, psiz, i, 0);
		}
		rsiz = strlcat(buf, player, siz);
		if (rsiz >= siz) {
			warn("nolan: string truncation happened while writing \
leaderboard.\nThis is probably because LB_MAX is too big.\n");
		}
	}

	if (!in_lb) {
		strlcat(buf, "...\n", siz);
		i = lb_max;
		while (i < nplayers && players[i].userid != userid)
			i++;
		write_player(player, psiz, i, 1);
		rsiz = strlcat(buf, player, siz);
		if (rsiz >= siz) {
			warn("nolan: string truncation happened while writing \
leaderboard.\nThis is probably because LB_MAX is too big.\n");
		}
	}
}

void
leaderboard(char *buf, size_t siz, char *categ, u64snowflake userid)
{
	unsigned int i = 2; /* ignore name and kingdom */

	/* -2 to not include userid and update */
	while (i < LENGTH(fields) - 2 &&
	                strcasecmp(fields[i], categ) != 0)
		i++;

	if (i == LENGTH(fields) - 2 || i == 5) { /* 5 = regional rank */
		write_invalid(buf, siz);
		return;
	}

	category = i;
	qsort(players, nplayers, sizeof(players[0]), compare);
	write_leaderboard(buf, siz, userid);
}

void
on_leaderboard(struct discord *client, const struct discord_message *event)
{
	char buf[MAX_MESSAGE_LEN];

	if (event->author->bot)
		return;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	if (strlen(event->content) == 0)
		write_invalid(buf, sizeof(buf));
	else
		leaderboard(buf, sizeof(buf),
		            event->content, event->author->id);

	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

void
on_leaderboard_interaction(struct discord *client,
                           const struct discord_interaction *event)
{
	char buf[MAX_MESSAGE_LEN];

	if (!event->data->options) {
		write_invalid(buf, sizeof(buf));
	} else {
		leaderboard(buf, sizeof(buf),
		            event->data->options->array[0].value,
		            event->member->user->id);
	}

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


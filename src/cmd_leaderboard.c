#include <string.h>
#include <stdlib.h>
#include "nolan.h"

static void write_invalid(char *buf, size_t siz);
static int compare(const void *p1, const void *p2);
static void write_player(char *buf, size_t siz, int i);
static void write_leaderboard(char *buf, size_t siz, u64snowflake userid);

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

static void
write_invalid(char *buf, size_t siz)
{
	unsigned long i;

	cpstr(buf, "NO WRONG, this is not a valid category.\n", siz);
	catstr(buf, "Valid categories are:\n", siz);
	for (i = 2; i < LENGTH(fields) - 1; i++) {
		if (i == 5) /* regional rank */
			continue;
		catstr(buf, fields[i], siz);
		catstr(buf, "\n", siz);
	}
}

static int
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

static void
write_player(char *buf, size_t siz, int i)
{
	size_t ssiz = 32;
	char *plt, stat[ssiz];

	/* *buf = '\0'; */
	snprintf(buf, siz, "%d. %s: ", i + 1, players[i].name);
	if (category == 7) { /* playtime */
		plt = playtime_to_str(((long *)&players[i])[category]);
		catstr(buf, plt, siz);
		free(plt);
	} else {
		snprintf(stat, ssiz, "%'ld", ((long *)&players[i])[category]);
		catstr(buf, stat, siz);
		if (i == 18) /* distance */
			catstr(buf, "m", siz);
	}
	catstr(buf, "\n", siz);
}

static void
write_leaderboard(char *buf, size_t siz, u64snowflake userid)
{
	int in_lb = 0;
	unsigned long i, lb_max = MIN(nplayers, LB_MAX);
	size_t psiz = 128;
	char player[psiz];
	/* siz = (lb_max + 2) * 64; */

	cpstr(buf, fields[category], siz);
	catstr(buf, ":\n", siz);
	for (i = 0; i < lb_max; i++) {
		if (userid == players[i].userid)
			in_lb = 1;
		write_player(player, psiz, i);
		catstr(buf, player, siz);
	}

	if (!in_lb) {
		catstr(buf, "...\n", siz);
		i = lb_max;
		while (i < nplayers && players[i].userid != userid)
			i++;
		write_player(player, psiz, i);
		catstr(buf, player, siz);
	}
}

void
leaderboard(char *buf, size_t siz, char *categ, u64snowflake userid)
{
	unsigned long i = 2; /* ignore name and kingdom */

	while (i < LENGTH(fields) - 1 &&
	                strcasecmp(fields[i], categ) != 0)
		i++;

	if (i == LENGTH(fields) - 1 || i == 5) { /* 5 = regional rank */
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
	size_t siz = DISCORD_MAX_MESSAGE_LEN;
	char buf[siz];

	if (event->author->bot)
		return;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif

	if (strlen(event->content) == 0)
		write_invalid(buf, siz);
	else
		leaderboard(buf, siz, event->content, event->author->id);

	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

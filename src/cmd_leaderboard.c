/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <stdlib.h>
#include <string.h>

#include "nolan.h"

static void write_invalid(char *buf, size_t siz);
static void split(Player *head, Player **front, Player **back);
static Player *compare(Player *p1, Player *p2);
static void sort(Player **head);
static void write_player(char *buf, size_t siz, Player *player, unsigned rank,
                         bool author);
static void write_leaderboard(char *buf, size_t siz, u64snowflake userid);
static void leaderboard(char *buf, size_t siz, char *txt, u64snowflake userid);

static unsigned int category = 0;

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
		if (i == REGIONAL_RANK)
			continue;
		strlcat(buf, fields[i], siz);
		strlcat(buf, "\n", siz);
	}
}

void
split(Player *head, Player **front, Player **back)
{
	Player *fast, *slow;

	slow = head;
	fast = head->next;

	while (fast) {
		fast = fast->next;
		if (fast) {
			slow = slow->next;
			fast = fast->next;
		}

	}

	*front = head;
	*back = slow->next;
	slow->next = NULL;
}

Player *
compare(Player *p1, Player *p2)
{
	Player *res;
	uint32_t s1, s2;

	if (p1 == NULL)
		return p2;
	if (p2 == NULL)
		return p1;

	s1 = U32CAST(p1)[category];
	s2 = U32CAST(p2)[category];

	if (category == GLOBAL_RANK || category == COMPETITIVE_RANK) {
		if (s2 == 0 || (s1 != 0 && s1 < s2)) {
			res = p1;
			res->next = compare(p1->next, p2);
		} else {
			res = p2;
			res->next = compare(p1, p2->next);
		}
	} else {
		if (s1 > s2) {
			res = p1;
			res->next = compare(p1->next, p2);
		} else {
			res = p2;
			res->next = compare(p1, p2->next);
		}

	}

	return res;
}

void
sort(Player **head)
{
	Player *front, *back;

	if ((head == NULL) || (*head == NULL) || ((*head)->next == NULL))
		return;

	split(*head, &front, &back);
	sort(&front);
	sort(&back);

	*head = compare(front, back);
}

void
write_player(char *buf, size_t siz, Player *player, unsigned rank, bool author)
{
	char *plt, stat[32];

	if (author)
		snprintf(buf, siz, "%u. **%s**: ", rank, player->name);
	else
		snprintf(buf, siz, "%u. %s: ", rank, player->name);
	if (category == PLAYTIME) {
		plt = playtime_to_str(U32CAST(player)[category]);
		strlcat(buf, plt, siz);
		free(plt);
	} else {
		ufmt(stat, sizeof(stat), U32CAST(player)[category]);
		strlcat(buf, stat, siz);
		if (category == DISTANCE)
			strlcat(buf, "m", siz);
	}
	strlcat(buf, "\n", siz);
}

void
write_leaderboard(char *buf, size_t siz, u64snowflake userid)
{
	Player *player;
	char player_buf[256];
	unsigned int rank = 1;
	bool in_lb = false;

	strlcpy(buf, fields[category], siz);
	strlcat(buf, ":\n", siz);
	for (player = player_head; player; player = player->next, rank++) {
		if (rank > LB_MAX)
			break;

		if (player->userid == userid) {
			in_lb = true;
			write_player(player_buf, sizeof(player_buf), player,
			             rank, true);
		} else {
			write_player(player_buf, sizeof(player_buf), player,
			             rank, false);
		}
		if (strlcat(buf, player_buf, siz) >= siz) {
			log_warn("%s: string truncation\n"
			         "\033[33mhint:\033[39m this is probably "
			         "because LB_MAX is too big", __func__);
			return;
		}
	}

	if (in_lb)
		return;

	for (; player && player->userid != userid; player = player->next, rank++);
	if (player == NULL) /* not a player */
		return;

	strlcat(buf, "...\n", siz);
	write_player(player_buf, sizeof(player_buf), player, rank, true);
	if (strlcat(buf, player_buf, siz) >= siz) {
		log_warn("%s: string truncation\n\033[33mhint:\033[39m this "
		         "is probably because LB_MAX is too big", __func__);
	}
}

void
leaderboard(char *buf, size_t siz, char *txt, u64snowflake userid)
{
	unsigned int i = 2; /* ignore name and kingdom */

	/* -2 to not include userid and update */
	while (i < LENGTH(fields) - 2 &&
	                strcasecmp(fields[i], txt) != 0)
		i++;

	if (i == LENGTH(fields) - 2 || i == REGIONAL_RANK) {
		write_invalid(buf, siz);
		return;
	}

	pthread_mutex_lock(&player_mutex);
	category = i;
	sort(&player_head);
	write_leaderboard(buf, siz, userid);
	pthread_mutex_unlock(&player_mutex);
}

void
on_leaderboard(struct discord *client, const struct discord_message *ev)
{
	char buf[MAX_MESSAGE_LEN];

	if (ev->author->bot)
		return;

#ifdef DEVEL
	if (ev->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	log_info("%s", __func__);
	if (strlen(ev->content) == 0)
		write_invalid(buf, sizeof(buf));
	else
		leaderboard(buf, sizeof(buf),
		            ev->content, ev->author->id);

	discord_send_message(client, ev->channel_id, "%s", buf);
}

void
on_leaderboard_interaction(struct discord *client,
                           const struct discord_interaction *ev)
{
	char buf[MAX_MESSAGE_LEN];

	log_info("%s", __func__);
	if (!ev->data->options) {
		write_invalid(buf, sizeof(buf));
	} else {
		leaderboard(buf, sizeof(buf),
		            ev->data->options->array[0].value,
		            ev->member->user->id);
	}

	discord_send_interaction_message(client, ev->id, ev->token, "%s", buf);
}


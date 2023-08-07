/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "nolan.h"

static void write_invalid_category(char *buf, size_t siz);
static void write_invalid_value(char *buf, size_t siz, char *fmt, ...);
static void correct(char *buf, size_t siz, char *category, char *val,
                    u64snowflake userid, u64snowflake guild_id,
                    struct discord *client);
static char *get_value(char *category);

void
create_slash_correct(struct discord *client)
{

	struct discord_application_command_option_choice stat_choices[] = {
		{
			.name = "Name",
			.value = "\"Name\"",
		},
		{
			.name = "Kingdom",
			.value = "\"Kingdom\"",
		},
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
			.name = "Regional Rank",
			.value = "\"Regional Rank\"",
		},
		{
			.name = "Competitive Rank",
			.value = "\"Competitive Rank\"",
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
			.name = "stat",
			.description = "stat name",
			.choices = &(struct discord_application_command_option_choices)
			{
				.size = LENGTH(stat_choices),
				.array = stat_choices
			},
			.required = true
		},
		{
			.type = DISCORD_APPLICATION_OPTION_STRING,
			.name = "new_value",
			.description = "the corrected value",
			.required = true
		}
	};
	struct discord_create_global_application_command cmd = {
		.name = "correct",
		.description = "Correct a stat",
		.options = &(struct discord_application_command_options)
		{
			.size = LENGTH(options),
			.array = options
		}
	};
	discord_create_global_application_command(client, APP_ID, &cmd, NULL);
}

void
write_invalid_category(char *buf, size_t siz)
{
	unsigned int i;

	strlcpy(buf, "NO WRONG, this is not a valid category.\n", siz);
	strlcat(buf, "Valid categories are:\n", siz);
	for (i = 0; i < LENGTH(fields) - 2; i++) {
		if (i == PLAYTIME)
			continue;
		strlcat(buf, fields[i], siz);
		strlcat(buf, "\n", siz);
	}
}

void
write_invalid_value(char *buf, size_t siz, char *fmt, ...)
{
	va_list ap;
	size_t s;

	s = strlcpy(buf, "NO WRONG, this is not a correct value.\n", siz);
	va_start(ap, fmt);
	s += vsnprintf(buf + s, siz - s, fmt, ap);
	va_end(ap);
	if (s >= siz)
		log_warn("%s: string truncation", __func__);
}

void
correct(char *buf, size_t siz, char *category, char *val, u64snowflake userid,
        u64snowflake guild_id, struct discord *client)
{
	Player *player;
	uint32_t old, new;
	size_t i = 0, s = 0;

	pthread_mutex_lock(&player_mutex);
	player = find_player(userid);
	if (player == NULL) {
		strlcpy(buf, "Please register before trying to correct your "
		        "stats.", siz);
		return;
	}

	while (i < LENGTH(fields) - 2 && strcasecmp(fields[i], category) != 0)
		i++;

	if (i == LENGTH(fields) - 2 || i == PLAYTIME) {
		write_invalid_category(buf, siz);
		return;
	}

	if (i == NAME) {
		if (!VALID_STATS(val)) {
			write_invalid_value(buf, siz,
			                    "%c is not a valid char.", DELIM);
			return;
		}
		if (strlen(val) >= MAX_USERNAME_SIZ) {
			write_invalid_value(buf, siz, "Too big username.");
			return;
		}
		snprintf(buf, siz, "<@%lu>\n%s: %s -> %s", userid,
		         fields[i], player->name, val);
		strlcpy(player->name, val, MAX_USERNAME_SIZ);
	} else if (i == KINGDOM) {
		if (!VALID_STATS(val)) {
			write_invalid_value(buf, siz,
			                    "%c is not a valid char.", DELIM);
			return;
		}
		if (strlen(val) >= MAX_KINGDOM_SIZ) {
			write_invalid_value(buf, siz, "Too big kingdom name.");
			return;
		}
		snprintf(buf, siz, "<@%lu>\n%s: %s -> %s", userid,
		         fields[i], player->kingdom, val);
		strlcpy(player->kingdom, val, MAX_KINGDOM_SIZ);
	} else {
		old = U32CAST(player)[i];
		new = trim_stat(val);
		if (new == 0 || new == UINT32_MAX) {
			write_invalid_value(buf, siz, "Too big or zero.");
			return;
		}
		s += snprintf(buf, siz, "<@%lu>\n%s: ", userid, fields[i]);
		s += ufmt(buf + s, siz - s, old);
		if (i == DISTANCE) s += strlcpy(buf + s, "m", siz - s);
		s += strlcpy(buf + s, " -> ", siz - s);
		s += ufmt(buf + s, siz - s, new);
		if (i == DISTANCE) s += strlcpy(buf + s, "m", siz - s);
		U32CAST(player)[i] = new;
	}

	update_file(player);
#ifdef DEVEL
	UNUSED(guild_id);
	UNUSED(client);
#else
	if (guild_id == ROLE_GUILD_ID)
		update_roles(client, player);
#endif /* DEVEL */
	pthread_mutex_unlock(&player_mutex);
}

char *
get_value(char *category)
{
	char *endcat = strchr(category, ' ');

	if (!endcat)
		return NULL;
	*endcat = '\0';

	if (strcasecmp(category, fields[NAME]) != 0 &&
	                strcasecmp(category, fields[KINGDOM]) != 0) {
		*endcat = ' ';
		endcat = strrchr(category, ' ');
		*endcat = '\0';
	}

	return endcat + 1;
}

void
on_correct(struct discord *client, const struct discord_message *ev)
{
	char buf[MAX_MESSAGE_LEN], *category, *val;

	if (ev->author->bot)
		return;

#ifdef DEVEL
	if (ev->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	log_info("%s", __func__);
	if (strlen(ev->content) == 0) {
		write_invalid_category(buf, sizeof(buf));
	} else {
		category = xstrdup(ev->content);
		if (!(val = get_value(category))) {
			write_invalid_value(buf, sizeof(buf),
			                    "Hint: No value 😜");
		} else {
			correct(buf, sizeof(buf), category, val,
			        ev->author->id, ev->guild_id, client);
		}
		free(category);
	}

	discord_send_message(client, ev->channel_id, "%s", buf);
}

void
on_correct_interaction(struct discord *client,
                       const struct discord_interaction *ev)
{
	char buf[MAX_MESSAGE_LEN];

	log_info("%s", __func__);
	if (!ev->data->options) {
		discord_send_interaction_message(client, ev->id, ev->token,
		                                 "idk please ping <@%lu>",
		                                 ADMIN);
		return;
	}

	correct(buf, sizeof(buf), ev->data->options->array[0].value,
	        ev->data->options->array[1].value, ev->member->user->id,
	        ev->guild_id, client);

	discord_send_interaction_message(client, ev->id, ev->token, "%s", buf);
}

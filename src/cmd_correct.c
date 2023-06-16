/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "nolan.h"

static void write_invalid_category(char *buf, size_t siz);
static void write_invalid_value(char *buf, size_t siz, char *fmt, ...);
static void correct(char *buf, size_t siz, char *category, char *val,
                    u64snowflake userid);
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
		WARN("string truncation");
}

void
correct(char *buf, size_t siz, char *category, char *val, u64snowflake userid)
{
	unsigned int i = 0, j = 0;
	long old, new;
	size_t s = 0;

	while (i < nplayers && players[i].userid != userid)
		i++;

	if (i == nplayers) {
		strlcpy(buf, "Please register before trying to correct your \
stats", siz);
		return;
	}

	while (j < LENGTH(fields) - 2 && strcasecmp(fields[j], category) != 0)
		j++;

	if (j == LENGTH(fields) - 2 || j == PLAYTIME) {
		write_invalid_category(buf, siz);
		return;
	}

	if (check_delim(val)) {
		write_invalid_value(buf, siz, "%c is not a valid char", DELIM);
		return;
	}

	if (j == NAME) {
		if (strlen(val) > MAX_USERNAME_LEN) {
			write_invalid_value(buf, siz, "Too big username.");
			return;
		}
		snprintf(buf, siz, "<@%lu>\n%s: %s -> %s", userid,
		         fields[j], players[i].name, val);
		strlcpy(players[i].name, val, MAX_USERNAME_LEN);
	} else if (j == KINGDOM) {
		if (strlen(val) > MAX_KINGDOM_LEN) {
			write_invalid_value(buf, siz, "Too big kingdom name.");
			return;
		}
		snprintf(buf, siz, "<@%lu>\n%s: %s -> %s", userid,
		         fields[j], players[i].kingdom, val);
		strlcpy(players[i].kingdom, val, MAX_KINGDOM_LEN);
	} else {
		old = ((long *)&players[i])[j];
		new = strtol(val, NULL, 10);
		if (new == 0 || new == LONG_MAX) {
			write_invalid_value(buf, siz, "Too big or zero.");
			return;
		}
		s += snprintf(buf, siz, "<@%lu>\n%s: ", userid, fields[j]);
		s += ufmt(buf + s, siz - s, old);
		if (j == DISTANCE) s += strlcpy(buf + s, "m", siz - s);
		s += strlcpy(buf + s, " -> ", siz - s);
		s += ufmt(buf + s, siz - s, new);
		if (j == DISTANCE) s += strlcpy(buf + s, "m", siz - s);
		((long *)&players[i])[j] = new;
	}

	/* TODO: update roles */

	update_file(&players[i]);
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
on_correct(struct discord *client, const struct discord_message *event)
{
	char buf[MAX_MESSAGE_LEN], *category, *val;

	if (event->author->bot)
		return;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	LOG("start");
	if (strlen(event->content) == 0) {
		write_invalid_category(buf, sizeof(buf));
	} else {
		category = strdup(event->content);
		if (!(val = get_value(category))) {
			write_invalid_value(buf, sizeof(buf),
			                    "Hint: No value 😜");
		} else {
			correct(buf, sizeof(buf), category, val,
			        event->author->id);
		}
		free(category);
	}

	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
	LOG("end");
}

void
on_correct_interaction(struct discord *client,
                       const struct discord_interaction *event)
{
	char buf[MAX_MESSAGE_LEN];

	LOG("start");
	if (!event->data->options) {
		snprintf(buf, sizeof(buf), "idk please ping <@%lu>", ADMIN);
	} else {
		correct(buf, sizeof(buf),
		        event->data->options->array[0].value,
		        event->data->options->array[1].value,
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
	LOG("end");
}

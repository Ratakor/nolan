#include <stdlib.h>
#include <string.h>

#include "nolan.h"

static void write_invalid_category(char *buf, size_t siz);
static void write_invalid_value(char *buf, size_t siz, char *fmt, ...);
int check_delim(const char *val);
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
		.description = "Shows Orna information about a user",
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
		if (i == 7) /* playtime */
			continue;
		strlcat(buf, fields[i], siz);
		strlcat(buf, "\n", siz);
	}
}

void
write_invalid_value(char *buf, size_t siz, char *fmt, ...)
{
	va_list ap;
	char *p;

	siz -= strlcpy(buf, "NO WRONG, this is not a correct value.\n", siz);
	p = strchr(buf, '\0');

	va_start(ap, fmt);
	vsnprintf(p, siz, fmt, ap);
	va_end(ap);
}

int
check_delim(const char *val)
{
	do {
		if (*val == DELIM)
			return 1;
	} while (*val++);
	return 0;
}

void
correct(char *buf, size_t siz, char *category, char *val, u64snowflake userid)
{
	unsigned int i = 0, f = 0;
	long old, new;

	while (i < nplayers && players[i].userid != userid)
		i++;

	if (i == nplayers) {
		strlcpy(buf, "Please register before trying to correct your \
stats", siz);
		return;
	}

	while (f < LENGTH(fields) - 2 && strcasecmp(fields[f], category) != 0)
		f++;

	if (f == LENGTH(fields) - 2 || f == 7) { /* playtime */
		write_invalid_category(buf, siz);
		return;
	}

	if (check_delim(val)) {
		write_invalid_value(buf, siz, "%c is not a valid char", DELIM);
		return;
	}

	if (f == 0) { /* name */
		if (strlen(val) > MAX_USERNAME_LEN) {
			write_invalid_value(buf, siz, "Too big username.");
			return;
		}
		snprintf(buf, siz, "<@%lu>\n%s: %s -> %s", userid,
		         fields[f], players[i].name, val);
		strlcpy(players[i].name, val, MAX_USERNAME_LEN);
	} else if (f == 1) { /* kingdom */
		if (strlen(val) > MAX_KINGDOM_LEN) {
			write_invalid_value(buf, siz, "Too big kingdom name.");
			return;
		}
		snprintf(buf, siz, "<@%lu>\n%s: %s -> %s", userid,
		         fields[f], players[i].kingdom, val);
		strlcpy(players[i].kingdom, val, MAX_KINGDOM_LEN);
	} else {
		old = ((long *)&players[i])[f];
		new = strtoul(val, NULL, 10);
		if (new == 0) {
			write_invalid_value(buf, siz, "Use numbers.");
		}
		snprintf(buf, siz, "<@%lu>\n%s: %'ld -> %'ld", userid,
		         fields[f], old, new);
		((long *)&players[i])[f] = new;
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

	if (strcasecmp(category, fields[0]) != 0 &&
	                strcasecmp(category, fields[1]) != 0) {
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
}

void
on_correct_interaction(struct discord *client,
                       const struct discord_interaction *event)
{
	char buf[MAX_MESSAGE_LEN];

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
}

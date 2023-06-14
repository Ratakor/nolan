#include <stdlib.h>
#include <string.h>

#include "nolan.h"

static void write_invalid(char *buf, size_t siz);
static void info_from_uid(char *buf, size_t siz, u64snowflake userid);
static void info_from_txt(char *buf, size_t siz, char *txt);

void
create_slash_info(struct discord *client)
{
	struct discord_application_command_option options[] = {
		{
			.type = DISCORD_APPLICATION_OPTION_STRING,
			.name = "user",
			.description = "discord username or @",
		},
	};
	struct discord_create_global_application_command cmd = {
		.name = "info",
		.description = "Shows Orna information about a user",
		.options = &(struct discord_application_command_options)
		{
			.size = LENGTH(options),
			.array = options
		}
	};
	discord_create_global_application_command(client, APP_ID, &cmd, NULL);
}

u64snowflake
str_to_uid(char *id)
{
	char *start = id, *end = strchr(id, '\0') - 1;

	if (strncmp(start, "<@", 2) == 0 && strncmp(end, ">", 1) == 0)
		return strtoul(start + 2, NULL, 10);
	return 0;
}

void
write_invalid(char *buf, size_t siz)
{
	strlcpy(buf, "This player does not exist in the database.\n", siz);
	strlcat(buf, "To check a player's info type /info ", siz);
	strlcat(buf, "@username or /info username.\n", siz);
	strlcat(buf, "To check your info just type /info.", siz);
}

void
write_info(char *buf, size_t siz, const Player *player)
{
	unsigned int i;
	char *p = buf, *plt;

	/* -2 to not include upadte and userid */
	for (i = 0; i < LENGTH(fields) - 2; i++) {
		if (i == NAME) {
			snprintf(p, siz, "%s: %s\n", fields[i], player->name);
		} else if (i == KINGDOM) {
			if (strcmp(player->kingdom, "(null)") != 0) {
				snprintf(p, siz, "%s: %s\n", fields[i],
				         player->kingdom);
			}
		} else if (i == PLAYTIME) {
			plt = playtime_to_str(((const long *)player)[i]);
			snprintf(p, siz, "%s: %s\n", fields[i], plt);
			free(plt);
		} else if (((const long *)player)[i]) {
			snprintf(p, siz, "%s: %'ld", fields[i],
			         ((const long *)player)[i]);
			if (i == DISTANCE)
				strlcat(buf, "m", siz);
			strlcat(buf, "\n", siz);
		}
		p = strchr(buf, '\0');
	}
}

void
info_from_uid(char *buf, size_t siz, u64snowflake userid)
{
	unsigned int i = 0;

	while (i < nplayers && players[i].userid != userid)
		i++;

	if (i == nplayers)
		write_invalid(buf, siz);
	else
		write_info(buf, siz, &players[i]);
}

void
info_from_txt(char *buf, size_t siz, char *txt)
{
	unsigned int i = 0;
	u64snowflake userid;

	userid = str_to_uid(txt);
	if (userid) {
		info_from_uid(buf, siz, userid);
		return;
	}

	while (i < nplayers && strcasecmp(players[i].name, txt) != 0)
		i++;
	if (i == nplayers)
		write_invalid(buf, siz);
	else
		write_info(buf, siz, &players[i]);
}

void
on_info(struct discord *client, const struct discord_message *event)
{
	char buf[MAX_MESSAGE_LEN];

	if (event->author->bot)
		return;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	LOG("start");
	if (strlen(event->content) == 0)
		info_from_uid(buf, sizeof(buf), event->author->id);
	else
		info_from_txt(buf, sizeof(buf), event->content);

	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
	LOG("end");
}

void
on_info_interaction(struct discord *client,
                    const struct discord_interaction *event)
{
	char buf[MAX_MESSAGE_LEN];

	LOG("start");
	if (!event->data->options)
		info_from_uid(buf, sizeof(buf), event->member->user->id);
	else
		info_from_txt(buf, sizeof(buf),
		              event->data->options->array[0].value);

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

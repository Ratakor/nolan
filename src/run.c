#include <string.h>

#include "nolan.h"

void
on_interaction(struct discord *client, const struct discord_interaction *event)
{
	if (event->type != DISCORD_INTERACTION_APPLICATION_COMMAND)
		return;
	if (!event->data)
		return;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	if (strcmp(event->data->name, "help") == 0)
		on_help_interaction(client, event);
	else if (strcmp(event->data->name, "stats") == 0)
		on_stats_interaction(client, event);
	else if (strcmp(event->data->name, "info") == 0)
		on_info_interaction(client, event);
	else if (strcmp(event->data->name, "leaderboard") == 0)
		on_leaderboard_interaction(client, event);
	else if (strcmp(event->data->name, "correct") == 0)
		on_correct_interaction(client, event);
	else if (strcmp(event->data->name, "source") == 0)
		on_source_interaction(client, event);
	else if (strcmp(event->data->name, "lbraid") == 0)
		on_lbraid_interaction(client, event);
	else if (strcmp(event->data->name, "uraid") == 0)
		on_uraid_interaction(client, event);
}

void
on_message(struct discord *client, const struct discord_message *event)
{
	unsigned int i;

	if (event->author->bot)
		return;
	if (event->attachments->size == 0)
		return;
	if (strchr(event->attachments->array->filename, '.') == NULL)
		return;
	if (strcmp(event->attachments->array->content_type, "image/jpeg") != 0
	                && strcmp(event->attachments->array->content_type,
	                          "image/png") != 0)
		return;

#ifdef DEVEL
	if (event->channel_id == DEVEL)
		on_stats(client, event);
	return;
#endif /* DEVEL */

	for (i = 0; i < LENGTH(stats_ids); i++) {
		if (event->channel_id == stats_ids[i]) {
			on_stats(client, event);
			return;
		}
	}

	for (i = 0; i < LENGTH(raids_ids); i++) {
		if (event->channel_id == raids_ids[i]) {
			on_raids(client, event);
			return;
		}
	}
}

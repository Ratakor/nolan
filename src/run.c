/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <string.h>

#include "nolan.h"

void
on_interaction(struct discord *client, const struct discord_interaction *ev)
{
	if (ev->type != DISCORD_INTERACTION_APPLICATION_COMMAND)
		return;
	if (!ev->data)
		return;

#ifdef DEVEL
	if (ev->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	if (strcmp(ev->data->name, "help") == 0)
		on_help_interaction(client, ev);
	else if (strcmp(ev->data->name, "stats") == 0)
		on_stats_interaction(client, ev);
	else if (strcmp(ev->data->name, "info") == 0)
		on_info_interaction(client, ev);
	else if (strcmp(ev->data->name, "leaderboard") == 0)
		on_leaderboard_interaction(client, ev);
	else if (strcmp(ev->data->name, "correct") == 0)
		on_correct_interaction(client, ev);
	else if (strcmp(ev->data->name, "source") == 0)
		on_source_interaction(client, ev);
	else if (strcmp(ev->data->name, "lbraid") == 0)
		on_lbraid_interaction(client, ev);
	else if (strcmp(ev->data->name, "uraid") == 0)
		on_uraid_interaction(client, ev);
	else if (strcmp(ev->data->name, "time") == 0)
		on_time_interaction(client, ev);
}

void
on_message(struct discord *client, const struct discord_message *ev)
{
	unsigned int i;

	if (ev->author->bot)
		return;
	if (ev->attachments->size == 0)
		return;
	if (strchr(ev->attachments->array->filename, '.') == NULL)
		return;
	if (strcmp(ev->attachments->array->content_type, "image/jpeg") != 0
	                && strcmp(ev->attachments->array->content_type,
	                          "image/png") != 0)
		return;

#ifdef DEVEL
	if (ev->channel_id == DEVEL)
		on_raids(client, ev);
	return;
#endif /* DEVEL */

	for (i = 0; i < LENGTH(stats_ids); i++) {
		if (ev->channel_id == stats_ids[i]) {
			on_stats(client, ev);
			return;
		}
	}

	for (i = 0; i < LENGTH(raids_ids); i++) {
		if (ev->channel_id == raids_ids[i]) {
			on_raids(client, ev);
			return;
		}
	}
}

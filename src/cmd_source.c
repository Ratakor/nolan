/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <stdlib.h>
#include <string.h>

#include "nolan.h"

static char *load_source(size_t *fszp);
static char *load_sorted_source(size_t *fszp, char *kingdom);

void
create_slash_source(struct discord *client)
{
	struct discord_application_command_option options[] = {
		{
			.type = DISCORD_APPLICATION_OPTION_STRING,
			.name = "kingdom",
			.description = "optional kingdom name to sort the file",
		},
	};
	struct discord_create_global_application_command cmd = {
		.name = "source",
		.description = "Returns the source file with everyone stats",
		.options = &(struct discord_application_command_options)
		{
			.size = LENGTH(options),
			.array = options
		},
	};
	discord_create_global_application_command(client, APP_ID, &cmd, NULL);
}

char *
load_source(size_t *fszp)
{
	FILE *fp;
	char *res, line[LINE_SIZE], *end;
	size_t mfsz = LINE_SIZE + nplayers * LINE_SIZE;

	fp = efopen(STATS_FILE, "r");
	res = emalloc(mfsz);
	*res = '\0';
	while (fgets(line, LINE_SIZE, fp) != NULL) {
		/* skip everything after codex */
		if ((end = nstrchr(line, DELIM, CODEX + 1))) {
			*end = '\n';
			*(end + 1) = '\0';
		}
		strlcat(res, line, mfsz);
	}

	fclose(fp);
	*fszp = strlen(res);
	return res;
}

char *
load_sorted_source(size_t *fszp, char *kingdom)
{
	FILE *fp;
	char *res, line[LINE_SIZE], *kd, *endkd, *end;
	size_t mfsz = LINE_SIZE + nplayers * LINE_SIZE;

	fp = efopen(STATS_FILE, "r");
	res = emalloc(mfsz);
	fgets(line, LINE_SIZE, fp); /* fields name */
	/* skip everything after codex */
	if ((end = nstrchr(line, DELIM, CODEX + 1))) {
		*end = '\n';
		*(end + 1) = '\0';
	}
	strlcpy(res, line, mfsz);
	while (fgets(line, LINE_SIZE, fp) != NULL) {
		kd = strchr(line, DELIM) + 1;
		if ((endkd = nstrchr(line, DELIM, 2)))
			* endkd = '\0';
		if (strcmp(kd, kingdom) == 0) {
			if (endkd)
				*endkd = DELIM;
			/* skip everything after codex */
			if ((end = nstrchr(line, DELIM, CODEX + 1))) {
				*end = '\n';
				*(end + 1) = '\0';
			}
			strlcat(res, line, mfsz);
		}
	}

	fclose(fp);
	*fszp = strlen(res);
	return res;
}

void
on_source(struct discord *client, const struct discord_message *event)
{
	size_t fsiz = 0;
	char *fbuf = NULL;

	if (event->author->bot)
		return;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	LOG("start");
	if (strlen(event->content) == 0)
		fbuf = load_source(&fsiz);
	else
		fbuf = load_sorted_source(&fsiz, event->content);

	struct discord_attachment attachment = {
		.filename = FILENAME,
		.content = fbuf,
		.size = fsiz
	};
	struct discord_attachments attachments = {
		.size = 1,
		.array = &attachment
	};
	struct discord_create_message msg = {
		.attachments = &attachments
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
	free(fbuf);
	LOG("end");
}

void
on_source_interaction(struct discord *client,
                      const struct discord_interaction *event)
{
	size_t fsiz = 0;
	char *fbuf = NULL;

	LOG("start");
	if (!event->data->options)
		fbuf = load_source(&fsiz);
	else
		fbuf = load_sorted_source(&fsiz,
		                          event->data->options->array[0].value);

	struct discord_attachment attachment = {
		.filename = FILENAME,
		.content = fbuf,
		.size = fsiz
	};
	struct discord_attachments attachments = {
		.size = 1,
		.array = &attachment
	};
	struct discord_interaction_response params = {
		.type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
		.data = &(struct discord_interaction_callback_data)
		{
			.attachments = &attachments
		}
	};
	discord_create_interaction_response(client, event->id, event->token,
	                                    &params, NULL);
	free(fbuf);
	LOG("end");
}

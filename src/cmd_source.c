/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <stdlib.h>
#include <string.h>

#include "nolan.h"

static char *load_source(void);
static char *load_sorted_source(char *kingdom);

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
load_source(void)
{
	FILE *fp;
	char *res = NULL, line[LINE_SIZE], *end;
	size_t llen, len = 0;

	fp = xfopen(STATS_FILE, "r");
	while (fgets(line, sizeof(line), fp)) {
		/* skip everything after codex */
		if ((end = nstrchr(line, DELIM, CODEX + 1))) {
			*end = '\n';
			*(end + 1) = '\0';
		}
		llen = strlen(line);
		res = xrealloc(res, len + llen + 1);
		memcpy(res + len, line, llen);
		len += llen;
	}
	fclose(fp);

	return res;
}

char *
load_sorted_source(char *kingdom)
{
	FILE *fp;
	char *res, line[LINE_SIZE], *kd, *endkd, *end;
	size_t llen, len;

	fp = xfopen(STATS_FILE, "r");
	if (fgets(line, LINE_SIZE, fp) == NULL) /* fields name */
		die(1, "Field name line in %s is wrong", STATS_FILE);
	/* skip everything after codex */
	if ((end = nstrchr(line, DELIM, CODEX + 1))) {
		*end = '\n';
		*(end + 1) = '\0';
	}
	res = xstrdup(line);
	len = strlen(res);
	while (fgets(line, LINE_SIZE, fp)) {
		kd = strchr(line, DELIM) + 1;
		if ((endkd = nstrchr(line, DELIM, KINGDOM + 1)))
			* endkd = '\0';
		if (strcmp(kd, kingdom) != 0)
			continue;
		if (endkd)
			*endkd = DELIM;
		/* skip everything after codex */
		if ((end = nstrchr(line, DELIM, CODEX + 1))) {
			*end = '\n';
			*(end + 1) = '\0';
		}
		llen = strlen(line);
		res = xrealloc(res, len + llen + 1);
		memcpy(res + len, line, llen);
		len += llen;
	}
	fclose(fp);

	return res;
}

void
on_source(struct discord *client, const struct discord_message *event)
{
	char *fbuf = NULL;

	if (event->author->bot)
		return;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	log_info("%s", __func__);
	if (strlen(event->content) == 0)
		fbuf = load_source();
	else
		fbuf = load_sorted_source(event->content);

	struct discord_attachment attachment = {
		.filename = FILENAME,
		.content = fbuf,
		.size = strlen(fbuf)
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
}

void
on_source_interaction(struct discord *client,
                      const struct discord_interaction *event)
{
	char *fbuf = NULL;

	log_info("%s", __func__);
	if (!event->data->options)
		fbuf = load_source();
	else
		fbuf = load_sorted_source(event->data->options->array[0].value);

	struct discord_attachment attachment = {
		.filename = FILENAME,
		.content = fbuf,
		.size = strlen(fbuf)
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
}

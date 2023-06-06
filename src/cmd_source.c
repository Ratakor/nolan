#include <stdlib.h>
#include <string.h>

#include "nolan.h"

static char *sort_source(char *kingdom, size_t *fszp);

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
sort_source(char *kingdom, size_t *fszp)
{
	FILE *fp;
	char *res, line[LINE_SIZE], *kd, *endkd;
	size_t mfsz = LINE_SIZE + nplayers * LINE_SIZE;

	if ((fp = fopen(STATS_FILE, "r")) == NULL)
		die("nolan: Failed to open %s (read)\n", STATS_FILE);

	res = emalloc(mfsz);
	fgets(line, LINE_SIZE, fp); /* fields name */
	strlcpy(res, line, mfsz);
	while (fgets(line, LINE_SIZE, fp) != NULL) {
		kd = strchr(line, DELIM) + 1;
		endkd = nstrchr(line, DELIM, 2);
		if (endkd)
			*endkd = '\0';
		if (strcmp(kd, kingdom) == 0) {
			*endkd = DELIM;
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

	if (strlen(event->content) == 0)
		fbuf = cog_load_whole_file(STATS_FILE, &fsiz);
	else
		fbuf = sort_source(event->content, &fsiz);

	struct discord_attachment attachment = {
		.filename = STATS_FILE,
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
}

void
on_source_interaction(struct discord *client,
                      const struct discord_interaction *event)
{
	size_t fsiz = 0;
	char *fbuf = NULL;

	if (!event->data || !event->data->options)
		fbuf = cog_load_whole_file(STATS_FILE, &fsiz);
	else
		fbuf = sort_source(event->data->options->array[0].value, &fsiz);

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
}

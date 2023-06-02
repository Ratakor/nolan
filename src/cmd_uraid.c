#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "nolan.h"

static void write_invalid(char *buf, size_t siz);
static unsigned long parse_file(char *fname, char *username);
static void write_uraid(char *buf, int siz, char *username,
                        unsigned long *dmgs);
static unsigned long *load_files(char *username);
static void uraid(char *buf, size_t siz, char *username);

void
create_slash_uraid(struct discord *client)
{
	struct discord_application_command_option options[] = {
		{
			.type = DISCORD_APPLICATION_OPTION_STRING,
			.name = "username",
			.description = "username in Orna",
			.required = true
		},
	};
	struct discord_create_global_application_command cmd = {
		.name = "uraid",
		.description = "Shows the user raid damage for the last 7 days",
		.options = &(struct discord_application_command_options)
		{
			.size = LENGTH(options),
			.array = options
		},
	};
	discord_create_global_application_command(client, APP_ID, &cmd, NULL);
}

static void
write_invalid(char *buf, size_t siz)
{
	strlcpy(buf, "NO WRONG YOU MUST USE AN ARGUMENT.\n", siz);
	strlcat(buf, "Valid argument is an Orna username.\n", siz);
}

static unsigned long
parse_file(char *fname, char *username)
{
	FILE *fp;
	unsigned long dmg;
	char line[LINE_SIZE], *endname;

	if ((fp = fopen(fname, "r")) == NULL)
		return 0;

	while (fgets(line, LINE_SIZE, fp)) {
		endname = strchr(line, DELIM);
		dmg = strtoul(endname + 1, NULL, 10);
		if (endname)
			*endname = '\0';
		if (strcmp(username, line) == 0) {
			fclose(fp);
			return dmg;
		}
	}

	fclose(fp);
	return 0;
}

static unsigned long *
load_files(char *username)
{
	int i;
	unsigned long *dmgs = calloc(7, sizeof(unsigned long));
	long day = time(NULL) / 86400;
	char fname[128];

	for (i = 0; i < 6; i++) {
		snprintf(fname, sizeof(fname), "%s%ld.csv",
		         RAIDS_FOLDER, day - i);
		if (file_exists(fname)) {
			dmgs[i] = parse_file(fname, username);
		}
	}

	return dmgs;
}

static void
write_uraid(char *buf, int siz, char *username, unsigned long *dmgs)
{
	int i, n = 1;
	char *p;
	unsigned long total = 0;

	siz -= snprintf(buf, siz, "%s raids stats for last 7 days\n", username);
	p = strchr(buf, '\0');
	for (i = 6; i >= 0; i--) {
		siz -= snprintf(p, siz, "day %d: %'lu damage\n", n++, dmgs[i]);
		if (siz <= 0)
			warn("nolan: truncation in write_uraid\n");
		total += dmgs[i];
		p = strchr(buf, '\0');
	}
	snprintf(p, siz, "\ntotal: %'lu damage\n", total);

}

static void
uraid(char *buf, size_t siz, char *username)
{
	unsigned long *dmgs;

	dmgs = load_files(username);
	write_uraid(buf, siz, username, dmgs);
	free(dmgs);
}

void
on_uraid(struct discord *client, const struct discord_message *event)
{
	size_t siz = DISCORD_MAX_MESSAGE_LEN;
	char buf[siz];

	if (event->author->bot)
		return;

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	if (strlen(event->content) == 0)
		write_invalid(buf, siz);
	else
		uraid(buf, siz, event->content);

	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

void
on_uraid_interaction(struct discord *client,
                     const struct discord_interaction *event)
{
	size_t siz = DISCORD_MAX_MESSAGE_LEN;
	char buf[siz];

	if (!event->data || !event->data->options)
		write_invalid(buf, siz);
	else
		uraid(buf, siz, event->data->options->array[0].value);

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

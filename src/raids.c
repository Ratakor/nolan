#include <stdlib.h>
#include <string.h>
#include "nolan.h"

void
raids(struct discord *client, const struct discord_message *event)
{
	char *txt, *line, *endline;

	curl(event->attachments->array->url, "./images/raids.jpg");
	txt = ocr("./images/raids.jpg");

	/* TODO */
	line = txt;
	while (line) {
		endline = strchr(line, '\n');
		if (endline)
			*endline = '\0';
		if (strncmp(line, "+ Raid options", 14) == 0) {
			line = endline + 1;
			break;
		}
		line = endline ? (endline + 1) : 0;
	}

	struct discord_create_message msg = {
		.content = line
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
	free(txt);
}

/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#include <concord/discord.h>
#include <concord/discord-internal.h>

#include <locale.h>
#include <signal.h>

#include "nolan.h"

static struct discord *client;
Player *player_head;
pthread_mutex_t player_mutex = PTHREAD_MUTEX_INITIALIZER;
const char *fields[] = {
	"Name",
	"Kingdom",
	"Level",
	"Ascension",
	"Global Rank",
	"Regional Rank",
	"Competitive Rank",
	"Playtime",
	"Monsters Slain",
	"Bosses Slain",
	"Players Defeated",
	"Quests Completed",
	"Areas Explored",
	"Areas Taken",
	"Dungeons Cleared",
	"Coliseum Wins",
	"Items Upgraded",
	"Fish Caught",
	"Distance Travelled",
	"Reputation",
	"Endless Record",
	"Codex",
	"Last Update",
	"User ID",
};

/* This function is under MIT License, Copyright (c) 2022 Cogmasters */
static void
log_callback(log_Event *ev)
{
	char buf[16];

	buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
	fprintf(ev->udata, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ", buf,
	        level_colors[ev->level], level_strings[ev->level], ev->file,
	        ev->line);
	vfprintf(ev->udata, ev->fmt, ev->ap);
	fputc('\n', ev->udata);
	fflush(ev->udata);
}

static void
cleanup(void)
{
	Player *player, *next;

	discord_cleanup(client);
	ccord_global_cleanup();
	pthread_mutex_lock(&player_mutex);
	for (player = player_head; player; player = next) {
		next = player->next;
		free(player);
	}
	player_head = NULL;
	pthread_mutex_unlock(&player_mutex);
}

static void
sighandler(int sig)
{
	UNUSED(sig);
	ccord_shutdown_async();
}

int
main(void)
{
	char *src[] = { "src", "source" };
	char *lb[] = { "lb", "leaderboard" };

	create_folders();
	create_stats_file();
	init_players();

	/* init curl, ccord global and client */
	client = discord_init(TOKEN);

	/* client->conf.http = try (calloc(1, sizeof(*(client->conf.http)))); */
	/* client->conf.http->f = stderr; */
	logconf_add_callback(&client->conf, &log_callback, stderr, LOG_WARN);
	discord_add_intents(client, DISCORD_GATEWAY_MESSAGE_CONTENT |
	                    DISCORD_GATEWAY_GUILD_MEMBERS);
	discord_set_prefix(client, PREFIX);
	create_slash_commands(client);
	discord_set_on_ready(client, on_ready);
	discord_set_on_interaction_create(client, on_interaction);
	discord_set_on_message_create(client, on_message);
	discord_set_on_commands(client, lb, LENGTH(lb), on_leaderboard);
	discord_set_on_command(client, "info", on_info);
	discord_set_on_commands(client, src, LENGTH(src), on_source);
	discord_set_on_command(client, "help", on_help);
	discord_set_on_command(client, "lbraid", on_lbraid);
	discord_set_on_command(client, "uraid", on_uraid);
	discord_set_on_command(client, "correct", on_correct);
	discord_set_on_command(client, "time", on_time);

	setlocale(LC_ALL, "");
	signal(SIGINT, &sighandler);

	discord_run(client);

	cleanup();

	return EXIT_SUCCESS;
}

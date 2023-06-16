#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include "concord/discord.h"
#include "concord/discord-internal.h"

#include "nolan.h"

Player players[MAX_PLAYERS];
size_t nplayers;
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

/* bug in concord */
#include <signal.h>
struct discord *cp;
static void
sighandler(int signal)
{
	UNUSED(signal);
	struct discord_create_message msg = {
		.content = "I died"
	};
#ifdef DEVEL
	discord_create_message(cp, DEVEL, &msg, NULL);
#else
	discord_create_message(cp, 1110767040890941590, &msg, NULL);
#endif /* DEVEL */
	DIE("segmentation fault");
}
/* bug in concord */

int
main(int argc, char *argv[])
{
	char *src[] = { "src", "source" };
	char *lb[] = { "lb", "leaderboard" };
	struct discord *client;

#ifdef DEVEL
	UNUSED(argv);
#else
	if (getuid() != 0)
		DIE("please run %s as root", argv[0]);
#endif /* DEVEL */

	UNUSED(argc);
	setlocale(LC_NUMERIC, "");
	create_folders();
	create_stats_file();
	init_players();

	ccord_global_init(); /* init curl too */
	client = discord_init(TOKEN); /* init ccord_global_init() too */

	/* bug in concord */
	cp = client;
	signal(SIGSEGV, sighandler);
	/* bug in concord */

	logconf_set_quiet(&client->conf, 1); /* 0 to enable native ccord log */
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

	discord_run(client);

	discord_cleanup(client);
	ccord_global_cleanup();

	return EXIT_SUCCESS;
}

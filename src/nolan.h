#ifndef NOLAN_H
#define NOLAN_H

#include <concord/discord.h>

#include "../config.h"
#include "util.h"

#define MAX_PLAYERS      LENGTH(kingdoms) * 50 /* might need to update that */
#define MAX_SLAYERS      50
#define LINE_SIZE        300 + 1
#define MAX_MESSAGE_LEN  2000 + 1
#define MAX_USERNAME_LEN 32 + 1
#define MAX_KINGDOM_LEN  32 + 1
#ifdef DEVEL
#define SAVE_FOLDER      "./"
#else
#define SAVE_FOLDER      "/var/lib/nolan/"
#endif /* DEVEL */
#define IMAGES_FOLDER    SAVE_FOLDER "images/"
#define RAIDS_FOLDER     SAVE_FOLDER "raids/"
#define STATS_FILE       SAVE_FOLDER FILENAME
#define ROLE_GUILD_ID    999691133103919135 /* this is only for to Orna FR */

/* ALL FIELDS MUST HAVE THE SAME SIZE */
typedef struct {
	char *name;
	char *kingdom;
	long level;
	long ascension;
	long global;
	long regional;
	long competitive;
	long playtime;
	long monsters;
	long bosses;
	long players;
	long quests;
	long explored;
	long taken;
	long dungeons;
	long coliseum;
	long items;
	long fish;
	long distance;
	long reputation;
	long endless;
	long codex;
	time_t update;
	u64snowflake userid;
} Player;

/* this is for raids */
typedef struct {
	char *name;
	unsigned long damage;
	int found_in_file;
} Slayer;

extern Player players[MAX_PLAYERS];
extern size_t nplayers;
extern const char *fields[24];

/* init.c */
void create_folders(void);
void create_stats_file(void);
void init_players(void);
void create_slash_commands(struct discord *client);
void on_interaction(struct discord *client,
		const struct discord_interaction *event);
void on_ready(struct discord *client, const struct discord_ready *event);
void on_message(struct discord *client, const struct discord_message *event);

/* ocr.c */
void curl(char *url, char *fname);
int crop(char *fname, int type);
char *ocr(char *fname, char *lang);

/* stats.c */
void create_slash_stats(struct discord *client);
char *playtime_to_str(long playtime);
void on_stats(struct discord *client, const struct discord_message *event);
void on_stats_interaction(struct discord *client,
		const struct discord_interaction *event);

/* raids.c */
void on_raids(struct discord *client, const struct discord_message *event);

/* roles.c */
void update_roles(struct discord *client, u64snowflake userid, Player *player);

/* cmd_help.c */
void create_slash_help(struct discord *client);
void on_help(struct discord *client, const struct discord_message *event);
void on_help_interaction(struct discord *client,
		const struct discord_interaction *event);

/* cmd_info.c */
void create_slash_info(struct discord *client);
void write_info(char *buf, size_t siz, const Player *player);
void on_info(struct discord *client, const struct discord_message *event);
void on_info_interaction(struct discord *client,
		const struct discord_interaction *event);

/* cmd_leaderboard.c */
void create_slash_leaderboard(struct discord *client);
void on_leaderboard(struct discord *client,
		const struct discord_message *event);
void on_leaderboard_interaction(struct discord *client,
		const struct discord_interaction *event);

/* cmd_source.c */
void create_slash_source(struct discord *client);
void on_source(struct discord *client, const struct discord_message *event);
void on_source_interaction(struct discord *client,
		const struct discord_interaction *event);

/* cmd_lbraid.c */
void create_slash_lbraid(struct discord *client);
void on_lbraid(struct discord *client, const struct discord_message *event);
void on_lbraid_interaction(struct discord *client,
		const struct discord_interaction *event);

/* cmd_uraid.c */
void create_slash_uraid(struct discord *client);
void on_uraid(struct discord *client, const struct discord_message *event);
void on_uraid_interaction(struct discord *client,
		const struct discord_interaction *event);

#endif /* NOLAN_H */

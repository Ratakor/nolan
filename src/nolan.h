/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#ifndef NOLAN_H
#define NOLAN_H

#include <concord/discord.h>
#include <concord/log.h>

#include "../config.h"
#include "../libre/libre.h"

/* normally 50 but let's do * 3 to prevent from buffer overflow :) */
#define MAX_SLAYERS      50 * 3
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

enum {
	NAME,
	KINGDOM,
	LEVEL,
	ASCENSION,
	GLOBAL_RANK,
	REGIONAL_RANK,
	COMPETITIVE_RANK,
	PLAYTIME,
	MONSTERS,
	BOSSES,
	PLAYERS_DEFEATED,
	QUESTS,
	AREAS_EXPLORED,
	AREAS_TAKEN,
	DUNGEONS,
	COLISEUM,
	ITEMS,
	FISH,
	DISTANCE,
	REPUTATION,
	ENDLESS,
	CODEX,
	UPDATE,
	USERID,
};

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
	uint32_t damage;
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
void on_ready(struct discord *client, const struct discord_ready *event);

/* run.c */
void on_interaction(struct discord *client,
                    const struct discord_interaction *event);
void on_message(struct discord *client, const struct discord_message *event);

/* ocr.c */
char *curl(char *url);
unsigned int curl_file(char *url, char *fname);
int crop(char *fname, int type);
char *ocr(const char *fname, const char *lang);

/* stats.c */
void create_slash_stats(struct discord *client);
bool check_delim(const char *val);
long trim_stat(const char *str);
char *playtime_to_str(long playtime);
void update_file(Player *player);
void on_stats(struct discord *client, const struct discord_message *event);
void on_stats_interaction(struct discord *client,
                          const struct discord_interaction *event);

/* raids.c */
void on_raids(struct discord *client, const struct discord_message *event);
void parse_file(char *fname, Slayer slayers[], size_t *nslayers);
void load_files(Slayer slayers[], size_t *nslayers);

/* roles.c */
void update_roles(struct discord *client, Player *player);

/* cmd_help.c */
void create_slash_help(struct discord *client);
void on_help(struct discord *client, const struct discord_message *event);
void on_help_interaction(struct discord *client,
                         const struct discord_interaction *event);

/* cmd_info.c */
u64snowflake str_to_uid(char *id);
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

/* cmd_correct.c */
void create_slash_correct(struct discord *client);
void on_correct(struct discord *client, const struct discord_message *event);
void on_correct_interaction(struct discord *client,
                            const struct discord_interaction *event);

/* cmd_time.c */
void create_slash_time(struct discord *client);
void on_time(struct discord *client, const struct discord_message *event);
void on_time_interaction(struct discord *client,
                            const struct discord_interaction *event);

#endif /* NOLAN_H */

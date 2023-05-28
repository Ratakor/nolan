#include <concord/discord.h>

#include "../config.h"
#include "util.h"

#define MAX_PLAYERS  LENGTH(kingdoms) * 50
#define LINE_SIZE    300 + 1
#define SAVE_FOLDER  "/var/lib/nolan/"
#define IMAGE_FOLDER SAVE_FOLDER "images/"
#define STATS_FILE   SAVE_FOLDER FILENAME

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
	u64snowflake userid;
} Player;

extern Player players[MAX_PLAYERS];
extern size_t nplayers;
extern const char *fields[23];

/* init.c */
void create_folders(void);
void create_stats_file(void);
void init_players(void);
void create_slash_commands(struct discord *client);
void on_interaction(struct discord *client, const struct discord_interaction *event);
void on_ready(struct discord *client, const struct discord_ready *event);
void on_message(struct discord *client, const struct discord_message *event);

/* ocr.c */
void curl(char *url, char *fname);
char *ocr(char *fname);

/* stats.c */
char *playtime_to_str(long playtime);
Player create_player(unsigned int line);
void stats(struct discord *client, const struct discord_message *event);

/* raids.c */
void raids(struct discord *client, const struct discord_message *event);

/* cmd_help.c */
void create_slash_help(struct discord *client);
void help(char *buf, size_t siz);
void on_help(struct discord *client, const struct discord_message *event);

/* cmd_info.c */
void create_slash_info(struct discord *client);
void info_from_uid(char *buf, size_t siz, u64snowflake userid);
void info_from_txt(char *buf, size_t siz, char *txt);
void on_info(struct discord *client, const struct discord_message *event);

/* cmd_leaderboard.c */
void create_slash_leaderboard(struct discord *client);
void leaderboard(char *buf, size_t siz, char *txt, u64snowflake userid);
void on_leaderboard(struct discord *client, const struct discord_message *event);

/* cmd_source.c */
void create_slash_source(struct discord *client);
char *sort_source(char *kingdom, size_t *fszp);
void on_source(struct discord *client, const struct discord_message *event);

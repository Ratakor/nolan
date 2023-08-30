/* Copywrong © 2023 Ratakor. See LICENSE file for license details. */

#ifndef NOLAN_H
#define NOLAN_H

#include <pthread.h>
#include <concord/discord.h>
#include <concord/log.h>

#include "dalloc.h"
#include "../config.h"

/* normally 50 but let's do * 3 to prevent from buffer overflow :) */
#define MAX_SLAYERS      50 * 3
#define LINE_SIZE        300 + 1
#define MAX_MESSAGE_LEN  2000 + 1
#define MAX_USERNAME_SIZ 32
#define MAX_KINGDOM_SIZ  32

#ifdef DEVEL
#define SAVE_FOLDER      "./"
#else
#define SAVE_FOLDER      "/var/lib/nolan/"
#endif /* DEVEL */

#define IMAGES_FOLDER    SAVE_FOLDER "images/"
#define RAIDS_FOLDER     SAVE_FOLDER "raids/"
#define STATS_FILE       SAVE_FOLDER FILENAME

#define MAX(X, Y)        ((X) > (Y) ? (X) : (Y))
#define MIN(X, Y)        ((X) < (Y) ? (X) : (Y))
#define LENGTH(X)        (sizeof(X) / sizeof(X[0]))
#define STRLEN(X)        (sizeof(X) - 1)
#define UNUSED(X)        ((void)(X))
#define VALID_STATS(X)   (strchr(X, DELIM) == 0)
#define U32CAST2(player)\
	((uint32_t *)((char *)(player) + 32 + 32) - 2)

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

typedef struct Player Player;
struct Player {
	char name[MAX_USERNAME_SIZ];
	char kingdom[MAX_KINGDOM_SIZ];
	uint32_t level;
	uint32_t ascension;
	uint32_t global;
	uint32_t regional;
	uint32_t competitive;
	uint32_t playtime;
	uint32_t monsters;
	uint32_t bosses;
	uint32_t players;
	uint32_t quests;
	uint32_t explored;
	uint32_t taken;
	uint32_t dungeons;
	uint32_t coliseum;
	uint32_t items;
	uint32_t fish;
	uint32_t distance;
	uint32_t reputation;
	uint32_t endless;
	uint32_t codex;
	time_t update;
	u64snowflake userid;
	Player *next;
};

/* this is for raids */
typedef struct {
	char *name;
	uint32_t damage;
	int found_in_file;
} Slayer;

extern char *progname;
extern Player *player_head;
extern pthread_mutex_t player_mutex;
extern const char *fields[24];

/* util.c */
void warn(const char *fmt, ...);
void die(int status, const char *fmt, ...);
char *nstrchr(const char *s, int c, size_t n);
size_t ufmt(char *dst, size_t dsiz, uintmax_t n);
size_t ifmt(char *dst, size_t dsiz, intmax_t n);
size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);
#ifdef DALLOC
#define xmalloc(siz)        malloc(siz)
#define xcalloc(nmemb, siz) calloc(nmemb, siz)
#define xrealloc(p, siz)    realloc(p, siz)
#define xstrdup(s)          strdup(s)
#else
void *xmalloc(size_t siz);
void *xcalloc(size_t nmemb, size_t siz);
void *xrealloc(void *p, size_t siz);
char *xstrdup(const char *s);
#endif /* DALLOC */
FILE *xfopen(const char *filename, const char *mode);
int file_exists(const char *filename);
Player *find_player(u64snowflake userid);
void discord_send_message(struct discord *client, u64snowflake channel_id,
                          const char *fmt, ...);
void discord_send_interaction_message(struct discord *client, u64snowflake id,
                                      const char *token, const char *fmt, ...);

/* init.c */
void create_folders(void);
void create_stats_file(void);
void init_players(void);
void create_slash_commands(struct discord *client);
void on_ready(struct discord *client, const struct discord_ready *ev);

/* run.c */
void on_interaction(struct discord *client,
                    const struct discord_interaction *ev);
void on_message(struct discord *client, const struct discord_message *ev);

/* ocr.c */
char *curl(char *url);
CURLcode curl_file(char *url, char *fname);
int crop(char *fname, int type);
char *ocr(const char *fname, const char *lang);

/* stats.c */
void create_slash_stats(struct discord *client);
uint32_t trim_stat(const char *str);
char *playtime_to_str(uint32_t playtime);
void update_file(Player *player);
void on_stats(struct discord *client, const struct discord_message *ev);
void on_stats_interaction(struct discord *client,
                          const struct discord_interaction *ev);

/* raids.c */
void on_raids(struct discord *client, const struct discord_message *ev);
size_t load_files(Slayer slayers[]);

/* roles.c */
void update_roles(struct discord *client, Player *player);

/* cmd_help.c */
void create_slash_help(struct discord *client);
void on_help(struct discord *client, const struct discord_message *ev);
void on_help_interaction(struct discord *client,
                         const struct discord_interaction *ev);

/* cmd_info.c */
u64snowflake str_to_uid(char *id);
void create_slash_info(struct discord *client);
void write_info(char *buf, size_t siz, const Player *player);
void on_info(struct discord *client, const struct discord_message *ev);
void on_info_interaction(struct discord *client,
                         const struct discord_interaction *ev);

/* cmd_leaderboard.c */
void create_slash_leaderboard(struct discord *client);
void on_leaderboard(struct discord *client,
                    const struct discord_message *ev);
void on_leaderboard_interaction(struct discord *client,
                                const struct discord_interaction *ev);

/* cmd_source.c */
void create_slash_source(struct discord *client);
void on_source(struct discord *client, const struct discord_message *ev);
void on_source_interaction(struct discord *client,
                           const struct discord_interaction *ev);

/* cmd_lbraid.c */
void create_slash_lbraid(struct discord *client);
void on_lbraid(struct discord *client, const struct discord_message *ev);
void on_lbraid_interaction(struct discord *client,
                           const struct discord_interaction *ev);

/* cmd_uraid.c */
void create_slash_uraid(struct discord *client);
void on_uraid(struct discord *client, const struct discord_message *ev);
void on_uraid_interaction(struct discord *client,
                          const struct discord_interaction *ev);

/* cmd_correct.c */
void create_slash_correct(struct discord *client);
void on_correct(struct discord *client, const struct discord_message *ev);
void on_correct_interaction(struct discord *client,
                            const struct discord_interaction *ev);

/* cmd_time.c */
void create_slash_time(struct discord *client);
void on_time(struct discord *client, const struct discord_message *ev);
void on_time_interaction(struct discord *client,
                         const struct discord_interaction *ev);

#endif /* NOLAN_H */

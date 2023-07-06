#define DELIM         ','                 /* Delimiter for the save file */
#define FILENAME      "source.csv"        /* Filename of the save file */
#define LB_MAX        10                  /* Max players to be shown with ?lb */
#define APP_ID        0UL                 /* The bot's application ID */
#define ADMIN         0UL                 /* Admin user ID for bug reports */
#define RAID_GUILD_ID 0UL                 /* Server ID for raids */
#define ROLE_GUILD_ID 0UL                 /* This is only for to Orna FR */
#define MAX_PLAYERS   50                  /* Max players for stats */
#define TOKEN         "YOUR-BOT-TOKEN"
#define PREFIX        "?"

/* list of channel ids to check for stats screenshots */
static const u64snowflake stats_ids[] = {
	0,
};

/* list of channel ids to check for raids screenshots */
static const u64snowflake raids_ids[] = {
	0,
};

/* list of chinese slayers for raids screenshots */
static const u64snowflake cn_slayer_ids[] = {
	0,
};

/* list of japanese slayers for raids screenshots */
static const u64snowflake jp_slayer_ids[] = {
	0,
};

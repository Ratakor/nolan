#define DELIM         ','                 /* Delimiter for the save file */
#define FILENAME      "source.csv"        /* Filename of the save file */
#define LB_MAX        10                  /* Max players to be shown with ?lb */
#define APP_ID        1109604402102273245 /* The bot's application ID */
#define ADMIN         277534384175841280  /* Admin user ID for bug reports */
#define RAID_GUILD_ID 0                   /* Server ID for raids */
#define TOKEN         "YOUR-BOT-TOKEN"
#define PREFIX        "?"

static const int kingdom_verification = 0; /* 0 means no verification */
static const int use_embed            = 0; /* 0 means no embed on info */

/* list of channel ids to check for stats screenshots */
static const u64snowflake stats_ids[] = {
	1110185440285302855,
	1110767040890941590,
};

/* list of channel ids to check for raids screenshots */
static const u64snowflake raids_ids[] = {
	1112094855691239616,
};

/* list of chinese slayers for raids screenshots */
static const u64snowflake cn_slayer_ids[] = {
	0,
};

/* list of japanese slayers for raids screenshots */
static const u64snowflake jp_slayer_ids[] = {
	0,
};

/* list of kingdom to accept if verification is on */
static const char *kingdoms[] = {
	"Scream of Terra",
	"Volcano of Terra",
	"Whisper of Terra",
};

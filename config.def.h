#define RAIDS_ID 1109604402102276217 /* Channel ID for raids */
#define DELIM    ','                 /* Delimiter for the save file */
#define FILENAME "players.csv"       /* Filename of the save file */
#define LB_MAX   10                  /* Max players to be shown with !lb */
#define TOKEN    "YOUR-BOT-TOKEN"
#define PREFIX   "?"

static const int kingdom_verification = 0; /* 0 means no verification */

 /* list of ids to check for stats image */
static const u64snowflake stats_ids[] = {
	1110185440285302855,
	1110767040890941590
};

/* list of kingdom to accept if verification is on */
static const char *kingdoms[] = {
	"Scream of Terra",
	"Volcano of Terra",
	"Whisper of Terra",
};

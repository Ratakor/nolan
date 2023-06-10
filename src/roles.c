#include "nolan.h"

static void level(struct discord *client, u64snowflake userid, Player *player);
static void playtime(struct discord *client, u64snowflake userid, Player *player);
static void monsters(struct discord *client, u64snowflake userid, Player *player);
static void bosses(struct discord *client, u64snowflake userid, Player *player);
static void _players(struct discord *client, u64snowflake userid, Player *player);
static void quests(struct discord *client, u64snowflake userid, Player *player);
static void dungeons(struct discord *client, u64snowflake userid, Player *player);
static void coliseum(struct discord *client, u64snowflake userid, Player *player);
static void items(struct discord *client, u64snowflake userid, Player *player);
static void fish(struct discord *client, u64snowflake userid, Player *player);
static void endless(struct discord *client, u64snowflake userid, Player *player);
static void codex(struct discord *client, u64snowflake userid, Player *player);

/* TODO: remove deprecated roles, e.g t10 role when arisen */

void
level(struct discord *client, u64snowflake userid, Player *player)
{
	if (player->level == 250) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              999698224665341982, NULL, NULL);
	} else if (player->level >= 225) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              999698288104190002, NULL, NULL);
	} else if (player->level >= 200) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              999698328856051792, NULL, NULL);
	} else if (player->level >= 175) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              999698552768970875, NULL, NULL);
	} else if (player->level >= 150) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              999700504051453973, NULL, NULL);
	} else if (player->level >= 125) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              999700550209785866, NULL, NULL);
	} else if (player->level >= 100) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              999700639313567744, NULL, NULL);
	} else if (player->level >= 75) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              999700691310366760, NULL, NULL);
	} else if (player->level >= 50) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              999700747350462505, NULL, NULL);
	} else if (player->level >= 25) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              999700794230177842, NULL, NULL);
	} else if (player->level >= 1) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              999700841281896508, NULL, NULL);
	}
}

void
playtime(struct discord *client, u64snowflake userid, Player *player)
{
	long playtime = player->playtime / 24;

	if (playtime >= 150) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1114895554254753873, NULL, NULL);
	} else if (playtime >= 100) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1114909422947410013, NULL, NULL);
	} else if (playtime >= 50) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1114909817463656460, NULL, NULL);
	}
}

void
monsters(struct discord *client, u64snowflake userid, Player *player)
{
	if (player->monsters >= 500000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1114908471985438832, NULL, NULL);
	} else if (player->monsters >= 250000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1000114023045537923, NULL, NULL);
	} else if (player->monsters >= 100000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1114908461004759050, NULL, NULL);
	}
}

void
bosses(struct discord *client, u64snowflake userid, Player *player)
{
	if (player->bosses >= 500000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1000113783043268719, NULL, NULL);
	} else if (player->bosses >= 250000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1000113738046787654, NULL, NULL);
	} else if (player->bosses >= 100000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1114907696878071832, NULL, NULL);
	}
}

void
_players(struct discord *client, u64snowflake userid, Player *player)
{
	if (player->players >= 50000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              999720879502151901, NULL, NULL);
	} else if (player->players >= 10000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              999720827656347668, NULL, NULL);
	}
}

void
quests(struct discord *client, u64snowflake userid, Player *player)
{
	if (player->quests >= 5000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1029862081383628882, NULL, NULL);
	} else if (player->quests >= 3000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1029862056335245442, NULL, NULL);
	} else if (player->quests >= 1000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1029861894904877087, NULL, NULL);
	}
}

void
dungeons(struct discord *client, u64snowflake userid, Player *player)
{
	if (player->dungeons >= 25000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1114908526050017330, NULL, NULL);
	} else if (player->dungeons >= 10000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              999720735184527500, NULL, NULL);
	} else if (player->dungeons >= 2000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              999720622806532206, NULL, NULL);
	}
}

void
coliseum(struct discord *client, u64snowflake userid, Player *player)
{
	if (player->coliseum >= 100) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1000112930043469836, NULL, NULL);
	}
}

void
items(struct discord *client, u64snowflake userid, Player *player)
{
	if (player->items >= 3000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1029862435391295580, NULL, NULL);
	} else if (player->items >= 2000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1029862373080698974, NULL, NULL);
	} else if (player->items >= 1000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1029862100346089522, NULL, NULL);
	}
}

void
fish(struct discord *client, u64snowflake userid, Player *player)
{
	if (player->fish >= 5000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1114896335456436335, NULL, NULL);
	} else if (player->fish >= 2500) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1114897128058269696, NULL, NULL);
	} else if (player->fish >= 1000) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1114896253155819580, NULL, NULL);
	}
}

void
endless(struct discord *client, u64snowflake userid, Player *player)
{
	if (player->endless >= 500) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1000116691390451736, NULL, NULL);
	} else if (player->endless >= 400) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1000116521823121509, NULL, NULL);
	} else if (player->endless >= 300) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1000116301269844128, NULL, NULL);
	} else if (player->endless >= 200) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1000115997652570114, NULL, NULL);
	}
}

void
codex(struct discord *client, u64snowflake userid, Player *player)
{
	if (player->codex >= 400) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1000114579604508753, NULL, NULL);
	} else if (player->codex >= 300) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1000114528375279697, NULL, NULL);
	} else if (player->codex >= 200) {
		discord_add_guild_member_role(client, ROLE_GUILD_ID, userid,
		                              1000114428747972659, NULL, NULL);
	}
}

/* this is really specific to Orna FR */
void
update_roles(struct discord *client, Player *player)
{
	u64snowflake userid = player->userid;

	level(client, userid, player);
	/* ascension */
	/* global */
	/* regional */
	/* competitive */
	playtime(client, userid, player);
	monsters(client, userid, player);
	bosses(client, userid, player);
	_players(client, userid, player);
	quests(client, userid, player);
	/* explored */
	/* taken */
	dungeons(client, userid, player);
	coliseum(client, userid, player);
	items(client, userid, player);
	fish(client, userid, player);
	/* distance */
	/* reputation */
	endless(client, userid, player);
	codex(client, userid, player);
}

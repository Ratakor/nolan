#include <stdio.h>
#include <string.h>
#include <time.h>

#include "nolan.h"

static void timepercent(char *buf, size_t siz);

void
create_slash_time(struct discord *client)
{
	struct discord_create_global_application_command cmd = {
		.name = "time",
		.description = "Shows time percentage",
	};
	discord_create_global_application_command(client, APP_ID, &cmd, NULL);
}

void
timepercent(char *buf, size_t siz)
{
	struct tm *tm;
	time_t t;
	int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int year, leap, d, w, m, y;
	double dpercent, wpercent, mpercent, ypercent;
	char dbar[11], wbar[11], mbar[11], ybar[11];

	memset(dbar, '-', STRLEN(dbar));
	memset(wbar, '-', STRLEN(wbar));
	memset(mbar, '-', STRLEN(mbar));
	memset(ybar, '-', STRLEN(ybar));
	t = time(NULL);
	tm = gmtime(&t);
	year = tm->tm_year;

	if (year < 1900)
		year += 1900;

	if (year % 400 == 0)
		leap = 1;
	else if (year % 100 == 0)
		leap = 0;
	else
		leap = (year % 400 == 0);

	if (leap)
		days[1] = 29;

	d = (tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec);

	if (tm->tm_wday == 0)
		w = d;
	else
		w = (tm->tm_wday - 1) * 86400 + d;

	m = (tm->tm_mday - 1) * 86400 + d;

	if (tm->tm_yday == 0)
		y = d;
	else
		y = (tm->tm_yday - 1) * 86400 + d;

	dpercent = ((double)d * 100) / 86400;
	wpercent = ((double)w * 100) / (86400 * 7);
	mpercent = ((double)m * 100) / (86400 * days[tm->tm_mon]);
	ypercent = ((double)y * 100) / (86400 * (365 + leap));

	memset(dbar, '#', (int)dpercent / 10);
	memset(wbar, '#', (int)wpercent / 10);
	memset(mbar, '#', (int)mpercent / 10);
	memset(ybar, '#', (int)ypercent / 10);
	dbar[STRLEN(dbar)] = '\0';
	wbar[STRLEN(wbar)] = '\0';
	mbar[STRLEN(mbar)] = '\0';
	ybar[STRLEN(ybar)] = '\0';

	snprintf(buf, siz,
	         "Time progress:\n"
	         "```\n"
	         "time:  %02d:%02d:%02d\n"
	         "day:   %s %05.2f%%\n"
	         "week:  %s %05.2f%%\n"
	         "month: %s %05.2f%%\n"
	         "year:  %s %05.2f%%\n"
	         "```",
	         tm->tm_hour, tm->tm_min, tm->tm_sec,
	         dbar, dpercent,
	         wbar, wpercent,
	         mbar, mpercent,
	         ybar, ypercent);
}

void
on_time(struct discord *client, const struct discord_message *event)
{
	char buf[MAX_MESSAGE_LEN];

#ifdef DEVEL
	if (event->channel_id != DEVEL)
		return;
#endif /* DEVEL */

	log_info("%s", __func__);
	timepercent(buf, sizeof(buf));
	struct discord_create_message msg = {
		.content = buf
	};
	discord_create_message(client, event->channel_id, &msg, NULL);
}

void
on_time_interaction(struct discord *client,
                    const struct discord_interaction *event)
{
	char buf[MAX_MESSAGE_LEN];

	log_info("%s", __func__);
	timepercent(buf, sizeof(buf));
	struct discord_interaction_response params = {
		.type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
		.data = &(struct discord_interaction_callback_data)
		{
			.content = buf,
		}
	};
	discord_create_interaction_response(client, event->id, event->token,
	                                    &params, NULL);
}

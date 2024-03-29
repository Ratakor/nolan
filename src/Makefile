# Copywrong © 2023 Ratakor. See LICENSE file for license details.

BIN          = ../nolan
CONFIG_H     = ../config.h
CC           = zig cc

OBJS         = cmd_correct.o     \
               cmd_help.o        \
               cmd_info.o        \
               cmd_lbraid.o      \
               cmd_leaderboard.o \
               cmd_source.o      \
               cmd_time.o        \
               cmd_uraid.o       \
               dalloc.o          \
               init.o            \
               main.o            \
               ocr.o             \
               raids.o           \
               roles.o           \
               run.o             \
               stats.o           \
               util.o

ZIG_OBJS     = quote.o

DISCORDLIBS  = -ldiscord -lcurl -lpthread
TESSLIBS     = -ltesseract -lleptonica
GDLIBS       = -lgd -lpng -lz -ljpeg -lfreetype -lm

WFLAGS       = -pedantic -Werror -Wall -Wextra -Waggregate-return -Wshadow\
               -Wmissing-prototypes -Wunused-macros -Wdouble-promotion -Wpadded
CFLAGS      += -std=c99 -D_DEFAULT_SOURCE ${WFLAGS}
LDFLAGS     += ${DISCORDLIBS} ${TESSLIBS} ${GDLIBS}

all: ${BIN}

echo:
	@echo "CC       = ${CC}"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "OBJS     = ${OBJS}"

${BIN}: ${CONFIG_H} ${OBJS} ${ZIG_OBJS}
	${CC} ${OBJS} ${ZIG_OBJS} ${LDFLAGS} -o $@

${ZIG_OBJS}:
	zig build-obj -OReleaseFast -lc -lcurl ${@:.o=.zig}

${CONFIG_H}:
	@cp ../config.def.h $@

clean:
	rm -f ${OBJS} ${ZIG_OBJS} ${BIN}

.PHONY: all echo clean

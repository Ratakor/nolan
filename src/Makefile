# Copywrong © 2023 Ratakor. See LICENSE file for license details.

BIN          = ../nolan
CONFIG_H     = ../config.h
LIBRE_DIR    = ../libre

LIBRE_OBJS   = ${LIBRE_DIR}/dalloc.o \
               ${LIBRE_DIR}/ubik.o

OBJS         = cmd_correct.o     \
               cmd_help.o        \
               cmd_info.o        \
               cmd_lbraid.o      \
               cmd_leaderboard.o \
               cmd_source.o      \
               cmd_uraid.o       \
               init.o            \
               main.o            \
               ocr.o             \
               raids.o           \
               roles.o           \
               run.o             \
               stats.o

DISCORDLIBS  = -ldiscord -lcurl -lpthread
TESSLIBS     = -ltesseract -lleptonica
GDLIBS       = -lgd -lpng -lz -ljpeg -lfreetype -lm

WFLAGS       = -Werror -Wall -Wextra -Wpedantic -Waggregate-return -Wshadow\
               -Wmissing-prototypes -Wunused-macros -Wdouble-promotion
DFLAGS       = -O0 -g -DDALLOC
CFLAGS      += -std=c99 -D_DEFAULT_SOURCE ${WFLAGS}
LDFLAGS     += ${DISCORDLIBS} ${TESSLIBS} ${GDLIBS}

all: ${BIN}

echo:
	@echo "CC       = ${CC}"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "OBJS     = ${OBJS}"

${BIN}: ${CONFIG_H} ${OBJS}
	${CC} ${OBJS} ${LIBRE_OBJS} ${LDFLAGS} -o $@

${CONFIG_H}:
	@cp ../config.def.h $@

debug:
	@CFLAGS="${DFLAGS}" ${MAKE}

clean:
	rm -f ${OBJS} ${BIN}

.PHONY: all echo debug clean

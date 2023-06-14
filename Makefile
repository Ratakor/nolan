NAME         = nolan
PREFIX      ?= /usr/local
CC           = cc

BUILD_DIR   ?= build
SRC_DIR     ?= src

SRCS        := ${wildcard ${SRC_DIR}/*.c}
OBJS        := ${SRCS:%=${BUILD_DIR}/%.o}

DISCORDLIBS  = -ldiscord -lcurl -lpthread
TESSLIBS     = -ltesseract -lleptonica
GDLIBS       = -lgd -lpng -lz -ljpeg -lfreetype -lm

WARN_FLAGS   = -Werror -Wall -Wextra -Wno-format -Wmissing-prototypes\
               -Waggregate-return -Wunused-macros -Wshadow -Wcast-align
CFLAGS      += -std=c99 -pedantic -D_DEFAULT_SOURCE ${WARN_FLAGS}
LDFLAGS     += ${DISCORDLIBS} ${TESSLIBS} ${GDLIBS}

all: options ${NAME}

options:
	@echo ${NAME} build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

${BUILD_DIR}/%.c.o: %.c
	mkdir -p ${dir $@}
	${CC} -c ${CFLAGS} $< -o $@

${OBJS}: config.h

config.h:
	cp config.def.h $@

${NAME}: ${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS}

clean:
	rm -rf ${NAME} ${BUILD_DIR}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${NAME} ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/${NAME}

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/${NAME}

.PHONY: all options clean install uninstall

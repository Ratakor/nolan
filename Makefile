NAME         = nolan
PREFIX      ?= /usr/local
CC          ?= cc

BUILD_DIR   ?= build
SRC_DIR     ?= src

SRCS        := ${wildcard ${SRC_DIR}/*.c}
OBJS        := ${SRCS:%=${BUILD_DIR}/%.o}

DEBUG_FLAGS  = -O0 -g -W -Wall -Wmissing-prototypes
CFLAGS      += -std=c99 -pedantic -D_DEFAULT_SOURCE
LDFLAGS     += -ltesseract -lleptonica -ldiscord -lcurl -lpthread -lgd

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

debug:
	@CFLAGS="${DEBUG_FLAGS}" ${MAKE}

clean:
	rm -rf ${NAME} ${BUILD_DIR}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${NAME} ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/${NAME}

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/${NAME}

.PHONY: all options debug clean install uninstall

NAME     = nolan
PREFIX  ?= /usr/local
CC      ?= cc

DEBUG    = -g -W -Wmissing-prototypes
CFLAGS  += -std=c99 -pedantic -Wall -O2 -D_DEFAULT_SOURCE
LDFLAGS += -ltesseract -lleptonica -ldiscord -lcurl -lpthread

SRC = ${wildcard *.c}
OBJ = ${SRC:.c=.o}

all: options ${NAME}

options:
	@echo nolan build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h

config.h:
	cp config.def.h $@

${NAME}: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

debug: CFLAGS += ${DEBUG}
debug: all

clean:
	rm -f ${NAME} ${OBJ}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${NAME} ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/${NAME}

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/${NAME}

.PHONY: all options debug clean install uninstall

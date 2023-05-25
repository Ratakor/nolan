PREFIX  ?= /usr/local
CC      ?= cc
DEBUG    = -g -W -Wmissing-prototypes
CFLAGS  += -std=c99 -pedantic -Wall -O2 -D_DEFAULT_SOURCE
LDFLAGS += -ltesseract -lleptonica -ldiscord -lcurl -lpthread

SRC = nolan.c util.c
OBJ = ${SRC:.c=.o}

all: options nolan

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

nolan: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f nolan ${OBJ}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f nolan ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/nolan

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/nolan

.PHONY: all options clean install uninstall

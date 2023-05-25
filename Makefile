PREFIX   ?= /usr/local
CC       ?= cc
DEBUG     = -g -W -Wmissing-prototypes
CFLAGS   += -std=c99 -pedantic -Wall -O2 -D_DEFAULT_SOURCE ${DEBUG}
LDLFLAGS += -ltesseract -lleptonica -ldiscord -lcurl -lpthread

SRC = nolan.c util.c
OBJ = ${SRC:.c=.o}

all: nolan

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h

config.h:
	cp config.def.h $@

nolan: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDLFLAGS}

clean:
	rm -f *.o

distclean: clean
	rm -f nolan

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f nolan ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/nolan

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/nolan

.PHONY: all clean distclean install uninstall

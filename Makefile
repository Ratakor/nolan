PREFIX   ?= /usr/local
CC       ?= cc
DEBUG     = -g -W -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
CFLAGS   += -std=c99 -pedantic -Wall -O2 ${DEBUG}
LDLFLAGS += -ltesseract -lleptonica -ldiscord -lcurl -lpthread

SRC = nolan.c
OBJ = ${SRC:.c=.o}

all: nolan

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.json

config.json:
	cp config.def.json $@

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

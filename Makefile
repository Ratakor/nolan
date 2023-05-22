PREFIX = /usr/local
CC = cc
#CFLAGS = -g -Wall -W -ansi -Werror
DEBUG_CFLAGS = -g -Wall -W -ansi -pedantic -Werror -Wfloat-equal\
	-Wpointer-arith -Wbad-function-cast -Wcast-qual -Wcast-align\
       	-Waggregate-return -Wstrict-prototypes -Wmissing-prototypes\
       	-Wmissing-declarations -Wnested-externs -Wunreachable-code\
	-Wundef -Wshadow
LDLFLAGS = -ltesseract -lleptonica -ldiscord -lcurl


SRC = nolan.c
OBJ = ${SRC:.c=.o}

all: nolan

.c.o:
	${CC} -c ${CFLAGS} $<

nolan: ${OBJ}
	${CC} -o  $@  ${OBJ} ${LDLFLAGS}

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

.PHONY: all debug clean distclean install uninstall

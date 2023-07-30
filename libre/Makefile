# Copyright © 2023 Ratakor. See LICENSE file for license details.

PREFIX   ?= /usr/local
LIBDIR    = ${DESTDIR}${PREFIX}/lib
INCDIR    = ${DESTDIR}${PREFIX}/include/libre
MANDIR    = ${DESTDIR}${PREFIX}/share/man/man3

LIB       = libre.a
LIBS      = dalloc ubik

OBJS      = ${LIBS:%=src/%.o}
HEADERS   = ${LIBS:%=inc/%.h} inc/evil.h
MANPAGES  = ${LIBS:%=man/%.3} man/evil.3

CFLAGS   += -std=c99 -pedantic -pthread -I./inc
ARFLAGS   = -crs

all: ${LIB}

.c.o:
	${CC} ${CFLAGS} ${CPPFLAGS} -c $< -o $@

${LIB}: ${OBJS}
	${AR} ${ARFLAGS} $@ ${OBJS}

clean:
	rm -f ${LIB} ${OBJS}

install: all
	@mkdir -p ${LIBDIR}
	@mkdir -p ${INCDIR}
	@mkdir -p ${MANDIR}
	cp -f ${LIB} ${LIBDIR}
	cp -f ${HEADERS} ${INCDIR}
	cp -f ${MANPAGES} ${MANDIR}
	@chmod 644 ${LIBDIR}/${LIB}
	@chmod 644 ${HEADERS:inc%=${INCDIR}%}
	@chmod 644 ${MANPAGES:man%=${MANDIR}%}

uninstall:
	rm -f ${LIBDIR}/${LIB}
	rm -rf ${INCDIR}
	rm -f ${MANPAGES:man%=${MANDIR}%}

.PHONY: all clean install uninstall

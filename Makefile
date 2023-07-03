# Copywrong © 2023 Ratakor. See LICENSE file for license details.

NAME       = nolan
PREFIX    ?= /usr/local
CC         = cc

LIBRE_DIR  = libre
SRC_DIR    = src

CFLAGS    ?= -O3
LDFLAGS   ?= -s

all: build

echo:
	@echo ${NAME} build options:
	@echo "CC       = ${CC}"
	@echo "PREFIX   = ${PREFIX}"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"

build:
	@CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" ${MAKE} -C ${LIBRE_DIR}
	@CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" ${MAKE} -C ${SRC_DIR}

debug:
	@${MAKE} -C ${LIBRE_DIR} $@
	@${MAKE} -C ${SRC_DIR} $@

clean:
	@${MAKE} -C ${LIBRE_DIR} $@
	@${MAKE} -C $(SRC_DIR) $@

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${NAME} ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/${NAME}

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/${NAME}

.PHONY: all echo build debug clean install uninstall

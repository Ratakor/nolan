OBJS    = dalloc.o \
          ubik.o

DFLAGS  = -O0 -g -DDALLOC
CFLAGS += -std=c99 -pedantic -pthread -D_POSIX_C_SOURCE=200809L

all: ${OBJS}

echo:
	@echo "CC      = ${CC}"
	@echo "CFLAGS  = ${CFLAGS}"
	@echo "OBJS    = ${OBJS}"

${OBJS}:
	${CC} ${CFLAGS} -c ${@:.o=}/${@:.o=.c} -o $@

debug:
	@CFLAGS="${DFLAGS}" ${MAKE}

clean:
	rm -f ${OBJS}

.PHONY: all echo debug clean

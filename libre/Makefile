OBJS    = cthread.o \
          dalloc.o  \
          util.o

DFLAGS  = -O0 -g -DDALLOC
CFLAGS += -std=c99 -pthread -D_DEFAULT_SOURCE

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

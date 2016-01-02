VERSION := 1.4

# maximum number of subpatterns recognised
MAX_SUBS := 16

STRIP := ${CROSS_COMPILE}strip

CC := ${CROSS_COMPILE}gcc
CFLAGS := -Wall -Werror -pedantic -std=c99 -Os \
          -DMAX_SUBS=${MAX_SUBS} -DVERSION="\"${VERSION}\""
INSTALL := install

ifneq (${DEBUG},)
CFLAGS += -DDEBUG -g
endif

MF2B_SRC := action.c config.c helper.c logging.c matchlist.c substr.c
MF2B_OBJ := $(patsubst %.c,%.o,${MF2B_SRC})

mf2b: ${MF2B_OBJ}

strip: mf2b
	${STRIP} mf2b
	
clean:
	-rm -f ${MF2B_OBJ} mf2b.o mf2b

install: mf2b
	${INSTALL} -D -s -m 0755 mf2b ${DESTDIR}/usr/sbin/mf2b
	${INSTALL} -D -m 0644 mf2b.conf ${DESTDIR}/etc/mf2b.conf
	${INSTALL} -D -m 0644 mf2b.8 \
		${DESTDIR}/usr/share/man/man8/mf2b.8
	${INSTALL} -D -m 0644 mf2b.conf.5 \
		${DESTDIR}/usr/share/man/man5/mf2b.conf.5

tarball:
	git archive --format=tar --prefix=mf2b-${VERSION}/ ${VERSION} | \
		gzip -9 -c > mf2b-${VERSION}.tar.gz

cppcheck:
	cppcheck --enable=all --inconclusive --std=c99 .

test: tests/unit mf2b tests/run

tests/unit tests/run:
	${MAKE} -C $@

.PHONY: clean install tarball cppcheck test tests/unit tests/run

# See LICENSE file for copyright and license details.

include config.mk

SRC = mt.c util.c logo.c
OBJ = ${SRC:.c=.o}

all: mt

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	cp config.def.h $@

mt: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f mt ${OBJ}

dist: clean
	rm -rf "mt-${VERSION}"
	mkdir -p "mt-${VERSION}"
	cp -R Makefile LICENSE config.def.h config.mk mt.c util.c util.h \
	      logo.c logo.h mt.svg "mt-${VERSION}"
	tar -cf - "mt-${VERSION}" | bzip2 > "mt-${VERSION}.tar.bz2"
	rm -rf "mt-${VERSION}"

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f mt ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/mt

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/mt

.PHONY: all clean dist install uninstall
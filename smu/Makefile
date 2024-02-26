.POSIX:

NAME = smu
VERSION = 1.5

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# flags
SMU_CFLAGS = -DVERSION=\"${VERSION}\" ${CFLAGS}
SMU_LDFLAGS = ${LDFLAGS}

SRC = ${NAME}.c
OBJ = ${SRC:.c=.o}
BIN = ${NAME}
MAN1 = ${NAME}.1

all: ${BIN}

.c.o:
	${CC} -c ${SMU_CFLAGS} $<

${OBJ}:

${BIN}: ${OBJ}
	${CC} -o $@ ${OBJ} ${SMU_LDFLAGS}

clean:
	rm -f ${BIN} ${OBJ} "${NAME}-${VERSION}.tar.gz"

dist: clean
	mkdir -p "${NAME}-${VERSION}"
	cp -R LICENSE README testdoc Makefile ${MAN1} ${SRC} "${NAME}-${VERSION}"
	tar -cf - "${NAME}-${VERSION}" | \
		gzip -c > "${NAME}-${VERSION}.tar.gz"
	rm -rf "${NAME}-${VERSION}"

install: all
	mkdir -p "${DESTDIR}${PREFIX}/bin"
	cp -f ${BIN} "${DESTDIR}${PREFIX}/bin"
	chmod 755 "${DESTDIR}${PREFIX}/bin/${BIN}"
	mkdir -p "${DESTDIR}${MANPREFIX}/man1"
	cp -f ${MAN1} "${DESTDIR}${MANPREFIX}/man1"
	chmod 644 "${DESTDIR}${MANPREFIX}/man1/${MAN1}"

uninstall:
	rm -f "${DESTDIR}${PREFIX}/bin/${BIN}" \
		"${DESTDIR}${MANPREFIX}/man1/${MAN1}"

.PHONY: all clean dist install uninstall

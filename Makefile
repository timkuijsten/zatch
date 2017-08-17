OS=$(shell uname)

PROG=zatch

ifndef USRDIR
	USRDIR=/usr/local
endif
BINDIR=$(USRDIR)/bin
MANDIR=$(USRDIR)/share/man

OBJ=zatch.o
CFLAGS=-Wall

LDFLAGS=-framework CoreServices

INSTALL_DIR=install -dm 755
INSTALL_BIN=install -m 555
INSTALL_MAN=install -m 444

${PROG}: ${OBJ}
	$(CC) ${CFLAGS} -o $@ ${OBJ} ${LDFLAGS}

%.o: %.c
	$(CC) ${CFLAGS} -c $<

install:
	${INSTALL_DIR} ${DESTDIR}${BINDIR}
	${INSTALL_DIR} ${DESTDIR}${MANDIR}/man1
	${INSTALL_BIN} ${PROG} ${DESTDIR}${BINDIR}
	${INSTALL_MAN} ${PROG}.1 ${DESTDIR}${MANDIR}/man1

depend:
	$(CC) ${CFLAGS} -E -MM *.c > .depend

.PHONY: clean
clean:
	rm -f ${OBJ} zatch

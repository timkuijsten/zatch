CFLAGS = -Wall -Wextra -pedantic -Wno-unused-parameter
LDFLAGS=-framework CoreServices

INSTALL_BIN 	= install -m 0555
INSTALL_MAN	= install -m 0444

PREFIX	= /usr/local
BINDIR	= $(PREFIX)/bin
MANDIR	= $(PREFIX)/share/man

zatch: zatch.c
	$(CC) ${CFLAGS} -o $@ zatch.c ${LDFLAGS}

install: zatch
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	$(INSTALL_BIN) zatch $(DESTDIR)$(BINDIR)
	$(INSTALL_MAN) zatch.1 $(DESTDIR)$(MANDIR)/man1

manhtml:
	mandoc -T html -Ostyle=man.css zatch.1 > zatch.1.html

.PHONY: clean
clean:
	rm -f zatch

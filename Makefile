ALL=xdu
OBJS=xdu.o xwin.o

CFLAGS=-g -O2 -Wall -Wno-switch -Wextra -Wno-unused-parameter
LDLIBS=-lXt -lXaw -lX11

DESTDIR=
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/share/man

all: $(ALL)

xdu: $(OBJS)

clean: FRC
	rm -f $(ALL) $(OBJS)

install: FRC all
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(MANDIR)/man1
	install -m0755 $(ALL) $(DESTDIR)$(BINDIR)
	install -m0644 $(ALL:=.1) $(DESTDIR)$(MANDIR)/man1

FRC:

CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99 -O2
LDFLAGS =
EXECUTABLE = metinfo

.PHONY: all clean install uninstall

all: $(EXECUTABLE)

$(EXECUTABLE): metinfo.o
	$(CC) $(LDFLAGS) -o $@ $^

metinfo.o: metinfo.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(EXECUTABLE) *.o

install: all
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(EXECUTABLE) $(DESTDIR)/usr/local/bin

uninstall:
	rm -f $(DESTDIR)/usr/local/bin/$(EXECUTABLE)

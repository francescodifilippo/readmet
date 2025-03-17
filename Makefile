CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99 -O2
LDFLAGS =
EXECUTABLE = readmet

.PHONY: all clean install uninstall

all: $(EXECUTABLE)

$(EXECUTABLE): readmet.o
	$(CC) $(LDFLAGS) -o $@ $^

readmet.o: readmet.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(EXECUTABLE) *.o

install: all
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(EXECUTABLE) $(DESTDIR)/usr/local/bin

uninstall:
	rm -f $(DESTDIR)/usr/local/bin/$(EXECUTABLE)

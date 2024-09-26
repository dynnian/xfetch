all: xfetch

WARNINGS = -Wall
DEBUG = -ggdb -fno-omit-frame-pointer
OPTIMIZE = -O2

passgen: Makefile xfetch.c
	$(CC) -o $@ $(WARNINGS) $(DEBUG) $(OPTIMIZE) xfetch.c

clean:
	rm -f xfetch

install:
	echo "Installing is not supported"

run:
	./xfetch

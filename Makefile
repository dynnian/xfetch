all: xfetch

WARNINGS = -Wall
DEBUG = -ggdb -fno-omit-frame-pointer
OPTIMIZE = -O2
LIBS = -lX11 -lwayland-client

xfetch: Makefile xfetch.c
	$(CC) -o $@ $(WARNINGS) $(DEBUG) $(OPTIMIZE) xfetch.c $(LIBS)

clean:
	rm -f xfetch

install:
	echo "Installing is not supported"

run:
	./xfetch

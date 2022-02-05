CC=/bin/g++

CFILES=$(wildcard src/*.cpp)
LIBFILES=$(wildcard src/discord/*.cpp)
CFLAGS=-Llib/ -l:discord_game_sdk.so -lpthread

all: config build/rpcpp

config:
	if [[ -f "rpcpp" ]]; then rm rpcpp; fi
	cp example rpcpp
	sed s/PATH/$(shell pwd | sed 's/\//\\\\\//g')/g -i rpcpp

build/rpcpp: $(CFILES)
	mkdir -p build
	$(CC) $(CFILES) $(LIBFILES) $(CFLAGS) -o $@

clean:
	rm -rf tmp build example

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f rpcpp ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/rpcpp

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/rpcpp

.PHONY: all clean install uninstall

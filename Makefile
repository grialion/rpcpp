CC=/usr/bin/g++

CPPFILES=$(wildcard src/*.cpp)
HPPFILES=$(wildcard src/*.hpp)
LIBFILES=$(wildcard src/discord/*.cpp)
CFLAGS=-Llib/ -l:discord_game_sdk.so -lpthread -lX11

build/rpcpp: $(CPPFILES) $(HPPFILES)
	mkdir -p build
	$(CC) $(CPPFILES) $(LIBFILES) $(CFLAGS) -o $@

clean:
	rm -rf tmp build

install: build/rpcpp
	mkdir -p ${DESTDIR}${PREFIX}/bin
	mkdir -p ${DESTDIR}${PREFIX}/lib
	cp -f build/rpcpp ${DESTDIR}${PREFIX}/bin
	cp -f lib/discord_game_sdk.so ${DESTDIR}${PREFIX}/lib
	chmod 755 ${DESTDIR}${PREFIX}/bin/rpcpp

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/rpcpp

.PHONY: clean install uninstall
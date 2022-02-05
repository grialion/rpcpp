CC=/bin/g++

CFILES=$(wildcard src/*.cpp)
LIBFILES=$(wildcard src/discord/*.cpp)
CFLAGS=-Llib/ -l:discord_game_sdk.so -lpthread

build/rpcpp: $(CFILES)
	mkdir -p build
	$(CC) $(CFILES) $(LIBFILES) $(CFLAGS) -o $@


config:
	if [[ -f "rpcpp" ]]; then rm rpcpp; fi
	cp example rpcpp
	sed s/PATH/$(shell pwd | sed 's/\//\\\\\//g')/g -i rpcpp
	chmod +x start.sh rpcfetch.sh

clean:
	rm -rf tmp build rpcpp

install: config build/rpcpp
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f rpcpp ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/rpcpp

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/rpcpp

.PHONY: config clean install uninstall

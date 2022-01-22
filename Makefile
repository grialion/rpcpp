CC=/bin/g++

CFILES=$(wildcard src/*.cpp)
LIBFILES=$(wildcard src/discord/*.cpp)
CFLAGS=-Llib/ -l:discord_game_sdk.so -lpthread


build/rpcpp: $(CFILES)
	mkdir -p build
	$(CC) $(CFILES) $(LIBFILES) $(CFLAGS) -o $@
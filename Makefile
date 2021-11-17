INCLUDES:=-I./ -I./lib/cwalk/include -I./include
WARNINGS:=-Wall -Wextra -Wno-pointer-arith -Wno-unused-parameter
ifeq ($(RELEASE),)
OPTFLAGS:=-O1 -g
else
OPTFLAGS:=-O3 -ffast-math -flto
endif
CFLAGS+=-std=c17 $(WARNINGS) $(OPTFLAGS) $(INCLUDES)

ARES_OBJS:=$(patsubst %,./build/%.o,loop srv cwalk)

ROVER_OBJS:=$(patsubst %,./build/%.o,dynarr line draw gui conn piper gem dynstr loop main cwalk)
ROVER_LDFLAGS:=$(shell pkg-config --libs x11) -lutil

all: ./build/ ./build/rover ./build/ares

./build/:
	mkdir -p $@

./build/rover: $(ROVER_OBJS)
	$(CC) $^ -o $@ $(ROVER_LDFLAGS) $(OPTFLAGS)
ifneq ($(RELEASE),)
	$(STRIP) $@
endif

./build/ares: $(ARES_OBJS)
	$(CC) $^ -o $@ $(OPTFLAGS)
ifneq ($(RELEASE),)
	$(STRIP) $@
endif

./build/%.o: ./src/%.c
	$(CC) -c $^ -o $@ $(CFLAGS)

./build/cwalk.o: ./lib/cwalk/src/cwalk.c
	$(CC) -c $^ -o $@ $(CFLAGS)

clean:
	rm -f ./build/*

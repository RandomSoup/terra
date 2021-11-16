LDFLAGS:=$(shell pkg-config --libs gl glfw3 epoxy) -lutil
WARNINGS:=-Wall -Wextra -Wno-pointer-arith -Wno-unused-parameter
ifeq ($(RELEASE),)
OPTFLAGS:=-O1 -g
else
OPTFLAGS:=-O3 -ffast-math -flto
endif
INCLUDES:=-I./ -I./lib/cwalk/include -I./src
CFLAGS+=-std=c17 $(shell pkg-config --cflags gl glfw3 epoxy) $(WARNINGS) $(OPTFLAGS) $(INCLUDES)
OBJS:=$(patsubst %,./build/%.o,dynarr line draw main piper gem dynstr loop worker) ./build/cwalk.o
STRIP?=strip

all: ./build/ ./build/rover

./build/:
	mkdir -p $@

./build/rover: $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS) $(OPTFLAGS)
ifneq ($(RELEASE),)
	$(STRIP) $@
endif

./build/%.o: ./src/%.c
	$(CC) -c $^ -o $@ $(CFLAGS)

./build/cwalk.o: ./lib/cwalk/src/cwalk.c
	$(CC) -c $^ -o $@ $(CFLAGS)

clean:
	rm -f $(OBJS) ./build/a.out

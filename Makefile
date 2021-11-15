LDFLAGS:=$(shell pkg-config --libs gl glfw3 epoxy) -lutil
WARNINGS:=-Wall -Wextra -Wno-pointer-arith -Wno-unused-parameter
ifeq ($(RELEASE),)
OPTFLAGS:=-O1 -g
else
OPTFLAGS:=-O3 -ffast-math -flto
endif
CFLAGS+=-I./ -I./src -std=c17 $(shell pkg-config --cflags gl glfw3 epoxy) $(WARNINGS) $(OPTFLAGS)
OBJS:=$(patsubst %,./build/%.o,dynarr line draw main piper gem dynstr loop worker)

all: ./build/ ./build/a.out

./build/:
	mkdir -p $@

./build/a.out: $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

./build/%.o: ./src/%.c
	$(CC) -c $^ -o $@ $(CFLAGS)

clean:
	rm -f $(OBJS) ./build/a.out

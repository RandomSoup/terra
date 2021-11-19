OPTFLAGS:=-O3 -ffast-math -flto
WARNINGS:=-Wall -Wextra -Wno-pointer-arith -Wno-unused-parameter
CFLAGS:=-std=c17 $(OPTFLAGS) $(WARNINGS)
LDFLAGS:=$(OPTFLAGS)

ifeq ($(RELEASE),)
	OPTFLAGS+=-g
endif

all:

define decl
SRC_$1=$(patsubst %,./src/$1/%.c,$2)
OBJ_$1=$(patsubst %,./build/$1/%.o,$2)

all: ./build/$1

./build/$1:
	@mkdir -p $$@

./build/$1/%.o: ./src/$1/%.c
	$$(CC) -o $$@ -c $$< $$(CFLAGS) $$(CFLAGS_$1)
endef

define exec
all: ./build/$1/$1

./build/$1/$1: $(OBJ_$1) $2
	$$(CC) -o $$@ $$^ $$(LDFLAGS) $$(LDFLAGS_$1)
endef

define arlib
all: ./build/$1/lib$1.ar

./build/$1/lib$1.ar: $(OBJ_$1) $2
	$(AR) rcs $$@ $$^ $$(LDFLAGS_$1)
endef

clean:
	rm -rf ./build/*

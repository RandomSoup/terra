OPTFLAGS:=-O3 -ffast-math
ifeq ($(RELEASE),)
	OPTFLAGS+=-g
else
	OPTFLAGS+=-flto
endif

WARNINGS:=-Wall -Wextra -Wno-pointer-arith -Wno-unused-parameter
CFLAGS:=-std=c17 $(OPTFLAGS) $(WARNINGS)
LDFLAGS:=$(OPTFLAGS)
PREFIX?=/usr/local
INSTALL?=install
IFLAGS:=-D

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
install-exec: ./build/$1/$1

./build/$1/$1: $(OBJ_$1) $2
	$$(CC) -o $$@ $$^ $$(LDFLAGS) $$(LDFLAGS_$1)
endef

define lib
all: ./build/$1/$1.ar ./build/$1/$1.so
install-lib: ./build/$1/$1.ar ./build/$1/$1.so

./build/$1/$1.ar: $(OBJ_$1) $2
	$(AR) rcs $$@ $$^ $$(LDFLAGS_$1)

./build/$1/$1.so: $(OBJ_$1) $2
	$(CC) -fPIC -shared -o $$@ $$^ $$(LDFLAGS_$1)
endef

define dev
install-dev: $(patsubst %,./include/%.h,$1)
endef

clean:
	rm -rf ./build/*

install-exec:
	$(INSTALL) $(IFLAGS) -s $^ -t $(PREFIX)/bin

install-lib:
	$(INSTALL) $(IFLAGS) $^ -t $(PREFIX)/lib

install-dev:
	$(INSTALL) $(IFLAGS) $^ -t $(PREFIX)/include/$(PROJECT)

install: install-exec install-lib install-dev

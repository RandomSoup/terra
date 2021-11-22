PROJECT:=terra

include common.mk

CFLAGS+=-I./ -I./include

$(eval $(call decl,libterra,util loop dynstr dynarr piper path gem pget))
$(eval $(call decl,libpcgi,pcgi))

$(eval $(call decl,rover,main gui draw line rover))
$(eval $(call decl,ares,main clients))

$(eval $(call exec,rover,$(OBJ_libterra)))
$(eval $(call exec,ares,$(patsubst %,./build/libterra/%.o,util loop dynarr)))
$(eval $(call lib,libpcgi,$(patsubst %,./build/libterra/%.o,util)))
$(eval $(call lib,libterra))
$(eval $(call dev,terra pcgi))

LDFLAGS_rover:=$(shell pkg-config --libs x11) -lutil
LDFLAGS_ares:=-lrt

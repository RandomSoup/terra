PROJECT:=terra

include common.mk

CFLAGS+=-I./ -I./include

$(eval $(call decl,common,util loop dynstr dynarr))
$(eval $(call decl,rover,main piper gui draw line gem rover))
$(eval $(call decl,ares,main clients))
$(eval $(call decl,pcgi,pcgi))

$(eval $(call exec,rover,$(OBJ_common)))
$(eval $(call exec,ares,$(OBJ_common)))
$(eval $(call arlib,pcgi,./build/common/util.o))
$(eval $(call dev,common pcgi))

LDFLAGS_rover:=$(shell pkg-config --libs x11) -lutil
LDFLAGS_ares:=-lrt

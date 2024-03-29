R := ./
O := $Rout/
C :=

include util.mk

CFLAGS := -std=c99
CXXFLAGS := -std=c++98
CPPFLAGS := -Wall -Wextra -pedantic
CPPFLAGS += -O2 -g
CPPFLAGS += -MD
CPPFLAGS += -I.
CPPFLAGS += -DVERSION='"$(shell ./version.sh)"'

CPPFLAGS += $(shell sdl-config --cflags)
LDLIBS += $(shell sdl-config --libs)

include game/module.mk
include editor/module.mk

.PHONY: clean
clean:
	$(if $Q,@echo "  CLEAN $C")
	$Qrm -rf $C

.PHONY: test
test:
	@echo "TODO: implement test"

.PHONY: distclean
distclean:
	rm -rf $O

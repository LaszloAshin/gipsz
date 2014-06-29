.PHONY: all
all: ed

CPPFLAGS := -Wall -Wextra -pedantic
CPPFLAGS += -O2 -g
CPPFLAGS += -MD

CPPFLAGS += $(shell sdl-config --cflags)
LDFLAGS += $(shell sdl-config --libs)

ed_SOURCES := \
	bsp.c \
	ed.c \
	gr.c \
	line.c \
	main.c \
	object.c \
	sector.c \
	st2.c \
	st.c \
	vertex.c

ed_OBJECTS := $(addsuffix .o,$(basename $(ed_SOURCES)))
-include $(addsuffix .d,$(basename $(ed_OBJECTS)))

ed: $(ed_OBJECTS)

.PHONY: clean
clean:
	rm -f ed *.o *.d

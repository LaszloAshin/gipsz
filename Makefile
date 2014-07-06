.PHONY: all
all: build/sdl

CPPFLAGS := -Wall -Wextra -pedantic
CPPFLAGS += -O2 -g
CPPFLAGS += -MD

CPPFLAGS += $(shell sdl-config --cflags)
LDFLAGS += $(shell sdl-config --libs)

LDLIBS += -lGL

sdl_SOURCES := \
	bbox.c \
	bsp.c \
	cmd.c \
	console.c \
	game.c \
	input.c \
	main.c \
	menu.c \
	mm.c \
	model.c \
	obj.c \
	ogl.c \
	player.c \
	render.c \
	tex.c \
	tga.c \
	tx2.c

sdl_OBJECTS := $(addprefix build/sdl.d/,$(addsuffix .o,$(basename $(sdl_SOURCES))))
-include $(addsuffix .d,$(basename $(sdl_OBJECTS)))

build/sdl: $(sdl_OBJECTS)
	mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(LDLIBS) $(LOADLIBES) $^ $(OUTPUT_OPTION)

build/sdl.d/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) -c $< $(CPPFLAGS) $(CFLAGS) $(OUTPUT_OPTION)

.PHONY: clean
clean:
	rm -rf build/sdl*

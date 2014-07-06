R := ./
O := $Rout/

.PHONY: all
all: $Ogame $Oeditor

CPPFLAGS := -Wall -Wextra -pedantic
CPPFLAGS += -O2 -g
CPPFLAGS += -MD

CPPFLAGS += $(shell sdl-config --cflags)
LDFLAGS += $(shell sdl-config --libs)

game_SOURCES := \
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

game_OBJECTS := $(addprefix $Ogame.d/,$(addsuffix .o,$(basename $(game_SOURCES))))
-include $(addsuffix .d,$(basename $(game_OBJECTS)))

$Ogame: LDLIBS += -lGL

$Ogame: $(game_OBJECTS)
	mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(LDLIBS) $(LOADLIBES) $^ $(OUTPUT_OPTION)

$Ogame.d/%.o: game/%.c
	mkdir -p $(@D)
	$(CC) -c $< $(CPPFLAGS) $(CFLAGS) $(OUTPUT_OPTION)

editor_SOURCES := \
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

editor_OBJECTS := $(addprefix $Oeditor.d/,$(addsuffix .o,$(basename $(editor_SOURCES))))
-include $(addsuffix .d,$(basename $(editor_OBJECTS)))

$Oeditor: $(editor_OBJECTS)
	mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(LDLIBS) $(LOADLIBES) $^ $(OUTPUT_OPTION)

$Oeditor.d/%.o: editor/%.c
	mkdir -p $(@D)
	$(CC) -c $< $(CPPFLAGS) $(CFLAGS) $(OUTPUT_OPTION)

.PHONY: clean
clean:
	rm -rf $Ogame* $Oeditor*

R := ./
O := $Rout/

ifndef V
Q := @
endif

.PHONY: all
all: $Ogame $Oeditor

CFLAGS := -std=c99
CXXFLAGS := -std=c++98
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

define compile-c-steps
	$(if $Q,@echo "  CC    $@")
	$Qmkdir -p $(@D)
	$Q$(CC) -c $< $(CPPFLAGS) $(CFLAGS) $(OUTPUT_OPTION)
endef

define link-executable-steps
	$(if $Q,@echo "  LINK  $@")
	$Qmkdir -p $(@D)
	$Q$(CC) $(LDFLAGS) $(LDLIBS) $(LOADLIBES) $^ $(OUTPUT_OPTION)
endef

$Ogame: $(game_OBJECTS)
	$(call link-executable-steps)

$Ogame.d/%.o: game/%.c
	$(call compile-c-steps)

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
	$(call link-executable-steps)

$Oeditor.d/%.o: editor/%.c
	$(call compile-c-steps)

.PHONY: clean
clean:
	$(if $Q,@echo "  CLEAN")
	$Qrm -rf $Ogame* $Oeditor*

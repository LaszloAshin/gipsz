# game/module.mk

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

game_OBJECTS := $(addprefix $Od/game/,$(addsuffix .o,$(basename $(game_SOURCES))))
-include $(addsuffix .d,$(basename $(game_OBJECTS)))

$Ogame: LDLIBS += -lGL

$Ogame: $(game_OBJECTS)
	$(call link-executable-steps)

.PHONY: game
game: $Ogame

all: game

C += $Ogame* $Od/game

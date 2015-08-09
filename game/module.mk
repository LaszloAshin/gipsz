# game/module.mk

game_SOURCES := \
	aabb.cc \
	bsp.cc \
	cmd.cc \
	console.cc \
	game.cc \
	input.cc \
	main.cc \
	menu.cc \
	mm.cc \
	model.cc \
	obj.cc \
	ogl.cc \
	player.cc \
	render.cc \
	tex.cc \
	tga.cc \
	tx2.cc

game_OBJECTS := $(addprefix $Od/game/,$(addsuffix .o,$(basename $(game_SOURCES))))
-include $(addsuffix .d,$(basename $(game_OBJECTS)))

$Ogipsz-game: LDLIBS += -lGL

$Ogipsz-game: $(game_OBJECTS)
	$(call link-executable-steps)

.PHONY: game
game: $Ogipsz-game

all: game

C += $Ogipsz-game $Od/game

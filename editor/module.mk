# editor/module.mk

editor_SOURCES := \
	bsp.c \
	ed.c \
	gr.c \
	line.c \
	main.c \
	object.c \
	sector.c \
	st.c \
	vertex.c

editor_OBJECTS := $(addprefix $Od/editor/,$(addsuffix .o,$(basename $(editor_SOURCES))))
-include $(addsuffix .d,$(basename $(editor_OBJECTS)))

$Oeditor: $(editor_OBJECTS)
	$(call link-executable-steps)

all: $Oeditor

C += $Oeditor* $Od/editor

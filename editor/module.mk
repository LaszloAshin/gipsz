# editor/module.mk

editor_SOURCES := \
	bsp2.cc \
	ed.cc \
	gr.cc \
	line.cc \
	main.cc \
	object.cc \
	sector.cc \
	st.cc \
	vertex.cc

editor_OBJECTS := $(addprefix $Od/editor/,$(addsuffix .o,$(basename $(editor_SOURCES))))
-include $(addsuffix .d,$(basename $(editor_OBJECTS)))

$Oeditor: $(editor_OBJECTS)
	$(call link-executable-steps)

.PHONY: editor
editor: $Oeditor

all: editor

C += $Oeditor* $Od/editor

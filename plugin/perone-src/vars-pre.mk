CFLAGS_EXTRA := -I../../src

ifeq ($(PERONE_PLATFORM), wasm32)
	CFLAGS_EXTRA += -I../perone-src/wasm
endif

UI_PLUGIN_DIR := ..
UI_CPPFLAGS = -I../../vst3/src $(shell pkg-config --cflags xcb)
UI_CXX_SRCS_EXTRA := build/src/asid_gui.cpp build/src/gui-x.cpp
UI_LDLIBS = $(shell pkg-config --libs xcb)

# The existing GUI sources are compiled as C++ by the other plugin targets.
build/src/asid_gui.cpp: ../../vst3/src/asid_gui.c
	mkdir -p $(dir $@)
	cp $< $@

build/src/gui-x.cpp: ../../vst3/src/gui-x.c
	mkdir -p $(dir $@)
	cp $< $@

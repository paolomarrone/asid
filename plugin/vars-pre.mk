ASID_SRC_DIR := ../../src
ASID_GUI_DIR := ../../vst3/src
VST3_C_API_DIR ?= ../../../vst3_c_api

C_SRCS_EXTRA := $(ASID_SRC_DIR)/asid.c $(ASID_SRC_DIR)/mos_8580_filter.c
CXX_SRCS_EXTRA := build/src/asid_gui.cpp
CFLAGS_EXTRA := -I$(ASID_SRC_DIR) -I$(ASID_GUI_DIR)
CXXFLAGS_EXTRA := $(CFLAGS_EXTRA)
LDLIBS_EXTRA := -lm

ifeq ($(TEMPLATE), vst3)
	CFLAGS_EXTRA += -I$(VST3_C_API_DIR)
endif

ifeq ($(OS), Windows_NT)
	CXX_SRCS_EXTRA += build/src/gui-win32.cpp
	LDFLAGS_EXTRA := -mwindows
else ifeq ($(UNAME_S), Darwin)
	MM_SRCS_EXTRA := $(ASID_GUI_DIR)/gui-cocoa.mm
	LDLIBS_EXTRA += -framework Cocoa -lobjc
else
	CXX_SRCS_EXTRA += build/src/gui-x.cpp
	CXXFLAGS_EXTRA += $(shell pkg-config --cflags xcb)
	LDLIBS_EXTRA += $(shell pkg-config --libs xcb)
endif

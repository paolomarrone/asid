# The existing GUI sources use C++ even where the extension is .c.
build/src/asid_gui.cpp: $(ASID_GUI_DIR)/asid_gui.c | build/src
	cp $< $@

build/src/gui-x.cpp: $(ASID_GUI_DIR)/gui-x.c | build/src
	cp $< $@

build/src/gui-win32.cpp: $(ASID_GUI_DIR)/gui-win32.c | build/src
	cp $< $@

# Tibia's compile recipes pass $^ to the compiler. Keep headers out of that list.
$(C_OBJS) $(CXX_OBJS) $(MM_OBJS): .EXTRA_PREREQS := $(wildcard \
	$(PLUGIN_DIR)/*.h $(API_DIR)/*.h $(DATA_DIR)/src/*.h \
	$(ASID_SRC_DIR)/*.h $(ASID_GUI_DIR)/*.h)

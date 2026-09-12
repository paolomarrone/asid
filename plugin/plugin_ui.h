/*
 * Tibia
 *
 * Copyright (C) 2024 Orastron Srl unipersonale
 *
 * Tibia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Tibia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tibia.  If not, see <http://www.gnu.org/licenses/>.
 *
 * File author: Stefano D'Angelo
 */

#include "asid_gui.h"
#include <stdlib.h>

typedef struct {
	void *			widget;
	asid_gui		gui;
	asid_gui_view		view;
	float			parameters[4];
	plugin_ui_callbacks	cbs;
} plugin_ui;

static void plugin_ui_get_default_size(uint32_t *width, uint32_t *height) {
	*width = asid_gui_get_default_width(NULL);
	*height = asid_gui_get_default_height(NULL);
}

static float plugin_ui_get_parameter_cb(asid_gui gui, uint32_t id) {
	plugin_ui *instance = (plugin_ui *)asid_gui_get_data(gui);
	return instance->parameters[id];
}

static void plugin_ui_set_parameter_cb(asid_gui gui, uint32_t id, float value) {
	plugin_ui *instance = (plugin_ui *)asid_gui_get_data(gui);
	static const size_t indices[] = {
		plugin_parameter_cutoff, plugin_parameter_lfo_amount, plugin_parameter_lfo_speed
	};
	instance->parameters[id] = value;
	const float percent = 100.f * value;
	instance->cbs.set_parameter_begin(instance->cbs.handle, indices[id], percent);
	instance->cbs.set_parameter(instance->cbs.handle, indices[id], percent);
	instance->cbs.set_parameter_end(instance->cbs.handle, indices[id], percent);
}

static plugin_ui *plugin_ui_create(char has_parent, void *parent, plugin_ui_callbacks *cbs) {
	plugin_ui *instance = (plugin_ui *)calloc(1, sizeof(plugin_ui));
	if (instance == NULL)
		return NULL;
	instance->cbs = *cbs;
	instance->parameters[0] = 1.f;
	instance->gui = asid_gui_new(plugin_ui_get_parameter_cb, plugin_ui_set_parameter_cb, instance);
	if (instance->gui == NULL) {
		free(instance);
		return NULL;
	}
	// The legacy X11 and Win32 backends expect a pointer to the parent handle.
#if defined(__linux__)
	uint32_t parent_window = (uint32_t)(uintptr_t)parent;
	void *native_parent = has_parent ? &parent_window : NULL;
#elif defined(_WIN32) || defined(__CYGWIN__)
	void *native_parent = has_parent ? &parent : NULL;
#else
	void *native_parent = has_parent ? parent : NULL;
#endif
	instance->view = asid_gui_view_new(instance->gui, native_parent);
	if (instance->view == NULL) {
		asid_gui_free(instance->gui);
		free(instance);
		return NULL;
	}
#ifdef __linux__
	instance->widget = (void *)(uintptr_t)*(uint32_t *)asid_gui_view_get_handle(instance->view);
#else
	instance->widget = asid_gui_view_get_handle(instance->view);
#endif
	return instance;
}

static void plugin_ui_free(plugin_ui *instance) {
	asid_gui_view_free(instance->view);
	asid_gui_free(instance->gui);
	free(instance);
}

static void plugin_ui_idle(plugin_ui *instance) {
	asid_gui_process_events(instance->gui);
	asid_gui_view_on_timeout(instance->view);
}

static void plugin_ui_set_parameter(plugin_ui *instance, size_t index, float value) {
	uint32_t id;
	switch (index) {
	case plugin_parameter_cutoff: id = 0; break;
	case plugin_parameter_lfo_amount: id = 1; break;
	case plugin_parameter_lfo_speed: id = 2; break;
	case plugin_parameter_mod_cutoff: id = 3; break;
	default: return;
	}
	instance->parameters[id] = 0.01f * value;
	asid_gui_on_param_set(instance->gui, id, instance->parameters[id]);
}

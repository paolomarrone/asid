/*
 * Tibia
 *
 * Copyright (C) 2023, 2024 Orastron Srl unipersonale
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

#include "asid.h"
#include <limits.h>
#include <string.h>

typedef struct plugin {
	asid	instance;
	char	bypass;
	float	mod_cutoff;
} plugin;

static int plugin_init(plugin *instance, const plugin_callbacks *cbs) {
	(void)cbs;
	instance->instance = asid_new();
	instance->bypass = 0;
	instance->mod_cutoff = 0.f;
	return instance->instance == NULL ? -1 : 0;
}

static void plugin_fini(plugin *instance) {
	asid_free(instance->instance);
}

static void plugin_set_sample_rate(plugin *instance, float sample_rate) {
	asid_set_sample_rate(instance->instance, sample_rate);
}

static size_t plugin_mem_req(plugin *instance) {
	(void)instance;
	return 0;
}

static void plugin_mem_set(plugin *instance, void *mem) {
	(void)instance;
	(void)mem;
}

static void plugin_reset(plugin *instance) {
	asid_reset(instance->instance);
	instance->mod_cutoff = 0.f;
}

static void plugin_set_parameter(plugin *instance, size_t index, float value) {
	switch (index) {
	case plugin_parameter_cutoff:
		asid_set_parameter(instance->instance, 0, 0.01f * value);
		break;
	case plugin_parameter_lfo_amount:
		asid_set_parameter(instance->instance, 1, 0.01f * value);
		break;
	case plugin_parameter_lfo_speed:
		asid_set_parameter(instance->instance, 2, 0.01f * value);
		break;
	case plugin_parameter_bypass:
		instance->bypass = value >= 0.5f;
		break;
	}
}

static float plugin_get_parameter(plugin *instance, size_t index) {
	return index == plugin_parameter_mod_cutoff ? instance->mod_cutoff : 0.f;
}

static void plugin_process(plugin *instance, const float **inputs, float **outputs, size_t n_samples) {
	if (n_samples == 0)
		return;
	if (instance->bypass) {
		memmove(outputs[0], inputs[0], n_samples * sizeof(float));
		return;
	}

	for (size_t offset = 0; offset < n_samples;) {
		const size_t left = n_samples - offset;
		const int n = left > INT_MAX ? INT_MAX : (int)left;
		const float *x[] = { inputs[0] + offset };
		float *y[] = { outputs[0] + offset };
		asid_process(instance->instance, x, y, n);
		offset += (size_t)n;
	}
	instance->mod_cutoff = 100.f * asid_get_parameter(instance->instance, 3);
}

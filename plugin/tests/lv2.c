#include <assert.h>
#include <dlfcn.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <lv2/core/lv2.h>
#include <lv2/ui/ui.h>
#include <lv2/urid/urid.h>
#include "asid.h"

#define N 1024

static unsigned edits;
static void write_parameter(LV2UI_Controller controller, uint32_t port, uint32_t size,
	uint32_t format, const void *buffer) {
	(void)controller;
	assert(port == 2 && size == sizeof(float) && format == 0);
	assert(*(const float *)buffer > 50.f && *(const float *)buffer <= 100.f);
	edits++;
}

static LV2_URID map_uri(LV2_URID_Map_Handle handle, const char *uri) {
	(void)handle;
	static char uris[32][256];
	static uint32_t count;
	for (uint32_t i = 0; i < count; i++)
		if (!strcmp(uri, uris[i]))
			return i + 1;
	assert(count < 32 && strlen(uri) < sizeof(uris[0]));
	strcpy(uris[count], uri);
	return ++count;
}

static void check_ui(void *library, const char *bundle, const LV2_Feature *map) {
	Display *display = XOpenDisplay(NULL);
	if (!display) {
		puts("UI: skipped (no X11 display)");
		return;
	}
	Window parent = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 404, 284, 0, 0, 0);
	XSync(display, False);
	LV2_Feature parent_feature = { LV2_UI__parent, (void *)(uintptr_t)parent };
	const LV2_Feature *features[] = { map, &parent_feature, NULL };
	const LV2UI_Descriptor *(*descriptor)(uint32_t) = dlsym(library, "lv2ui_descriptor");
	assert(descriptor && descriptor(0) && !descriptor(1));
	const LV2UI_Descriptor *ui = descriptor(0);
	LV2UI_Widget widget = NULL;
	LV2UI_Handle handle = ui->instantiate(ui, "https://www.orastron.com/asid", bundle,
		write_parameter, NULL, &widget, features);
	assert(handle && widget);
	const LV2UI_Idle_Interface *idle = ui->extension_data(LV2_UI__idleInterface);
	assert(idle);
	float value = 50.f;
	ui->port_event(handle, 2, sizeof(value), 0, &value);
	ui->port_event(handle, 6, sizeof(value), 0, &value);
	idle->idle(handle);

	XEvent event = {0};
	event.xbutton.type = ButtonPress;
	event.xbutton.display = display;
	event.xbutton.window = (Window)widget;
	event.xbutton.button = Button1;
	event.xbutton.x = 70;
	event.xbutton.y = 150;
	XSendEvent(display, (Window)widget, False, ButtonPressMask, &event);
	event.xmotion.type = MotionNotify;
	event.xmotion.state = Button1Mask;
	event.xmotion.y = 100;
	XSendEvent(display, (Window)widget, False, PointerMotionMask, &event);
	XSync(display, False);
	for (int i = 0; i < 10 && !edits; i++)
		idle->idle(handle);
	assert(edits);
	XResizeWindow(display, (Window)widget, 808, 568);
	XSync(display, False);
	idle->idle(handle);
	ui->cleanup(handle);
	XDestroyWindow(display, parent);
	XCloseDisplay(display);
	puts("UI: embedding, host updates, editing and resizing passed");
}

int main(int argc, char **argv) {
	assert(argc == 2);
	char path[4096];
	snprintf(path, sizeof(path), "%sasid.so", argv[1]);
	void *library = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	if (!library) { fprintf(stderr, "%s\n", dlerror()); return 1; }
	const LV2_Descriptor *(*descriptor)(uint32_t) = dlsym(library, "lv2_descriptor");
	assert(descriptor && descriptor(0) && !descriptor(1));
	const LV2_Descriptor *d = descriptor(0);
	assert(!strcmp(d->URI, "https://www.orastron.com/asid"));
	LV2_URID_Map map = { NULL, map_uri };
	LV2_Feature map_feature = { LV2_URID__map, &map };
	const LV2_Feature *features[] = { &map_feature, NULL };
	LV2_Handle handle = d->instantiate(d, 48000, argv[1], features);
	assert(handle);
	float x[N], y[N], expected[N], controls[] = { 50.f, 75.f, 25.f, 1.f, 0.f };
	for (int i = 0; i < N; i++) x[i] = 0.2f * sinf(0.13f * i);
	d->connect_port(handle, 0, x);
	d->connect_port(handle, 1, y);
	for (uint32_t i = 0; i < 5; i++) d->connect_port(handle, 2 + i, &controls[i]);
	d->activate(handle);
	d->run(handle, 0);
	assert(controls[4] == 0.f);
	asid reference = asid_new();
	assert(reference);
	asid_set_sample_rate(reference, 48000);
	for (int i = 0; i < 3; i++) asid_set_parameter(reference, i, 0.01f * controls[i]);
	asid_reset(reference);
	const float *in[] = { x };
	float *out[] = { expected };
	for (int block = 0; block < 8; block++) {
		controls[0] = block * 12.5f;
		asid_set_parameter(reference, 0, 0.01f * controls[0]);
		asid_process(reference, in, out, N);
		d->run(handle, N);
		for (int i = 0; i < N; i++) assert(isfinite(y[i]) && fabsf(y[i] - expected[i]) < 1e-5f);
		assert(fabsf(controls[4] - 100.f * asid_get_parameter(reference, 3)) < 1e-5f);
	}
	controls[3] = 0.f; // LV2 Enabled is the inverse of bypass.
	d->run(handle, N);
	assert(!memcmp(x, y, sizeof(x)));
	d->connect_port(handle, 1, x);
	d->run(handle, N);
	assert(!memcmp(x, y, sizeof(x)));
	controls[3] = 1.f;
	asid_process(reference, in, out, N);
	d->run(handle, N);
	for (int i = 0; i < N; i++) assert(isfinite(x[i]) && fabsf(x[i] - expected[i]) < 1e-5f);
	if (d->deactivate) d->deactivate(handle);
	d->connect_port(handle, 5, NULL); // Optional Enabled defaults to active on activation.
	d->activate(handle);
	asid_reset(reference);
	asid_process(reference, in, out, N);
	d->run(handle, N);
	for (int i = 0; i < N; i++) assert(isfinite(x[i]) && fabsf(x[i] - expected[i]) < 1e-5f);
	asid_free(reference);
	if (d->deactivate) d->deactivate(handle);
	d->cleanup(handle);
	puts("LV2: loading, parameter mapping, metering, bypass and in-place processing passed");
	check_ui(library, argv[1], &map_feature);
	dlclose(library);
	return 0;
}

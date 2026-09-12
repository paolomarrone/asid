#include <assert.h>
#include <dlfcn.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include "asid.h"
#include "perone.h"
#include "perone_ui.h"

#define N 1024

static void unexpected_edit(void *handle, size_t index, float value) {
	(void)handle; (void)index; (void)value;
	assert(!"A host parameter update must not be echoed back");
}

static void check_ui(const char *path) {
	void *library = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	if (!library) fprintf(stderr, "%s\n", dlerror());
	assert(library && !dlsym(library, "perone_get_api"));
	const perone_ui_api *(*get)(uint32_t) = dlsym(library, "perone_ui_get_api");
	assert(get && !get(0) && !get(PERONE_UI_ABI_VERSION + 1));
	const perone_ui_api *ui = get(PERONE_UI_ABI_VERSION);
	assert(ui && !ui->msg_in);
	uint32_t width, height;
	ui->get_default_size(&width, &height);
	assert(width == 404 && height == 284);
	Display *display = XOpenDisplay(NULL);
	if (!display) {
		puts("Perone UI: loading passed; embedding skipped (no X11 display)");
		dlclose(library);
		return;
	}
	Window parent = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, width, height, 0, 0, 0);
	XSync(display, False);
	perone_ui_callbacks cbs = { NULL, NULL, NULL, unexpected_edit, unexpected_edit, unexpected_edit, NULL };
	assert(!ui->create(0, 1, (void *)(uintptr_t)parent, &cbs));
	void *view = ui->create(PERONE_UI_X11, 1, (void *)(uintptr_t)parent, &cbs);
	assert(view);
	Window widget = (Window)(uintptr_t)ui->get_widget(view);
	assert(widget);
	Window root, actual_parent, *children;
	unsigned count;
	XSync(display, False);
	assert(XQueryTree(display, widget, &root, &actual_parent, &children, &count));
	if (children) XFree(children);
	assert(actual_parent == parent);
	for (size_t i = 0; i < 5; i++) ui->set_parameter(view, i, i == 3 ? 0.f : 50.f);
	ui->idle(view);
	XResizeWindow(display, widget, 808, 568);
	XSync(display, False);
	ui->idle(view);
	ui->free(view);
	XDestroyWindow(display, parent);
	XCloseDisplay(display);
	dlclose(library);
	puts("Perone UI: ABI, embedding, parameter updates, resize and cleanup passed");
}

int main(int argc, char **argv) {
	assert(argc == 3);
	void *library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
	if (!library) fprintf(stderr, "%s\n", dlerror());
	assert(library && !dlsym(library, "perone_ui_get_api"));
	const perone_api *(*get)(uint32_t) = dlsym(library, "perone_get_api");
	assert(get && !get(1) && !get(PERONE_ABI_VERSION + 1));
	const perone_api *api = get(PERONE_ABI_VERSION);
	assert(api && api->set_parameter && api->get_parameter);
	assert(!api->midi_msg_in && !api->set_transport && !api->msg_in && !api->state_save && !api->state_load);
	void *instance = api->alloc();
	assert(instance);
	perone_callbacks cbs = {0};
	assert(api->init(instance, &cbs) == 0);
	asid reference = asid_new();
	assert(reference);
	float x[N], y[N], expected[N];
	const float *in[] = { x };
	float *out[] = { y }, *ref_out[] = { expected };
	for (int i = 0; i < N; i++) x[i] = 0.2f * sinf(0.13f * i);
	const float rates[] = { 48000.f, 96000.f };
	for (size_t rate = 0; rate < 2; rate++) {
		api->set_sample_rate(instance, rates[rate]);
		asid_set_sample_rate(reference, rates[rate]);
		assert(api->mem_req(instance) == 0);
		api->mem_set(instance, NULL);
		api->set_parameter(instance, 3, 0.f);
		api->reset(instance);
		asid_reset(reference);
		api->process(instance, NULL, NULL, 0);
		assert(api->get_parameter(instance, 4) == 0.f);
		for (int block = 0; block < 8; block++) {
			const float params[] = { block * 12.5f, 75.f, 25.f };
			for (size_t i = 0; i < 3; i++) {
				api->set_parameter(instance, i, params[i]);
				asid_set_parameter(reference, (int)i, 0.01f * params[i]);
			}
			api->process(instance, in, out, N);
			asid_process(reference, in, ref_out, N);
			for (int i = 0; i < N; i++) assert(isfinite(y[i]) && fabsf(y[i] - expected[i]) < 1e-5f);
			assert(fabsf(api->get_parameter(instance, 4) - 100.f * asid_get_parameter(reference, 3)) < 1e-5f);
		}
		api->set_parameter(instance, 3, 1.f);
		api->process(instance, in, out, N);
		assert(!memcmp(x, y, sizeof(x)));
		const float *same[] = { y };
		api->process(instance, same, out, N);
		assert(!memcmp(x, y, sizeof(x)));
	}
	asid_free(reference);
	api->fini(instance);
	api->free(instance);
	api->free(NULL);
	dlclose(library);
	puts("Perone: ABI, processing, parameters, metering, bypass and rate changes passed");
	check_ui(argv[2]);
	return 0;
}

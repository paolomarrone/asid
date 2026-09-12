const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");

const [bundle, source] = process.argv.slice(2);
const metadata = JSON.parse(fs.readFileSync(path.join(bundle, "product.json"), "utf8"));
assert.deepEqual(metadata, JSON.parse(fs.readFileSync(source, "utf8")));
const { product } = metadata;
const module_ = new WebAssembly.Module(fs.readFileSync(path.join(bundle, "wasm32", product.bundleName + ".wasm")));
assert.deepEqual(WebAssembly.Module.imports(module_), []);
const wasm = new WebAssembly.Instance(module_).exports;
wasm.__wasm_call_ctors();
assert.equal(wasm.perone_get_api(1), 0);
assert.equal(wasm.perone_get_api(3), 0);
const names = ["alloc", "free", "init", "fini", "set_sample_rate", "mem_req", "mem_set", "reset", "process",
	"set_parameter", "get_parameter", "midi_msg_in", "set_transport", "msg_in", "state_save", "state_load"];
const apiPtr = wasm.perone_get_api(2);
assert(apiPtr);
const api = Object.fromEntries(Array.from(new Uint32Array(wasm.memory.buffer, apiPtr, names.length),
	(index, i) => [names[i], index ? wasm.__indirect_function_table.get(index) : null]));
for (const name of names.slice(0, 11)) assert.equal(typeof api[name], "function", name);
for (const name of names.slice(11)) assert.equal(api[name], null, name);

const owned = [];
function alloc(size) {
	const ptr = wasm.calloc(1, size);
	assert(ptr);
	owned.push(ptr);
	return ptr;
}
const callbacks = alloc(16);
const instance = api.alloc();
assert(instance);
assert.equal(api.init(instance, callbacks), 0);
product.parameters.forEach((p, i) => {
	if (p.direction === "input") api.set_parameter(instance, i, p.defaultValue);
});
const index = id => product.parameters.findIndex(p => p.id === id);
const frames = 1024;
const input = alloc(frames * 4), output = alloc(frames * 4);
const inputs = alloc(4), outputs = alloc(4);
new DataView(wasm.memory.buffer).setUint32(inputs, input, true);
new DataView(wasm.memory.buffer).setUint32(outputs, output, true);
const samples = Float32Array.from({ length: frames }, (_, i) => 0.2 * Math.sin(0.13 * i));
new Float32Array(wasm.memory.buffer, input, frames).set(samples);
const readOutput = () => new Float32Array(wasm.memory.buffer, output, frames).slice();

for (const rate of [48000, 96000]) {
	api.set_sample_rate(instance, rate);
	assert.equal(api.mem_req(instance), 0);
	api.mem_set(instance, 0);
	api.reset(instance);
	api.process(instance, 0, 0, 0);
	assert.equal(api.get_parameter(instance, index("mod_cutoff")), 0);
	api.set_parameter(instance, index("bypass"), 0);
	api.set_parameter(instance, index("lfo_amount"), 75);
	api.set_parameter(instance, index("lfo_speed"), 25);
	let previous;
	for (const cutoff of [0, 25, 50, 100]) {
		api.set_parameter(instance, index("cutoff"), cutoff);
		api.reset(instance);
		api.process(instance, inputs, outputs, frames);
		const rendered = readOutput();
		assert(rendered.every(Number.isFinite));
		assert(rendered.some(x => Math.abs(x) > 1e-6));
		if (previous) assert.notDeepEqual(rendered, previous);
		previous = rendered;
		const meter = api.get_parameter(instance, index("mod_cutoff"));
		assert(Number.isFinite(meter) && meter >= 0 && meter <= 100);
		api.reset(instance);
		api.process(instance, inputs, outputs, frames);
		assert.deepEqual(readOutput(), rendered);
	}
	api.set_parameter(instance, index("bypass"), 1);
	api.process(instance, inputs, outputs, frames);
	assert.deepEqual(readOutput(), samples);
	api.process(instance, outputs, outputs, frames);
	assert.deepEqual(readOutput(), samples);
}
const oldMemory = wasm.memory.buffer;
alloc(oldMemory.byteLength + 65536);
assert.notEqual(wasm.memory.buffer, oldMemory);
api.process(instance, inputs, outputs, frames);
assert.deepEqual(readOutput(), samples);
api.fini(instance);
api.free(instance);
api.free(0);
owned.forEach(ptr => wasm.free(ptr));
console.log("Perone Wasm: metadata, ABI, processing, reset, parameters, bypass and memory growth passed");

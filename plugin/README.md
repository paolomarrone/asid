# A-SID with Tibia

This directory builds VST3, LV2 and Perone plugins using the adjacent `../../tibia`
checkout. The wrappers use the existing A-SID DSP and native GUI sources.

From the repository root:

```sh
sh plugin/tibia_gen.sh
make -C plugin/lv2
make -C plugin/vst3 VST3_C_API_DIR=/path/to/vst3_c_api
make -C plugin/perone
```

Generation needs Node.js and Tibia's `dot` module. Set `TIBIA_DIR` to use a
different Tibia checkout. The scripts work independently of the current directory.

The builds need Make and Clang/Clang++. VST3 needs
[Steinberg's VST3 C API header](https://github.com/steinbergmedia/vst3_c_api),
defaulting to a `vst3_c_api` checkout next to this repository. LV2 needs the LV2
headers and pkg-config. On Linux, both formats also need X11 and XCB development
packages. The native GUI backends are selected for Linux, Windows and macOS.

The bundles are generated under `plugin/vst3/build/asid.vst3`,
`plugin/lv2/build/asid.lv2` and `plugin/perone/build/asid.perone`.
Generated sources and binaries are ignored by Git.
`sh plugin/tibia_clean.sh` removes all generated projects and their shared API.

Perone's default build produces `product.json` and the native DSP library, e.g.
`x86_64-linux/asid.so`, without GUI dependencies. These optional builds add the
original X11 GUI and a standalone WebAssembly module to the same bundle:

```sh
make -C plugin/perone ui
make -C plugin/perone PERONE_PLATFORM=wasm32
```

The UI is `x86_64-linux/asid-ui.so`; the Wasm module is `wasm32/asid.wasm`.
Distribute the entire `asid.perone` directory. Native Perone targets ELF
platforms; its UI currently requires Linux/X11. Wasm needs Clang and wasm-ld.
`perone-src/` assembles the existing engine and wrapper into Perone's single
translation unit and supplies the small compatibility headers needed by Tibia's
freestanding Wasm runtime. Browser hosts can generate controls from `product.json`.

Cutoff, LFO Amount and LFO Speed use Tibia's 0–100 range, converted to the
engine's normalized values. Modulated Cutoff is converted back to 0–100.
Bypass passes audio through and pauses the engine; LV2 exposes its inverse as
the standard Enabled control. Bypass is available through the host, while the
custom GUI retains the original three controls.

Run the Linux integration check with:

```sh
sh plugin/test.sh
```

It builds and loads the LV2 bundle, compares processing against the existing
engine, and checks parameter changes, bypass and in-place processing. When an X11
display is available, it also creates the UI inside an unmapped test window and
checks parameter editing and resizing.

`sh plugin/test_perone.sh` builds and tests the native Perone DSP, X11 UI and
Wasm module. It checks ABI versions, metadata, audio against the original engine,
parameters, bypass, sample-rate changes and Wasm memory growth. UI embedding is
checked when an X11 display is available.

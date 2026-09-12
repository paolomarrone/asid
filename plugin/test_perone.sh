#!/bin/sh

set -eu

dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
sh "$dir/tibia_gen.sh"
make -C "$dir/perone"
make -C "$dir/perone" ui
make -C "$dir/perone" PERONE_PLATFORM=wasm32

test_dir=$(mktemp -d)
trap 'rm -rf -- "$test_dir"' EXIT HUP INT TERM
platform="$(uname -m)-$(uname -s | tr A-Z a-z)"
bundle="$dir/perone/build/asid.perone"

${CC:-clang} -O2 -I"$dir/../src" -I"$dir/perone/src" $(pkg-config --cflags x11) \
	"$dir/tests/perone.c" "$dir/../src/asid.c" "$dir/../src/mos_8580_filter.c" \
	-o "$test_dir/perone-test" -ldl -lm $(pkg-config --libs x11)
"$test_dir/perone-test" "$bundle/$platform/asid.so" "$bundle/$platform/asid-ui.so"
node "$dir/tests/perone_wasm.js" "$bundle" "$dir/tibia/product.json"

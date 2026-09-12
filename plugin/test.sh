#!/bin/sh

set -eu

dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
sh "$dir/tibia_gen.sh"
make -C "$dir/lv2"

test_dir=$(mktemp -d)
trap 'rm -rf -- "$test_dir"' EXIT HUP INT TERM

${CC:-clang} -O3 -I"$dir/../src" $(pkg-config --cflags lv2 x11) \
	"$dir/tests/lv2.c" "$dir/../src/asid.c" "$dir/../src/mos_8580_filter.c" \
	-o "$test_dir/lv2-test" -ldl -lm $(pkg-config --libs x11)
"$test_dir/lv2-test" "$dir/lv2/build/asid.lv2/"

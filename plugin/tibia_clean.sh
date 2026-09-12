#!/bin/sh

set -eu

dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
rm -fr -- "$dir/api" "$dir/vst3" "$dir/lv2" "$dir/perone"

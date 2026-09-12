#!/bin/sh

set -eu

dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TIBIA_DIR=${TIBIA_DIR:-"$dir/../../tibia"}
data="$dir/tibia/product.json,$dir/tibia/company.json"

"$TIBIA_DIR/tibia" "$data" "$TIBIA_DIR/templates/api" "$dir/api"
for format in vst3 lv2; do
	"$TIBIA_DIR/tibia" "$data,$dir/tibia/$format.json" "$TIBIA_DIR/templates/$format" "$dir/$format"
	"$TIBIA_DIR/tibia" "$data,$dir/tibia/$format.json,$dir/tibia/make.json,$dir/tibia/$format-make.json" "$TIBIA_DIR/templates/$format-make" "$dir/$format"
done

"$TIBIA_DIR/tibia" "$data" "$TIBIA_DIR/templates/perone" "$dir/perone"
"$TIBIA_DIR/tibia" "$data,$dir/tibia/make.json,$dir/tibia/perone-make.json" "$TIBIA_DIR/templates/perone-make" "$dir/perone"

#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

if [ "$#" -ne 2 ]; then echo "usage: $0 BUILD_DIR OUTPUT_DIR" >&2; exit 2; fi
BUILD_DIR=$(CDPATH= cd -- "$1" && pwd)
mkdir -p "$2"
OUTPUT_DIR=$(CDPATH= cd -- "$2" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ASSETS="$ROOT_DIR/packaging/portmaster/curseddrone"
STAGE="$OUTPUT_DIR/curseddrone-stage"
PACKAGE_ROOT="$STAGE/curseddrone"
for binary in cursed-drone-sdl cursed-drone-probe; do
  [ -x "$BUILD_DIR/$binary" ] || { echo "missing binary: $BUILD_DIR/$binary" >&2; exit 1; }
done
rm -rf "$STAGE"
mkdir -p "$PACKAGE_ROOT/curseddrone/licenses" "$PACKAGE_ROOT/curseddrone/assets/scales" "$PACKAGE_ROOT/curseddrone/docs"
cp "$ASSETS/port.json" "$PACKAGE_ROOT/port.json"
cp "$ASSETS/gameinfo.xml" "$PACKAGE_ROOT/gameinfo.xml"
cp "$ASSETS/README.md" "$PACKAGE_ROOT/README.md"
cp "$ASSETS/Cursed Drone.sh" "$PACKAGE_ROOT/Cursed Drone.sh"
cp "$ROOT_DIR/assets/branding/cursed-drone-splash.bmp" "$PACKAGE_ROOT/curseddrone/assets/cursed-drone-splash.bmp"
cp "$ROOT_DIR/assets/scales/"*.scl "$PACKAGE_ROOT/curseddrone/assets/scales/"
cp "$ROOT_DIR/docs/install.en.md" "$ROOT_DIR/docs/install.ru.md" "$ROOT_DIR/docs/workflow.en.md" "$ROOT_DIR/docs/workflow.ru.md" "$PACKAGE_ROOT/curseddrone/docs/"
cp "$ROOT_DIR/LICENSE" "$PACKAGE_ROOT/curseddrone/licenses/GPL-3.0.txt"
cp "$ROOT_DIR/THIRD_PARTY_NOTICES.md" "$PACKAGE_ROOT/curseddrone/licenses/THIRD_PARTY_NOTICES.md"
cp "$ROOT_DIR/third_party/font512/LICENSE" "$PACKAGE_ROOT/curseddrone/licenses/font512-UNLICENSE.txt"
cp "$ROOT_DIR/third_party/PLAITS_LICENSE.txt" "$PACKAGE_ROOT/curseddrone/licenses/Musical-engine-MIT.txt"
cp "$BUILD_DIR/cursed-drone-sdl" "$PACKAGE_ROOT/curseddrone/cursed-drone-sdl.aarch64"
cp "$BUILD_DIR/cursed-drone-probe" "$PACKAGE_ROOT/curseddrone/cursed-drone-probe.aarch64"
chmod +x "$PACKAGE_ROOT/Cursed Drone.sh" "$PACKAGE_ROOT/curseddrone/"*.aarch64
(cd "$PACKAGE_ROOT" && zip -9 -r "$OUTPUT_DIR/curseddrone-aarch64-test.zip" .)
rm -rf "$STAGE"
echo "$OUTPUT_DIR/curseddrone-aarch64-test.zip"

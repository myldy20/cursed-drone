#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 BUILD_DIR OUTPUT_DIR" >&2
    exit 2
fi

BUILD_DIR=$(CDPATH= cd -- "$1" && pwd)
mkdir -p "$2"
OUTPUT_DIR=$(CDPATH= cd -- "$2" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PUBLIC_ASSETS="$ROOT_DIR/packaging/portmaster/curseddrone"
LAB_ASSETS="$ROOT_DIR/packaging/portmaster/experiment"
STAGE="$OUTPUT_DIR/curseddrone-lab-stage"
PACKAGE_ROOT="$STAGE/package"
APP_ROOT="$PACKAGE_ROOT/curseddrone-lab"

for binary in cursed-drone-sdl cursed-drone-probe; do
    if [ ! -x "$BUILD_DIR/$binary" ]; then
        echo "missing binary: $BUILD_DIR/$binary" >&2
        exit 1
    fi
done

rm -rf "$STAGE"
mkdir -p \
    "$APP_ROOT/licenses" \
    "$APP_ROOT/assets/scales" \
    "$APP_ROOT/docs" \
    "$APP_ROOT/conf/scales"

cp "$LAB_ASSETS/port.json" "$PACKAGE_ROOT/port.json"
cp "$LAB_ASSETS/gameinfo.xml" "$PACKAGE_ROOT/gameinfo.xml"
cp "$PUBLIC_ASSETS/README.md" "$PACKAGE_ROOT/README.md"
cp "$LAB_ASSETS/Cursed Drone Lab.sh" "$PACKAGE_ROOT/Cursed Drone Lab.sh"
cp "$ROOT_DIR/assets/branding/cursed-drone-splash.bmp" "$APP_ROOT/assets/cursed-drone-splash.bmp"
cp "$ROOT_DIR/assets/scales/"*.scl "$APP_ROOT/assets/scales/"
cp "$ROOT_DIR/assets/scales/"*.scl "$APP_ROOT/conf/scales/"
cp "$ROOT_DIR/docs/install.en.md" "$APP_ROOT/docs/install.en.md"
cp "$ROOT_DIR/docs/install.ru.md" "$APP_ROOT/docs/install.ru.md"
cp "$ROOT_DIR/docs/experiment-lab.en.md" "$APP_ROOT/docs/experiment-lab.en.md"
cp "$ROOT_DIR/docs/experiment-lab.ru.md" "$APP_ROOT/docs/experiment-lab.ru.md"
cp "$ROOT_DIR/LICENSE" "$APP_ROOT/licenses/GPL-3.0.txt"
cp "$ROOT_DIR/THIRD_PARTY_NOTICES.md" "$APP_ROOT/licenses/THIRD_PARTY_NOTICES.md"
cp "$ROOT_DIR/third_party/PLAITS_LICENSE.txt" "$APP_ROOT/licenses/Plaits-MIT.txt"
cp "$ROOT_DIR/third_party/font512/LICENSE" "$APP_ROOT/licenses/font512-UNLICENSE.txt"
cp "$BUILD_DIR/cursed-drone-sdl" "$APP_ROOT/cursed-drone-sdl.aarch64"
cp "$BUILD_DIR/cursed-drone-probe" "$APP_ROOT/cursed-drone-probe.aarch64"

chmod +x "$PACKAGE_ROOT/Cursed Drone Lab.sh" "$APP_ROOT/"*.aarch64

(cd "$PACKAGE_ROOT" && zip -9 -r "$OUTPUT_DIR/curseddrone-lab-aarch64-test.zip" .)
rm -rf "$STAGE"
echo "$OUTPUT_DIR/curseddrone-lab-aarch64-test.zip"

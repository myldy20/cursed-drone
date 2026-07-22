#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Myldy Design
# Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.
set -eu
if [ "$#" -ne 2 ]; then echo "usage: $0 BUILD_DIR OUTPUT_DIR" >&2; exit 2; fi
BUILD_DIR=$(CDPATH= cd -- "$1" && pwd)
mkdir -p "$2"
OUTPUT_DIR=$(CDPATH= cd -- "$2" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SOURCE_PAK="$ROOT_DIR/packaging/nextui/tg5040/Cursed Drone.pak"
STAGE="$OUTPUT_DIR/cursed-drone-nextui-stage"
PAK_DIR="$STAGE/Tools/tg5040/Cursed Drone.pak"
ZIP_PATH="$OUTPUT_DIR/cursed-drone-nextui-tg5040-test.zip"
[ -x "$BUILD_DIR/cursed-drone-sdl" ] || { echo "missing binary: $BUILD_DIR/cursed-drone-sdl" >&2; exit 1; }
rm -rf "$STAGE" "$ZIP_PATH"
mkdir -p "$PAK_DIR/assets/scales" "$PAK_DIR/docs" "$PAK_DIR/licenses"
cp "$SOURCE_PAK/launch.sh" "$PAK_DIR/launch.sh"
cp "$SOURCE_PAK/README.md" "$PAK_DIR/README.md"
cp "$BUILD_DIR/cursed-drone-sdl" "$PAK_DIR/cursed-drone-sdl.aarch64"
cp "$ROOT_DIR/assets/branding/cursed-drone-splash.bmp" "$PAK_DIR/assets/cursed-drone-splash.bmp"
cp "$ROOT_DIR/assets/scales/"*.scl "$PAK_DIR/assets/scales/"
cp "$ROOT_DIR/docs/install.nextui.en.md" "$ROOT_DIR/docs/install.nextui.ru.md" "$ROOT_DIR/docs/workflow.en.md" "$ROOT_DIR/docs/workflow.ru.md" "$PAK_DIR/docs/"
cp "$ROOT_DIR/LICENSE" "$PAK_DIR/licenses/GPL-3.0.txt"
cp "$ROOT_DIR/THIRD_PARTY_NOTICES.md" "$PAK_DIR/licenses/THIRD_PARTY_NOTICES.md"
cp "$ROOT_DIR/NOTICE.md" "$PAK_DIR/licenses/NOTICE.md"
cp "$ROOT_DIR/ADDITIONAL_TERMS.md" "$PAK_DIR/licenses/ADDITIONAL_TERMS.md"
cp "$ROOT_DIR/third_party/font512/LICENSE" "$PAK_DIR/licenses/font512-UNLICENSE.txt"
cp "$ROOT_DIR/third_party/PLAITS_LICENSE.txt" "$PAK_DIR/licenses/Musical-engine-MIT.txt"
chmod +x "$PAK_DIR/launch.sh" "$PAK_DIR/cursed-drone-sdl.aarch64"
(cd "$STAGE" && zip -9 -r "$ZIP_PATH" Tools)
rm -rf "$STAGE"
echo "$ZIP_PATH"

#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -u

PAK_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SD_ROOT=$(CDPATH= cd -- "$PAK_DIR/../../.." && pwd)
USERDATA_ROOT=${USERDATA_PATH:-$SD_ROOT/.userdata/tg5040}
LOG_ROOT=${LOGS_PATH:-$USERDATA_ROOT/logs}
DATA_DIR="$USERDATA_ROOT/cursed-drone"
BIN="$PAK_DIR/cursed-drone-sdl.aarch64"
LOG_FILE="$LOG_ROOT/Cursed Drone.txt"

mkdir -p "$DATA_DIR" "$LOG_ROOT"

export HOME="$DATA_DIR"
export XDG_CONFIG_HOME="$DATA_DIR"
export XDG_DATA_HOME="$DATA_DIR"
export CURSED_DRONE_DATA_DIR="$DATA_DIR"
export CURSED_DRONE_ASSET_DIR="$PAK_DIR/assets"
export CURSED_DRONE_HANDHELD=1
export SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS=0

cd "$PAK_DIR" || exit 1
chmod +x "$BIN" 2>/dev/null || true

if command -v syncsettings.elf >/dev/null 2>&1; then
    syncsettings.elf &
fi

"$BIN" > "$LOG_FILE" 2>&1
STATUS=$?

sync
exit "$STATUS"

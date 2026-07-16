#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

APP_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
BIN="$APP_DIR/bin/aarch64/cursed-drone-sdl"

export SDL_AUDIODRIVER=${SDL_AUDIODRIVER:-alsa}
export SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-kmsdrm}
export CURSED_DRONE_DATA_DIR=${CURSED_DRONE_DATA_DIR:-"$APP_DIR/data"}

if [ ! -x "$BIN" ]; then
    echo "Cursed Drone binary not found: $BIN" >&2
    exit 1
fi

exec "$BIN" "$@"

#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later

XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}

if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi

source "$controlfolder/control.txt"
[ -f "${controlfolder}/mod_${CFW_NAME}.txt" ] && source "${controlfolder}/mod_${CFW_NAME}.txt"
get_controls

GAMEDIR="/$directory/ports/curseddrone"
CONFDIR="$GAMEDIR/conf"
BIN="$GAMEDIR/cursed-drone-sdl.${DEVICE_ARCH}"
PROBE="$GAMEDIR/cursed-drone-probe.${DEVICE_ARCH}"

mkdir -p "$CONFDIR"
cd "$GAMEDIR" || exit 1

export XDG_CONFIG_HOME="$CONFDIR"
export XDG_DATA_HOME="$CONFDIR"
export CURSED_DRONE_DATA_DIR="$CONFDIR"
export CURSED_DRONE_HANDHELD=1
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
export LD_LIBRARY_PATH="$GAMEDIR/libs.${DEVICE_ARCH}:$LD_LIBRARY_PATH"

if [ ! -x "$BIN" ]; then
  echo "Cursed Drone binary is missing: $BIN" > "$CONFDIR/cursed-drone.log"
  exit 1
fi

if [ "${CURSED_DRONE_SKIP_PROBE:-0}" != "1" ] && [ ! -f "$CONFDIR/probe-v1.complete" ]; then
  "$PROBE" 2>&1 | tee "$CONFDIR/device-probe.log"
  probe_status=${PIPESTATUS[0]}
  if [ "$probe_status" -eq 0 ]; then
    touch "$CONFDIR/probe-v1.complete"
  else
    echo "Device probe failed with status $probe_status" >> "$CONFDIR/device-probe.log"
  fi
fi

$GPTOKEYB "cursed-drone-sdl.${DEVICE_ARCH}" &
pm_platform_helper "$BIN"
"$BIN" > "$CONFDIR/cursed-drone.log" 2>&1
app_status=$?
pm_finish
exit "$app_status"

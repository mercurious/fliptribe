#!/bin/sh
# fliptribe on the Retroid Pocket Flip 2 (ROCKNIX). Runs from wherever it is installed: portmart puts it in
# its apps root (e.g. /storage/games-external/fliptribe), flip2/install.sh in /storage/apps/fliptribe.
# Leave with the Guide (Home) button, or Shift(B) + Select.
[ -f /etc/profile ] && . /etc/profile >/dev/null 2>&1
APP=$(cd "$(dirname "$0")" && pwd)
LOG=$APP/logs/launch.log
mkdir -p "$APP/logs"
echo "===== $(date -u +%FT%TZ) launch" >> "$LOG"
export SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-wayland}
export SDL_AUDIODRIVER=${SDL_AUDIODRIVER:-pulseaudio}
export WAYLAND_DISPLAY=${WAYLAND_DISPLAY:-wayland-1}
export XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR:-/var/run/0-runtime-dir}
export SDL_GAMECONTROLLERCONFIG_FILE=${SDL_GAMECONTROLLERCONFIG_FILE:-/storage/.config/SDL-GameControllerDB/gamecontrollerdb.txt}
export FLIPTRIBE_DATA=$APP/data
export FLIPTRIBE_ASSETS=$APP/assets
export FLIPTRIBE_PATTERNS=$APP/patterns
cd "$APP"
exec "$APP/fliptribe" "$@" >> "$LOG" 2>&1

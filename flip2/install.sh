#!/usr/bin/env bash
# Installs dist/fliptribe onto a ROCKNIX handheld over ssh and writes the Ports entry.
#   flip2/install.sh [ssh-host]      default host: flip2-12g-usb   (TARGET=/storage/apps/fliptribe)
# The Ports entry appears in EmulationStation after a Ports gamelist refresh (or a reboot).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HOST=${1:-flip2-12g-usb}
TARGET=${TARGET:-/storage/apps/fliptribe}
[ -x "$ROOT/dist/fliptribe/fliptribe" ] || { echo "no build in dist/fliptribe; run build/build.sh first" >&2; exit 1; }
echo "installing to $HOST:$TARGET"
ssh "$HOST" "mkdir -p '$TARGET' /storage/roms/ports"
# keep the user's data dir (patterns they wrote) across installs
rsync -a --delete --exclude data --exclude logs "$ROOT/dist/fliptribe/" "$HOST:$TARGET/"
ssh "$HOST" "cat > '/storage/roms/ports/fliptribe.sh' <<'SH'
#!/bin/sh
# fliptribe: Korg Electribe EA-1 recreation. Written by fliptribe/flip2/install.sh.
exec /storage/apps/fliptribe/launch.sh \"\$@\"
SH
chmod +x /storage/roms/ports/fliptribe.sh '$TARGET/launch.sh' '$TARGET/fliptribe'"
echo "done: Ports > fliptribe"

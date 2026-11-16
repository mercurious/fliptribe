#!/usr/bin/env bash
# Packages dist/fliptribe (from build/build.sh) as the portmart release asset:
#   dist/fliptribe-aarch64.tar.gz + dist/fliptribe-aarch64.tar.gz.sha256
# The archive holds the app at its top level (fliptribe, launch.sh, assets/, patterns/, LICENSE, VERSION), the
# layout portmart unpacks into the app's home. data/ and logs/ are the player's: never packaged, kept on update.
#   build/package.sh <version>       e.g. 0.1.0-pre.1 (written to VERSION; the release tag is v<version>)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VERSION=${1:?usage: build/package.sh <version>}
SRC="$ROOT/dist/fliptribe"
[ -x "$SRC/fliptribe" ] && [ -x "$SRC/launch.sh" ] || { echo "no build in $SRC; run build/build.sh first" >&2; exit 1; }
[ ! -e "$SRC/data" ] && [ ! -e "$SRC/logs" ] || { echo "$SRC has data/ or logs/; rebuild it clean" >&2; exit 1; }
printf '%s\n' "$VERSION" >"$SRC/VERSION"
OUT="$ROOT/dist/fliptribe-aarch64.tar.gz"
tar -C "$SRC" --owner=0 --group=0 --sort=name -czf "$OUT" .
(cd "$ROOT/dist" && sha256sum fliptribe-aarch64.tar.gz >fliptribe-aarch64.tar.gz.sha256)
echo "packaged $OUT ($(du -k "$OUT" | cut -f1) KB, v$VERSION)"
cat "$OUT.sha256"

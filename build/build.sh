#!/usr/bin/env bash
# Builds fliptribe for an aarch64 ROCKNIX handheld inside an Ubuntu 24.04 container (glibc 2.39 < ROCKNIX 2.41).
# Needs podman (or ENGINE=docker). Output: dist/fliptribe/{fliptribe,assets,patterns,launch.sh}
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ENGINE=${ENGINE:-podman}; IMAGE=fliptribe-build
RUNAS=; [ "$ENGINE" = docker ] && RUNAS="--user $(id -u):$(id -g)"
$ENGINE image inspect $IMAGE >/dev/null 2>&1 || $ENGINE build -q -t $IMAGE -f "$ROOT/build/Containerfile" "$ROOT/build"
mkdir -p "$ROOT/build/work"
$ENGINE run --rm $RUNAS -v "$ROOT":/src:Z -w /src $IMAGE bash -c '
  set -e
  cmake -S . -B build/work -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null
  cmake --build build/work -j${JOBS:-4}
  strip build/work/fliptribe
'
OUT="$ROOT/dist/fliptribe"
rm -rf "$OUT"; mkdir -p "$OUT"
cp "$ROOT/build/work/fliptribe" "$OUT/"
cp -r "$ROOT/assets" "$ROOT/patterns" "$OUT/"
cp "$ROOT/flip2/launch.sh" "$OUT/"
cp "$ROOT/LICENSE" "$OUT/" 2>/dev/null || true
echo "built $OUT"
ls -la "$OUT"

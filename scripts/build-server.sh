#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="${WOWX_SERVER_BUILD_DIR:-$HOME/wowx-server-build}"
cmake -S "$root/upstream/vmangos" -B "$build_dir" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$root/server" \
  -DBUILD_EXTRACTORS=ON -DENABLE_CPPTRACE=OFF -DDEBUG_SYMBOLS=OFF -DUSE_PCH=ON
cmake --build "$build_dir" --parallel 6
cmake --install "$build_dir"

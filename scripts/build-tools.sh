#!/usr/bin/env bash
set -euo pipefail
export PATH=/mingw64/bin:/usr/bin:$PATH
cd "$(dirname "$0")/.."
cmake -S . -B build/host -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/mingw64/bin/gcc.exe \
    -DCMAKE_CXX_COMPILER=/mingw64/bin/g++.exe
cmake --build build/host --parallel 8

#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [[ "$(uname -s)" != Darwin ]]; then
    echo 'This script requires macOS and the Apple Xcode Command Line Tools.' >&2
    exit 1
fi
if ! /usr/bin/xcode-select -p >/dev/null 2>&1 || ! /usr/bin/xcrun --find clang++ >/dev/null 2>&1; then
    echo 'Install the Xcode Command Line Tools first: xcode-select --install' >&2
    echo 'When that installation finishes, run this script again.' >&2
    exit 1
fi

# Finder does not inherit the interactive shell PATH.
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
if command -v cmake >/dev/null 2>&1; then
    CMAKE="$(command -v cmake)"
else
    if ! command -v python3 >/dev/null 2>&1; then
        echo 'CMake is required. Install CMake 3.24 or newer from https://cmake.org/download/ and rerun.' >&2
        exit 1
    fi
    echo 'Installing a local copy of CMake in .tools; this does not modify system Python.'
    TOOL_ENV="$ROOT/.tools/cmake-venv"
    python3 -m venv "$TOOL_ENV"
    "$TOOL_ENV/bin/python" -m pip install --disable-pip-version-check 'cmake==3.31.6'
    CMAKE="$TOOL_ENV/bin/cmake"
fi

BUILD_DIR="${HLY_BUILD_DIR:-$ROOT/build-macos}"
JOBS="${CMAKE_BUILD_PARALLEL_LEVEL:-$(/usr/sbin/sysctl -n hw.physicalcpu)}"
if [[ "$JOBS" -gt 4 ]]; then JOBS=4; fi
export CMAKE_BUILD_PARALLEL_LEVEL="$JOBS"

"$CMAKE" -S "$ROOT" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DHLY_BUILD_PLUGIN=ON \
    -DHLY_BUILD_TESTS=ON \
    -DBUILD_TESTING=ON
"$CMAKE" --build "$BUILD_DIR" --config Release --parallel "$JOBS"
"$(dirname "$CMAKE")/ctest" --test-dir "$BUILD_DIR" -C Release --output-on-failure
"$ROOT/scripts/package-macos.sh" "$BUILD_DIR" "$ROOT/dist"
echo "Installer created in: $ROOT/dist"

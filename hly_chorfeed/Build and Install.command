#!/bin/bash
# Double-click this file on a Mac. It compiles locally, then opens Installer.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
finish() {
    result=$?
    if [[ "$result" -ne 0 ]]; then
        echo
        echo 'Build stopped. The error above explains what needs attention; no plugin was installed.'
    fi
    if [[ -t 0 ]]; then
        read -r -p 'Press Return to close this window. ' _ || true
    fi
    exit "$result"
}
trap finish EXIT
"$ROOT/scripts/build-macos.sh"
/usr/bin/open "$ROOT/dist/hly_chorfeed-0.1.0-macOS-universal.pkg"

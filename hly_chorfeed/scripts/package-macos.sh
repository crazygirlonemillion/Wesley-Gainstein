#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Run only after a successful real macOS build; this never fabricates binaries.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [[ "$(uname -s)" != Darwin ]]; then
    echo 'Packaging requires macOS (codesign, pkgbuild and productbuild).' >&2
    exit 1
fi
BUILD_DIR="${1:-$ROOT/build-macos}"
DEST="${2:-$ROOT/dist}"
VERSION=0.1.0
ARTEFACTS="$BUILD_DIR/HlyChorfeed_artefacts/Release"
VST3="$ARTEFACTS/VST3/hly_chorfeed.vst3"
AU="$ARTEFACTS/AU/hly_chorfeed.component"
for bundle in "$VST3" "$AU"; do
    if [[ ! -d "$bundle/Contents/MacOS" ]]; then
        echo "Missing compiled plugin bundle: $bundle" >&2
        exit 1
    fi
    binary="$bundle/Contents/MacOS/hly_chorfeed"
    if [[ ! -f "$binary" ]]; then
        echo "Missing plugin executable: $binary" >&2
        exit 1
    fi
    /usr/bin/lipo -verify_arch arm64 x86_64 "$binary"
done

mkdir -p "$DEST"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/hly-chorfeed-pkg.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
STAGE="$WORK/root"
mkdir -p "$STAGE/Library/Audio/Plug-Ins/VST3" "$STAGE/Library/Audio/Plug-Ins/Components"
/usr/bin/ditto "$VST3" "$STAGE/Library/Audio/Plug-Ins/VST3/hly_chorfeed.vst3"
/usr/bin/ditto "$AU" "$STAGE/Library/Audio/Plug-Ins/Components/hly_chorfeed.component"
for bundle in "$STAGE/Library/Audio/Plug-Ins/VST3/hly_chorfeed.vst3" "$STAGE/Library/Audio/Plug-Ins/Components/hly_chorfeed.component"; do
    /usr/bin/codesign --force --deep --sign - "$bundle"
    /usr/bin/codesign --verify --deep --strict "$bundle"
done

# Installer must use the declared system plugin locations, rather than locating
# an identically identified bundle in Downloads or in a previous build tree.
COMPONENTS="$WORK/components.plist"
/usr/bin/pkgbuild --analyze --root "$STAGE" "$COMPONENTS"
component_index=0
while /usr/libexec/PlistBuddy -c "Print :$component_index:RootRelativeBundlePath" "$COMPONENTS" >/dev/null 2>&1; do
    if ! /usr/libexec/PlistBuddy -c "Set :$component_index:BundleIsRelocatable false" "$COMPONENTS" 2>/dev/null; then
        /usr/libexec/PlistBuddy -c "Add :$component_index:BundleIsRelocatable bool false" "$COMPONENTS"
    fi
    if ! /usr/libexec/PlistBuddy -c "Set :$component_index:BundleHasStrictIdentifier true" "$COMPONENTS" 2>/dev/null; then
        /usr/libexec/PlistBuddy -c "Add :$component_index:BundleHasStrictIdentifier bool true" "$COMPONENTS"
    fi
    component_index=$((component_index + 1))
done
if [[ "$component_index" -ne 2 ]]; then
    echo "Expected exactly two plugin bundles in the installer; found $component_index." >&2
    exit 1
fi
/usr/bin/pkgbuild \
    --root "$STAGE" \
    --component-plist "$COMPONENTS" \
    --identifier audio.hly.chorfeed.plugins \
    --version "$VERSION" \
    --install-location / \
    --ownership recommended \
    "$WORK/hly_chorfeed-plugins.pkg"

mkdir -p "$WORK/resources"
cat "$ROOT/LICENSE.txt" "$ROOT/THIRD_PARTY.md" > "$WORK/resources/License.txt"
# The combined binary also contains JUCE's AGPL-licensed modules. Retain the
# framework's own license notice in the installer, alongside original-source GPL.
JUCE_NOTICE="$BUILD_DIR/_deps/juce-src/LICENSE.md"
if [[ -f "$JUCE_NOTICE" ]]; then
    cat "$JUCE_NOTICE" >> "$WORK/resources/License.txt"
fi
cat > "$WORK/resources/Welcome.txt" <<'WELCOME'
hly_chorfeed 0.1.0
Stereo feedback chorus, delay, filter and distortion.

This installer adds universal Apple Silicon / Intel VST3 and Audio Unit plugins to:
/Library/Audio/Plug-Ins/VST3
/Library/Audio/Plug-Ins/Components

macOS 11 or later is required. Quit your DAW before installing, then reopen it and rescan plugins if needed.

These personal-build plugins have ad-hoc signatures. This installer is unsigned and not Apple notarized.
WELCOME
cat > "$WORK/Distribution.xml" <<'XML'
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>hly_chorfeed</title>
    <welcome file="Welcome.txt"/>
    <license file="License.txt"/>
    <options customize="never" require-scripts="false" rootVolumeOnly="true"/>
    <allowed-os-versions><os-version min="11.0"/></allowed-os-versions>
    <choices-outline><line choice="plugins"/></choices-outline>
    <choice id="plugins" visible="false" title="hly_chorfeed VST3 and Audio Unit">
        <pkg-ref id="audio.hly.chorfeed.plugins"/>
    </choice>
    <pkg-ref id="audio.hly.chorfeed.plugins" version="0.1.0">hly_chorfeed-plugins.pkg</pkg-ref>
</installer-gui-script>
XML
/usr/bin/productbuild \
    --distribution "$WORK/Distribution.xml" \
    --resources "$WORK/resources" \
    --package-path "$WORK" \
    "$DEST/hly_chorfeed-$VERSION-macOS-universal.pkg"
/usr/sbin/pkgutil --check-signature "$DEST/hly_chorfeed-$VERSION-macOS-universal.pkg" || true

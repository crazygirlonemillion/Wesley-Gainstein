# Dependencies and notices

## Original hly_chorfeed source

The original source in `Source/`, `tests/` and the build/packaging scripts is offered under the GNU General Public License, version 3 or (at your option) any later version. See `LICENSE.txt`. This does not relicense external dependencies.

## JUCE

The CMake build fetches a pinned JUCE release from https://github.com/juce-framework/JUCE. The root `CMakeLists.txt` is the authority for the selected revision.

JUCE 8 modules are dual licensed under the GNU Affero General Public License version 3 and the commercial JUCE licence. This project uses the open-source option. JUCE's own `LICENSE.md`, module notices, third-party component notices and license texts remain in its fetched source. GPLv3 section 13 permits combination with AGPLv3 material; JUCE's AGPL provisions remain applicable to that part of the combined work. Distribution of binaries must include an appropriate corresponding-source offer or source supply under those terms.

JUCE may include third-party code for the VST3 SDK, font rendering, image handling and other platform facilities. Preserve its associated notices. A downloaded JUCE dependency is normally under `build-macos/_deps/juce-src`; an externally supplied JUCE checkout may live elsewhere.

## DSP and visual assets

This version implements its own DSP. Surge SST, ChowDSP and Signalsmith were considered during design but are not bundled or linked in this version.

The interface is drawn in code. It does not bundle Image-Line Morphine graphics, a Morphine font, audio samples or other extracted plugin assets. Morphine and Image-Line are names of their respective owners. hly_chorfeed is an independent personal project.

## Optional build tool

When no CMake executable is installed, the Mac build script uses a local Python virtual environment to install CMake 3.31.6 from PyPI. CMake is a separate build-time program and is not linked into the plugin. Its own distributions contain their license notices.

## Apple tools

Apple's Xcode Command Line Tools, SDKs, codesign, pkgbuild and productbuild are prerequisites on the build Mac. They are not redistributed in this kit. The installer produced by these scripts is unsigned; the plugin bundles are signed ad hoc for local execution, not with an Apple Developer identity.

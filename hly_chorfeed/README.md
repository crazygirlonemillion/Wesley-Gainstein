# hly_chorfeed 0.1.0

A stereo feedback chorus, delay, filter and distortion effect with a signed 3 × 3 routing matrix. The interface uses original programmatically drawn controls inspired by the charcoal, slate-blue and amber appearance of the legacy Morphine interface. No Morphine code, images or fonts are bundled. There is no pitch tracking.

**This download is a source/build kit, not a precompiled macOS plugin or installer.** A real macOS build is required to produce the universal VST3, Audio Unit and `.pkg`. The supplied build and packaging scripts do that on a Mac; a GitHub Actions workflow is included for a hosted Mac runner. No Apple Developer signing identity or notarization service is configured.

## Build and install on a Mac

Requires macOS 11 or later, internet for the first build, and the Xcode Command Line Tools. Intel and Apple Silicon are supported in the same universal binary. The build downloads the pinned JUCE dependency; if CMake is missing, it installs a pinned CMake into a project-local Python virtual environment.

1. Unzip this folder into a location you can write to, such as Documents.
2. If you have not installed Apple's tools, open Terminal and run `xcode-select --install`. Finish that installation first.
3. Double-click **Build and Install.command**. If macOS requires explicit permission to launch a downloaded command file, review the file and use macOS's Open/Privacy & Security controls for this file. Do not disable Gatekeeper globally.
4. The command compiles both plugin formats, runs the tests, creates `dist/hly_chorfeed-0.1.0-macOS-universal.pkg`, then opens Apple's Installer. Follow its prompts to install. The package requires administrator authorization because it installs system-wide plugins.
5. Reopen your DAW and rescan plugins if necessary. In Logic, select the Audio Unit; in VST3 hosts, select the VST3.

If Python cannot create the local CMake environment, install CMake 3.24 or newer from [cmake.org](https://cmake.org/download/) or your existing package manager and run again. In Terminal, the equivalent build command is:

```sh
bash scripts/build-macos.sh
```

The build command only opens Installer through the double-click launcher. It does not silently install anything. The `.pkg` and plugins use no Developer ID; plugin bundles are ad-hoc signed locally. The installer is unsigned and not Apple notarized. If macOS blocks it, review it and use the specific Open Anyway option in Privacy & Security. Hosts with stricter signing requirements may need a Developer ID signed and notarized release, which is not included here.

Installed locations:

- `/Library/Audio/Plug-Ins/VST3/hly_chorfeed.vst3`
- `/Library/Audio/Plug-Ins/Components/hly_chorfeed.component`

Remove those two bundles to uninstall. Install only the formats your host supports; the current simple installer includes both.

After installation, you can validate the Audio Unit manually from Terminal:

```sh
auval -v aufx Hlcf Hlyy
```

That validation has not been run in the Linux development environment. A successful AU validation is useful in addition to checking playback, automation, state recall and feedback tails inside your actual DAW.

## Routing and controls

Each processor receives its **INPUT SEND** amount of the original input plus the sum of matrix routes into that processor. **OUTPUT MIX** sets how much of each processor reaches the wet bus. The global **MIX** blends original input with that wet bus. These sends are independent of the matrix.

Read the feedback matrix **from rows to columns**: row DELAY, column FILTER sends the delay's output into the filter. Diagonal cells feed a processor back into itself. Every route is bipolar: negative values invert polarity; zero disconnects the route. The global **FEEDBACK** control scales all nine matrix routes together. The network uses one sample of history at its matrix connections, so host buffer size does not change the loop's fundamental delay.

| Section | Modes | Controls |
| --- | --- | --- |
| Filter | Smooth LP, Smooth HP, Smooth BP, Driven Ladder | Cutoff 20 Hz–20 kHz, resonance, drive |
| Delay | Ensemble, Digital, Dark | Time 1–2000 ms, rate, modulation depth, stereo width |
| Distortion | Soft Saturation, Hard Clip, Asymmetric, Wavefold | Drive, tone, bias |
| Matrix | Nine signed routes | −1 to +1 per route |
| Input/output | Three input sends and three wet output levels | Filter, delay, distortion |
| Global | Feedback, mix, output | Freeze and panic |

The default **Lush / Slow Bloom** preset starts with short modulated delays and restrained feedback. Longer times create echoes; short times and polarity changes create comb-filter and metallic textures. The filter and distortion can be routed inside repeating paths so their character accumulates on each pass. Factory presets can be selected from the header; DAW automation and plugin state recall are supported.

**FREEZE** reduces new input entering the network; it does not guarantee perfectly lossless, infinite sustain. Existing tails evolve according to the filter, distortion and matrix settings. **PANIC** clears the network's stored audio and processing state. Start with modest monitor levels when exploring multiple high-gain loops: controlled saturation and DC removal bound the internal signals, but feedback can intentionally sustain or change level rapidly.

The filter uses a topology-preserving state-variable design for LP/HP/BP and a driven four-stage ladder option. Ensemble uses three modulated delay voices with cubic interpolation. The Dark delay adds filtering and saturation. Distortion uses 4× interpolation and reconstruction filtering, followed by its tone filter; this reduces aliasing but does not make every extreme setting alias-free. Smooth parameter changes avoid abrupt routing jumps.

Pitch-follow controls are intentionally absent. Polyphonic and monophonic sources use the same manually controlled signal path.

## Build with a hosted Mac

The included `.github/workflows/macos.yml` can build on GitHub's `macos-14` runner. Add the project to a repository you control, then run **Build macOS installer** from its Actions tab. The run uploads an unsigned universal `.pkg` artifact. This kit does not create a repository, start a remote build, or publish a release automatically.

## Source and licensing

Original hly_chorfeed source is supplied under **GPL-3.0-or-later**; see `LICENSE.txt`. JUCE 8 is dual licensed under **AGPLv3/commercial**; this build uses its open-source terms, so the combined application must also respect JUCE's AGPL conditions. The fetched JUCE source retains its own notices and third-party licenses. See `THIRD_PARTY.md`.

You do not need to publish your private local changes for personal use. If distributing compiled builds, provide the corresponding source, build scripts and dependency revisions, and comply with the included licenses. This implementation currently uses original DSP rather than the Surge or ChowDSP libraries discussed during design.

## Verification status

See [TESTING.md](TESTING.md) for checks actually run in the development environment. A Linux DSP test pass or a successful script syntax check does **not** establish that the macOS wrapper, universal binary, Apple installer, DAW scanning or Audio Unit validation has passed. Those require a real Mac build and host testing. The `.pkg` script explicitly checks for both compiled macOS bundles and their Intel/Apple Silicon architectures before packaging.

# Verification — 0.1.0

Checked on Linux x86_64 with GCC 14 and pinned JUCE 8.0.6:

- Full JUCE shared code, VST3 wrapper, processor test executable and interface capture tool compiled and linked. Linux verification used the Release configuration with `-O1`.
- JUCE's VST3 helper loaded the built module and generated its module metadata.
- Both CTest executables passed. The DSP executable checks 11 cases: buffer-size independence, dry mix, silence/reset, feedback tails and stereo, zero width, matrix direction, extreme feedback at multiple sample rates, filter modes, distortion modes, Freeze, and non-finite inputs.
- Processor tests check parameter/state recall, all six factory presets, mono and stereo layouts, blocks larger than the advertised size, and Panic.
- DSP tests also passed AddressSanitizer and UndefinedBehaviorSanitizer. LeakSanitizer was disabled because the sandbox's process tracing prevents its use.
- The actual editor was rendered to `preview.png` using the compiled JUCE code. This is a Linux capture, not a macOS DAW screenshot.
- Two stereo 48 kHz audio examples were rendered from the engine. Each contains six seconds dry, six seconds processed, and a three-second tail. The dub example uses a separate illustrative routing.
- Mac shell scripts passed `bash -n`; the installer distribution XML parsed successfully. The packaging script refuses to run on Linux and requires actual universal Mac binaries.

**Not checked:** macOS compilation, Apple Silicon/Intel execution, Audio Unit validation, installation, DAW scanning/automation in a real host, Developer ID signing or notarization. No precompiled macOS plugin or `.pkg` is included in this source kit.

On a Mac, run `Build and Install.command` to build and test the project and generate the installer. After installing, Audio Unit validation can be run manually:

```sh
auval -v aufx Hlcf Hlyy
```

The GitHub workflow is a build recipe; no hosted Mac build has been run for this delivery.

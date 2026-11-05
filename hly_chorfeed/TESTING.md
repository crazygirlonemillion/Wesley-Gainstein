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

**Not checked:** execution on Intel hardware, manual scanning/automation in individual DAWs, installer execution on a user's Mac, Developer ID signing or notarization.

On a Mac, run `Build and Install.command` to build and test the project and generate the installer. After installing, Audio Unit validation can be run manually:

```sh
auval -v aufx Hlcf Hlyy
```

## GitHub macOS verification

[Successful build and validation](https://github.com/crazygirlonemillion/Wesley-Gainstein/actions/runs/42123470953), 2026-11-05, commit df3ce4ca0818b2cdb9d8781233d380d6a71145ad:

- macOS 26.6.2 ARM64 runner, AppleClang 21.
- Universal ARM64 and x86_64 VST3 and AU compiled; both CTest suites passed.
- Both binary architectures verified; staged plugin signatures verified.
- Real unsigned .pkg created with pkgbuild/productbuild.
- Native Audio Unit validation passed after refreshing AudioComponentRegistrar.
- [Installer artifact uploaded](https://github.com/crazygirlonemillion/Wesley-Gainstein/actions/runs/42123470953/artifacts/13460365857).

The x86_64 slices compiled successfully; Intel execution was not tested. The local preview remains a Linux capture.

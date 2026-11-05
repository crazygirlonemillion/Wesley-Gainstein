# HardKick

A hardstyle kick generator VST3 instrument for macOS (Apple Silicon + Intel).

**Download:** [HardKick-Installer.pkg](../../releases/latest/download/HardKick-Installer.pkg) (double-click installer) · [HardKick-mac.zip](../../releases/latest/download/HardKick-mac.zip) (manual)

## Install

1. Double-click **HardKick-Installer.pkg**. It installs to `/Library/Audio/Plug-Ins/VST3/`.
   - The installer isn't signed by Apple, so the first time macOS may refuse to open it. Go to **System Settings → Privacy & Security**, scroll down, click **Open Anyway**, then run it again.
2. FL Studio: **Options → Manage plugins → Find more plugins**, then add HardKick to a channel as an instrument.

Manual install (zip): move `HardKick.vst3` into `~/Library/Audio/Plug-Ins/VST3/`, then run
`xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/HardKick.vst3` once in Terminal.

## How it works

Each kick is built from three layers that share one pitch envelope, so they stay in phase:

- **Click** – a short filtered-noise + tone transient.
- **Punch** – a sine that sweeps down from *Start Pitch* to the tail pitch over *Sweep* time (*Curve* sets how fast it drops), with its own drive and decay.
- **Tail** – the oscillator (*Shape*: sine → triangle → saw) through two distortion stages (Soft, Hard, Fold, Crunch), a resonant low-pass with its own envelope, and a fade-in / hold / release envelope. *Pitch Drop* bends the tail down over its length.

**Pitch:** with *Key Track* on, the note you play sets the tail pitch. With *Octave* at −3, playing F5 in FL Studio gives an F1 (~43.7 Hz) tail. With *Key Track* off, every note uses *Fixed Pitch*.

**Master:** low cut, high cut, a soft clipper (*Clip*), velocity sensitivity and output level.

The kick is one-shot (note length doesn't matter) and the engine runs 4× oversampled at 44.1/48 kHz to keep the distortion clean.

Click the waveform or **PLAY** to audition. The cyan line shows the pitch envelope.

## Building

Every push to `main` builds the plugin on GitHub's Mac runners and publishes a release. To build locally on a Mac with Xcode command-line tools and CMake:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release
```

The headless engine test lives in `tests/render_test.cpp`.

Built with [JUCE](https://juce.com) (used under the AGPLv3; a JUCE commercial licence is needed to sell closed-source builds).

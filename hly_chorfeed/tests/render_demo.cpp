#include "../Source/DspEngine.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Renders identical musical input dry for six seconds, then with the default
// Lush / Slow Bloom engine for six seconds, followed by a three-second wet tail.
// Build: c++ -std=c++17 -O2 tests/render_demo.cpp -o render_demo
// Run: ./render_demo /path/to/hly_chorfeed-audio-demo.wav
namespace
{
void u16 (std::ofstream& out, std::uint16_t value)
{
    const char bytes[2] {static_cast<char> (value & 255), static_cast<char> ((value >> 8) & 255)};
    out.write (bytes, 2);
}
void u32 (std::ofstream& out, std::uint32_t value)
{
    u16 (out, static_cast<std::uint16_t> (value & 65535));
    u16 (out, static_cast<std::uint16_t> (value >> 16));
}
}
int main (int argc, char** argv)
{
    constexpr int rate = 48000, musicalLength = rate * 6, wetLength = rate * 9;
    std::vector<float> dry (musicalLength), left (wetLength), right (wetLength);
    constexpr double pi = 3.14159265358979323846;
    constexpr double notes[] {146.832384, 220.0, 293.664768, 349.228232, 440.0, 587.329536};
    for (int frame = 0; frame < musicalLength; ++frame)
    {
        double sample = 0;
        const double time = static_cast<double> (frame) / rate;
        for (int voice = 0; voice < 6; ++voice)
        {
            const double start = voice * 0.085 + std::floor (time / 1.5) * 1.5;
            const double elapsed = time - start;
            if (elapsed >= 0)
            {
                const double envelope = (1.0 - std::exp (-elapsed * 200.0)) * std::exp (-elapsed * 2.2);
                for (int harmonic = 1; harmonic <= 8; ++harmonic)
                    sample += envelope * std::sin (2.0 * pi * notes[voice] * harmonic * elapsed) / (harmonic * harmonic);
            }
        }
        dry[static_cast<std::size_t> (frame)] = static_cast<float> (sample * 0.11);
    }
    std::copy (dry.begin (), dry.end (), left.begin ()); right = left;
    hly::Params params;
    const bool dub = argc > 2 && std::string (argv[2]) == "dub";
    if (dub)
    {
        params.delayMode = 2; params.timeMs = 340; params.depthMs = 0.6f;
        params.rateHz = 0.2f; params.cutoff = 2400; params.resonance = 0.35f;
        params.driveDb = 9; params.filterDriveDb = 2; params.tone = 0.50f;
        params.feedback = 0.7f; params.mix = 0.7f; params.outputDb = -3;
        for (auto& row : params.matrix) for (auto& value : row) value = 0;
        params.matrix[1][2] = 0.85f; params.matrix[2][0] = 0.85f; params.matrix[0][1] = 0.90f;
        params.output[0] = 0.10f; params.output[1] = 0.95f;
    }
    hly::Engine engine; engine.setParams (params); engine.prepare (rate);
    const auto before = std::chrono::steady_clock::now ();
    for (int offset = 0; offset < wetLength; offset += 256)
        engine.process (left.data () + offset, right.data () + offset, std::min (256, wetLength - offset));
    const double seconds = std::chrono::duration<double> (std::chrono::steady_clock::now () - before).count ();
    const std::uint32_t bytes = (musicalLength + wetLength) * 2 * 2;
    std::ofstream out (argc > 1 ? argv[1] : "hly_chorfeed-audio-demo.wav", std::ios::binary);
    if (! out) { std::cerr << "Cannot create output WAV\n"; return 1; }
    out.write ("RIFF", 4); u32 (out, 36 + bytes); out.write ("WAVEfmt ", 8); u32 (out, 16);
    u16 (out, 1); u16 (out, 2); u32 (out, rate); u32 (out, rate * 4); u16 (out, 4); u16 (out, 16);
    out.write ("data", 4); u32 (out, bytes);
    const auto sample = [&out] (float value)
    {
        const auto pcm = static_cast<std::int16_t> (std::clamp (value, -1.0f, 1.0f) * 32767.0f);
        u16 (out, static_cast<std::uint16_t> (pcm));
    };
    for (const auto value : dry) { sample (value); sample (value); }
    for (int frame = 0; frame < wetLength; ++frame) { sample (left[static_cast<std::size_t> (frame)]); sample (right[static_cast<std::size_t> (frame)]); }
    std::cout << "Rendered 15s 48kHz stereo PCM: 0-6s dry, 6-12s " << (dub ? "dirty dub" : "lush") << ", 12-15s tail\n"
              << "Processed 9s of stereo in " << seconds << "s (" << (9.0 / seconds) << "x realtime)\n";
    return out ? 0 : 1;
}

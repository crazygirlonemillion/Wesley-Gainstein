#include "../Source/DspEngine.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

namespace
{
void require (bool result, const char* message)
{
    if (! result) throw std::runtime_error (message);
}
double energy (const std::vector<float>& audio)
{
    double sum = 0;
    for (auto x : audio) sum += x * x;
    return sum;
}
std::vector<float> signal (int length, double sr)
{
    std::vector<float> result (static_cast<std::size_t> (length));
    for (int i = 0; i < length; ++i)
        result[static_cast<std::size_t> (i)] = static_cast<float> (0.2 * std::sin (6.283185307179586 * 220.0 * i / sr)
                                                        + 0.06 * std::sin (6.283185307179586 * 1700.0 * i / sr));
    return result;
}
void blockIndependence ()
{
    auto a = signal (48000, 48000), b = a, c = a, d = a;
    hly::Params p;
    hly::Engine one, split;
    one.setParams (p); split.setParams (p);
    one.prepare (48000); split.prepare (48000);
    one.process (a.data (), b.data (), static_cast<int> (a.size ()));
    const int pattern[] {1, 7, 13, 64, 257, 511};
    int offset = 0, counter = 0;
    while (offset < static_cast<int> (c.size ()))
    {
        const auto count = std::min (pattern[counter++ % 6], static_cast<int> (c.size ()) - offset);
        split.setParams (p);
        split.process (c.data () + offset, d.data () + offset, count);
        offset += count;
    }
    require (a == c && b == d, "routing or smoothing depends on host block size");
}
void dryBypass ()
{
    hly::Params p; p.mix = 0.0f;
    hly::Engine e; e.setParams (p); e.prepare (48000);
    auto l = signal (4096, 48000), r = l, original = l;
    e.process (l.data (), r.data (), static_cast<int> (l.size ()));
    require (l == original && r == original, "mix=0 is not exact dry at unity output");
}
void initialSilenceAndReset ()
{
    hly::Engine e; e.prepare (48000);
    std::vector<float> l (12000), r (12000);
    e.process (l.data (), r.data (), 12000);
    require (energy (l) == 0 && energy (r) == 0, "unexcited network produces audio");
    l[0] = r[0] = 0.5f;
    e.process (l.data (), r.data (), 12000);
    require (energy (l) > 0, "impulse produces no output");
    e.reset ();
    std::fill (l.begin (), l.end (), 0.0f); std::fill (r.begin (), r.end (), 0.0f);
    e.process (l.data (), r.data (), 12000);
    require (energy (l) == 0 && energy (r) == 0, "panic/reset does not clear all audio state");
}
void tailAndStereo ()
{
    hly::Params p; p.mix = 1; p.width = 1; p.depthMs = 5; p.matrix[1][1] = 0.8f;
    hly::Engine e; e.setParams (p); e.prepare (48000);
    auto l = signal (48000, 48000), r = l;
    e.process (l.data (), r.data (), 48000);
    double difference = 0;
    for (std::size_t i = 0; i < l.size (); ++i) difference += std::abs (l[i] - r[i]);
    require (difference > 5, "ensemble has no stereo modulation");
    std::fill (l.begin (), l.end (), 0.0f); std::fill (r.begin (), r.end (), 0.0f);
    e.process (l.data (), r.data (), 48000);
    require (energy (l) > 0.001, "feedback network has no tail");
}
void monoCompatibleWidth ()
{
    hly::Params p; p.mix = 1; p.width = 0;
    hly::Engine e; e.setParams (p); e.prepare (48000);
    auto l = signal (24000, 48000), r = l;
    e.process (l.data (), r.data (), 24000);
    require (l == r, "width zero does not preserve dual mono");
}
void routingDirection ()
{
    double measured[2] {};
    for (int reverse = 0; reverse < 2; ++reverse)
    {
        hly::Params p; p.mix = 1; p.feedback = 0.75f;
        p.input[0] = p.input[2] = 0; p.input[1] = 1;
        p.output[0] = 1; p.output[1] = p.output[2] = 0;
        for (auto& row : p.matrix) for (auto& v : row) v = 0;
        p.matrix[reverse == 0 ? 1 : 0][reverse == 0 ? 0 : 1] = 0.75f;
        hly::Engine e; e.setParams (p); e.prepare (48000);
        auto l = signal (6000, 48000), r = l;
        e.process (l.data (), r.data (), 6000);
        measured[reverse] = energy (l);
    }
    require (measured[0] > 0.01 && measured[1] == 0, "matrix FROM/TO directions do not match API");
}
void extremeStability ()
{
    std::mt19937 generator (1234);
    std::uniform_real_distribution<float> random (-1.0f, 1.0f);
    for (double rate : {22050.0, 44100.0, 48000.0, 96000.0, 192000.0})
    {
        hly::Engine e; e.prepare (rate);
        std::vector<float> l (513), r (513);
        hly::Params p; p.feedback = 1.5f; p.resonance = 1; p.driveDb = 36; p.filterDriveDb = 30; p.mix = 1;
        for (int node = 0; node < 3; ++node) { p.input[node] = p.output[node] = 1; }
        for (int block = 0; block < 70; ++block)
        {
            p.filterMode = block % 4; p.delayMode = block % 3; p.distortionMode = block % 4;
            p.cutoff = block % 2 == 0 ? 20.0f : static_cast<float> (rate * 0.45);
            p.timeMs = block % 3 == 0 ? 1.0f : 2000.0f;
            p.depthMs = 30; p.rateHz = 20; p.bias = random (generator);
            for (auto& row : p.matrix) for (float& v : row) v = random (generator);
            e.setParams (p);
            for (std::size_t n = 0; n < l.size (); ++n) { l[n] = 2 * random (generator); r[n] = 2 * random (generator); }
            e.process (l.data (), r.data (), static_cast<int> (l.size ()));
            for (auto channel : {&l, &r}) for (auto v : *channel)
                require (std::isfinite (v) && std::abs (v) <= 4.01f, "nonfinite or runaway output at extreme controls");
        }
    }
}
void filterModes ()
{
    std::vector<double> energies;
    for (int mode = 0; mode < 4; ++mode)
    {
        hly::Params p; p.mix = 1; p.filterMode = mode; p.cutoff = 800;
        p.input[0] = p.output[0] = 1; p.input[1] = p.output[1] = 0;
        for (auto& row : p.matrix) for (auto& v : row) v = 0;
        hly::Engine e; e.setParams (p); e.prepare (48000);
        auto l = signal (12000, 48000), r = l;
        e.process (l.data (), r.data (), 12000);
        energies.push_back (energy (l));
    }
    for (std::size_t i = 0; i < energies.size (); ++i)
        for (std::size_t j = i + 1; j < energies.size (); ++j)
            require (std::abs (energies[i] - energies[j]) > 0.01, "filter models are indistinguishable");
}
void distortionModes ()
{
    std::vector<std::vector<float>> results;
    for (int mode = 0; mode < 4; ++mode)
    {
        hly::Params p; p.mix = 1; p.distortionMode = mode; p.driveDb = 24; p.bias = 0.1f;
        p.input[2] = p.output[2] = 1; p.input[1] = p.output[1] = 0; p.output[0] = 0;
        for (auto& row : p.matrix) for (auto& v : row) v = 0;
        hly::Engine e; e.setParams (p); e.prepare (48000);
        auto l = signal (12000, 48000), r = l;
        e.process (l.data (), r.data (), 12000);
        results.push_back (l);
    }
    for (std::size_t i = 0; i < results.size (); ++i)
        for (std::size_t j = i + 1; j < results.size (); ++j)
        {
            double difference = 0;
            for (std::size_t n = 0; n < results[i].size (); ++n) difference += std::abs (results[i][n] - results[j][n]);
            require (difference > 5, "distortion models are indistinguishable");
        }
}
void freezeExcludesNewInput ()
{
    hly::Params p; p.freeze = true; p.mix = 1;
    hly::Engine e; e.setParams (p); e.prepare (48000);
    auto l = signal (12000, 48000), r = l;
    e.process (l.data (), r.data (), 12000);
    require (energy (l) == 0 && energy (r) == 0, "freeze injects fresh input");
}
void hostileInputs ()
{
    hly::Params p; p.cutoff = std::numeric_limits<float>::quiet_NaN (); p.feedback = std::numeric_limits<float>::infinity (); p.outputDb = -std::numeric_limits<float>::infinity ();
    hly::Engine e; e.setParams (p); e.prepare (48000);
    std::vector<float> l (4096, std::numeric_limits<float>::quiet_NaN ()), r (4096, std::numeric_limits<float>::infinity ());
    e.process (l.data (), r.data (), 4096);
    for (std::size_t n = 0; n < l.size (); ++n) require (std::isfinite (l[n]) && std::isfinite (r[n]), "nonfinite host input propagated");
}
}

int main ()
{
    try
    {
        const std::pair<const char*, void (*) ()> tests[] {
            {"block independence", blockIndependence}, {"dry mix", dryBypass}, {"silence and reset", initialSilenceAndReset},
            {"feedback tail and stereo", tailAndStereo}, {"width zero", monoCompatibleWidth}, {"routing direction", routingDirection}, {"extreme stability", extremeStability},
            {"filter models", filterModes}, {"distortion models", distortionModes}, {"freeze", freezeExcludesNewInput}, {"nonfinite inputs", hostileInputs}
        };
        for (const auto& test : tests) { test.second (); std::cout << "PASS " << test.first << '\n'; }
    }
    catch (const std::exception& error) { std::cerr << "FAIL " << error.what () << '\n'; return EXIT_FAILURE; }
    return EXIT_SUCCESS;
}

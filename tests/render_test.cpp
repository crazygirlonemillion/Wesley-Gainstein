// Headless test: renders kicks with the engine, writes WAVs and prints sanity stats.
// Build: g++ -O2 -std=c++17 -I../Source render_test.cpp -o render_test
#include "KickEngine.h"
#include "Presets.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

static void writeWav (const std::string& path, const std::vector<float>& x, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    if (! f) return;
    auto w32 = [f] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto w16 = [f] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    const uint32_t dataBytes = (uint32_t) x.size() * 2;
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + dataBytes); std::fwrite ("WAVE", 1, 4, f);
    std::fwrite ("fmt ", 1, 4, f); w32 (16); w16 (1); w16 (1); w32 (sr); w32 (sr * 2); w16 (2); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (dataBytes);
    for (float s : x) { int v = (int) std::lround (std::max (-1.f, std::min (1.f, s)) * 32767.f); w16 ((uint16_t) (int16_t) v); }
    std::fclose (f);
}

// Estimates frequency in a window via zero crossings of a low-passed copy.
static double zcFreq (const std::vector<float>& x, int sr, double t0, double t1)
{
    int a = (int) (t0 * sr), b = std::min ((int) x.size(), (int) (t1 * sr));
    if (b - a < 10) return 0;
    double lp = 0, prev = 0; int crossings = 0; int first = -1, last = -1;
    const double c = 1.0 - std::exp (-2.0 * hk::kPi * 300.0 / sr);
    for (int i = a; i < b; ++i)
    {
        lp += c * (x[(size_t) i] - lp);
        if (prev < 0 && lp >= 0) { if (first < 0) first = i; last = i; ++crossings; }
        prev = lp;
    }
    if (crossings < 2) return 0;
    return (crossings - 1) * (double) sr / (last - first);
}

static int run (const char* name, const hk::KickParams& p, int sr, int note, bool write)
{
    hk::KickEngine e;
    e.prepare (sr);
    const double len = e.estimatedLengthSec (p) + 0.1;
    std::vector<float> out ((size_t) (len * sr), 0.f);
    e.noteOn (note, 1.0f);
    // render in odd-sized blocks like a host would
    size_t pos = 0; int blk = 0; const int sizes[] = { 512, 37, 256, 1, 480 };
    while (pos < out.size())
    {
        int nb = (int) std::min<size_t> ((size_t) sizes[blk++ % 5], out.size() - pos);
        e.render (out.data() + pos, nb, p);
        pos += (size_t) nb;
    }

    int bad = 0; double peak = 0, sumSq = 0, dc = 0;
    for (float s : out) { if (! std::isfinite (s)) ++bad; peak = std::max (peak, (double) std::abs (s)); sumSq += s * s; dc += s; }
    dc /= (double) out.size();

    // stays silent after voice ends?
    const bool ended = ! e.isPlaying();
    double tailHz = zcFreq (out, sr, (p.tailStart + p.tailAttack) * 0.001 + 0.03,
                            (p.tailStart + p.tailAttack + p.tailLength) * 0.001);
    hk::Derived d; d.update (p, sr);
    std::printf ("%-18s sr=%d peak=%.3f (%.1f dBFS) rms=%.3f dc=%.4f nan=%d ended=%d len=%.2fs tailHz~%.1f (expect %.1f)\n",
                 name, sr, peak, 20 * std::log10 (peak + 1e-12), std::sqrt (sumSq / out.size()), dc, bad, ended ? 1 : 0,
                 len, tailHz, d.tailFreqForNote (note) * std::exp2 (-p.pitchDrop * 0.3 / 12.0));
    if (write) writeWav (std::string ("out_") + name + ".wav", out, sr);
    return bad;
}

int main()
{
    int bad = 0;
    hk::KickParams def;
    bad += run ("default", def, 48000, 65, true);
    bad += run ("default_44k", def, 44100, 65, false);
    bad += run ("default_96k", def, 96000, 65, false);

    for (int i = 0; i < hk::numPresets(); ++i)
    {
        auto p = hk::presetParams (i);
        std::string nm = hk::presetName (i);
        for (auto& c : nm) if (c == ' ') c = '_';
        bad += run (nm.c_str(), p, 48000, 65, true);
    }

    // extreme settings
    hk::KickParams ex = def;
    ex.drive1 = 48; ex.dist1Type = hk::DistFold; ex.drive2 = 36; ex.dist2Type = hk::DistCrunch;
    ex.resonance = 1.0f; ex.filterEnvAmt = 6; ex.masterDrive = 24; ex.outputGain = 6; ex.tailShape = 1;
    ex.sweepTime = 5; ex.startPitch = 6000; ex.clickDecay = 50; ex.clickLevel = 1;
    bad += run ("extreme", ex, 44100, 65, true);

    hk::KickParams lo = def; lo.keyTrack = true; lo.octave = -5; lo.tailLength = 1500; lo.tailRelease = 1500;
    bad += run ("very_low", lo, 48000, 24, false);

    // fast retrigger
    {
        hk::KickEngine e; e.prepare (48000);
        std::vector<float> out (48000);
        for (int i = 0; i < 48000; i += 1000) { e.noteOn (65, 1.f); e.render (out.data() + i, 1000, def); }
        int nb = 0; for (float s : out) if (! std::isfinite (s)) ++nb;
        std::printf ("retrigger nan=%d\n", nb); bad += nb;
        writeWav ("out_retrigger.wav", out, 48000);
    }

    // CPU estimate
    {
        hk::KickEngine e; e.prepare (48000);
        std::vector<float> out (48000 * 10);
        auto t0 = std::clock();
        for (size_t i = 0; i < out.size(); i += 24000) { e.noteOn (65, 1.f); e.render (out.data() + i, 24000, def); }
        double secs = double (std::clock() - t0) / CLOCKS_PER_SEC;
        std::printf ("CPU: %.1f%% of one core at 48k (4x oversampling)\n", secs / 10.0 * 100.0);
    }

    std::printf (bad ? "FAILED\n" : "OK\n");
    return bad ? 1 : 0;
}

// HardKick - hardstyle kick generator engine
// Pure C++ (no JUCE dependency) so it can be tested headlessly.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace hk
{

constexpr double kPi = 3.14159265358979323846;

enum DistType : int { DistOff = 0, DistSoft, DistHard, DistFold, DistCrunch };

struct KickParams
{
    // Click
    float clickLevel   = 0.45f;    // 0..1
    float clickTone    = 4500.f;   // Hz
    float clickDecay   = 6.f;      // ms (to -40 dB)

    // Punch
    float punchLevel   = 0.85f;    // 0..1
    float startPitch   = 2200.f;   // Hz
    float sweepTime    = 55.f;     // ms
    float sweepCurve   = 0.55f;    // 0..1 (higher = faster initial drop)
    float punchDrive   = 6.f;      // dB
    float punchDecay   = 90.f;     // ms (to -40 dB)

    // Tail
    float tailLevel    = 0.8f;     // 0..1
    float tailShape    = 0.2f;     // 0 sine, 0.5 triangle, 1 saw
    float tailStart    = 35.f;     // ms
    float tailAttack   = 20.f;     // ms
    float tailLength   = 260.f;    // ms (hold)
    float tailRelease  = 160.f;    // ms (to -60 dB)
    float pitchDrop    = 2.f;      // semitones over the tail

    // Distortion (applied to tail)
    int   dist1Type    = DistSoft;
    float drive1       = 28.f;     // dB
    int   dist2Type    = DistHard;
    float drive2       = 6.f;      // dB

    // Tail filter
    float cutoff       = 3500.f;   // Hz
    float resonance    = 0.15f;    // 0..1
    float filterEnvAmt = 1.5f;     // octaves
    float filterDecay  = 120.f;    // ms (to -40 dB)

    // Pitch
    bool  keyTrack     = true;
    int   octave       = -3;       // octave offset when key tracking
    float fineTune     = 0.f;      // cents
    float tailPitch    = 43.65f;   // Hz when not key tracking (F1)

    // Master
    float lowCut       = 25.f;     // Hz
    float highCut      = 16000.f;  // Hz
    float masterDrive  = 2.f;      // dB into clipper
    float velocitySens = 0.f;      // 0..1
    float outputGain   = -3.f;     // dB
};

//==============================================================================
inline double dbToGain (double db)                        { return std::pow (10.0, db / 20.0); }
inline double clampd (double x, double lo, double hi)     { return x < lo ? lo : (x > hi ? hi : x); }
inline double midiToHz (double note)                      { return 440.0 * std::exp2 ((note - 69.0) / 12.0); }

inline double distort (int type, double x)
{
    switch (type)
    {
        case DistSoft:   return std::tanh (x);
        case DistHard:   return clampd (x, -1.0, 1.0);
        case DistFold:   return std::sin (x);
        case DistCrunch: return x >= 0.0 ? 1.0 - std::exp (-x)
                                         : -0.8 * (1.0 - std::exp (0.6 * x));
        default:         return x;
    }
}

// Transparent below 0.7, smooth knee up to a 1.0 ceiling.
inline double masterClip (double x)
{
    const double a = std::abs (x);
    if (a <= 0.7) return x;
    const double y = 0.7 + 0.3 * std::tanh ((a - 0.7) / 0.3);
    return x < 0.0 ? -y : y;
}

inline double polyBlep (double t, double dt)
{
    if (t < dt)       { t /= dt;          return t + t - t * t - 1.0; }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
    return 0.0;
}

// Topology-preserving-transform state variable filter
struct SVF
{
    double ic1 = 0.0, ic2 = 0.0;
    double a1 = 1.0, a2 = 0.0, a3 = 0.0, k = 1.414;

    void reset() { ic1 = ic2 = 0.0; }

    void set (double cutoffHz, double fs, double kIn)
    {
        const double g = std::tan (kPi * clampd (cutoffHz, 10.0, 0.45 * fs) / fs);
        k  = kIn;
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    inline void process (double v0, double& lp, double& bp, double& hp)
    {
        const double v3 = v0 - ic2;
        const double v1 = a1 * ic1 + a2 * v3;
        const double v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0 * v1 - ic1;
        ic2 = 2.0 * v2 - ic2;
        lp = v2; bp = v1; hp = v0 - k * v1 - v2;
    }

    inline double lowpass  (double x) { double l, b, h; process (x, l, b, h); return l; }
    inline double highpass (double x) { double l, b, h; process (x, l, b, h); return h; }
    inline double bandpass (double x) { double l, b, h; process (x, l, b, h); return b; }
};

struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void setLowpass (double fc, double fs, double q)
    {
        const double w = 2.0 * kPi * fc / fs, c = std::cos (w), s = std::sin (w);
        const double alpha = s / (2.0 * q), a0 = 1.0 + alpha;
        b0 = (1.0 - c) * 0.5 / a0; b1 = (1.0 - c) / a0; b2 = b0;
        a1 = -2.0 * c / a0;        a2 = (1.0 - alpha) / a0;
    }
    void reset() { z1 = z2 = 0.0; }
    inline double process (double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

//==============================================================================
// Per-block values derived from parameters (at the oversampled rate)
struct Derived
{
    double fs = 48000.0, invFs = 1.0 / 48000.0;

    double clickLevel = 0, punchLevel = 0, tailLevel = 0;
    double clickCoef = 0, punchCoef = 0, filterCoef = 0, releaseCoef = 0;
    double clickTone = 4000;

    double startPitch = 2000, sweepSec = 0.05, curvePow = 1;
    double tailStart = 0, attack = 0.01, length = 0.2, release = 0.1, tailTotal = 0.3, dropSemis = 0;

    double punchGain = 1, punchNorm = 1;
    int    t1 = DistSoft, t2 = DistOff;
    double g1 = 1, g2 = 1;
    double shape = 0;

    double cutoff = 3000, svfK = 1.7, envOct = 0;

    bool   keyTrack = true;
    int    octave = -3;
    double tuneRatio = 1, tailPitch = 43.65;
    double velSens = 0;

    void update (const KickParams& p, double fsIn)
    {
        fs = fsIn; invFs = 1.0 / fs;
        auto decayCoef = [this] (double ms, double lnTarget)
        {
            return std::exp (lnTarget / (std::max (0.1, ms) * 0.001 * fs));
        };
        const double ln40 = std::log (0.01), ln60 = std::log (0.001);

        clickLevel  = p.clickLevel;
        punchLevel  = p.punchLevel;
        tailLevel   = p.tailLevel;
        clickCoef   = decayCoef (p.clickDecay, ln40);
        punchCoef   = decayCoef (p.punchDecay, ln40);
        filterCoef  = decayCoef (p.filterDecay, ln40);
        releaseCoef = decayCoef (p.tailRelease, ln60);
        clickTone   = p.clickTone;

        startPitch  = std::max (20.0, (double) p.startPitch);
        sweepSec    = std::max (0.001, p.sweepTime * 0.001);
        curvePow    = std::exp2 (clampd (p.sweepCurve, 0.0, 1.0) * 4.0 - 1.0); // 0.5 .. 8

        tailStart   = std::max (0.0, p.tailStart * 0.001);
        attack      = std::max (0.0005, p.tailAttack * 0.001);
        length      = std::max (0.0, p.tailLength * 0.001);
        release     = std::max (0.005, p.tailRelease * 0.001);
        tailTotal   = tailStart + attack + length + release;
        dropSemis   = p.pitchDrop;

        punchGain   = dbToGain (p.punchDrive);
        punchNorm   = 1.0 / std::tanh (std::max (1.0, punchGain));

        t1 = p.dist1Type; t2 = p.dist2Type;
        g1 = dbToGain (p.drive1);
        g2 = dbToGain (p.drive2);
        shape = clampd (p.tailShape, 0.0, 1.0);

        cutoff = p.cutoff;
        svfK   = 2.0 * (1.0 - 0.95 * clampd (p.resonance, 0.0, 1.0));
        envOct = p.filterEnvAmt;

        keyTrack  = p.keyTrack;
        octave    = p.octave;
        tuneRatio = std::exp2 (p.fineTune / 1200.0);
        tailPitch = p.tailPitch;
        velSens   = clampd (p.velocitySens, 0.0, 1.0);
    }

    double tailFreqForNote (int note) const
    {
        const double base = keyTrack ? midiToHz (note + 12.0 * octave) : tailPitch;
        return clampd (base * tuneRatio, 5.0, 2000.0);
    }

    double estimatedLengthSec() const
    {
        // punch/click coefficients are -40 dB times; give them a little extra
        const double punchSec = std::log (1e-4) / (std::log (std::max (1e-12, punchCoef)) * fs);
        return std::max (tailTotal, punchSec) + 0.02;
    }
};

//==============================================================================
class Voice
{
public:
    bool active = false;

    void start (int midiNote, float velocity)
    {
        note = midiNote; vel = clampd (velocity, 0.0, 1.0);
        n = 0; phase = 0.0; clickPhase = 0.0;
        clickEnv = punchEnv = filterEnv = 1.0;
        relEnv = 1.0; inRelease = false;
        fade = 1.0; fadeStep = 0.0;
        filter.reset(); clickFilter.reset();
        dcX = dcY = 0.0;
        filterCounter = 0;
        active = true;
    }

    void fadeOut (double samples) { if (active) fadeStep = 1.0 / std::max (1.0, samples); }

    double lastFreq() const { return freq; }

    inline double process (const Derived& d)
    {
        const double t = (double) n * d.invFs;
        ++n;

        // ---- pitch
        const double fT = d.tailFreqForNote (note);
        if (t < d.sweepSec)
        {
            const double x = t / d.sweepSec;
            const double e = std::pow (1.0 - x, d.curvePow);
            freq = fT * std::exp (e * std::log (d.startPitch / fT));
        }
        else
        {
            const double span = std::max (0.001, d.tailTotal - d.sweepSec);
            const double y = clampd ((t - d.sweepSec) / span, 0.0, 1.0);
            freq = fT * std::exp2 (-d.dropSemis * y / 12.0);
        }

        const double dt = std::min (0.49, freq * d.invFs);
        phase += dt;
        if (phase >= 1.0) phase -= 1.0;

        // ---- oscillator
        const double s = std::sin (2.0 * kPi * phase);
        double osc = s;
        if (d.shape > 0.0)
        {
            double tri;
            if (phase < 0.25)      tri = 4.0 * phase;
            else if (phase < 0.75) tri = 2.0 - 4.0 * phase;
            else                   tri = 4.0 * phase - 4.0;

            if (d.shape <= 0.5)
            {
                const double m = d.shape * 2.0;
                osc = s + (tri - s) * m;
            }
            else
            {
                double tt = phase + 0.5; if (tt >= 1.0) tt -= 1.0;
                const double saw = 2.0 * tt - 1.0 - polyBlep (tt, dt);
                const double m = (d.shape - 0.5) * 2.0;
                osc = tri + (saw - tri) * m;
            }
        }

        double out = 0.0;

        // ---- click
        if (clickEnv > 1e-5 && d.clickLevel > 0.0)
        {
            clickFilter.set (d.clickTone, d.fs, 1.0);
            const double noise = clickFilter.bandpass (nextNoise()) * 2.2;
            clickPhase += d.clickTone * d.invFs;
            if (clickPhase >= 1.0) clickPhase -= 1.0;
            const double tone = std::sin (2.0 * kPi * clickPhase) * 0.5;
            out += (noise + tone) * clickEnv * d.clickLevel;
        }
        clickEnv *= d.clickCoef;

        // ---- punch (clean-ish sine sharing the oscillator phase)
        if (punchEnv > 1e-5 && d.punchLevel > 0.0)
        {
            const double pd = d.punchGain > 1.0 ? std::tanh (s * d.punchGain) * d.punchNorm : s;
            out += pd * punchEnv * d.punchLevel;
        }
        punchEnv *= d.punchCoef;

        // ---- tail
        double tailEnv = 0.0;
        if (t >= d.tailStart)
        {
            const double ta = t - d.tailStart;
            if (ta < d.attack)
            {
                const double x = ta / d.attack;
                tailEnv = x * x * (3.0 - 2.0 * x);
            }
            else if (ta < d.attack + d.length && ! inRelease)
            {
                tailEnv = 1.0;
            }
            else
            {
                if (! inRelease) { inRelease = true; relEnv = 1.0; }
                relEnv *= d.releaseCoef;
                tailEnv = relEnv;
            }
        }

        if (tailEnv > 1e-6 && d.tailLevel > 0.0)
        {
            double x = distort (d.t1, osc * d.g1);
            if (d.t2 != DistOff) x = distort (d.t2, x * d.g2);

            if ((filterCounter++ & 7) == 0)
            {
                const double fc = d.cutoff * std::exp2 (d.envOct * filterEnv);
                filter.set (clampd (fc, 20.0, 0.45 * d.fs), d.fs, d.svfK);
            }
            x = filter.lowpass (x);

            // DC blocker (crunch / folds can add offset)
            const double R = 1.0 - (2.0 * kPi * 8.0 * d.invFs);
            const double y = x - dcX + R * dcY;
            dcX = x; dcY = y;

            out += y * tailEnv * d.tailLevel;
        }
        filterEnv *= d.filterCoef;

        // ---- end of voice
        if (t > d.tailTotal && punchEnv < 1e-4 && clickEnv < 1e-4)
            active = false;

        const double velGain = 1.0 - d.velSens * (1.0 - vel);
        out *= velGain * fade;

        if (fadeStep > 0.0)
        {
            fade -= fadeStep;
            if (fade <= 0.0) { fade = 0.0; active = false; }
        }
        return out;
    }

private:
    inline double nextNoise()
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return (double) rng * (2.0 / 4294967295.0) - 1.0;
    }

    int note = 65;
    double vel = 1.0;
    int64_t n = 0;
    double phase = 0, clickPhase = 0, freq = 50;
    double clickEnv = 0, punchEnv = 0, filterEnv = 0, relEnv = 0;
    bool inRelease = false;
    double fade = 1.0, fadeStep = 0.0;
    SVF filter, clickFilter;
    double dcX = 0, dcY = 0;
    uint32_t rng = 0x9E3779B9u;
    uint32_t filterCounter = 0;
};

//==============================================================================
class KickEngine
{
public:
    void prepare (double sampleRate, int oversampling = 0)
    {
        baseFs = sampleRate;
        os = oversampling > 0 ? oversampling : (sampleRate >= 88000.0 ? 2 : 4);
        fsO = baseFs * os;

        static const double qs[4] = { 0.50979558, 0.60134489, 0.89997622, 2.56291545 };
        for (int i = 0; i < 4; ++i)
            aa[i].setLowpass (0.45 * baseFs, fsO, qs[i]);

        reset();
    }

    void reset()
    {
        for (auto& v : voices) v.active = false;
        for (auto& b : aa) b.reset();
        lowCut.reset(); highCut.reset();
    }

    void noteOn (int note, float velocity)
    {
        if (voices[current].active)
        {
            voices[current].fadeOut (fsO * 0.004);
            current ^= 1;
        }
        voices[current].start (note, velocity);
    }

    void allSoundOff()
    {
        for (auto& v : voices) v.fadeOut (fsO * 0.004);
    }

    bool isPlaying() const { return voices[0].active || voices[1].active; }

    // Renders mono output at the base sample rate. pitchOut (optional) receives the
    // current oscillator frequency of the newest voice for every output sample.
    void render (float* out, int numSamples, const KickParams& p, float* pitchOut = nullptr)
    {
        d.update (p, fsO);
        lowCut.set (p.lowCut, fsO, 1.41421356);
        highCut.set (std::min ((double) p.highCut, 0.45 * fsO), fsO, 1.41421356);
        const double drive = dbToGain (p.masterDrive);
        const double outGain = dbToGain (p.outputGain);

        for (int i = 0; i < numSamples; ++i)
        {
            double y = 0.0;
            for (int k = 0; k < os; ++k)
            {
                double x = 0.0;
                for (auto& v : voices)
                    if (v.active) x += v.process (d);

                x = lowCut.highpass (x);
                x = highCut.lowpass (x);
                x = masterClip (x * drive) * outGain;

                if (os > 1)
                    for (auto& b : aa) x = b.process (x);
                y = x;
            }
            out[i] = (float) y;
            if (pitchOut != nullptr) pitchOut[i] = (float) voices[current].lastFreq();
        }
    }

    double oversampledRate() const { return fsO; }
    double estimatedLengthSec (const KickParams& p) { Derived t; t.update (p, fsO); return t.estimatedLengthSec(); }

private:
    double baseFs = 48000, fsO = 192000;
    int os = 4;
    Voice voices[2];
    int current = 0;
    Derived d;
    SVF lowCut, highCut;
    Biquad aa[4];
};

//==============================================================================
// Offline render used for the waveform preview in the editor.
inline void renderPreview (const KickParams& p, double sampleRate, int note, double maxSeconds,
                           std::vector<float>& wave, std::vector<float>& pitch, int oversampling = 1)
{
    KickEngine e;
    e.prepare (sampleRate, oversampling);
    const double len = std::min (maxSeconds, e.estimatedLengthSec (p));
    const int n = std::max (1, (int) (len * sampleRate));
    wave.assign ((size_t) n, 0.0f);
    pitch.assign ((size_t) n, 0.0f);
    e.noteOn (note, 1.0f);
    e.render (wave.data(), n, p, pitch.data());
}

} // namespace hk

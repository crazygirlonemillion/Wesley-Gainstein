#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

// Original DSP for hly_chorfeed. No third-party DSP code is embedded here.
// Every routing edge uses the previous sample, including self-feedback: host
// block size therefore has no effect on the feedback network's tuning.
namespace hly
{
struct Params
{
    int filterMode = 0; // 0: SVF lowpass, 1: highpass, 2: bandpass, 3: driven ladder
    float cutoff = 3200.0f, resonance = 0.42f, filterDriveDb = 2.0f;
    int delayMode = 0; // 0: three-voice ensemble, 1: digital, 2: dark
    float timeMs = 18.0f, rateHz = 0.28f, depthMs = 3.2f, width = 1.0f;
    int distortionMode = 0; // 0: soft, 1: hard, 2: asymmetric, 3: fold
    float driveDb = 3.0f, tone = 0.58f, bias = 0.0f;
    float matrix[3][3] {{0.0f, 0.0f, 0.0f}, {0.25f, 0.10f, 0.0f}, {0.0f, 0.15f, 0.0f}};
    float input[3] {0.0f, 1.0f, 0.0f};
    float output[3] {0.30f, 0.80f, 0.0f};
    float feedback = 0.40f, mix = 0.50f, outputDb = 0.0f;
    bool freeze = false;
};

class Engine
{
public:
    void prepare (double rate)
    {
        sampleRate = std::isfinite (rate) ? std::clamp (rate, 8000.0, 384000.0) : 44100.0;
        smoothCoeff = static_cast<float> (1.0 - std::exp (-1.0 / (sampleRate * 0.020)));
        dcPole = static_cast<float> (std::exp (-2.0 * pi * 8.0 / sampleRate));
        for (auto& channel : delayBuffer)
            channel.assign (static_cast<std::size_t> (sampleRate * 2.12) + 16, 0.0f);
        for (auto& channel : oversampleFilters)
        {
            constexpr double q[3] {0.5176380902050415, 0.7071067811865476, 1.9318516525781366};
            for (int stage = 0; stage < 3; ++stage)
                channel[static_cast<std::size_t> (stage)].configure (0.1125, q[stage]);
        }
        prepared = true;
        reset ();
    }

    void reset () noexcept
    {
        for (auto& buffer : delayBuffer) std::fill (buffer.begin (), buffer.end (), 0.0f);
        writePosition = 0;
        phase = 0.0;
        nodeHistory = {};
        svf = {};
        ladder = {};
        darkState = {};
        distortionPrevious = {};
        toneState = {};
        dc = {};
        for (auto& channel : oversampleFilters)
            for (auto& stage : channel) stage.reset ();
        current = target;
        setModeWeightsImmediately ();
    }

    void setParams (const Params& p) noexcept
    {
        target = p;
        target.filterMode = std::clamp (p.filterMode, 0, 3);
        target.delayMode = std::clamp (p.delayMode, 0, 2);
        target.distortionMode = std::clamp (p.distortionMode, 0, 3);
        target.cutoff = clean (p.cutoff, 20.0f, static_cast<float> (sampleRate * 0.45), 3200.0f);
        target.resonance = clean (p.resonance, 0.0f, 1.0f, 0.42f);
        target.filterDriveDb = clean (p.filterDriveDb, 0.0f, 30.0f, 2.0f);
        target.timeMs = clean (p.timeMs, 1.0f, 2000.0f, 18.0f);
        target.rateHz = clean (p.rateHz, 0.01f, 20.0f, 0.28f);
        target.depthMs = clean (p.depthMs, 0.0f, 30.0f, 3.2f);
        target.width = clean (p.width, 0.0f, 2.0f, 1.0f);
        target.driveDb = clean (p.driveDb, 0.0f, 36.0f, 3.0f);
        target.tone = clean (p.tone, 0.0f, 1.0f, 0.58f);
        target.bias = clean (p.bias, -1.0f, 1.0f, 0.0f);
        target.feedback = clean (p.feedback, 0.0f, 1.5f, 0.4f);
        target.mix = clean (p.mix, 0.0f, 1.0f, 0.5f);
        target.outputDb = clean (p.outputDb, -60.0f, 12.0f, 0.0f);
        for (int i = 0; i < 3; ++i)
        {
            target.input[i] = clean (p.input[i], 0.0f, 1.0f, 0.0f);
            target.output[i] = clean (p.output[i], 0.0f, 1.0f, 0.0f);
            for (int j = 0; j < 3; ++j)
                target.matrix[i][j] = clean (p.matrix[i][j], -1.0f, 1.0f, 0.0f);
        }
        if (! prepared) current = target;
    }

    void process (float* left, float* right, int count) noexcept
    {
        if (! prepared || left == nullptr || right == nullptr || count <= 0) return;
        for (int n = 0; n < count; ++n)
        {
            smoothParameters ();
            // Coefficients are shared by L/R, but all filter states are separate.
            const float cutoff = std::clamp (current.cutoff, 20.0f, static_cast<float> (sampleRate * 0.45));
            const float g = std::tan (static_cast<float> (pi / sampleRate) * cutoff);
            const float damping = 2.0f - 1.985f * current.resonance;
            const float a1 = 1.0f / (1.0f + g * (g + damping));
            const float ladderG = std::tan (static_cast<float> (pi / (sampleRate * 2.0)) * cutoff);
            const float ladderPole = ladderG / (1.0f + ladderG);
            const float filterGain = dbGain (current.filterDriveDb);
            const float distortionGain = dbGain (current.driveDb);
            const float toneFrequency = 600.0f * std::pow (30.0f, current.tone);
            const float toneCoeff = static_cast<float> (1.0 - std::exp (-2.0 * pi * std::min (toneFrequency, static_cast<float> (sampleRate * 0.45)) / sampleRate));
            const float darkCoeff = static_cast<float> (1.0 - std::exp (-2.0 * pi * std::min (4500.0, sampleRate * 0.40) / sampleRate));
            const float outputGain = dbGain (current.outputDb);
            freezeAmount += smoothCoeff * ((target.freeze ? 1.0f : 0.0f) - freezeAmount);
            std::array<float, 2> dry {finite (left[n]), finite (right[n])};
            std::array<std::array<float, 3>, 2> next {};
            for (int ch = 0; ch < 2; ++ch)
            {
                float bus[3] {};
                for (int to = 0; to < 3; ++to)
                {
                    bus[to] = dry[static_cast<std::size_t> (ch)] * current.input[to] * (1.0f - freezeAmount);
                    for (int from = 0; from < 3; ++from)
                        bus[to] += nodeHistory[static_cast<std::size_t> (ch)][static_cast<std::size_t> (from)] * current.matrix[from][to] * current.feedback;
                    // A gradual analog-like bound on the internal bus allows
                    // self-oscillation without floating-point runaway.
                    bus[to] = bound (bus[to], 6.0f);
                }
                next[static_cast<std::size_t> (ch)][0] = filter (ch, bus[0], g, damping, a1, ladderPole, filterGain);
                next[static_cast<std::size_t> (ch)][1] = delay (ch, bus[1], darkCoeff);
                next[static_cast<std::size_t> (ch)][2] = distort (ch, bus[2], distortionGain, toneCoeff);
                float wet = 0.0f;
                for (int node = 0; node < 3; ++node)
                {
                    auto& value = next[static_cast<std::size_t> (ch)][static_cast<std::size_t> (node)];
                    value = dc[static_cast<std::size_t> (ch)][static_cast<std::size_t> (node)].process (finite (value), dcPole);
                    value = bound (value, 4.0f);
                    wet += value * current.output[node];
                }
                // Linear dry/wet is appropriate for a chorus: at 50% it mixes
                // equal dry and delayed signals without an extra gain boost.
                const float result = finite ((dry[static_cast<std::size_t> (ch)] * (1.0f - current.mix) + bound (wet, 4.0f) * current.mix) * outputGain);
                (ch == 0 ? left : right)[n] = result;
            }
            nodeHistory = next;
            writePosition = (writePosition + 1) % delayBuffer[0].size ();
            phase += current.rateHz / sampleRate;
            if (phase >= 1.0) phase -= std::floor (phase);
        }
    }

private:
    static constexpr double pi = 3.14159265358979323846;
    static float clean (float x, float lo, float hi, float fallback) noexcept { return std::isfinite (x) ? std::clamp (x, lo, hi) : fallback; }
    static float finite (float x) noexcept { return std::isfinite (x) ? x : 0.0f; }
    static float dbGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
    static float bound (float x, float ceiling) noexcept { return ceiling * std::tanh (finite (x) / ceiling); }
    struct DCBlock
    {
        float x = 0.0f, y = 0.0f;
        float process (float v, float pole) noexcept
        {
            const float next = v - x + pole * y;
            x = v; y = std::abs (next) < 1.0e-25f ? 0.0f : next;
            return y;
        }
    };
    struct Biquad
    {
        float b0 {}, b1 {}, b2 {}, a1 {}, a2 {}, z1 {}, z2 {};
        void configure (double frequency, double q) noexcept
        {
            const double w = 2.0 * pi * frequency, c = std::cos (w), alpha = std::sin (w) / (2.0 * q);
            const double d = 1.0 / (1.0 + alpha);
            b0 = static_cast<float> ((1.0 - c) * 0.5 * d); b1 = 2.0f * b0; b2 = b0;
            a1 = static_cast<float> (-2.0 * c * d); a2 = static_cast<float> ((1.0 - alpha) * d);
        }
        float process (float x) noexcept
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y;
            return y;
        }
        void reset () noexcept { z1 = z2 = 0.0f; }
    };
    struct SVFState { float ic1 {}, ic2 {}; };
    float filter (int ch, float x, float g, float k, float a1, float lp, float gain) noexcept
    {
        auto& state = svf[static_cast<std::size_t> (ch)];
        const float driven = std::tanh (x * gain);
        const float v3 = driven - state.ic2;
        const float band = a1 * state.ic1 + g * a1 * v3;
        const float low = state.ic2 + g * band;
        state.ic1 = bound (2.0f * band - state.ic1, 12.0f);
        state.ic2 = bound (2.0f * low - state.ic2, 12.0f);
        const float high = driven - k * band - low;
        // A nonlinear four-stage ladder, integrated at 2x rate. The global
        // feedback is explicit; bounded stage updates keep its drive musical.
        auto& stages = ladder[static_cast<std::size_t> (ch)];
        // Inactive nonlinear models are allowed to sleep. A model change fades
        // in over 20ms while its states settle, avoiding needless CPU in chorus.
        for (int os = 0; os < (filterWeights[3] > 1.0e-6f ? 2 : 0); ++os)
        {
            float signal = std::tanh (x * gain - 3.95f * current.resonance * stages[3]);
            for (auto& stage : stages)
            {
                const float v = (std::tanh (signal) - std::tanh (stage)) * lp;
                const float y = stage + v;
                stage = bound (y + v, 6.0f);
                signal = y;
            }
        }
        const float output[4] { low, high, band * std::sqrt (std::max (k, 0.08f)), stages[3] };
        float sum = 0.0f;
        for (int i = 0; i < 4; ++i) sum += output[i] * filterWeights[static_cast<std::size_t> (i)];
        return sum;
    }
    float readDelay (int ch, double samples) const noexcept
    {
        const auto& buffer = delayBuffer[static_cast<std::size_t> (ch)];
        const auto size = static_cast<std::ptrdiff_t> (buffer.size ());
        samples = std::clamp (samples, 2.0, static_cast<double> (size - 4));
        double position = static_cast<double> (writePosition) - samples;
        if (position < 0.0) position += static_cast<double> (size);
        const auto base = static_cast<std::ptrdiff_t> (position);
        const float frac = static_cast<float> (position - static_cast<double> (base));
        const auto at = [&] (std::ptrdiff_t index)
        {
            if (index < 0) index += size;
            if (index >= size) index -= size;
            return buffer[static_cast<std::size_t> (index)];
        };
        const float y0 = at (base - 1), y1 = at (base), y2 = at (base + 1), y3 = at (base + 2);
        // Four-point Catmull-Rom interpolation; efficient and continuously
        // varying, avoiding the staircase artifacts of integer delay changes.
        return y1 + 0.5f * frac * (y2 - y0 + frac * (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3 + frac * (3.0f * (y1 - y2) + y3 - y0)));
    }
    float delay (int ch, float x, float darkCoeff) noexcept
    {
        delayBuffer[static_cast<std::size_t> (ch)][writePosition] = x;
        const double base = current.timeMs * sampleRate * 0.001;
        const double depth = std::min (current.depthMs * sampleRate * 0.001, base * 0.90);
        const double stereoPhase = ch == 0 ? 0.0 : 0.25 * current.width;
        float ensemble = 0.0f;
        for (int voice = 0; voice < (delayWeights[0] > 1.0e-6f ? 3 : 0); ++voice)
        {
            const double angle = 2.0 * pi * (phase + stereoPhase + voice / 3.0);
            const double detune = 1.0 + (voice - 1) * 0.028;
            ensemble += readDelay (ch, base * detune + depth * std::sin (angle));
        }
        ensemble *= 1.0f / 3.0f;
        const float digital = readDelay (ch, base + depth * std::sin (2.0 * pi * (phase + stereoPhase)));
        auto& dark = darkState[static_cast<std::size_t> (ch)];
        dark += darkCoeff * (digital - dark);
        const float darkOutput = 1.25f * std::tanh (dark * 0.8f);
        return delayWeights[0] * ensemble + delayWeights[1] * digital + delayWeights[2] * darkOutput;
    }
    float distort (int ch, float x, float gain, float toneCoeff) noexcept
    {
        auto& previous = distortionPrevious[static_cast<std::size_t> (ch)];
        float result = 0.0f;
        // 4x waveshaping with linear input interpolation and a sixth-order
        // Butterworth reconstruction lowpass at 0.45 of the native sample rate.
        for (int step = 1; step <= 4; ++step)
        {
            const float v = (previous + (x - previous) * (step * 0.25f)) * gain;
            const float biased = v + current.bias;
            const auto asymmetric = [] (float z) { return z >= 0.0f ? std::tanh (z) : 0.65f * std::tanh (z * 1.5f); };
            const auto fold = [] (float z) { return static_cast<float> (2.0 / pi) * std::asin (std::sin (static_cast<float> (pi * 0.5) * z)); };
            result = 0.0f;
            if (distortionWeights[0] > 1.0e-6f) result += distortionWeights[0] * (std::tanh (biased) - std::tanh (current.bias));
            if (distortionWeights[1] > 1.0e-6f) result += distortionWeights[1] * (std::clamp (biased, -0.75f, 0.75f) - std::clamp (current.bias, -0.75f, 0.75f));
            if (distortionWeights[2] > 1.0e-6f) result += distortionWeights[2] * (asymmetric (biased) - asymmetric (current.bias));
            if (distortionWeights[3] > 1.0e-6f) result += distortionWeights[3] * (fold (biased) - fold (current.bias));
            for (auto& stage : oversampleFilters[static_cast<std::size_t> (ch)]) result = stage.process (result);
        }
        previous = x;
        auto& tone = toneState[static_cast<std::size_t> (ch)];
        tone += toneCoeff * (result - tone);
        return tone;
    }
    void setModeWeightsImmediately () noexcept
    {
        for (int i = 0; i < 4; ++i)
        {
            filterWeights[static_cast<std::size_t> (i)] = target.filterMode == i ? 1.0f : 0.0f;
            distortionWeights[static_cast<std::size_t> (i)] = target.distortionMode == i ? 1.0f : 0.0f;
        }
        for (int i = 0; i < 3; ++i) delayWeights[static_cast<std::size_t> (i)] = target.delayMode == i ? 1.0f : 0.0f;
        freezeAmount = target.freeze ? 1.0f : 0.0f;
    }
    void smoothParameters () noexcept
    {
        const auto smooth = [this] (float& x, float destination) { x += smoothCoeff * (destination - x); };
        smooth (current.cutoff, target.cutoff); smooth (current.resonance, target.resonance); smooth (current.filterDriveDb, target.filterDriveDb);
        smooth (current.timeMs, target.timeMs); smooth (current.rateHz, target.rateHz); smooth (current.depthMs, target.depthMs); smooth (current.width, target.width);
        smooth (current.driveDb, target.driveDb); smooth (current.tone, target.tone); smooth (current.bias, target.bias);
        smooth (current.feedback, target.feedback); smooth (current.mix, target.mix); smooth (current.outputDb, target.outputDb);
        for (int i = 0; i < 3; ++i)
        {
            smooth (current.input[i], target.input[i]); smooth (current.output[i], target.output[i]);
            for (int j = 0; j < 3; ++j) smooth (current.matrix[i][j], target.matrix[i][j]);
            smooth (delayWeights[static_cast<std::size_t> (i)], target.delayMode == i ? 1.0f : 0.0f);
        }
        for (int i = 0; i < 4; ++i)
        {
            smooth (filterWeights[static_cast<std::size_t> (i)], target.filterMode == i ? 1.0f : 0.0f);
            smooth (distortionWeights[static_cast<std::size_t> (i)], target.distortionMode == i ? 1.0f : 0.0f);
        }
    }
    double sampleRate = 44100.0, phase = 0.0;
    float smoothCoeff = 0.001f, dcPole = 0.999f, freezeAmount = 0.0f;
    Params current {}, target {};
    bool prepared = false;
    std::size_t writePosition = 0;
    std::array<std::vector<float>, 2> delayBuffer;
    std::array<std::array<float, 3>, 2> nodeHistory {};
    std::array<SVFState, 2> svf {};
    std::array<std::array<float, 4>, 2> ladder {};
    std::array<float, 2> darkState {}, distortionPrevious {}, toneState {};
    std::array<std::array<Biquad, 3>, 2> oversampleFilters {};
    std::array<std::array<DCBlock, 3>, 2> dc {};
    std::array<float, 4> filterWeights {}, distortionWeights {};
    std::array<float, 3> delayWeights {};
};
} // namespace hly

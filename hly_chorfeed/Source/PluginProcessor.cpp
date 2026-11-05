#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

namespace {
constexpr const char* ids[] = {
    "filterMode", "cutoff", "resonance", "filterDrive", "delayMode", "time", "rate", "depth", "width",
    "distortionMode", "drive", "tone", "bias",
    "m00", "m01", "m02", "m10", "m11", "m12", "m20", "m21", "m22",
    "in0", "in1", "in2", "out0", "out1", "out2", "feedback", "mix", "output", "freeze"
};
constexpr const char* programs[] = { "Lush / Slow Bloom", "Wide / Velvet", "Metal / Hollow Wire", "Echo / Dub Garden", "Drive / Frayed Edges", "Init / Clean Delay" };
}

juce::AudioProcessorValueTreeState::ParameterLayout HlyChorfeedAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto floating = [&] (const char* id, const juce::String& name, float lo, float hi, float step, float def, float skew = 1.0f, const juce::String& unit = {}) {
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float>(lo, hi, step, skew), def,
            juce::AudioParameterFloatAttributes().withLabel(unit)));
    };
    auto choice = [&] (const char* id, const char* name, juce::StringArray options) {
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { id, 1 }, name, options, 0));
    };
    choice("filterMode", "Filter model", { "Smooth LP", "Smooth HP", "Smooth BP", "Driven Ladder" });
    floating("cutoff", "Filter cutoff", 20, 20000, .01f, 3200, .25f, "Hz");
    floating("resonance", "Filter resonance", 0, .98f, .001f, .42f);
    floating("filterDrive", "Filter drive", 0, 24, .01f, 2, 1, "dB");
    choice("delayMode", "Delay model", { "Ensemble", "Digital", "Dark" });
    floating("time", "Delay time", 1, 2000, .01f, 18, .25f, "ms");
    floating("rate", "Modulation rate", .01f, 10, .001f, .28f, .35f, "Hz");
    floating("depth", "Modulation depth", 0, 15, .01f, 3.2f, 1, "ms");
    floating("width", "Stereo width", 0, 1.5f, .001f, 1);
    choice("distortionMode", "Distortion model", { "Soft Saturation", "Hard Clip", "Asymmetric", "Wavefold" });
    floating("drive", "Distortion drive", 0, 36, .01f, 3, 1, "dB");
    floating("tone", "Distortion tone", 0, 1, .001f, .58f);
    floating("bias", "Distortion bias", -1, 1, .001f, 0);
    for (int from = 0; from < 3; ++from)
        for (int to = 0; to < 3; ++to) {
            const float def = from == 1 && to == 0 ? .25f : from == 1 && to == 1 ? .10f : from == 2 && to == 1 ? .15f : 0;
            const juce::String id = "m" + juce::String(from) + juce::String(to);
            const juce::StringArray names { "Filter", "Delay", "Distortion" };
            floating(id.toRawUTF8(), names[from] + " to " + names[to], -1, 1, .001f, def);
        }
    for (int n = 0; n < 3; ++n) {
        const auto id = "in" + juce::String(n);
        floating(id.toRawUTF8(), "Input send " + juce::String(n + 1), 0, 1, .001f, n == 1 ? 1.0f : 0.0f);
    }
    for (int n = 0; n < 3; ++n) {
        const auto id = "out" + juce::String(n);
        floating(id.toRawUTF8(), "Output mix " + juce::String(n + 1), 0, 1, .001f, n == 0 ? .3f : n == 1 ? .8f : 0);
    }
    floating("feedback", "Global feedback", 0, 1.5f, .001f, .4f);
    floating("mix", "Dry wet", 0, 1, .001f, .5f);
    floating("output", "Output gain", -24, 6, .01f, 0, 1, "dB");
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "freeze", 1 }, "Freeze input", false));
    return layout;
}

HlyChorfeedAudioProcessor::HlyChorfeedAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "HLY_CHORFEED", createParameterLayout())
{
    static_assert(std::size(ids) == count);
    for (size_t n = 0; n < values.size(); ++n) values[n] = apvts.getRawParameterValue(ids[n]);
}

bool HlyChorfeedAudioProcessor::isBusesLayoutSupported(const BusesLayout& b) const
{
    return b.getMainInputChannelSet() == b.getMainOutputChannelSet()
        && (b.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
            || b.getMainOutputChannelSet() == juce::AudioChannelSet::mono());
}

void HlyChorfeedAudioProcessor::prepareToPlay(double sr, int block)
{
    monoScratch.assign(static_cast<size_t>(std::max(1, block)), 0);
    engine.setParams(readParams());
    engine.prepare(sr);
    peakL.store(0); peakR.store(0);
    panic.store(false);
}

hly::Params HlyChorfeedAudioProcessor::readParams() const
{
    const auto get = [&] (int i) { return values[static_cast<size_t>(i)]->load(std::memory_order_relaxed); };
    hly::Params p;
    p.filterMode = static_cast<int>(get(filterMode)); p.cutoff = get(cutoff);
    p.resonance = get(resonance); p.filterDriveDb = get(filterDrive);
    p.delayMode = static_cast<int>(get(delayMode)); p.timeMs = get(time);
    p.rateHz = get(rate); p.depthMs = get(depth); p.width = get(width);
    p.distortionMode = static_cast<int>(get(distortionMode)); p.driveDb = get(drive);
    p.tone = get(tone); p.bias = get(bias);
    for (int i = 0; i < 9; ++i) p.matrix[i / 3][i % 3] = get(matrixStart + i);
    for (int i = 0; i < 3; ++i) { p.input[i] = get(inputStart + i); p.output[i] = get(outputStart + i); }
    p.feedback = get(feedback); p.mix = get(mix); p.outputDb = get(output); p.freeze = get(freeze) >= .5f;
    return p;
}

void HlyChorfeedAudioProcessor::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (panic.exchange(false)) engine.reset();
    engine.setParams(readParams());
    const int channels = audio.getNumChannels(), samples = audio.getNumSamples();
    if (channels == 0 || samples == 0) return;
    auto* l = audio.getWritePointer(0);
    if (channels >= 2) engine.process(l, audio.getWritePointer(1), samples);
    else {
        // Hosts can send a larger block than advertised. Chunk using preallocated scratch.
        const int chunkSize = static_cast<int>(monoScratch.size());
        if (chunkSize == 0) return;
        for (int pos = 0; pos < samples; pos += chunkSize) {
            const int n = std::min(chunkSize, samples - pos);
            std::copy_n(l + pos, n, monoScratch.data());
            engine.process(l + pos, monoScratch.data(), n);
            for (int i = 0; i < n; ++i) l[pos + i] = .5f * (l[pos + i] + monoScratch[static_cast<size_t>(i)]);
        }
    }
    for (int c = 2; c < channels; ++c) audio.clear(c, 0, samples);
    const float release = static_cast<float>(std::exp(-samples / (std::max(8000.0, getSampleRate()) * .30)));
    peakL.store(std::max(audio.getMagnitude(0, 0, samples), peakL.load() * release));
    peakR.store(std::max(audio.getMagnitude(channels > 1 ? 1 : 0, 0, samples), peakR.load() * release));
}

int HlyChorfeedAudioProcessor::getNumPrograms() { return static_cast<int>(std::size(programs)); }
const juce::String HlyChorfeedAudioProcessor::getProgramName(int i)
{ return programs[juce::jlimit(0, getNumPrograms() - 1, i)]; }

void HlyChorfeedAudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= getNumPrograms()) return;
    // Reset all parameters to defaults first so presets are independent of previous edits.
    for (auto* parameter : getParameters()) parameter->setValueNotifyingHost(parameter->getDefaultValue());
    auto set = [&] (const char* id, float value) {
        auto* p = apvts.getParameter(id); p->setValueNotifyingHost(p->convertTo0to1(value));
    };
    switch (index) {
        case 1: set("time", 24); set("rate", .16f); set("depth", 5.2f); set("width", 1.4f);
                set("mix", .58f); set("cutoff", 6500); set("m11", .28f); break;
        case 2: set("time", 3.8f); set("depth", .12f); set("rate", .07f);
                set("m10", .8f); set("m01", -.75f); set("m11", -.6f);
                set("feedback", .93f); set("resonance", .75f); set("mix", .55f); break;
        case 3: set("delayMode", 2); set("time", 375); set("depth", .7f);
                set("cutoff", 1800); set("m10", .9f); set("m02", .8f); set("m21", .85f);
                set("m11", .3f); set("feedback", .85f); set("drive", 8);
                set("out0", 0); set("out1", 1); set("mix", .35f); break;
        case 4: set("distortionMode", 3); set("drive", 12); set("time", 12);
                set("m12", .8f); set("m21", -.65f); set("feedback", .8f);
                set("out0", 0); set("out1", .65f); set("out2", .35f); set("mix", .4f); break;
        case 5: set("delayMode", 1); set("time", 250); set("depth", 0);
                set("m10", 0); set("m11", 0); set("m21", 0);
                set("out0", 0); set("out1", 1); set("feedback", 0); set("mix", .3f); break;
        default: break;
    }
    currentProgram.store(index);
    requestPanic();
}

void HlyChorfeedAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty("program", currentProgram.load(), nullptr);
    state.setProperty("version", 1, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, dest);
}

void HlyChorfeedAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size)) {
        auto state = juce::ValueTree::fromXml(*xml);
        if (state.isValid() && state.hasType(apvts.state.getType())) {
            currentProgram.store(juce::jlimit(0, getNumPrograms() - 1, static_cast<int>(state.getProperty("program", 0))));
            apvts.replaceState(state);
            requestPanic();
        }
    }
}

juce::AudioProcessorEditor* HlyChorfeedAudioProcessor::createEditor()
{ return new HlyChorfeedAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{ return new HlyChorfeedAudioProcessor(); }

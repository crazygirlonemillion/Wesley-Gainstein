#include "../Source/PluginProcessor.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void set(HlyChorfeedAudioProcessor& p, const char* id, float v)
{
    auto* parameter = p.apvts.getParameter(id);
    require(parameter != nullptr, "Missing parameter");
    parameter->setValueNotifyingHost(parameter->convertTo0to1(v));
}
void exercise(HlyChorfeedAudioProcessor& p, int channels, int block)
{
    auto buses = p.getBusesLayout();
    buses.inputBuses.set(0, channels == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo());
    buses.outputBuses.set(0, buses.inputBuses[0]);
    require(p.setBusesLayout(buses), "Valid mono/stereo layout rejected");
    p.setRateAndBufferSizeDetails(48000, 64);
    p.prepareToPlay(48000, 64);
    juce::AudioBuffer<float> buffer(channels, block);
    juce::MidiBuffer midi;
    for (int b = 0; b < 50; ++b) {
        for (int c = 0; c < channels; ++c)
            for (int i = 0; i < block; ++i)
                buffer.setSample(c, i, .1f * std::sin(float(b * block + i) * .03f));
        p.processBlock(buffer, midi);
        for (int c = 0; c < channels; ++c)
            for (int i = 0; i < block; ++i)
                require(std::isfinite(buffer.getSample(c, i)), "Non-finite processor output");
    }
    p.releaseResources();
}
}
int main()
{
    try {
        juce::ScopedJuceInitialiser_GUI init;
        HlyChorfeedAudioProcessor p;
        require(p.getParameters().size() == 32, "Unexpected parameter count");
        set(p, "cutoff", 1734); set(p, "m20", -.71f); set(p, "freeze", 1);
        juce::MemoryBlock state; p.getStateInformation(state);
        HlyChorfeedAudioProcessor restored;
        restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        require(std::abs(restored.apvts.getRawParameterValue("cutoff")->load() - 1734) < .1f, "Cutoff did not restore");
        require(std::abs(restored.apvts.getRawParameterValue("m20")->load() + .71f) < .001f, "Matrix did not restore");
        require(restored.apvts.getRawParameterValue("freeze")->load() > .5f, "Freeze did not restore");
        const char invalid[] = "not a plugin state";
        restored.setStateInformation(invalid, sizeof(invalid));
        for (int i = 0; i < p.getNumPrograms(); ++i) {
            p.setCurrentProgram(i);
            require(p.getCurrentProgram() == i, "Program index mismatch");
            require(p.apvts.getRawParameterValue("freeze")->load() < .5f, "Preset retained freeze");
            exercise(p, 2, 127); exercise(p, 1, 511);
        }
        p.setCurrentProgram(0);
        p.setRateAndBufferSizeDetails(44100, 128);
        p.prepareToPlay(44100, 128);
        set(p, "mix", 1);
        juce::AudioBuffer<float> b(2, 128); b.clear(); b.setSample(0, 0, .5f);
        juce::MidiBuffer midi; p.processBlock(b, midi);
        p.requestPanic(); b.clear(); p.processBlock(b, midi);
        require(b.getMagnitude(0, 128) < 1.e-6f, "Panic retained previous audio");
        std::cout << "Processor tests passed: state, presets, mono/stereo, oversized blocks, panic.\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

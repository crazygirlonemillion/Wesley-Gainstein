#pragma once
#include <JuceHeader.h>
#include "DspEngine.h"
#include <array>
#include <atomic>

class HlyChorfeedAudioProcessor final : public juce::AudioProcessor
{
public:
    HlyChorfeedAudioProcessor();
    ~HlyChorfeedAudioProcessor() override = default;
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 120.0; }
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    void reset() override { panic.store(true); }
    void requestPanic() { panic.store(true); }

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> peakL { 0 }, peakR { 0 };
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
private:
    enum ParameterIndex {
        filterMode, cutoff, resonance, filterDrive, delayMode, time, rate, depth, width,
        distortionMode, drive, tone, bias, matrixStart, inputStart = matrixStart + 9,
        outputStart = inputStart + 3, feedback = outputStart + 3, mix, output, freeze, count
    };
    std::array<std::atomic<float>*, count> values {};
    hly::Engine engine;
    std::vector<float> monoScratch;
    std::atomic<bool> panic { false };
    std::atomic<int> currentProgram { 0 };
    hly::Params readParams() const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HlyChorfeedAudioProcessor)
};

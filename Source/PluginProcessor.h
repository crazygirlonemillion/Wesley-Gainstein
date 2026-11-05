#pragma once

#include <map>
#include <string>
#include <juce_audio_processors/juce_audio_processors.h>
#include "KickEngine.h"
#include "Presets.h"

namespace ids
{
    #define HK_ID(name, str) inline constexpr const char* name = str;
    HK_ID (clickLevel,   "click_level")
    HK_ID (clickTone,    "click_tone")
    HK_ID (clickDecay,   "click_decay")
    HK_ID (punchLevel,   "punch_level")
    HK_ID (startPitch,   "start_pitch")
    HK_ID (sweepTime,    "sweep_time")
    HK_ID (sweepCurve,   "sweep_curve")
    HK_ID (punchDrive,   "punch_drive")
    HK_ID (punchDecay,   "punch_decay")
    HK_ID (tailLevel,    "tail_level")
    HK_ID (tailShape,    "tail_shape")
    HK_ID (tailStart,    "tail_start")
    HK_ID (tailAttack,   "tail_attack")
    HK_ID (tailLength,   "tail_length")
    HK_ID (tailRelease,  "tail_release")
    HK_ID (pitchDrop,    "pitch_drop")
    HK_ID (dist1Type,    "dist1_type")
    HK_ID (drive1,       "drive1")
    HK_ID (dist2Type,    "dist2_type")
    HK_ID (drive2,       "drive2")
    HK_ID (cutoff,       "cutoff")
    HK_ID (resonance,    "resonance")
    HK_ID (filterEnv,    "filter_env")
    HK_ID (filterDecay,  "filter_decay")
    HK_ID (keyTrack,     "key_track")
    HK_ID (octave,       "octave")
    HK_ID (fineTune,     "fine_tune")
    HK_ID (tailPitch,    "tail_pitch")
    HK_ID (lowCut,       "low_cut")
    HK_ID (highCut,      "high_cut")
    HK_ID (masterDrive,  "master_drive")
    HK_ID (velocity,     "velocity")
    HK_ID (output,       "output")
    #undef HK_ID
}

class HardKickProcessor : public juce::AudioProcessor,
                          private juce::AudioProcessorValueTreeState::Listener
{
public:
    HardKickProcessor();
    ~HardKickProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override                        { return hk::numPresets(); }
    int getCurrentProgram() override                     { return currentPreset; }
    void setCurrentProgram (int index) override          { applyPreset (index); }
    const juce::String getProgramName (int index) override { return hk::presetName (index); }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    hk::KickParams snapshotParams() const;
    void applyPreset (int index);
    void triggerAudition() { auditionRequested.store (true); }

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> paramVersion { 0 };
    std::atomic<int> noteCounter  { 0 };   // bumps on every played kick (for UI flash)

    // Note used for preview + the Play button. Key tracking with octave -3 maps
    // this to F1 (~43.65 Hz).
    static constexpr int auditionNote = 65;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged (const juce::String&, float) override { ++paramVersion; }

    float get (const char* id) const;

    hk::KickEngine engine;
    std::atomic<bool> auditionRequested { false };
    int currentPreset = 0;
    std::vector<float> mono;
    // std::string keys: ids are short enough for small-string storage, so lookups
    // on the audio thread don't allocate.
    std::map<std::string, std::atomic<float>*> raw;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HardKickProcessor)
};

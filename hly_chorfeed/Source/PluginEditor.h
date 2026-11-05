#pragma once
#include "PluginProcessor.h"

class HlyChorfeedAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit HlyChorfeedAudioProcessorEditor (HlyChorfeedAudioProcessor&);
    ~HlyChorfeedAudioProcessorEditor() override;
    void resized() override;

private:
    class Surface;
    std::unique_ptr<Surface> surface;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HlyChorfeedAudioProcessorEditor)
};

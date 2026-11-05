#include "../Source/PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class CaptureApp final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "hly_capture"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    void initialise(const juce::String& args) override
    {
        processor = std::make_unique<HlyChorfeedAudioProcessor>();
        processor->setRateAndBufferSizeDetails(48000, 256);
        processor->prepareToPlay(48000, 256);
        editor.reset(processor->createEditor());
        editor->setVisible(true);
        auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 2.0f);
        juce::File output(args.isEmpty() ? "/workspace/hly_chorfeed-preview.png" : args.unquoted());
        if (auto stream = output.createOutputStream()) {
            juce::PNGImageFormat png;
            if (!png.writeImageToStream(image, *stream)) setApplicationReturnValue(1);
        } else { setApplicationReturnValue(1); }
        quit();
    }
    void shutdown() override { editor.reset(); processor.reset(); }
private:
    std::unique_ptr<HlyChorfeedAudioProcessor> processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
};
START_JUCE_APPLICATION(CaptureApp)

// Development tool: renders the plugin editor to a PNG without a host.
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File out (argc > 1 ? juce::String (argv[1]) : juce::String ("editor.png"));
    const int preset = argc > 2 ? juce::String (argv[2]).getIntValue() : 0;

    HardKickProcessor proc;
    proc.setPlayConfigDetails (0, 2, 48000.0, 512);
    proc.applyPreset (preset);
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setBounds (ed->getLocalBounds());

    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat png;
    png.writeImageToStream (img, os);
    std::printf ("wrote %s (%dx%d)\n", out.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    ed.reset();
    return 0;
}

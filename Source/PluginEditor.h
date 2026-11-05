#pragma once

#include "PluginProcessor.h"

namespace hkui
{
    namespace col
    {
        const juce::Colour bg       { 0xff111114 };
        const juce::Colour panel    { 0xff1b1b21 };
        const juce::Colour panelHi  { 0xff24242c };
        const juce::Colour edge     { 0xff2e2e38 };
        const juce::Colour text     { 0xffe8e8ee };
        const juce::Colour textDim  { 0xff8a8a98 };
        const juce::Colour accent   { 0xffff4a1c };
        const juce::Colour accent2  { 0xffffb020 };
        const juce::Colour pitch    { 0xff3fd0ff };
        const juce::Colour track    { 0xff33333e };
    }

    juce::Font font (float size, bool bold = false);

    //==========================================================================
    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel();
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                               float startAngle, float endAngle, juce::Slider&) override;
        juce::Label* createSliderTextBox (juce::Slider&) override;
        juce::Font getLabelFont (juce::Label&) override;
        void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override        { return font (12.5f, true); }
        juce::Font getPopupMenuFont() override                       { return font (13.0f); }
        void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
        juce::Font getTextButtonFont (juce::TextButton&, int) override { return font (13.0f, true); }
    };

    //==========================================================================
    class KnobSlot : public juce::Component
    {
    public:
        KnobSlot (juce::AudioProcessorValueTreeState&, const char* paramId, const juce::String& name);
        void resized() override;
        juce::Slider slider;
        juce::Label label;
    private:
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    class ChoiceSlot : public juce::Component
    {
    public:
        ChoiceSlot (juce::AudioProcessorValueTreeState&, const char* paramId, const juce::String& name);
        void resized() override;
        juce::ComboBox box;
        juce::Label label;
    private:
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    class ToggleSlot : public juce::Component
    {
    public:
        ToggleSlot (juce::AudioProcessorValueTreeState&, const char* paramId, const juce::String& name);
        void resized() override;
        juce::ToggleButton button;
        juce::Label label;
    private:
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    };

    //==========================================================================
    class WaveformView : public juce::Component
    {
    public:
        std::function<void()> onClick;

        void setData (std::vector<float> wave, std::vector<float> pitch, double sampleRate);
        void flash()  { flashAmount = 1.0f; repaint(); }
        void tick();

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
        void mouseEnter (const juce::MouseEvent&) override { hover = true;  repaint(); }
        void mouseExit (const juce::MouseEvent&) override  { hover = false; repaint(); }

    private:
        std::vector<float> wave, pitch;
        double sr = 32000.0;
        float peak = 0.0f;
        float flashAmount = 0.0f;
        bool hover = false;
    };
}

//==============================================================================
class HardKickEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit HardKickEditor (HardKickProcessor&);
    ~HardKickEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Section
    {
        juce::String title;
        std::vector<juce::Component*> items;
        juce::Rectangle<int> area {};
    };

    void timerCallback() override;
    void refreshPreview();
    hkui::KnobSlot* knob (const char* id, const juce::String& name);
    void layoutRow (juce::Rectangle<int> row, std::initializer_list<Section*> sections);

    HardKickProcessor& proc;
    juce::TooltipWindow tooltips { this, 700 };
    hkui::LookAndFeel lnf;

    juce::ComboBox presetBox;
    juce::TextButton playButton { "PLAY" };
    hkui::WaveformView waveform;

    std::vector<std::unique_ptr<juce::Component>> slots;
    Section click, punch, pitch, tail, dist, filter, master;

    int lastParamVersion = -1;
    int lastNoteCounter = 0;
    juce::uint32 lastRenderMs = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HardKickEditor)
};

#include "PluginEditor.h"
#include <cmath>

namespace
{
constexpr float pi = juce::MathConstants<float>::pi;
const juce::Colour panelTop (0xff444548), panelBottom (0xff242527);
const juce::Colour slate (0xff323746), slateLight (0xff626b87);
const juce::Colour lettering (0xffd1d2d2), muted (0xff9da0a8), amber (0xffffb532);

juce::Font condensed (float height, bool bold = true)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain))
        .withHorizontalScale (0.78f);
}

void text (juce::Graphics& g, const juce::String& str, juce::Rectangle<float> area,
           float height = 11.0f, juce::Colour colour = lettering,
           juce::Justification align = juce::Justification::centred)
{
    g.setColour (colour);
    g.setFont (condensed (height));
    g.drawText (str, area, align, false);
}

void panel (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colour (0xff101114));
    g.fillRoundedRectangle (r, 4.0f);
    g.setGradientFill (juce::ColourGradient (panelTop, r.getX(), r.getY(),
                                           panelBottom, r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r.reduced (1.8f), 3.0f);
    g.setColour (juce::Colour (0xffa6a7a8));
    g.drawRoundedRectangle (r.reduced (0.7f), 3.5f, 1.0f);
    g.setColour (juce::Colour (0xff68696b));
    g.drawRoundedRectangle (r.reduced (2.2f), 2.0f, 0.65f);
}

void screen (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colour (0xff090b0e));
    g.fillRoundedRectangle (r, 2.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2e323f), r.getX(), r.getY(),
                                           juce::Colour (0xff171b25), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r.reduced (2.0f), 1.0f);
    g.setColour (slateLight);
    g.drawRoundedRectangle (r.reduced (1.0f), 1.5f, 0.7f);
}

class MorphineLook final : public juce::LookAndFeel_V4
{
public:
    MorphineLook()
    {
        setColour (juce::Label::textColourId, lettering);
        setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffbdc7e2));
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff282c36));
        setColour (juce::Slider::textBoxOutlineColourId, slateLight);
        setColour (juce::Slider::textBoxHighlightColourId, amber.withAlpha (0.35f));
        setColour (juce::ComboBox::textColourId, juce::Colour (0xffc0cbe8));
        setColour (juce::ComboBox::backgroundColourId, slate);
        setColour (juce::ComboBox::outlineColourId, slateLight);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff252833));
        setColour (juce::PopupMenu::textColourId, lettering);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff62502d));
        setColour (juce::PopupMenu::highlightedTextColourId, amber);
        setColour (juce::TooltipWindow::backgroundColourId, slate);
        setColour (juce::TooltipWindow::textColourId, lettering);
        setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff20242e));
        setColour (juce::TextEditor::textColourId, lettering);
        setColour (juce::TextEditor::highlightColourId, amber.withAlpha (0.3f));
    }

    juce::Font getLabelFont (juce::Label&) override { return condensed (11.0f); }
    juce::Font getComboBoxFont (juce::ComboBox& box) override
    {
        return condensed (box.getHeight() > 24 ? 15.0f : 11.0f);
    }
    juce::Font getPopupMenuFont() override { return condensed (14.0f); }
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (6, 1, box.getWidth() - 23, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox&) override
    {
        const juce::Rectangle<float> r (0.0f, 0.0f, (float) width, (float) height);
        screen (g, r);
        g.setColour (juce::Colour (0xffbac5e7));
        const float cx = width - 11.0f, cy = height * 0.5f;
        juce::Path p;
        p.addTriangle (cx - 3.5f, cy - 2.0f, cx + 3.5f, cy - 2.0f, cx, cy + 2.5f);
        g.fillPath (p);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float position, float start, float end, juce::Slider&) override
    {
        const float d = juce::jmin ((float) w, (float) h) - 5.0f;
        const float cx = x + w * 0.5f, cy = y + h * 0.5f;
        const juce::Rectangle<float> outer (cx - d / 2.0f, cy - d / 2.0f, d, d);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (outer.translated (0, 1.4f).expanded (1.0f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff515153), cx, outer.getY(),
                                               juce::Colour (0xff0e0e10), cx, outer.getBottom(), false));
        g.fillEllipse (outer);
        g.setColour (juce::Colour (0xff070809));
        g.drawEllipse (outer.reduced (0.6f), 1.0f);
        const float arcRadius = d * 0.43f;
        juce::Path backgroundArc, activeArc;
        backgroundArc.addCentredArc (cx, cy, arcRadius, arcRadius, 0, start, end, true);
        activeArc.addCentredArc (cx, cy, arcRadius, arcRadius, 0, start,
                                start + position * (end - start), true);
        g.setColour (juce::Colour (0xff343439));
        g.strokePath (backgroundArc, juce::PathStrokeType (2.1f));
        g.setColour (amber);
        g.strokePath (activeArc, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        // A subtle scalloped cap, matching the compact legacy control rather than chrome hardware.
        const float radius = d * 0.325f;
        juce::Path cap;
        for (int i = 0; i < 80; ++i)
        {
            const float a = i * juce::MathConstants<float>::twoPi / 80.0f;
            const float r = radius * (1.0f - 0.045f * std::cos (a * 10.0f));
            const float px = cx + std::sin (a) * r, py = cy - std::cos (a) * r;
            if (i == 0) cap.startNewSubPath (px, py); else cap.lineTo (px, py);
        }
        cap.closeSubPath();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc2c3c2), cx - radius * .6f,
                                               cy - radius, juce::Colour (0xff626568),
                                               cx + radius * .7f, cy + radius, false));
        g.fillPath (cap);
        g.setColour (juce::Colour (0xff252729));
        g.strokePath (cap, juce::PathStrokeType (0.8f));
        const float angle = start + position * (end - start);
        g.setColour (juce::Colour (0xff27292a));
        g.drawLine (cx + std::sin (angle) * radius * .50f,
                    cy - std::cos (angle) * radius * .50f,
                    cx + std::sin (angle) * radius * .92f,
                    cy - std::cos (angle) * radius * .92f, 1.7f);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float, float, juce::Slider::SliderStyle style,
                           juce::Slider& slider) override
    {
        const bool vertical = style == juce::Slider::LinearVertical;
        const float midX = x + width * .5f, midY = y + height * .5f;
        const auto track = vertical ? juce::Rectangle<float> (midX - 2.5f, (float) y, 5, (float) height)
                                    : juce::Rectangle<float> ((float) x, midY - 2.5f, (float) width, 5);
        g.setColour (juce::Colour (0xff0a0b0d));
        g.fillRoundedRectangle (track.expanded (1.0f), 1.0f);
        g.setColour (juce::Colour (0xff5a5b5e));
        g.drawRoundedRectangle (track, .7f, .6f);
        g.setColour (amber.withAlpha (.90f));
        if (vertical)
            g.fillRect (midX - .65f, sliderPos, 1.3f, juce::jmax (0.0f, y + height - sliderPos));
        else
        {
            const bool bipolar = slider.getMinimum() < 0;
            const float zero = bipolar ? x + width * .5f : (float) x;
            g.fillRect (juce::jmin (zero, sliderPos), midY - .65f,
                        std::abs (sliderPos - zero), 1.3f);
            if (bipolar)
            {
                g.setColour (muted);
                g.drawVerticalLine (juce::roundToInt (zero), midY - 7, midY - 4);
            }
        }
        const juce::Rectangle<float> thumb = vertical
            ? juce::Rectangle<float> (midX - 7.0f, sliderPos - 3.0f, 14.0f, 6.0f)
            : juce::Rectangle<float> (sliderPos - 3.0f, midY - 6.0f, 6.0f, 12.0f);
        g.setColour (juce::Colours::black.withAlpha (.7f));
        g.fillRoundedRectangle (thumb.translated (0, 1).expanded (.7f), 1.3f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb0b0ac), thumb.getX(), thumb.getY(),
                                               juce::Colour (0xff6d7071), thumb.getX(), thumb.getBottom(), false));
        g.fillRoundedRectangle (thumb, .8f);
        g.setColour (juce::Colour (0xffc4c4be));
        g.drawHorizontalLine (juce::roundToInt (thumb.getY() + .5f), thumb.getX() + 1, thumb.getRight() - 1);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (.5f);
        g.setColour (juce::Colour (0xff090a0c));
        g.fillRoundedRectangle (r, 2.0f);
        auto top = b.getToggleState() ? amber : juce::Colour (over ? 0xff666770 : 0xff525357);
        auto bottom = b.getToggleState() ? juce::Colour (0xff9f6817) : juce::Colour (0xff2d2f34);
        if (down) top = top.darker (.2f);
        g.setGradientFill (juce::ColourGradient (top, 0, r.getY(), bottom, 0, r.getBottom(), false));
        g.fillRoundedRectangle (r.reduced (2), 1.0f);
        g.setColour (juce::Colour (0xff85878d));
        g.drawRoundedRectangle (r.reduced (1.0f), 1.5f, .7f);
    }
    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        text (g, b.getButtonText(), b.getLocalBounds().toFloat(), 12,
              b.getToggleState() ? juce::Colour (0xff302814) : lettering);
    }
};
}

class HlyChorfeedAudioProcessorEditor::Surface final : public juce::Component, private juce::Timer
{
public:
    explicit Surface (HlyChorfeedAudioProcessor& p) : processor (p), tooltip (this, 650)
    {
        setLookAndFeel (&look);
        setOpaque (true);
        auto knob = [this] (const char* id, const char* title, juce::Rectangle<int> r, const char* hint)
        {
            return addSlider (id, title, r, juce::Slider::RotaryHorizontalVerticalDrag, hint);
        };
        addChoice ("filterMode", { "Smooth LP", "Smooth HP", "Smooth BP", "Driven Ladder" }, { 109, 61, 165, 21 });
        addChoice ("delayMode", { "Ensemble", "Digital", "Dark" }, { 391, 61, 165, 21 });
        addChoice ("distortionMode", { "Soft Saturation", "Hard Clip", "Asymmetric", "Wavefold" }, { 673, 61, 165, 21 });
        knob ("cutoff", "CUTOFF", { 24, 100, 77, 75 }, "Filter cutoff frequency. Double-click any control to reset; shift-drag for fine adjustment.");
        knob ("resonance", "RESONANCE", { 110, 100, 77, 75 }, "Resonant emphasis near the cutoff. High settings can ring inside feedback loops.");
        knob ("filterDrive", "DRIVE", { 196, 100, 77, 75 }, "Drive into the filter, in decibels.");
        knob ("time", "TIME", { 304, 100, 60, 75 }, "Base delay time, 1–2000 ms. Short times create chorus and comb textures.");
        knob ("rate", "RATE", { 371, 100, 60, 75 }, "Delay modulation speed in hertz.");
        knob ("depth", "DEPTH", { 438, 100, 60, 75 }, "Delay modulation depth in milliseconds.");
        knob ("width", "WIDTH", { 505, 100, 60, 75 }, "Stereo spread of the modulated delay voices.");
        knob ("drive", "DRIVE", { 588, 100, 77, 75 }, "Distortion input drive in decibels.");
        knob ("tone", "TONE", { 674, 100, 77, 75 }, "Post-distortion tone: left darkens, right opens the high frequencies.");
        knob ("bias", "BIAS", { 760, 100, 77, 75 }, "Waveshaper asymmetry. The circuit removes residual DC before feeding back.");

        static const char* names[] = { "Filter", "Delay", "Distortion" };
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col)
            {
                const juce::String id = "m" + juce::String (row) + juce::String (col);
                const auto hint = juce::String (names[row]) + " output → " + names[col]
                    + " input. Negative amounts invert polarity; centre is no connection.";
                addSlider (id, {}, { 105 + col * 139, 337 + row * 40, 129, 37 },
                           juce::Slider::LinearHorizontal, hint);
            }
        for (int i = 0; i < 3; ++i)
        {
            addSlider ("in" + juce::String (i), {}, { 555 + i * 43, 317, 37, 126 },
                       juce::Slider::LinearVertical,
                       juce::String ("Dry input sent directly to ") + names[i] + ".");
            addSlider ("out" + juce::String (i), {}, { 711 + i * 43, 317, 37, 126 },
                       juce::Slider::LinearVertical,
                       juce::String (names[i]) + " contribution to the wet output.");
        }
        knob ("feedback", "FEEDBACK", { 18, 484, 126, 42 }, "Master feedback multiplier. Above unity can sustain or grow; use Panic to clear all stored sound.");
        knob ("mix", "MIX", { 149, 484, 113, 42 }, "Dry/wet blend. The Lush preset starts with dry signal mixed with the chorus.");
        knob ("output", "OUTPUT", { 267, 484, 126, 42 }, "Final output trim in decibels.");

        freeze.setButtonText ("FREEZE");
        freeze.setClickingTogglesState (true);
        freeze.setTooltip ("Stops new audio from entering the feedback network. Existing sound circulates according to the matrix; decay depends on your settings.");
        addAndMakeVisible (freeze);
        freeze.setBounds (405, 488, 77, 31);
        freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "freeze", freeze);
        panic.setButtonText ("PANIC");
        panic.setTooltip ("Clear all delay and feedback memory immediately. This does not alter your parameter settings.");
        panic.onClick = [this] { processor.requestPanic(); };
        addAndMakeVisible (panic);
        panic.setBounds (490, 488, 70, 31);

        addAndMakeVisible (presets);
        for (int i = 0; i < processor.getNumPrograms(); ++i)
            presets.addItem (processor.getProgramName (i), i + 1);
        presets.setSelectedId (processor.getCurrentProgram() + 1, juce::dontSendNotification);
        presets.onChange = [this]
        {
            if (presets.getSelectedId() > 0)
                processor.setCurrentProgram (presets.getSelectedId() - 1);
        };
        presets.setTooltip ("Factory starting points. Selecting a preset replaces the current settings.");
        presets.setBounds (332, 10, 420, 28);
        previous.setButtonText ("<"); next.setButtonText (">");
        addAndMakeVisible (previous); addAndMakeVisible (next);
        previous.setBounds (296, 10, 29, 28); next.setBounds (759, 10, 29, 28);
        previous.onClick = [this] { changePreset (-1); };
        next.onClick = [this] { changePreset (1); };
        startTimerHz (25);
    }

    ~Surface() override { stopTimer(); setLookAndFeel (nullptr); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff17191b));
        panel (g, { 5, 5, 850, 39 });
        // A faint diagonal sheen echoes Morphine's restrained header treatment.
        juce::Path sheen;
        sheen.addTriangle (170, 7, 292, 7, 292, 39);
        g.setColour (juce::Colours::white.withAlpha (.035f)); g.fillPath (sheen);
        text (g, "hly_chorfeed", { 19, 10, 260, 28 }, 26, lettering, juce::Justification::centredLeft);
        text (g, "STEREO FX", { 796, 10, 51, 28 }, 10, muted);
        for (int i = 0; i < 3; ++i)
        {
            const float x = 10.0f + i * 282.0f;
            panel (g, { x, 52, 276, 222 });
            g.setColour (juce::Colour (0xff18191b)); g.fillRect (x + 5, 85.0f, 266.0f, 1.0f);
            led (g, { x + 11, 65, 7, 7 });
            text (g, i == 0 ? "FILTER" : i == 1 ? "DELAY" : "DISTORTION",
                  { x + 23, 61, i == 2 ? 76.0f : 70.0f, 20 }, 13, lettering, juce::Justification::centredLeft);
        }
        drawFilterGraph (g, { 22, 186, 252, 74 });
        drawDelayGraph (g, { 304, 186, 252, 74 });
        drawDistortionGraph (g, { 586, 186, 252, 74 });

        panel (g, { 10, 282, 526, 184 });
        screen (g, { 20, 291, 151, 20 });
        text (g, "FEEDBACK MATRIX", { 25, 292, 141, 18 }, 12, juce::Colour (0xffc2cbe4));
        screen (g, { 20, 316, 506, 139 });
        text (g, "FROM / TO", { 22, 317, 75, 18 }, 10, muted);
        const char* matrixNames[] = { "FILTER", "DELAY", "DIST" };
        for (int i = 0; i < 3; ++i)
        {
            text (g, matrixNames[i], { 105.0f + i * 139, 317, 129, 18 }, 11);
            text (g, matrixNames[i], { 25, 341.0f + i * 40, 69, 25 }, 11, lettering, juce::Justification::centredLeft);
        }
        g.setColour (juce::Colour (0xff707783).withAlpha (.55f));
        for (int i = 0; i < 4; ++i)
            g.drawHorizontalLine (336 + i * 40, 22, 524);
        for (int i = 0; i < 4; ++i)
            g.drawVerticalLine (98 + i * 139, 318, 454);
        for (int group = 0; group < 2; ++group)
        {
            const float x = 544.0f + group * 156;
            panel (g, { x, 282, 150, 184 });
            led (g, { x + 10, 297, 6, 6 });
            text (g, group == 0 ? "INPUT SEND" : "OUTPUT MIX", { x + 21, 291, 120, 18 }, 12,
                  lettering, juce::Justification::centredLeft);
            const char* faderNames[] = { "FLT", "DLY", "DST" };
            for (int i = 0; i < 3; ++i)
            {
                for (int tick = 0; tick <= 5; ++tick)
                {
                    g.setColour (juce::Colour (0xff777b80));
                    const float ty = 324.0f + tick * 18.0f;
                    const float tx = x + 25 + i * 43;
                    g.drawLine (tx - 11, ty, tx - 7, ty, .6f);
                    g.drawLine (tx + 7, ty, tx + 11, ty, .6f);
                }
                text (g, faderNames[i], { x + 10 + i * 43, 445, 39, 14 }, 10, muted);
            }
        }
        panel (g, { 10, 474, 840, 57 });
        screen (g, { 576, 485, 260, 36 });
        drawMeter (g, 588, 493, peakLeft, "L");
        drawMeter (g, 588, 507, peakRight, "R");
        if (peakLeft >= 0.999f || peakRight >= 0.999f)
            text (g, "!", { 823, 485, 10, 36 }, 13, amber);
    }

private:
    struct SliderControl
    {
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    struct ChoiceControl
    {
        std::unique_ptr<juce::ComboBox> combo;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };
    HlyChorfeedAudioProcessor& processor;
    MorphineLook look;
    juce::TooltipWindow tooltip;
    std::vector<SliderControl> sliders;
    std::vector<ChoiceControl> choices;
    juce::ComboBox presets;
    juce::TextButton previous, next, freeze, panic;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment;
    float peakLeft = 0, peakRight = 0, phase = 0;

    static void led (juce::Graphics& g, juce::Rectangle<float> r)
    {
        g.setColour (juce::Colour (0xff8b671a)); g.fillRect (r.expanded (1));
        g.setColour (juce::Colour (0xffffce48)); g.fillRect (r);
        g.setColour (juce::Colour (0xffffe99c)); g.fillRect (r.reduced (1.5f));
    }
    float value (const char* id) const
    {
        if (auto* v = processor.apvts.getRawParameterValue (id)) return v->load();
        return 0;
    }
    juce::Slider* addSlider (const juce::String& id, const juce::String& name,
                            juce::Rectangle<int> bounds, juce::Slider::SliderStyle style,
                            const juce::String& hint)
    {
        SliderControl control;
        control.slider = std::make_unique<juce::Slider>();
        auto& s = *control.slider;
        s.setSliderStyle (style);
        s.setTooltip (hint);
        s.setName (name.isEmpty() ? id : name);
        s.setVelocityBasedMode (false);
        s.setVelocityModeParameters (0.5, 1, 0.0, true, juce::ModifierKeys::shiftModifier);
        s.setScrollWheelEnabled (false);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false,
                           style == juce::Slider::LinearVertical ? 34 : style == juce::Slider::LinearHorizontal ? 61 : 70, 16);
        s.setRotaryParameters (pi * 1.22f, pi * 2.78f, true);
        const bool footer = bounds.getY() >= 480;
        if (!name.isEmpty())
        {
            control.label = std::make_unique<juce::Label>();
            control.label->setText (name, juce::dontSendNotification);
            control.label->setJustificationType (juce::Justification::centred);
            addAndMakeVisible (*control.label);
            if (footer)
            {
                control.label->setBounds (bounds.getX() + 43, bounds.getY() - 1, bounds.getWidth() - 43, 16);
                s.setTextBoxStyle (juce::Slider::TextBoxRight, false, bounds.getWidth() - 47, 16);
            }
            else
            {
                control.label->setBounds (bounds.removeFromTop (16));
            }
        }
        s.setBounds (bounds);
        addAndMakeVisible (s);
        control.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, id, s);
        // Override the attachment's generic parameter formatting with readable units.
        s.textFromValueFunction = [id] (double v)
        {
            if (id == "cutoff") return v >= 1000 ? juce::String (v / 1000, 2) + " kHz" : juce::String (v, 0) + " Hz";
            if (id == "time" || id == "depth") return juce::String (v, v >= 100 ? 0 : 1) + " ms";
            if (id == "rate") return juce::String (v, 2) + " Hz";
            if (id == "drive" || id == "filterDrive" || id == "output") return juce::String (v, 1) + " dB";
            if (id == "width" || id == "mix") return juce::String (v * 100, 0) + "%";
            if (id.startsWithChar ('m') && id != "mix") return (v >= 0 ? "+" : "") + juce::String (v, 2);
            return juce::String (v, 2);
        };
        s.valueFromTextFunction = [id] (const juce::String& entered)
        {
            auto str = entered.trim().toLowerCase();
            double v = str.getDoubleValue();
            if (id == "cutoff" && str.containsChar ('k')) v *= 1000;
            if (id == "mix" || id == "width") v /= 100;
            return v;
        };
        s.updateText();
        if (auto* parameter = processor.apvts.getParameter (id))
            s.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));
        auto* result = control.slider.get();
        sliders.push_back (std::move (control));
        return result;
    }
    void addChoice (const juce::String& id, const juce::StringArray& entries, juce::Rectangle<int> bounds)
    {
        ChoiceControl control;
        control.combo = std::make_unique<juce::ComboBox>();
        control.combo->addItemList (entries, 1);
        control.combo->setBounds (bounds);
        control.combo->setTooltip ("Choose the processing algorithm for this section.");
        addAndMakeVisible (*control.combo);
        control.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, id, *control.combo);
        choices.push_back (std::move (control));
    }
    void changePreset (int delta)
    {
        const int count = processor.getNumPrograms();
        if (count > 0)
        {
            const int selected = (processor.getCurrentProgram() + delta + count) % count;
            processor.setCurrentProgram (selected);
            presets.setSelectedId (selected + 1, juce::dontSendNotification);
        }
    }
    static void graphGrid (juce::Graphics& g, juce::Rectangle<float> r)
    {
        screen (g, r);
        g.setColour (juce::Colour (0xff8590ac).withAlpha (.15f));
        for (int i = 1; i < 8; ++i)
            g.drawVerticalLine (juce::roundToInt (r.getX() + r.getWidth() * i / 8), r.getY() + 2, r.getBottom() - 2);
        for (int i = 1; i < 4; ++i)
            g.drawHorizontalLine (juce::roundToInt (r.getY() + r.getHeight() * i / 4), r.getX() + 2, r.getRight() - 2);
    }
    void drawFilterGraph (juce::Graphics& g, juce::Rectangle<float> r)
    {
        graphGrid (g, r);
        auto plot = r.reduced (5);
        const float cutoff = value ("cutoff"), resonance = value ("resonance");
        const int mode = juce::roundToInt (value ("filterMode"));
        juce::Path path;
        for (int i = 0; i <= 180; ++i)
        {
            const float f = 20.0f * std::pow (1000.0f, i / 180.0f);
            const float ratio = f / juce::jmax (20.0f, cutoff);
            const float damping = 2.0f - 1.985f * resonance;
            const float q = 1.0f / damping;
            const float den = std::sqrt (std::pow (1 - ratio * ratio, 2.0f) + ratio * ratio / (q * q));
            float magnitude = 1.0f / juce::jmax (.001f, den);
            if (mode == 1) magnitude *= ratio * ratio;
            else if (mode == 2) magnitude *= ratio * std::sqrt (juce::jmax (.08f, damping));
            else if (mode == 3) magnitude = magnitude * magnitude;
            const float db = juce::Decibels::gainToDecibels (magnitude, -50.0f);
            const float x = plot.getX() + plot.getWidth() * i / 180;
            const float y = plot.getY() + plot.getHeight() * juce::jlimit (0.0f, 1.0f, (18 - db) / 66.0f);
            if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
        }
        g.setColour (amber); g.strokePath (path, juce::PathStrokeType (1.4f));
        text (g, "RESPONSE", { r.getX() + 6, r.getBottom() - 13, 70, 10 }, 8, muted, juce::Justification::centredLeft);
    }
    void drawDelayGraph (juce::Graphics& g, juce::Rectangle<float> r)
    {
        graphGrid (g, r);
        const float depth = juce::jlimit (.02f, .4f, value ("depth") / 30.0f);
        for (int channel = 0; channel < 2; ++channel)
        {
            juce::Path path;
            for (int i = 0; i <= 150; ++i)
            {
                const float x = r.getX() + 3 + (r.getWidth() - 6) * i / 150;
                const float a = i / 150.0f * pi * 4 + phase + channel * pi * .5f * value ("width");
                const float y = r.getCentreY() - std::sin (a) * r.getHeight() * depth;
                if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
            }
            g.setColour (amber.withAlpha (channel == 0 ? .95f : .43f));
            g.strokePath (path, juce::PathStrokeType (1.25f));
        }
        text (g, "MODULATION", { r.getX() + 6, r.getBottom() - 13, 90, 10 }, 8, muted, juce::Justification::centredLeft);
    }
    void drawDistortionGraph (juce::Graphics& g, juce::Rectangle<float> r)
    {
        graphGrid (g, r);
        const int mode = juce::roundToInt (value ("distortionMode"));
        const float gain = juce::Decibels::decibelsToGain (value ("drive"));
        const float bias = value ("bias");
        auto shape = [mode] (float x)
        {
            if (mode == 1) return juce::jlimit (-.75f, .75f, x);
            if (mode == 2) return x >= 0 ? std::tanh (x) : .65f * std::tanh (1.5f * x);
            if (mode == 3) return 2.0f / pi * std::asin (std::sin (x * pi * .5f));
            return std::tanh (x);
        };
        juce::Path path;
        for (int i = 0; i <= 180; ++i)
        {
            const float input = i / 90.0f - 1.0f;
            const float y = shape (input * gain + bias) - shape (bias);
            const float px = r.getX() + 4 + (r.getWidth() - 8) * i / 180;
            const float py = r.getCentreY() - juce::jlimit (-1.1f, 1.1f, y) * (r.getHeight() - 12) * .4f;
            if (i == 0) path.startNewSubPath (px, py); else path.lineTo (px, py);
        }
        g.setColour (amber); g.strokePath (path, juce::PathStrokeType (1.4f));
        text (g, "TRANSFER", { r.getX() + 6, r.getBottom() - 13, 70, 10 }, 8, muted, juce::Justification::centredLeft);
    }
    static void drawMeter (juce::Graphics& g, int x, int y, float peak, const char* name)
    {
        text (g, name, { (float) x, (float) y - 1, 9, 10 }, 9, muted);
        const float db = juce::Decibels::gainToDecibels (peak, -60.0f);
        const int lit = juce::roundToInt (juce::jlimit (0.0f, 1.0f, (db + 60) / 60) * 39);
        for (int i = 0; i < 39; ++i)
        {
            g.setColour (i < lit ? (i > 35 ? juce::Colour (0xffff7842) : amber) : juce::Colour (0xff383a3c));
            g.fillRect (x + 14 + i * 5, y, 3, 7);
        }
    }
    void timerCallback() override
    {
        peakLeft = juce::jmax (processor.peakL.load(), peakLeft * .88f);
        peakRight = juce::jmax (processor.peakR.load(), peakRight * .88f);
        phase += value ("rate") * juce::MathConstants<float>::twoPi / 25.0f;
        phase = std::fmod (phase, juce::MathConstants<float>::twoPi);
        presets.setSelectedId (processor.getCurrentProgram() + 1, juce::dontSendNotification);
        repaint();
    }
};

HlyChorfeedAudioProcessorEditor::HlyChorfeedAudioProcessorEditor (HlyChorfeedAudioProcessor& p)
    : AudioProcessorEditor (&p), surface (std::make_unique<Surface> (p))
{
    addAndMakeVisible (*surface);
    setResizable (true, true);
    setResizeLimits (688, 432, 1290, 810);
    if (auto* sizeLimits = getConstrainer()) sizeLimits->setFixedAspectRatio (860.0 / 540.0);
    setSize (860, 540);
}

HlyChorfeedAudioProcessorEditor::~HlyChorfeedAudioProcessorEditor() = default;

void HlyChorfeedAudioProcessorEditor::resized()
{
    surface->setBounds (0, 0, 860, 540);
    surface->setTransform (juce::AffineTransform::scale ((float) getWidth() / 860.0f,
                                                      (float) getHeight() / 540.0f));
}

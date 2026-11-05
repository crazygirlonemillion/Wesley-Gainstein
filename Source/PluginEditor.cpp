#include "PluginEditor.h"

using namespace juce;

namespace hkui
{

Font font (float size, bool bold)
{
    return Font (FontOptions (size, bold ? Font::bold : Font::plain));
}

//==============================================================================
LookAndFeel::LookAndFeel()
{
    setColour (ResizableWindow::backgroundColourId, col::bg);
    setColour (Label::textColourId, col::text);
    setColour (Slider::textBoxTextColourId, col::textDim);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Slider::textBoxHighlightColourId, col::accent.withAlpha (0.4f));
    setColour (TextEditor::textColourId, col::text);
    setColour (TextEditor::backgroundColourId, col::panelHi);
    setColour (TextEditor::highlightColourId, col::accent.withAlpha (0.4f));
    setColour (TextEditor::outlineColourId, Colours::transparentBlack);
    setColour (TextEditor::focusedOutlineColourId, col::accent);
    setColour (CaretComponent::caretColourId, col::accent);
    setColour (ComboBox::backgroundColourId, col::panelHi);
    setColour (ComboBox::textColourId, col::text);
    setColour (ComboBox::outlineColourId, col::edge);
    setColour (ComboBox::arrowColourId, col::accent);
    setColour (PopupMenu::backgroundColourId, col::panel);
    setColour (PopupMenu::textColourId, col::text);
    setColour (PopupMenu::highlightedBackgroundColourId, col::accent);
    setColour (PopupMenu::highlightedTextColourId, Colours::black);
    setColour (TextButton::buttonColourId, col::accent);
    setColour (TextButton::textColourOffId, Colours::black);
    setColour (TextButton::textColourOnId, Colours::black);
    setColour (TooltipWindow::backgroundColourId, col::panelHi);
    setColour (TooltipWindow::textColourId, col::text);
}

void LookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                    float a0, float a1, Slider& s)
{
    auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float size = jmin (bounds.getWidth(), bounds.getHeight());
    auto r = Rectangle<float> (size, size).withCentre (bounds.getCentre());
    const float radius = size * 0.5f;
    const auto c = r.getCentre();
    const float angle = a0 + pos * (a1 - a0);
    const float lw = jmax (2.5f, radius * 0.13f);
    const float arcR = radius - lw * 0.5f;

    Path trackPath;
    trackPath.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, a0, a1, true);
    g.setColour (col::track);
    g.strokePath (trackPath, PathStrokeType (lw, PathStrokeType::curved, PathStrokeType::rounded));

    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float from = bipolar ? a0 + (float) s.valueToProportionOfLength (0.0) * (a1 - a0) : a0;
    if (std::abs (angle - from) > 0.001f)
    {
        Path valuePath;
        valuePath.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, jmin (from, angle), jmax (from, angle), true);
        const bool hot = s.isMouseOverOrDragging();
        g.setGradientFill (ColourGradient (col::accent2, c.x - radius, c.y + radius,
                                           col::accent, c.x + radius, c.y - radius, false));
        if (hot) g.setOpacity (1.0f);
        g.strokePath (valuePath, PathStrokeType (lw, PathStrokeType::curved, PathStrokeType::rounded));
    }

    auto body = r.reduced (lw * 1.9f);
    g.setGradientFill (ColourGradient (Colour (0xff3a3a46), body.getCentreX(), body.getY(),
                                       Colour (0xff1e1e25), body.getCentreX(), body.getBottom(), false));
    g.fillEllipse (body);
    g.setColour (Colour (0xff4a4a58).withAlpha (0.7f));
    g.drawEllipse (body.reduced (0.5f), 1.0f);

    const float pl = body.getWidth() * 0.5f;
    Path pointer;
    pointer.addRoundedRectangle (-1.4f, -pl + 2.5f, 2.8f, pl * 0.48f, 1.4f);
    g.setColour (Colours::white.withAlpha (0.92f));
    g.fillPath (pointer, AffineTransform::rotation (angle).translated (c.x, c.y));
}

Font LookAndFeel::getLabelFont (Label& l)
{
    if (dynamic_cast<Slider*> (l.getParentComponent()) != nullptr)
        return font (11.0f);
    return l.getFont();
}

Label* LookAndFeel::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (font (11.0f));
    l->setColour (Label::textColourId, col::textDim);
    l->setColour (Label::outlineColourId, Colours::transparentBlack);
    l->setColour (Label::backgroundColourId, Colours::transparentBlack);
    l->setColour (Label::textWhenEditingColourId, col::text);
    l->setColour (Label::backgroundWhenEditingColourId, col::panelHi);
    l->setColour (Label::outlineWhenEditingColourId, col::accent);
    return l;
}

void LookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (box.isMouseOver() ? col::accent.withAlpha (0.7f) : col::edge);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);

    Path arrow;
    const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (col::accent);
    g.fillPath (arrow);
}

void LookAndFeel::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (6, 1, box.getWidth() - 24, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (Justification::centredLeft);
}

void LookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat();
    auto pill = Rectangle<float> (40.0f, 20.0f).withCentre (r.getCentre());
    const bool on = b.getToggleState();
    g.setColour (on ? col::accent : col::track);
    g.fillRoundedRectangle (pill, 10.0f);
    if (highlighted)
    {
        g.setColour (Colours::white.withAlpha (0.15f));
        g.drawRoundedRectangle (pill, 10.0f, 1.0f);
    }
    auto knobR = Rectangle<float> (16.0f, 16.0f)
                     .withCentre ({ on ? pill.getRight() - 10.0f : pill.getX() + 10.0f, pill.getCentreY() });
    g.setColour (on ? Colours::white : col::textDim);
    g.fillEllipse (knobR);

    g.setColour (on ? col::text : col::textDim);
    g.setFont (font (10.5f, true));
    g.drawText (on ? "ON" : "OFF", r.withTop (pill.getBottom() + 3.0f).withHeight (14.0f), Justification::centredTop);
}

void LookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour& base, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    auto c = base;
    if (down)             c = c.darker (0.25f);
    else if (highlighted) c = c.brighter (0.15f);
    g.setColour (c);
    g.fillRoundedRectangle (r, 6.0f);
}

//==============================================================================
static void styleLabel (Label& l, const String& name)
{
    l.setText (name.toUpperCase(), dontSendNotification);
    l.setJustificationType (Justification::centred);
    l.setFont (font (10.5f, true));
    l.setColour (Label::textColourId, col::text.withAlpha (0.85f));
    l.setInterceptsMouseClicks (false, false);
}

KnobSlot::KnobSlot (AudioProcessorValueTreeState& state, const char* paramId, const String& name)
{
    slider.setSliderStyle (Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (Slider::TextBoxBelow, false, 78, 15);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (220);
    slider.setVelocityBasedMode (false);
    slider.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    slider.setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    slider.setColour (Slider::textBoxTextColourId, col::textDim);
    styleLabel (label, name);
    addAndMakeVisible (label);
    addAndMakeVisible (slider);
    attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);
    if (auto* p = state.getParameter (paramId))
        slider.setTooltip (p->getName (64) + "  (double-click to reset, cmd-drag for fine)");
}

void KnobSlot::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromTop (14));
    slider.setBounds (b);
}

ChoiceSlot::ChoiceSlot (AudioProcessorValueTreeState& state, const char* paramId, const String& name)
{
    if (auto* p = dynamic_cast<AudioParameterChoice*> (state.getParameter (paramId)))
        box.addItemList (p->choices, 1);
    box.setJustificationType (Justification::centredLeft);
    styleLabel (label, name);
    addAndMakeVisible (label);
    addAndMakeVisible (box);
    attachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (state, paramId, box);
}

void ChoiceSlot::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromTop (14));
    box.setBounds (b.withSizeKeepingCentre (jmin (b.getWidth() - 6, 92), 24).translated (0, -6));
}

ToggleSlot::ToggleSlot (AudioProcessorValueTreeState& state, const char* paramId, const String& name)
{
    styleLabel (label, name);
    addAndMakeVisible (label);
    addAndMakeVisible (button);
    attachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (state, paramId, button);
}

void ToggleSlot::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromTop (14));
    button.setBounds (b.withSizeKeepingCentre (jmin (b.getWidth(), 60), 44).translated (0, -4));
}

//==============================================================================
void WaveformView::setData (std::vector<float> w, std::vector<float> p, double sampleRate)
{
    wave = std::move (w);
    pitch = std::move (p);
    sr = sampleRate;
    peak = 0.0f;
    for (float s : wave) peak = jmax (peak, std::abs (s));
    repaint();
}

void WaveformView::tick()
{
    if (flashAmount > 0.0f)
    {
        flashAmount = jmax (0.0f, flashAmount - 0.12f);
        repaint();
    }
}

void WaveformView::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b0b0e));
    g.fillRoundedRectangle (r, 8.0f);

    auto plot = r.reduced (10.0f, 8.0f).withTrimmedBottom (14.0f);
    const float midY = plot.getCentreY();
    const float halfH = plot.getHeight() * 0.5f;
    const int n = (int) wave.size();
    const double lenSec = n / sr;

    // time grid
    if (n > 1)
    {
        const double step = lenSec > 1.5 ? 0.25 : (lenSec > 0.6 ? 0.1 : 0.05);
        g.setFont (font (10.0f));
        for (double t = 0.0; t <= lenSec + 1e-9; t += step)
        {
            const float xx = plot.getX() + (float) (t / lenSec) * plot.getWidth();
            g.setColour (Colours::white.withAlpha (0.05f));
            g.drawVerticalLine (roundToInt (xx), plot.getY(), plot.getBottom());
            g.setColour (col::textDim.withAlpha (0.8f));
            g.drawText (String (roundToInt (t * 1000.0)) + " ms",
                        Rectangle<float> (xx - 30.0f, plot.getBottom() + 1.0f, 60.0f, 13.0f), Justification::centred);
        }
    }

    // 0 dBFS lines and centre
    g.setColour (Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (roundToInt (midY), plot.getX(), plot.getRight());
    const float dashes[] = { 3.0f, 4.0f };
    g.setColour (col::accent.withAlpha (0.25f));
    g.drawDashedLine ({ plot.getX(), plot.getY(), plot.getRight(), plot.getY() }, dashes, 2, 1.0f);
    g.drawDashedLine ({ plot.getX(), plot.getBottom(), plot.getRight(), plot.getBottom() }, dashes, 2, 1.0f);

    if (n > 1)
    {
        // waveform (min/max per pixel column)
        const int cols = jmax (1, (int) plot.getWidth());
        Path fill;
        std::vector<float> mins ((size_t) cols), maxs ((size_t) cols);
        for (int c = 0; c < cols; ++c)
        {
            const int a = (int) ((int64) c * n / cols);
            const int b = jmax (a + 1, (int) ((int64) (c + 1) * n / cols));
            float lo = 1e9f, hi = -1e9f;
            for (int i = a; i < b && i < n; ++i) { lo = jmin (lo, wave[(size_t) i]); hi = jmax (hi, wave[(size_t) i]); }
            mins[(size_t) c] = jlimit (-1.1f, 1.1f, lo);
            maxs[(size_t) c] = jlimit (-1.1f, 1.1f, hi);
        }
        fill.startNewSubPath (plot.getX(), midY - maxs[0] * halfH);
        for (int c = 1; c < cols; ++c)
            fill.lineTo (plot.getX() + (float) c, midY - maxs[(size_t) c] * halfH);
        for (int c = cols - 1; c >= 0; --c)
            fill.lineTo (plot.getX() + (float) c, midY - mins[(size_t) c] * halfH);
        fill.closeSubPath();

        g.setGradientFill (ColourGradient (col::accent2, plot.getX(), plot.getY(),
                                           col::accent, plot.getX(), midY, true));
        g.fillPath (fill);
        g.setColour (col::accent.brighter (0.3f).withAlpha (0.6f));
        g.strokePath (fill, PathStrokeType (0.8f));

        // pitch trace (log scale 20 Hz .. 10 kHz)
        auto yForHz = [&] (float hz)
        {
            const float norm = (std::log2 (jlimit (20.0f, 10000.0f, hz)) - std::log2 (20.0f))
                               / (std::log2 (10000.0f) - std::log2 (20.0f));
            return plot.getBottom() - norm * plot.getHeight();
        };
        Path pp;
        for (int c = 0; c < cols; ++c)
        {
            const int i = jmin (n - 1, (int) ((int64) c * n / cols));
            const float yy = yForHz (pitch[(size_t) i]);
            if (c == 0) pp.startNewSubPath (plot.getX(), yy);
            else        pp.lineTo (plot.getX() + (float) c, yy);
        }
        g.setColour (col::pitch.withAlpha (0.9f));
        g.strokePath (pp, PathStrokeType (1.6f));

        g.setFont (font (9.5f));
        for (float hz : { 50.0f, 100.0f, 500.0f, 1000.0f, 5000.0f })
        {
            const float yy = yForHz (hz);
            g.setColour (col::pitch.withAlpha (0.12f));
            g.drawHorizontalLine (roundToInt (yy), plot.getRight() - 26.0f, plot.getRight());
            g.setColour (col::pitch.withAlpha (0.6f));
            g.drawText (hz >= 1000.0f ? String (hz / 1000.0f, 0) + "k" : String ((int) hz),
                        Rectangle<float> (plot.getRight() - 52.0f, yy - 6.0f, 24.0f, 12.0f), Justification::centredRight);
        }
    }

    // overlay info
    g.setFont (font (11.0f, true));
    g.setColour (col::text.withAlpha (0.9f));
    const String peakStr = peak > 0.0f ? String (Decibels::gainToDecibels (peak), 1) + " dB" : "-inf";
    g.drawText ("LENGTH " + String (roundToInt (lenSec * 1000.0)) + " ms    PEAK " + peakStr,
                Rectangle<float> (plot.getX() + 4.0f, plot.getY() + 2.0f, 300.0f, 16.0f), Justification::centredLeft);
    g.setColour (col::pitch.withAlpha (0.85f));
    g.drawText ("PITCH", Rectangle<float> (plot.getRight() - 120.0f, plot.getY() + 2.0f, 60.0f, 16.0f), Justification::centredRight);
    g.setColour (col::textDim);
    g.setFont (font (10.5f));
    g.drawText (hover ? "click to play" : "", Rectangle<float> (plot.getRight() - 110.0f, plot.getY() + 18.0f, 104.0f, 14.0f),
                Justification::centredRight);

    // flash border when a kick fires
    g.setColour (col::accent.withAlpha (0.15f + 0.6f * flashAmount));
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f + 1.5f * flashAmount);
}

} // namespace hkui

//==============================================================================
static constexpr int kWidth = 1000, kHeight = 616;

HardKickEditor::HardKickEditor (HardKickProcessor& p)
    : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    auto& s = proc.apvts;

    auto addChoice = [&] (const char* id, const String& name)
    {
        auto c = std::make_unique<hkui::ChoiceSlot> (s, id, name);
        auto* raw = c.get();
        addAndMakeVisible (*raw);
        slots.push_back (std::move (c));
        return raw;
    };
    auto addToggle = [&] (const char* id, const String& name)
    {
        auto t = std::make_unique<hkui::ToggleSlot> (s, id, name);
        auto* raw = t.get();
        addAndMakeVisible (*raw);
        slots.push_back (std::move (t));
        return raw;
    };

    click = { "CLICK", { knob (ids::clickLevel, "Level"), knob (ids::clickTone, "Tone"), knob (ids::clickDecay, "Decay") } };

    punch = { "PUNCH", { knob (ids::punchLevel, "Level"), knob (ids::startPitch, "Start Pitch"),
                         knob (ids::sweepTime, "Sweep"), knob (ids::sweepCurve, "Curve"),
                         knob (ids::punchDrive, "Drive"), knob (ids::punchDecay, "Decay") } };

    pitch = { "PITCH", { addToggle (ids::keyTrack, "Key Track"), knob (ids::octave, "Octave"),
                         knob (ids::fineTune, "Fine"), knob (ids::tailPitch, "Fixed Pitch") } };

    tail = { "TAIL", { knob (ids::tailLevel, "Level"), knob (ids::tailShape, "Shape"),
                       knob (ids::tailStart, "Start"), knob (ids::tailAttack, "Attack"),
                       knob (ids::tailLength, "Length"), knob (ids::tailRelease, "Release"),
                       knob (ids::pitchDrop, "Pitch Drop") } };

    dist = { "DISTORTION", { addChoice (ids::dist1Type, "Stage 1"), knob (ids::drive1, "Drive 1"),
                             addChoice (ids::dist2Type, "Stage 2"), knob (ids::drive2, "Drive 2") } };

    filter = { "TAIL FILTER", { knob (ids::cutoff, "Cutoff"), knob (ids::resonance, "Reso"),
                                knob (ids::filterEnv, "Env Amt"), knob (ids::filterDecay, "Env Decay") } };

    master = { "MASTER", { knob (ids::lowCut, "Low Cut"), knob (ids::highCut, "High Cut"),
                           knob (ids::masterDrive, "Clip"), knob (ids::velocity, "Velocity"),
                           knob (ids::output, "Output") } };

    for (int i = 0; i < hk::numPresets(); ++i)
        presetBox.addItem (hk::presetName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0) proc.applyPreset (id - 1);
    };
    addAndMakeVisible (presetBox);

    playButton.onClick = [this] { proc.triggerAudition(); };
    playButton.setTooltip ("Play the kick (F1)");
    addAndMakeVisible (playButton);

    waveform.onClick = [this] { proc.triggerAudition(); };
    addAndMakeVisible (waveform);

    setSize (kWidth, kHeight);
    refreshPreview();
    lastNoteCounter = proc.noteCounter.load();
    startTimerHz (30);
}

HardKickEditor::~HardKickEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

hkui::KnobSlot* HardKickEditor::knob (const char* id, const String& name)
{
    auto k = std::make_unique<hkui::KnobSlot> (proc.apvts, id, name);
    auto* raw = k.get();
    addAndMakeVisible (*raw);
    slots.push_back (std::move (k));
    return raw;
}

void HardKickEditor::refreshPreview()
{
    std::vector<float> wave, pitchTrace;
    const double previewRate = 48000.0;
    hk::renderPreview (proc.snapshotParams(), previewRate, HardKickProcessor::auditionNote, 3.2, wave, pitchTrace, 2);
    waveform.setData (std::move (wave), std::move (pitchTrace), previewRate);
    lastParamVersion = proc.paramVersion.load();
    lastRenderMs = Time::getMillisecondCounter();
}

void HardKickEditor::timerCallback()
{
    if (proc.paramVersion.load() != lastParamVersion
        && Time::getMillisecondCounter() - lastRenderMs > 40)
    {
        refreshPreview();
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    }

    const int nc = proc.noteCounter.load();
    if (nc != lastNoteCounter)
    {
        lastNoteCounter = nc;
        waveform.flash();
    }
    waveform.tick();
}

void HardKickEditor::paint (Graphics& g)
{
    g.fillAll (hkui::col::bg);

    // header
    auto header = getLocalBounds().reduced (12).removeFromTop (44);
    g.setColour (hkui::col::text);
    g.setFont (hkui::font (24.0f, true));
    g.drawText ("HARD", header.removeFromLeft (64), Justification::centredLeft);
    g.setColour (hkui::col::accent);
    g.drawText ("KICK", header.removeFromLeft (62), Justification::centredLeft);
    g.setColour (hkui::col::textDim);
    g.setFont (hkui::font (11.5f));
    g.drawText ("hardstyle kick generator", header.removeFromLeft (180).translated (4, 2), Justification::centredLeft);

    // section panels
    for (auto* sec : { &click, &punch, &pitch, &tail, &dist, &filter, &master })
    {
        auto a = sec->area.toFloat();
        g.setColour (hkui::col::panel);
        g.fillRoundedRectangle (a, 8.0f);
        g.setColour (hkui::col::edge);
        g.drawRoundedRectangle (a.reduced (0.5f), 8.0f, 1.0f);

        auto titleArea = sec->area.reduced (10, 0).removeFromTop (22);
        g.setColour (hkui::col::accent);
        g.fillRoundedRectangle ((float) titleArea.getX(), (float) titleArea.getCentreY() - 3.0f, 3.0f, 6.0f, 1.5f);
        g.setColour (hkui::col::text);
        g.setFont (hkui::font (11.5f, true));
        g.drawText (sec->title, titleArea.withTrimmedLeft (8), Justification::centredLeft);
    }
}

void HardKickEditor::layoutRow (Rectangle<int> row, std::initializer_list<Section*> sections)
{
    const int gap = 8, pad = 6;
    int totalSlots = 0;
    for (auto* s : sections) totalSlots += (int) s->items.size();

    const int usable = row.getWidth() - gap * ((int) sections.size() - 1) - 2 * pad * (int) sections.size();
    const float slotW = (float) usable / (float) totalSlots;

    float x = (float) row.getX();
    for (auto* s : sections)
    {
        const int w = roundToInt (slotW * (float) s->items.size()) + 2 * pad;
        s->area = Rectangle<int> (roundToInt (x), row.getY(), w, row.getHeight());

        auto inner = s->area.reduced (pad, 0).withTrimmedTop (24).withTrimmedBottom (6);
        float ix = (float) inner.getX();
        for (auto* item : s->items)
        {
            item->setBounds (Rectangle<int> (roundToInt (ix), inner.getY(), roundToInt (slotW), inner.getHeight()));
            ix += slotW;
        }
        x += (float) w + (float) gap;
    }
}

void HardKickEditor::resized()
{
    auto b = getLocalBounds().reduced (12);

    auto header = b.removeFromTop (44);
    playButton.setBounds (header.removeFromRight (84).withSizeKeepingCentre (84, 30));
    header.removeFromRight (10);
    presetBox.setBounds (header.removeFromRight (200).withSizeKeepingCentre (200, 28));

    b.removeFromTop (6);
    waveform.setBounds (b.removeFromTop (150));
    b.removeFromTop (8);

    const int rowH = (b.getHeight() - 16) / 3;
    layoutRow (b.removeFromTop (rowH), { &click, &punch, &pitch });
    b.removeFromTop (8);
    layoutRow (b.removeFromTop (rowH), { &tail, &dist });
    b.removeFromTop (8);
    layoutRow (b.removeFromTop (rowH), { &filter, &master });
}

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

//==============================================================================
namespace
{
    String fmtHz (float v, int)    { return v >= 1000.f ? String (v / 1000.f, 2) + " kHz" : String (v, v < 100.f ? 1 : 0) + " Hz"; }
    String fmtMs (float v, int)    { return v >= 1000.f ? String (v / 1000.f, 2) + " s"   : String (v, v < 10.f ? 1 : 0) + " ms"; }
    String fmtDb (float v, int)    { return (v > 0.f ? "+" : "") + String (v, 1) + " dB"; }
    String fmtPct (float v, int)   { return String (roundToInt (v * 100.f)) + "%"; }
    String fmtSt (float v, int)    { return String (v, 1) + " st"; }
    String fmtCt (float v, int)    { return (v > 0.f ? "+" : "") + String (roundToInt (v)) + " ct"; }
    String fmtOct (float v, int)   { return (v > 0.f ? "+" : "") + String (v, 1) + " oct"; }
    String fmtShape (float v, int)
    {
        if (v < 0.02f) return "Sine";
        if (std::abs (v - 0.5f) < 0.02f) return "Tri";
        if (v > 0.98f) return "Saw";
        return v < 0.5f ? "Sin/Tri " + String (roundToInt (v * 200.f)) + "%"
                        : "Tri/Saw " + String (roundToInt ((v - 0.5f) * 200.f)) + "%";
    }

    float parseHz (const String& t)
    {
        auto s = t.trim().toLowerCase();
        float v = s.getFloatValue();
        return s.contains ("k") ? v * 1000.f : v;
    }
    float parseMs (const String& t)
    {
        auto s = t.trim().toLowerCase();
        float v = s.getFloatValue();
        return (s.endsWith ("s") && ! s.endsWith ("ms")) ? v * 1000.f : v;
    }
    float parsePlain (const String& t) { return t.trim().getFloatValue(); }
    float parsePct (const String& t)   { return t.trim().getFloatValue() / 100.f; }

    using FmtFn   = String (*) (float, int);
    using ParseFn = float (*) (const String&);

    std::unique_ptr<AudioParameterFloat> makeFloat (const char* id, const char* name,
                                                    float lo, float hi, float def,
                                                    float centre, FmtFn fmt, ParseFn parse,
                                                    float interval = 0.f)
    {
        NormalisableRange<float> range (lo, hi, interval);
        if (centre > lo && centre < hi)
            range.setSkewForCentre (centre);

        return std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, range, def,
            AudioParameterFloatAttributes()
                .withStringFromValueFunction ([fmt] (float v, int n) { return fmt (v, n); })
                .withValueFromStringFunction ([parse] (const String& t) { return parse (t); }));
    }
}

//==============================================================================
AudioProcessorValueTreeState::ParameterLayout HardKickProcessor::createLayout()
{
    const hk::KickParams d;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    // Click
    p.push_back (makeFloat (ids::clickLevel, "Click Level", 0.f, 1.f, d.clickLevel, 0.f, fmtPct, parsePct));
    p.push_back (makeFloat (ids::clickTone, "Click Tone", 1000.f, 15000.f, d.clickTone, 4000.f, fmtHz, parseHz));
    p.push_back (makeFloat (ids::clickDecay, "Click Decay", 1.f, 50.f, d.clickDecay, 8.f, fmtMs, parseMs));

    // Punch
    p.push_back (makeFloat (ids::punchLevel, "Punch Level", 0.f, 1.f, d.punchLevel, 0.f, fmtPct, parsePct));
    p.push_back (makeFloat (ids::startPitch, "Start Pitch", 100.f, 6000.f, d.startPitch, 1200.f, fmtHz, parseHz));
    p.push_back (makeFloat (ids::sweepTime, "Sweep Time", 5.f, 300.f, d.sweepTime, 60.f, fmtMs, parseMs));
    p.push_back (makeFloat (ids::sweepCurve, "Sweep Curve", 0.f, 1.f, d.sweepCurve, 0.f, fmtPct, parsePct));
    p.push_back (makeFloat (ids::punchDrive, "Punch Drive", 0.f, 24.f, d.punchDrive, 0.f, fmtDb, parsePlain));
    p.push_back (makeFloat (ids::punchDecay, "Punch Decay", 10.f, 500.f, d.punchDecay, 100.f, fmtMs, parseMs));

    // Tail
    p.push_back (makeFloat (ids::tailLevel, "Tail Level", 0.f, 1.f, d.tailLevel, 0.f, fmtPct, parsePct));
    p.push_back (makeFloat (ids::tailShape, "Tail Shape", 0.f, 1.f, d.tailShape, 0.f, fmtShape, parsePct));
    p.push_back (makeFloat (ids::tailStart, "Tail Start", 0.f, 200.f, d.tailStart, 40.f, fmtMs, parseMs));
    p.push_back (makeFloat (ids::tailAttack, "Tail Attack", 1.f, 150.f, d.tailAttack, 25.f, fmtMs, parseMs));
    p.push_back (makeFloat (ids::tailLength, "Tail Length", 0.f, 1500.f, d.tailLength, 250.f, fmtMs, parseMs));
    p.push_back (makeFloat (ids::tailRelease, "Tail Release", 5.f, 1500.f, d.tailRelease, 200.f, fmtMs, parseMs));
    p.push_back (makeFloat (ids::pitchDrop, "Pitch Drop", 0.f, 24.f, d.pitchDrop, 4.f, fmtSt, parsePlain));

    // Distortion
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ids::dist1Type, 1 }, "Dist 1 Type",
                     StringArray { "Soft", "Hard", "Fold", "Crunch" }, d.dist1Type - 1));
    p.push_back (makeFloat (ids::drive1, "Dist 1 Drive", 0.f, 48.f, d.drive1, 0.f, fmtDb, parsePlain));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ids::dist2Type, 1 }, "Dist 2 Type",
                     StringArray { "Off", "Soft", "Hard", "Fold", "Crunch" }, d.dist2Type));
    p.push_back (makeFloat (ids::drive2, "Dist 2 Drive", 0.f, 36.f, d.drive2, 0.f, fmtDb, parsePlain));

    // Filter
    p.push_back (makeFloat (ids::cutoff, "Cutoff", 100.f, 20000.f, d.cutoff, 2000.f, fmtHz, parseHz));
    p.push_back (makeFloat (ids::resonance, "Resonance", 0.f, 1.f, d.resonance, 0.f, fmtPct, parsePct));
    p.push_back (makeFloat (ids::filterEnv, "Filter Env", -4.f, 6.f, d.filterEnvAmt, 0.f, fmtOct, parsePlain));
    p.push_back (makeFloat (ids::filterDecay, "Filter Decay", 5.f, 1000.f, d.filterDecay, 120.f, fmtMs, parseMs));

    // Pitch
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ids::keyTrack, 1 }, "Key Track", d.keyTrack));
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { ids::octave, 1 }, "Octave", -5, 1, d.octave,
                     AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int)
                         { return (v > 0 ? "+" : "") + String (v) + " oct"; })));
    p.push_back (makeFloat (ids::fineTune, "Fine Tune", -100.f, 100.f, d.fineTune, 0.f, fmtCt, parsePlain));
    p.push_back (makeFloat (ids::tailPitch, "Tail Pitch", 25.f, 120.f, d.tailPitch, 50.f, fmtHz, parseHz));

    // Master
    p.push_back (makeFloat (ids::lowCut, "Low Cut", 10.f, 200.f, d.lowCut, 40.f, fmtHz, parseHz));
    p.push_back (makeFloat (ids::highCut, "High Cut", 1000.f, 20000.f, d.highCut, 8000.f, fmtHz, parseHz));
    p.push_back (makeFloat (ids::masterDrive, "Clip Drive", 0.f, 24.f, d.masterDrive, 0.f, fmtDb, parsePlain));
    p.push_back (makeFloat (ids::velocity, "Velocity", 0.f, 1.f, d.velocitySens, 0.f, fmtPct, parsePct));
    p.push_back (makeFloat (ids::output, "Output", -24.f, 6.f, d.outputGain, 0.f, fmtDb, parsePlain));

    return { p.begin(), p.end() };
}

//==============================================================================
HardKickProcessor::HardKickProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "HardKickState", createLayout())
{
    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<RangedAudioParameter*> (param))
        {
            raw[ranged->getParameterID().toStdString()] = apvts.getRawParameterValue (ranged->getParameterID());
            apvts.addParameterListener (ranged->getParameterID(), this);
        }
}

HardKickProcessor::~HardKickProcessor()
{
    for (auto& kv : raw)
        apvts.removeParameterListener (String (kv.first), this);
}

float HardKickProcessor::get (const char* id) const
{
    auto it = raw.find (std::string (id));
    return it != raw.end() ? it->second->load() : 0.0f;
}

hk::KickParams HardKickProcessor::snapshotParams() const
{
    hk::KickParams p;
    p.clickLevel   = get (ids::clickLevel);
    p.clickTone    = get (ids::clickTone);
    p.clickDecay   = get (ids::clickDecay);
    p.punchLevel   = get (ids::punchLevel);
    p.startPitch   = get (ids::startPitch);
    p.sweepTime    = get (ids::sweepTime);
    p.sweepCurve   = get (ids::sweepCurve);
    p.punchDrive   = get (ids::punchDrive);
    p.punchDecay   = get (ids::punchDecay);
    p.tailLevel    = get (ids::tailLevel);
    p.tailShape    = get (ids::tailShape);
    p.tailStart    = get (ids::tailStart);
    p.tailAttack   = get (ids::tailAttack);
    p.tailLength   = get (ids::tailLength);
    p.tailRelease  = get (ids::tailRelease);
    p.pitchDrop    = get (ids::pitchDrop);
    p.dist1Type    = roundToInt (get (ids::dist1Type)) + 1;   // Soft..Crunch
    p.drive1       = get (ids::drive1);
    p.dist2Type    = roundToInt (get (ids::dist2Type));       // Off..Crunch
    p.drive2       = get (ids::drive2);
    p.cutoff       = get (ids::cutoff);
    p.resonance    = get (ids::resonance);
    p.filterEnvAmt = get (ids::filterEnv);
    p.filterDecay  = get (ids::filterDecay);
    p.keyTrack     = get (ids::keyTrack) > 0.5f;
    p.octave       = roundToInt (get (ids::octave));
    p.fineTune     = get (ids::fineTune);
    p.tailPitch    = get (ids::tailPitch);
    p.lowCut       = get (ids::lowCut);
    p.highCut      = get (ids::highCut);
    p.masterDrive  = get (ids::masterDrive);
    p.velocitySens = get (ids::velocity);
    p.outputGain   = get (ids::output);
    return p;
}

void HardKickProcessor::applyPreset (int index)
{
    if (index < 0 || index >= hk::numPresets())
        return;

    currentPreset = index;
    const auto p = hk::presetParams (index);

    auto set = [this] (const char* id, float value)
    {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };

    set (ids::clickLevel, p.clickLevel);   set (ids::clickTone, p.clickTone);   set (ids::clickDecay, p.clickDecay);
    set (ids::punchLevel, p.punchLevel);   set (ids::startPitch, p.startPitch); set (ids::sweepTime, p.sweepTime);
    set (ids::sweepCurve, p.sweepCurve);   set (ids::punchDrive, p.punchDrive); set (ids::punchDecay, p.punchDecay);
    set (ids::tailLevel, p.tailLevel);     set (ids::tailShape, p.tailShape);   set (ids::tailStart, p.tailStart);
    set (ids::tailAttack, p.tailAttack);   set (ids::tailLength, p.tailLength); set (ids::tailRelease, p.tailRelease);
    set (ids::pitchDrop, p.pitchDrop);
    set (ids::dist1Type, (float) (p.dist1Type - 1)); set (ids::drive1, p.drive1);
    set (ids::dist2Type, (float) p.dist2Type);       set (ids::drive2, p.drive2);
    set (ids::cutoff, p.cutoff);           set (ids::resonance, p.resonance);
    set (ids::filterEnv, p.filterEnvAmt);  set (ids::filterDecay, p.filterDecay);
    set (ids::keyTrack, p.keyTrack ? 1.f : 0.f); set (ids::octave, (float) p.octave);
    set (ids::fineTune, p.fineTune);       set (ids::tailPitch, p.tailPitch);
    set (ids::lowCut, p.lowCut);           set (ids::highCut, p.highCut);
    set (ids::masterDrive, p.masterDrive); set (ids::velocity, p.velocitySens);
    set (ids::output, p.outputGain);

    ++paramVersion;
}

//==============================================================================
void HardKickProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    mono.assign ((size_t) jmax (1, samplesPerBlock), 0.0f);
}

bool HardKickProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == AudioChannelSet::mono() || out == AudioChannelSet::stereo();
}

void HardKickProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    if (numSamples <= 0)
        return;

    if ((int) mono.size() < numSamples)
        mono.resize ((size_t) numSamples);   // only if the host exceeds the announced block size

    const auto params = snapshotParams();

    if (auditionRequested.exchange (false))
    {
        engine.noteOn (auditionNote, 1.0f);
        ++noteCounter;
    }

    int pos = 0;
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int at = jlimit (0, numSamples, meta.samplePosition);

        if (at > pos)
        {
            engine.render (mono.data() + pos, at - pos, params);
            pos = at;
        }

        if (msg.isNoteOn())
        {
            engine.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
            ++noteCounter;
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            engine.allSoundOff();
        }
    }

    if (pos < numSamples)
        engine.render (mono.data() + pos, numSamples - pos, params);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, mono.data(), numSamples);
}

//==============================================================================
void HardKickProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void HardKickProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = ValueTree::fromXml (*xml);
            currentPreset = state.getProperty ("preset", 0);
            apvts.replaceState (state);
            ++paramVersion;
        }
}

AudioProcessorEditor* HardKickProcessor::createEditor()
{
    return new HardKickEditor (*this);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HardKickProcessor();
}

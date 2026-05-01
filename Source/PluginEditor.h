#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ---------------------------------------------------------------------------
// Palette — v0.2.0 "Vignelli-correct" register.
// Single 10px line + 8 colored segments + ink-square endpoints. Olivetti
// red is reserved for the DRIVE stage and the wordmark period; not used as
// an active-state colour anywhere else.
// ---------------------------------------------------------------------------
namespace BigDawgColors
{
    // Surface
    static const juce::Colour paper      { 0xFFDDDED4 };  // page background
    static const juce::Colour panelLight { 0xFFE8E9E0 };  // lighter card variant
    static const juce::Colour ink        { 0xFF1A1A1A };  // text, outlines, ticks
    static const juce::Colour bypass     { 0xFFB5B5AE };  // desaturated diagram segment
    static const juce::Colour cyan       { 0xFF4A90B8 };  // also Vibrato stage colour

    // Eight stage colours. Verbal-spec / user picks; verify in v0.2.1
    // once tokens.jsx is recovered.
    static const juce::Colour stDrive    { 0xFFC8342E };  // 01 DRIVE     Olivetti red
    static const juce::Colour stDetune   { 0xFF1F3A8A };  // 02 DETUNE    deep blue
    static const juce::Colour stBitcrush { 0xFFE8842E };  // 03 BITCRUSH  orange
    static const juce::Colour stChorus   { 0xFF2A8C5A };  // 04 CHORUS    green
    static const juce::Colour stVibrato  { 0xFF4A90B8 };  // 05 VIBRATO   cyan
    static const juce::Colour stWow      { 0xFFE8C82E };  // 06 WOW       yellow
    static const juce::Colour stSpring   { 0xFF6A4A8C };  // 07 SPRING    purple
    static const juce::Colour stEQ       { 0xFF1A1A1A };  // 08 EQ        ink
    static const juce::Colour stOut      { 0xFF1A1A1A };  //    OUT       ink
}

// ---------------------------------------------------------------------------
// BigDawgLookAndFeel — typography helpers (Archivo Black + IBM Plex Mono).
// ---------------------------------------------------------------------------
class BigDawgLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BigDawgLookAndFeel();

    static juce::Font makeDisplayFont(float heightPx);                 // Archivo Black
    static juce::Font makeMonoFont(float heightPx, bool bold = false); // IBM Plex Mono
    static juce::Font makeMonoTabularFont(float heightPx);             // tabular figures for value readouts
};

// ---------------------------------------------------------------------------
// Knob — D2 white-disc with colored indicator line.
//   - paper-filled circle, 1.5px ink hairline outline
//   - single ink/colour indicator line, -135° (min) to +135° (max)
//   - colour matches the stage the knob belongs to
//   - label below in Plex Mono caps; tabular value readout below that
//   - carries the v0.1.3 visual override for hi-fi mode
// ---------------------------------------------------------------------------
class Knob : public juce::Slider
{
public:
    enum class Format { Float2, Hz, Cents, Percent, DB, Bipolar };

    Knob(const juce::String& label, Format format, juce::Colour stageColour);

    void paint(juce::Graphics& g) override;

    juce::String formatValue() const;

    // v0.1.3 hi-fi visual sync.
    void setVisualOverride(bool active, float overrideValue);

private:
    double currentDisplayValue() const;

    juce::String  nameLabel;
    Format        format;
    juce::Colour  stageColour;

    bool   overrideActive = false;
    float  overrideValue  = 0.0f;
};

// ---------------------------------------------------------------------------
// BoxToggle — N side-by-side labeled cells (radio group of 2..N).
//   Active cell: ink fill, paper text. Inactive: paper fill, ink text + border.
// ---------------------------------------------------------------------------
class BoxToggle : public juce::Component
{
public:
    BoxToggle(const juce::String& leftLabel, const juce::String& rightLabel);
    BoxToggle(juce::StringArray labelList);

    void setValue(int v, juce::NotificationType n = juce::sendNotification);
    int  getValue() const { return value; }
    int  getNumLabels() const { return labels.size(); }

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

    std::function<void(int)> onChange;

private:
    juce::StringArray labels;
    int value = 0;
};

// ---------------------------------------------------------------------------
// Choice / Bool toggle attachments — bind a BoxToggle to APVTS.
// ---------------------------------------------------------------------------
class ChoiceToggleAttachment : private juce::AudioProcessorValueTreeState::Listener
{
public:
    ChoiceToggleAttachment(juce::AudioProcessorValueTreeState& state,
                           const juce::String& paramID,
                           BoxToggle& toggle);
    ~ChoiceToggleAttachment() override;

private:
    void parameterChanged(const juce::String& id, float newValue) override;
    juce::AudioProcessorValueTreeState& apvts;
    juce::String id;
    BoxToggle& box;
};

class BoolToggleAttachment : private juce::AudioProcessorValueTreeState::Listener
{
public:
    BoolToggleAttachment(juce::AudioProcessorValueTreeState& state,
                         const juce::String& paramID,
                         BoxToggle& toggle);
    ~BoolToggleAttachment() override;

private:
    void parameterChanged(const juce::String& id, float newValue) override;
    juce::AudioProcessorValueTreeState& apvts;
    juce::String id;
    BoxToggle& box;
};

// ---------------------------------------------------------------------------
// SignalDiagram — the horizontal flow line.
//   Single 10px line, eight colored segments meeting at boundaries with
//   1px ink ticks at transitions, 12×12 ink endpoint squares (IN/OUT),
//   stage labels + 01-08 sub-numbers below.
//
//   Bypass-aware: polls APVTS at 30 Hz; bypassed segments render in
//   `bypass` grey with their label dimmed in lockstep.
// ---------------------------------------------------------------------------
class SignalDiagram : public juce::Component, private juce::Timer
{
public:
    explicit SignalDiagram(DemarcoToneProcessor& p);
    ~SignalDiagram() override;

    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;

    DemarcoToneProcessor& proc;

    // Last-seen bypass state per stage; only repaint when something changed.
    std::array<bool, 8> bypassedLastSeen { false, false, false, false,
                                            false, false, false, false };
};

// ---------------------------------------------------------------------------
// PresetSelector — header center cluster:
//   V0.1   PRESET 03/05   < ELVIS (THE SAD ONE) >
//   - chevrons cycle prev/next
//   - clicking the name opens a popup of all presets
// ---------------------------------------------------------------------------
class PresetSelector : public juce::Component
{
public:
    explicit PresetSelector(DemarcoToneProcessor& p);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

    void refresh(); // pick up programmatic preset changes (preset load via API)

private:
    juce::Rectangle<int> leftChevronRect()  const;
    juce::Rectangle<int> rightChevronRect() const;
    juce::Rectangle<int> nameRect()         const;

    DemarcoToneProcessor& proc;
};

// ---------------------------------------------------------------------------
// Main editor
// ---------------------------------------------------------------------------
class DemarcoToneEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit DemarcoToneEditor(DemarcoToneProcessor&);
    ~DemarcoToneEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;     // hi-fi visual override sync (v0.1.3)

    DemarcoToneProcessor& proc;
    BigDawgLookAndFeel lnf;

    // ── Header ────────────────────────────────────────────────────────────
    PresetSelector presetSelector { proc };
    BoxToggle      hifiToggle     { "LO-FI", "HI-FI" };

    // ── Signal diagram ───────────────────────────────────────────────────
    SignalDiagram  diagram        { proc };

    // ── Stage controls ───────────────────────────────────────────────────
    // 01 DRIVE
    Knob driveKnob { "DRIVE", Knob::Format::Float2, BigDawgColors::stDrive };

    // 02 DETUNE
    BoxToggle detuneOnToggle { "OFF", "ON" };
    Knob detuneKnob { "CENTS", Knob::Format::Cents, BigDawgColors::stDetune };

    // 03 BITCRUSH
    Knob bitcrushBitsKnob { "BITS", Knob::Format::Float2, BigDawgColors::stBitcrush };
    Knob bitcrushRateKnob { "RATE", Knob::Format::Float2, BigDawgColors::stBitcrush };

    // 04 CHORUS / FLANGER
    BoxToggle chorusOnToggle    { "OFF", "ON" };
    BoxToggle chorusModeToggle  { "CHO", "FLG" };
    BoxToggle chorusShapeToggle { "SINE", "TRI" };
    Knob chorusRate  { "RATE",  Knob::Format::Hz,     BigDawgColors::stChorus };
    Knob chorusDepth { "DEPTH", Knob::Format::Float2, BigDawgColors::stChorus };
    Knob chorusMix   { "MIX",   Knob::Format::Float2, BigDawgColors::stChorus };
    Knob chorusWidth { "WIDTH", Knob::Format::Float2, BigDawgColors::stChorus };

    // 05 VIBRATO
    BoxToggle vibratoOnToggle { "OFF", "ON" };
    Knob vibratoRate  { "RATE",  Knob::Format::Hz,     BigDawgColors::stVibrato };
    Knob vibratoDepth { "DEPTH", Knob::Format::Float2, BigDawgColors::stVibrato };

    // 06 WOW
    Knob wowFlutter { "AMOUNT", Knob::Format::Float2, BigDawgColors::stWow };

    // 07 SPRING
    BoxToggle reverbOnToggle    { "OFF", "ON" };
    BoxToggle reverbModeToggle  { juce::StringArray{ "SPR", "SLP", "PLT" } };
    Knob reverbMix  { "MIX",  Knob::Format::Float2,  BigDawgColors::stSpring };
    Knob reverbTone { "TONE", Knob::Format::Bipolar, BigDawgColors::stSpring };

    // 08 EQ — placeholder for v0.2.1 wiring; no controls in v0.2.0.

    // OUT
    Knob outputTrim { "TRIM", Knob::Format::DB, BigDawgColors::stOut };

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SA> driveAtt, detuneAtt,
        bitcrushBitsAtt, bitcrushRateAtt,
        chorusRateAtt, chorusDepthAtt, chorusMixAtt, chorusWidthAtt,
        vibratoRateAtt, vibratoDepthAtt,
        wowFlutterAtt, reverbMixAtt, reverbToneAtt, outputTrimAtt;

    std::unique_ptr<BoolToggleAttachment>   detuneOnAtt;
    std::unique_ptr<BoolToggleAttachment>   chorusOnAtt;
    std::unique_ptr<BoolToggleAttachment>   vibratoOnAtt;
    std::unique_ptr<BoolToggleAttachment>   reverbOnAtt;
    std::unique_ptr<BoolToggleAttachment>   hifiAtt;
    std::unique_ptr<ChoiceToggleAttachment> chorusShapeAtt;
    std::unique_ptr<ChoiceToggleAttachment> chorusModeAtt;
    std::unique_ptr<ChoiceToggleAttachment> reverbModeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemarcoToneEditor)
};

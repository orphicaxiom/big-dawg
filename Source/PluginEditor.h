#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ---------------------------------------------------------------------------
// Palette — "imaginary 1981 hardware unit" register.
// Sits on the shelf next to a CE-2 or Space Echo; not a SaaS UI, not a photo.
// ---------------------------------------------------------------------------
namespace BigDawgColors
{
    static const juce::Colour paper         { 0xFFE8E9E0 };  // pale sage canvas
    static const juce::Colour panel         { 0xFFE0E2D8 };  // section panels, 2% darker than paper
    static const juce::Colour ink           { 0xFF1A1A1A };  // rules, text, knob rings

    // Three-tier accent (same hue family, only S and V change).
    static const juce::Colour redPrimary    { 0xFFC8342E };  // full accent: Drive, Reverb Mix, Out Trim
    static const juce::Colour redSecondary  { 0xFF9A4A46 };  // muted: Chorus/Vibrato Rate+Depth, Wow Amount
    static const juce::Colour redTertiary   { 0xFF8A6B5E };  // desaturated warmth: Chorus Mix/Width, Detune Cents, Reverb Tone

    static const juce::Colour cyan          { 0xFF4A90B8 };  // reserved for modulation viz
}

// ---------------------------------------------------------------------------
// BigDawgLookAndFeel — flat, hairline, no skeuomorphism
// ---------------------------------------------------------------------------
class BigDawgLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BigDawgLookAndFeel();

    static juce::Font makeDisplayFont(float heightPx);
    static juce::Font makeMonoFont(float heightPx, bool bold = false);
};

// ---------------------------------------------------------------------------
// ArcKnob — thin-ring arc slider
//   - 270° sweep, 7 o'clock to 5 o'clock
//   - ink track + tiered-red value arc
//   - param label above (all-caps mono)
//   - value readout below (mono)
// ---------------------------------------------------------------------------
class ArcKnob : public juce::Slider
{
public:
    enum class Format { Float2, Hz, Cents, Percent, DB, Bipolar };
    enum class Tier   { Primary, Secondary, Tertiary };

    ArcKnob(const juce::String& label,
            Format format = Format::Float2,
            Tier   tier   = Tier::Primary);

    void paint(juce::Graphics& g) override;

    juce::String formatValue() const;
    juce::Colour tierColour() const;

private:
    juce::String nameLabel;
    Format       format;
    Tier         tier;
};

// ---------------------------------------------------------------------------
// BoxToggle — N side-by-side labeled cells (radio group of 2..N).
//   Active cell: ink fill, paper text. Inactive: paper fill, ink text + border.
//   Value = selected cell index, 0..N-1.
// ---------------------------------------------------------------------------
class BoxToggle : public juce::Component
{
public:
    // Two-label convenience ctor (preserves existing call sites).
    BoxToggle(const juce::String& leftLabel, const juce::String& rightLabel);
    // Arbitrary-N ctor.
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
// ChoiceToggleAttachment — binds a BoxToggle to an AudioParameterChoice
// (2 items) by listening both ways.
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

// ---------------------------------------------------------------------------
// BoolToggleAttachment — binds a BoxToggle (OFF=0 / ON=1) to an
// AudioParameterBool. Left = OFF, right = ON.
// ---------------------------------------------------------------------------
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
// PresetStepper — "< VICEROY >" clickable strip. Left chevron goes to prev
// preset, right chevron to next. Label is the current preset name.
// ---------------------------------------------------------------------------
class PresetStepper : public juce::Component
{
public:
    PresetStepper();

    void setPresets(const juce::StringArray& names);
    void setCurrentIndex(int idx);
    int  getCurrentIndex() const { return index; }

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

    std::function<void(int)> onSelect;

private:
    juce::Rectangle<int> leftHit() const;
    juce::Rectangle<int> rightHit() const;

    juce::StringArray names;
    int index = 0;
};

// ---------------------------------------------------------------------------
// Main editor
// ---------------------------------------------------------------------------
class DemarcoToneEditor : public juce::AudioProcessorEditor
{
public:
    explicit DemarcoToneEditor(DemarcoToneProcessor&);
    ~DemarcoToneEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    DemarcoToneProcessor& proc;
    BigDawgLookAndFeel lnf;

    // 01 DRIVE — Primary (headline action)
    ArcKnob driveKnob { "DRIVE", ArcKnob::Format::Float2, ArcKnob::Tier::Primary };
    // 02 DETUNE — Tertiary (shaping offset)
    BoxToggle detuneOnToggle { "OFF", "ON" };
    ArcKnob detuneKnob { "CENTS", ArcKnob::Format::Cents, ArcKnob::Tier::Tertiary };
    // 03 BITCRUSH — Tertiary (character / shaping)
    ArcKnob bitcrushBitsKnob { "BITS", ArcKnob::Format::Float2, ArcKnob::Tier::Tertiary };
    ArcKnob bitcrushRateKnob { "RATE", ArcKnob::Format::Float2, ArcKnob::Tier::Tertiary };
    // 04 CHORUS / FLANGER — Rate/Depth Secondary, Mix/Width Tertiary
    BoxToggle chorusModeToggle { "CHORUS", "FLANGER" };
    BoxToggle chorusOnToggle { "OFF", "ON" };
    ArcKnob chorusRate  { "RATE",  ArcKnob::Format::Hz,     ArcKnob::Tier::Secondary };
    ArcKnob chorusDepth { "DEPTH", ArcKnob::Format::Float2, ArcKnob::Tier::Secondary };
    ArcKnob chorusMix   { "MIX",   ArcKnob::Format::Float2, ArcKnob::Tier::Tertiary };
    ArcKnob chorusWidth { "WIDTH", ArcKnob::Format::Float2, ArcKnob::Tier::Tertiary };
    BoxToggle chorusShapeToggle { "SINE", "TRI" };
    // 05 VIBRATO — Rate/Depth Secondary
    BoxToggle vibratoOnToggle { "OFF", "ON" };
    ArcKnob vibratoRate  { "RATE",  ArcKnob::Format::Hz,     ArcKnob::Tier::Secondary };
    ArcKnob vibratoDepth { "DEPTH", ArcKnob::Format::Float2, ArcKnob::Tier::Secondary };
    // 06 WOW — Secondary (signature Viceroy element)
    ArcKnob wowFlutter { "AMOUNT", ArcKnob::Format::Float2, ArcKnob::Tier::Secondary };
    // 07 REVERB — Mix Primary, Tone Tertiary
    BoxToggle reverbModeToggle { juce::StringArray{ "SPRING", "SLAP", "PLATE" } };
    BoxToggle reverbOnToggle { "OFF", "ON" };
    ArcKnob reverbMix  { "MIX",  ArcKnob::Format::Float2,  ArcKnob::Tier::Primary };
    ArcKnob reverbTone { "TONE", ArcKnob::Format::Bipolar, ArcKnob::Tier::Tertiary };
    // OUT — Primary
    ArcKnob outputTrim { "TRIM", ArcKnob::Format::DB, ArcKnob::Tier::Primary };

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

    // Top bar — global mode (HI-FI scales lo-fi stages down at DSP boundary).
    BoxToggle hifiToggle { "LO-FI", "HI-FI" };

    PresetStepper presetStepper;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemarcoToneEditor)
};

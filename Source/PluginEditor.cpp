#include "PluginEditor.h"

using namespace BigDawgColors;

// ---------------------------------------------------------------------------
// BigDawgLookAndFeel
// ---------------------------------------------------------------------------
BigDawgLookAndFeel::BigDawgLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, paper);
    setColour(juce::Slider::textBoxTextColourId, ink);
    setColour(juce::Slider::textBoxBackgroundColourId, paper);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

juce::Font BigDawgLookAndFeel::makeDisplayFont(float heightPx)
{
    juce::Font f(juce::FontOptions("Archivo Black", heightPx, juce::Font::plain));
    if (f.getTypefaceName() != "Archivo Black")
        f = juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(),
                                         heightPx, juce::Font::bold));
    return f;
}

juce::Font BigDawgLookAndFeel::makeMonoFont(float heightPx, bool bold)
{
    const int style = bold ? juce::Font::bold : juce::Font::plain;
    juce::Font f(juce::FontOptions("IBM Plex Mono", heightPx, style));
    if (f.getTypefaceName() != "IBM Plex Mono")
        f = juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                         heightPx, style));
    return f;
}

// ---------------------------------------------------------------------------
// ArcKnob
// ---------------------------------------------------------------------------
ArcKnob::ArcKnob(const juce::String& label, Format fmt, Tier t)
    : nameLabel(label), format(fmt), tier(t)
{
    setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,   // 7 o'clock
                        juce::MathConstants<float>::pi * 2.75f,   // 5 o'clock
                        true);
    setColour(juce::Slider::rotarySliderFillColourId, tierColour());
    setColour(juce::Slider::rotarySliderOutlineColourId, ink);
    setColour(juce::Slider::textBoxTextColourId, ink);
}

juce::Colour ArcKnob::tierColour() const
{
    switch (tier)
    {
        case Tier::Primary:   return redPrimary;
        case Tier::Secondary: return redSecondary;
        case Tier::Tertiary:  return redTertiary;
    }
    return redPrimary;
}

double ArcKnob::currentDisplayValue() const
{
    return overrideActive ? (double)overrideValue : getValue();
}

void ArcKnob::setVisualOverride(bool active, float v)
{
    if (overrideActive == active && std::abs(overrideValue - v) < 1e-4f)
        return;
    overrideActive = active;
    overrideValue  = v;
    repaint();
}

juce::String ArcKnob::formatValue() const
{
    const double v = currentDisplayValue();
    switch (format)
    {
        case Format::Hz:      return juce::String(v, 2) + " HZ";
        case Format::Cents:   return (v > 0.0 ? "+" : "") + juce::String((int)std::round(v));
        case Format::Percent: return juce::String((int)std::round(v * 100.0)) + "%";
        case Format::DB:      return (v > 0.0 ? "+" : "") + juce::String(v, 1) + " DB";
        case Format::Bipolar: return (v > 0.0 ? "+" : "") + juce::String(v, 2);
        case Format::Float2:
        default:              return juce::String(v, 2);
    }
}

void ArcKnob::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const float labelH = 12.0f;
    const float valueH = 12.0f;

    // Reduced opacity when displaying a hi-fi-derived value, signalling
    // "not your input — this is what the mode is doing."
    const float alpha   = overrideActive ? 0.55f : 1.0f;
    const auto  textInk = ink.withMultipliedAlpha(alpha);

    // Label above
    g.setColour(textInk);
    g.setFont(BigDawgLookAndFeel::makeMonoFont(10.0f, true));
    g.drawFittedText(nameLabel.toUpperCase(),
                     bounds.removeFromTop(labelH).toNearestInt(),
                     juce::Justification::centred, 1);

    // Value below
    auto valueBox = bounds.removeFromBottom(valueH);
    g.setColour(textInk);
    g.setFont(BigDawgLookAndFeel::makeMonoFont(10.0f, false));
    g.drawFittedText(formatValue(),
                     valueBox.toNearestInt(),
                     juce::Justification::centred, 1);

    // Arc area (square, centered)
    const float side = std::min(bounds.getWidth(), bounds.getHeight()) - 4.0f;
    auto arcArea = juce::Rectangle<float>(side, side).withCentre(bounds.getCentre());

    const float cx = arcArea.getCentreX();
    const float cy = arcArea.getCentreY();
    const float radius = side * 0.5f - 2.0f;

    const float startAngle = juce::MathConstants<float>::pi * 1.25f;
    const float endAngle   = juce::MathConstants<float>::pi * 2.75f;

    const double range = getMaximum() - getMinimum();
    const double dispV = currentDisplayValue();
    const double norm  = range > 0.0 ? (dispV - getMinimum()) / range : 0.0;
    const float valAngle = startAngle + (float)norm * (endAngle - startAngle);

    // Ink track
    juce::Path track;
    track.addCentredArc(cx, cy, radius, radius, 0.0f,
                        startAngle, endAngle, true);
    g.setColour(ink.withMultipliedAlpha(alpha));
    g.strokePath(track, juce::PathStrokeType(1.5f));

    // Red value arc — for bipolar params, draw from center (mid-angle), else from start.
    juce::Path valueArc;
    if (format == Format::Bipolar || (getMinimum() < 0.0 && getMaximum() > 0.0))
    {
        const double midNorm = -getMinimum() / range;
        const float midAngle = startAngle + (float)midNorm * (endAngle - startAngle);
        if (valAngle >= midAngle)
            valueArc.addCentredArc(cx, cy, radius, radius, 0.0f,
                                   midAngle, valAngle, true);
        else
            valueArc.addCentredArc(cx, cy, radius, radius, 0.0f,
                                   valAngle, midAngle, true);
    }
    else
    {
        valueArc.addCentredArc(cx, cy, radius, radius, 0.0f,
                               startAngle, valAngle, true);
    }
    const auto accent = tierColour().withMultipliedAlpha(alpha);
    g.setColour(accent);
    g.strokePath(valueArc, juce::PathStrokeType(2.5f));

    // Small tick at the indicator position
    const float tx = cx + std::cos(valAngle - juce::MathConstants<float>::halfPi) * radius;
    const float ty = cy + std::sin(valAngle - juce::MathConstants<float>::halfPi) * radius;
    g.setColour(accent);
    g.fillEllipse(tx - 2.0f, ty - 2.0f, 4.0f, 4.0f);
}

// ---------------------------------------------------------------------------
// BoxToggle
// ---------------------------------------------------------------------------
BoxToggle::BoxToggle(const juce::String& l, const juce::String& r)
    : labels(juce::StringArray{ l, r }) {}

BoxToggle::BoxToggle(juce::StringArray labelList)
    : labels(std::move(labelList))
{
    if (labels.isEmpty())
        labels.add("-");
}

void BoxToggle::setValue(int v, juce::NotificationType n)
{
    v = juce::jlimit(0, juce::jmax(0, labels.size() - 1), v);
    if (v == value) return;
    value = v;
    repaint();
    if (n != juce::dontSendNotification && onChange)
        onChange(value);
}

void BoxToggle::mouseDown(const juce::MouseEvent& e)
{
    const int n = juce::jmax(1, labels.size());
    const float cellW = (float)getWidth() / (float)n;
    const int newVal = juce::jlimit(0, n - 1, (int)((float)e.x / cellW));
    setValue(newVal);
}

void BoxToggle::paint(juce::Graphics& g)
{
    const int n = juce::jmax(1, labels.size());
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const float cellW = bounds.getWidth() / (float)n;

    g.setFont(BigDawgLookAndFeel::makeMonoFont(10.0f, true));

    for (int i = 0; i < n; ++i)
    {
        auto box = bounds.withX(bounds.getX() + cellW * (float)i).withWidth(cellW);
        const bool active = (i == value);
        if (active)
        {
            g.setColour(ink);
            g.fillRect(box);
            g.setColour(paper);
        }
        else
        {
            g.setColour(paper);
            g.fillRect(box);
            g.setColour(ink);
            g.drawRect(box, 1.5f);
        }
        g.drawFittedText(labels[i], box.toNearestInt(),
                         juce::Justification::centred, 1);
    }
}

// ---------------------------------------------------------------------------
// ChoiceToggleAttachment
// ---------------------------------------------------------------------------
ChoiceToggleAttachment::ChoiceToggleAttachment(juce::AudioProcessorValueTreeState& state,
                                               const juce::String& paramID,
                                               BoxToggle& toggle)
    : apvts(state), id(paramID), box(toggle)
{
    if (auto* p = apvts.getRawParameterValue(id))
        box.setValue((int)p->load(), juce::dontSendNotification);

    apvts.addParameterListener(id, this);

    box.onChange = [this](int v)
    {
        if (auto* p = apvts.getParameter(id))
        {
            // AudioParameterChoice range is 0..numChoices-1; convert v to 0..1.
            const float num = juce::jmax(1.0f, (float)p->getNormalisableRange().end);
            p->setValueNotifyingHost((float)v / num);
        }
    };
}

ChoiceToggleAttachment::~ChoiceToggleAttachment()
{
    apvts.removeParameterListener(id, this);
}

void ChoiceToggleAttachment::parameterChanged(const juce::String&, float newValue)
{
    box.setValue((int)newValue, juce::dontSendNotification);
}

// ---------------------------------------------------------------------------
// BoolToggleAttachment
// ---------------------------------------------------------------------------
BoolToggleAttachment::BoolToggleAttachment(juce::AudioProcessorValueTreeState& state,
                                           const juce::String& paramID,
                                           BoxToggle& toggle)
    : apvts(state), id(paramID), box(toggle)
{
    if (auto* p = apvts.getRawParameterValue(id))
        box.setValue(p->load() > 0.5f ? 1 : 0, juce::dontSendNotification);

    apvts.addParameterListener(id, this);

    box.onChange = [this](int v)
    {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(v > 0 ? 1.0f : 0.0f);
    };
}

BoolToggleAttachment::~BoolToggleAttachment()
{
    apvts.removeParameterListener(id, this);
}

void BoolToggleAttachment::parameterChanged(const juce::String&, float newValue)
{
    box.setValue(newValue > 0.5f ? 1 : 0, juce::dontSendNotification);
}

// ---------------------------------------------------------------------------
// PresetStepper — "< VICEROY >" clickable
// ---------------------------------------------------------------------------
PresetStepper::PresetStepper() { setInterceptsMouseClicks(true, false); }

void PresetStepper::setPresets(const juce::StringArray& n)
{
    names = n;
    if (index >= names.size()) index = juce::jmax(0, names.size() - 1);
    repaint();
}

void PresetStepper::setCurrentIndex(int i)
{
    if (names.isEmpty()) return;
    index = juce::jlimit(0, names.size() - 1, i);
    repaint();
}

juce::Rectangle<int> PresetStepper::leftHit() const
{
    return getLocalBounds().removeFromLeft(18);
}

juce::Rectangle<int> PresetStepper::rightHit() const
{
    auto r = getLocalBounds();
    return r.removeFromRight(18);
}

void PresetStepper::mouseDown(const juce::MouseEvent& e)
{
    if (names.isEmpty()) return;

    // Chevrons cycle prev / next.
    if (leftHit().contains(e.getPosition()) || rightHit().contains(e.getPosition()))
    {
        const int step = leftHit().contains(e.getPosition()) ? -1 : 1;
        const int newIndex = (index + step + names.size()) % names.size();
        if (newIndex != index)
        {
            index = newIndex;
            repaint();
            if (onSelect) onSelect(index);
        }
        return;
    }

    // Click on the label area → popup menu listing all presets.
    juce::PopupMenu menu;
    for (int i = 0; i < names.size(); ++i)
        menu.addItem(i + 1, names[i], true, i == index);

    juce::Component::SafePointer<PresetStepper> safe(this);
    menu.showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(this),
        [safe](int result)
        {
            if (safe == nullptr || result <= 0) return;
            const int newIdx = result - 1;
            if (newIdx != safe->index)
            {
                safe->index = newIdx;
                safe->repaint();
                if (safe->onSelect) safe->onSelect(newIdx);
            }
        });
}

void PresetStepper::paint(juce::Graphics& g)
{
    auto r = getLocalBounds();

    g.setColour(ink);
    g.setFont(BigDawgLookAndFeel::makeMonoFont(11.0f, true));

    // Left chevron
    auto left = r.removeFromLeft(18);
    g.drawFittedText("<", left, juce::Justification::centred, 1);

    // Right chevron
    auto right = r.removeFromRight(18);
    g.drawFittedText(">", right, juce::Justification::centred, 1);

    // Label
    const juce::String label = names.isEmpty() ? juce::String("—")
                                                : names[index].toUpperCase();
    g.drawFittedText(label, r, juce::Justification::centred, 1);
}

// ---------------------------------------------------------------------------
// Editor
// ---------------------------------------------------------------------------
DemarcoToneEditor::DemarcoToneEditor(DemarcoToneProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&lnf);

    auto addKnob = [this](ArcKnob& k)
    {
        k.setLookAndFeel(&lnf);
        addAndMakeVisible(k);
    };

    addKnob(driveKnob);
    addKnob(detuneKnob);
    addKnob(bitcrushBitsKnob);
    addKnob(bitcrushRateKnob);
    addKnob(chorusRate);
    addKnob(chorusDepth);
    addKnob(chorusMix);
    addKnob(chorusWidth);
    addKnob(vibratoRate);
    addKnob(vibratoDepth);
    addKnob(wowFlutter);
    addKnob(reverbMix);
    addKnob(reverbTone);
    addKnob(outputTrim);

    addAndMakeVisible(detuneOnToggle);
    addAndMakeVisible(chorusModeToggle);
    addAndMakeVisible(chorusOnToggle);
    addAndMakeVisible(chorusShapeToggle);
    addAndMakeVisible(vibratoOnToggle);
    addAndMakeVisible(reverbModeToggle);
    addAndMakeVisible(reverbOnToggle);
    addAndMakeVisible(hifiToggle);

    // Detune — double-click to zero
    detuneKnob.setDoubleClickReturnValue(true, 0.0);

    // Parameter attachments
    auto& apvts = proc.getAPVTS();
    driveAtt        = std::make_unique<SA>(apvts, ParamID::drive,        driveKnob);
    detuneAtt       = std::make_unique<SA>(apvts, ParamID::detune,       detuneKnob);
    bitcrushBitsAtt = std::make_unique<SA>(apvts, ParamID::bitcrushBits, bitcrushBitsKnob);
    bitcrushRateAtt = std::make_unique<SA>(apvts, ParamID::bitcrushRate, bitcrushRateKnob);
    chorusRateAtt   = std::make_unique<SA>(apvts, ParamID::chorusRate,   chorusRate);
    chorusDepthAtt  = std::make_unique<SA>(apvts, ParamID::chorusDepth,  chorusDepth);
    chorusMixAtt    = std::make_unique<SA>(apvts, ParamID::chorusMix,    chorusMix);
    chorusWidthAtt  = std::make_unique<SA>(apvts, ParamID::chorusWidth,  chorusWidth);
    vibratoRateAtt  = std::make_unique<SA>(apvts, ParamID::vibratoRate,  vibratoRate);
    vibratoDepthAtt = std::make_unique<SA>(apvts, ParamID::vibratoDepth, vibratoDepth);
    wowFlutterAtt   = std::make_unique<SA>(apvts, ParamID::wowFlutter,   wowFlutter);
    reverbMixAtt    = std::make_unique<SA>(apvts, ParamID::reverbMix,    reverbMix);
    reverbToneAtt   = std::make_unique<SA>(apvts, ParamID::reverbTone,   reverbTone);
    outputTrimAtt   = std::make_unique<SA>(apvts, ParamID::outputTrim,   outputTrim);

    detuneOnAtt  = std::make_unique<BoolToggleAttachment>(
        apvts, ParamID::detuneOn,  detuneOnToggle);
    chorusOnAtt  = std::make_unique<BoolToggleAttachment>(
        apvts, ParamID::chorusOn,  chorusOnToggle);
    vibratoOnAtt = std::make_unique<BoolToggleAttachment>(
        apvts, ParamID::vibratoOn, vibratoOnToggle);
    reverbOnAtt  = std::make_unique<BoolToggleAttachment>(
        apvts, ParamID::reverbOn,  reverbOnToggle);
    hifiAtt      = std::make_unique<BoolToggleAttachment>(
        apvts, ParamID::hifiMode,  hifiToggle);
    chorusShapeAtt = std::make_unique<ChoiceToggleAttachment>(
        apvts, ParamID::chorusShape, chorusShapeToggle);
    chorusModeAtt  = std::make_unique<ChoiceToggleAttachment>(
        apvts, ParamID::chorusMode,  chorusModeToggle);
    reverbModeAtt  = std::make_unique<ChoiceToggleAttachment>(
        apvts, ParamID::reverbMode,  reverbModeToggle);

    // Preset stepper
    addAndMakeVisible(presetStepper);
    presetStepper.setPresets(proc.getPresetNames());
    presetStepper.setCurrentIndex(proc.getCurrentPresetIndex());
    presetStepper.onSelect = [this](int idx) { proc.loadPreset(idx); repaint(); };

    setSize(840, 240);
    setResizable(false, false);

    // Hi-fi visual sync — 30 Hz is more than enough for state changes.
    startTimerHz(30);
}

void DemarcoToneEditor::timerCallback()
{
    auto& apvts = proc.getAPVTS();
    auto getF = [&apvts](const juce::String& id) -> float
    {
        if (auto* raw = apvts.getRawParameterValue(id))
            return raw->load();
        return 0.0f;
    };

    const bool hifi = getF(ParamID::hifiMode) >= 0.5f;

    auto applyOverride = [hifi](ArcKnob& k, float scaled)
    {
        k.setVisualOverride(hifi, scaled);
        k.setEnabled(!hifi);
    };

    applyOverride(driveKnob,        getF(ParamID::drive)        * HifiScale::drive);
    applyOverride(detuneKnob,       getF(ParamID::detune)       * HifiScale::detune);
    applyOverride(bitcrushBitsKnob, HifiScale::bitcrushBitsBypass);
    applyOverride(bitcrushRateKnob, HifiScale::bitcrushRateBypass);
    applyOverride(chorusMix,        getF(ParamID::chorusMix)    * HifiScale::chorusMix);
    applyOverride(vibratoDepth,     getF(ParamID::vibratoDepth) * HifiScale::vibratoDepth);
    applyOverride(wowFlutter,       getF(ParamID::wowFlutter)   * HifiScale::wow);
}

DemarcoToneEditor::~DemarcoToneEditor()
{
    setLookAndFeel(nullptr);
    for (auto* k : { &driveKnob, &detuneKnob,
                     &bitcrushBitsKnob, &bitcrushRateKnob,
                     &chorusRate, &chorusDepth,
                     &chorusMix, &chorusWidth, &vibratoRate, &vibratoDepth,
                     &wowFlutter, &reverbMix, &reverbTone, &outputTrim })
        k->setLookAndFeel(nullptr);
}

// ---------------------------------------------------------------------------
void DemarcoToneEditor::paint(juce::Graphics& g)
{
    g.fillAll(paper);

    // ---- Top bar ----
    auto area = getLocalBounds();
    auto topBar = area.removeFromTop(32);

    g.setColour(ink);
    // Left: BIG DAWG + red period
    auto leftPad = topBar.reduced(14, 6);
    auto displayFont = BigDawgLookAndFeel::makeDisplayFont(18.0f);
    g.setFont(displayFont);
    const juce::String brand = "BIG DAWG";
    const int brandW = (int)std::ceil(juce::GlyphArrangement::getStringWidth(displayFont, brand));
    g.drawFittedText(brand,
                     leftPad.withWidth(brandW),
                     juce::Justification::centredLeft, 1);
    // Red period — brand mark always at full saturation
    g.setColour(redPrimary);
    auto periodBox = leftPad.withX(leftPad.getX() + brandW).withWidth(14);
    g.drawFittedText(".", periodBox, juce::Justification::centredLeft, 1);

    // Right: "V0.1" (static) + PresetStepper (its own child component,
    // positioned in resized()).
    g.setColour(ink);
    g.setFont(BigDawgLookAndFeel::makeMonoFont(11.0f, false));
    const int versionW = 40;
    auto versionBox = juce::Rectangle<int>(
        presetStepper.getX() - versionW - 8, 0, versionW, 32);
    g.drawFittedText("V0.1", versionBox, juce::Justification::centredRight, 1);

    // Hard rule under top bar
    g.setColour(ink);
    g.fillRect(0, 32, getWidth(), 1);

    // ---- Section headers + hairline dividers ----
    // 8 columns: 01 DRIVE, 02 DETUNE, 03 BITCRUSH, 04 CHORUS, 05 VIBRATO,
    //            06 WOW, 07 REVERB, OUT. Total 840.
    const int yContent = 33;
    const int hContent = getHeight() - yContent;
    const int widths[8] = { 70, 70, 80, 190, 140, 70, 160, 60 };
    const juce::String nums[8]  = { "01", "02", "03", "04", "05", "06", "07", "OUT" };
    // Name is empty where a header-band toggle replaces it (CHORUS / REVERB),
    // and for the OUT column which already has no name.
    const juce::String names[8] = { "DRIVE", "DETUNE", "BITCRUSH",
                                    "", "VIBRATO", "WOW", "", "" };

    int x = 0;
    // Typography — Plex Mono (system mono fallback) across the board.
    //   numeral  : 10px bold — matches param labels for visual rhythm
    //   name     : 11px bold — slightly larger, the section's "headline"
    auto numFont  = BigDawgLookAndFeel::makeMonoFont(10.0f, true);
    auto nameFont = BigDawgLookAndFeel::makeMonoFont(11.0f, true);

    for (int i = 0; i < 8; ++i)
    {
        auto col = juce::Rectangle<int>(x, yContent, widths[i], hContent);

        // Panel fill — 2% darker than paper. Leave 1px on the right edge
        // (except last column) so the lighter paper shows through as a
        // subtle inter-section gutter. No ink strokes.
        g.setColour(panel);
        g.fillRect(col.withTrimmedRight(i < 7 ? 1 : 0));

        // Header band — 30px tall. 6px vertical padding gives 18px of
        // text area: 10px numeral row + 2px gap + 6..13px name row (depending
        // on what's left). For a 30-tall band reduced by (8,6) we get 18
        // vertical → split into num(10) + nameRow(remainder, ~8px) which
        // drawFittedText renders cleanly at 11px nominal.
        auto header = col.withHeight(30).reduced(8, 6);
        g.setColour(ink);
        if (i < 7)
        {
            // numeral on top-left, then name directly below (unless the
            // header is hosting a mode toggle — CHORUS / REVERB).
            auto numBox = header.removeFromTop(10);
            g.setFont(numFont);
            g.drawFittedText(nums[i], numBox, juce::Justification::topLeft, 1);

            if (names[i].isNotEmpty())
            {
                g.setFont(nameFont);
                g.drawFittedText(names[i], header,
                                 juce::Justification::topLeft, 1);
            }
        }
        else
        {
            // OUT section — single label in the full header area
            g.setFont(nameFont);
            g.drawFittedText(nums[i], header,
                             juce::Justification::topLeft, 1);
        }

        x += widths[i];
    }
}

// ---------------------------------------------------------------------------
void DemarcoToneEditor::resized()
{
    const int yContent = 33;

    // Top-bar right side, right-aligned, in this order (right to left):
    //   PresetStepper  |  V0.1  |  HI-FI toggle
    const int stepperW = 140;
    const int stepperH = 20;
    presetStepper.setBounds(getWidth() - stepperW - 14,
                            (32 - stepperH) / 2,
                            stepperW, stepperH);

    // V0.1 is painted in paint() as text; reserve 40px to its left.
    // HI-FI toggle sits to the left of V0.1 with a 14px gap.
    const int hifiW = 78;     // 39px per cell (LO-FI / HI-FI), readable at 10px mono bold
    const int hifiH = 18;
    const int hifiX = presetStepper.getX() - 8 /*stepper-V0.1 gap*/
                      - 40 /*V0.1*/ - 14 /*V0.1-toggle gap*/ - hifiW;
    hifiToggle.setBounds(hifiX, (32 - hifiH) / 2, hifiW, hifiH);

    // Column x-origins (must match paint()) — 8 columns, total 840.
    // 0=DRIVE 1=DETUNE 2=BITCRUSH 3=CHORUS 4=VIBRATO 5=WOW 6=REVERB 7=OUT
    const int widths[8] = { 70, 70, 80, 190, 140, 70, 160, 60 };
    int x0[8];
    int acc = 0;
    for (int i = 0; i < 8; ++i) { x0[i] = acc; acc += widths[i]; }

    // Vertical zones within each column:
    //   header band:  yContent .. yContent+30
    //   toggle band:  yContent+30 .. yContent+60  (used by CHORUS / VIBRATO)
    //   knob band:    yContent+60 .. 240
    const int headerH = 30;
    const int toggleH = 30;

    auto columnKnobRow = [&](int colIndex, int knobCount, std::function<void(int, juce::Rectangle<int>)> place)
    {
        const int xStart = x0[colIndex] + 6;
        const int xEnd   = x0[colIndex] + widths[colIndex] - 6;
        const int w      = xEnd - xStart;
        const int perKnob = w / juce::jmax(1, knobCount);
        const int yKnob = yContent + headerH + toggleH - 6;
        const int hKnob = getHeight() - yKnob - 4;
        for (int i = 0; i < knobCount; ++i)
        {
            auto box = juce::Rectangle<int>(xStart + i * perKnob, yKnob, perKnob, hKnob);
            place(i, box);
        }
    };

    // 01 DRIVE (col 0)
    columnKnobRow(0, 1, [this](int, juce::Rectangle<int> r)
    {
        driveKnob.setBounds(r.reduced(4, 2));
    });

    // 02 DETUNE (col 1) — OFF/ON toggle above the CENTS knob
    {
        auto toggleBand = juce::Rectangle<int>(x0[1] + 8, yContent + headerH - 2,
                                               widths[1] - 16, toggleH - 4);
        auto t = toggleBand.withSizeKeepingCentre(52, 16);
        detuneOnToggle.setBounds(t);
    }
    columnKnobRow(1, 1, [this](int, juce::Rectangle<int> r)
    {
        detuneKnob.setBounds(r.reduced(4, 2));
    });

    // 03 BITCRUSH (col 2) — two knobs side-by-side (BITS, RATE)
    columnKnobRow(2, 2, [this](int i, juce::Rectangle<int> r)
    {
        ArcKnob* knobs[] = { &bitcrushBitsKnob, &bitcrushRateKnob };
        knobs[i]->setBounds(r.reduced(2, 2));
    });

    // 04 CHORUS / FLANGER (col 3)
    //   Header band: mode toggle (CHORUS / FLANGER) replaces the static name.
    //   Toggle band: OFF/ON + SINE/TRI side-by-side (unchanged from prior layout).
    //   Knob band: 4 knobs.
    {
        // Mode toggle sits inside the header, to the right of the numeral.
        const int modeY = yContent + 6;                // roughly aligned with numeral
        const int modeH = 16;
        const int modeW = widths[3] - 30;              // leave space for the numeral on the left
        chorusModeToggle.setBounds(x0[3] + 24, modeY, modeW, modeH);
    }
    {
        auto toggleBand = juce::Rectangle<int>(x0[3] + 8, yContent + headerH - 2,
                                               widths[3] - 16, toggleH - 4);
        const int tw = 70, th = 18, gap = 14;
        const int totalW = tw * 2 + gap;
        const int tx = toggleBand.getCentreX() - totalW / 2;
        const int ty = toggleBand.getCentreY() - th / 2;
        chorusOnToggle.setBounds(tx,              ty, tw, th);
        chorusShapeToggle.setBounds(tx + tw + gap, ty, tw, th);
    }
    columnKnobRow(3, 4, [this](int i, juce::Rectangle<int> r)
    {
        ArcKnob* knobs[] = { &chorusRate, &chorusDepth, &chorusMix, &chorusWidth };
        knobs[i]->setBounds(r.reduced(3, 2));
    });

    // 05 VIBRATO (col 4) — OFF/ON toggle + 2 knobs
    {
        auto toggleBand = juce::Rectangle<int>(x0[4] + 8, yContent + headerH - 2,
                                               widths[4] - 16, toggleH - 4);
        auto t = toggleBand.withSizeKeepingCentre(80, 18);
        vibratoOnToggle.setBounds(t);
    }
    columnKnobRow(4, 2, [this](int i, juce::Rectangle<int> r)
    {
        ArcKnob* knobs[] = { &vibratoRate, &vibratoDepth };
        knobs[i]->setBounds(r.reduced(4, 2));
    });

    // 06 WOW (col 5)
    columnKnobRow(5, 1, [this](int, juce::Rectangle<int> r)
    {
        wowFlutter.setBounds(r.reduced(4, 2));
    });

    // 07 REVERB (col 6)
    //   Header band: mode toggle (SPRING / SLAP / PLATE) replaces the static name.
    //   Toggle band: OFF/ON.
    //   Knob band: MIX + TONE.
    {
        const int modeY = yContent + 6;
        const int modeH = 16;
        const int modeW = widths[6] - 30;              // leave space for numeral
        reverbModeToggle.setBounds(x0[6] + 24, modeY, modeW, modeH);
    }
    {
        auto toggleBand = juce::Rectangle<int>(x0[6] + 8, yContent + headerH - 2,
                                               widths[6] - 16, toggleH - 4);
        auto t = toggleBand.withSizeKeepingCentre(70, 18);
        reverbOnToggle.setBounds(t);
    }
    columnKnobRow(6, 2, [this](int i, juce::Rectangle<int> r)
    {
        ArcKnob* knobs[] = { &reverbMix, &reverbTone };
        knobs[i]->setBounds(r.reduced(4, 2));
    });

    // OUT (col 7)
    columnKnobRow(7, 1, [this](int, juce::Rectangle<int> r)
    {
        outputTrim.setBounds(r.reduced(4, 2));
    });
}

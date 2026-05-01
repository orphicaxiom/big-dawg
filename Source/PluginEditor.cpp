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

juce::Font BigDawgLookAndFeel::makeMonoTabularFont(float heightPx)
{
    // Plex Mono is monospaced, so digits already align at the same width;
    // tabular figures are implicit. Function exists for intent labelling.
    return makeMonoFont(heightPx, false);
}

// ---------------------------------------------------------------------------
// Knob
// ---------------------------------------------------------------------------
Knob::Knob(const juce::String& label, Format fmt, juce::Colour stage)
    : nameLabel(label), format(fmt), stageColour(stage)
{
    setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,   // 7:30 (mirrored: -135°)
                        juce::MathConstants<float>::pi * 2.75f,   // 4:30 (+135°)
                        true);
    setColour(juce::Slider::textBoxTextColourId, ink);
}

double Knob::currentDisplayValue() const
{
    return overrideActive ? (double)overrideValue : getValue();
}

void Knob::setVisualOverride(bool active, float v)
{
    if (overrideActive == active && std::abs(overrideValue - v) < 1e-4f)
        return;
    overrideActive = active;
    overrideValue  = v;
    repaint();
}

juce::String Knob::formatValue() const
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

void Knob::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    const float labelH = 12.0f;
    const float valueH = 12.0f;
    const float gap    = 2.0f;

    // Reserve the bottom rows for label + value, leaving the rest for the disc.
    auto valueBox = bounds.removeFromBottom(valueH);
    bounds.removeFromBottom(gap);
    auto labelBox = bounds.removeFromBottom(labelH);
    bounds.removeFromBottom(gap);
    auto knobBox  = bounds;

    // Disc geometry — paper fill, 1.5px ink outline, fits within knobBox.
    const float diameter = std::min(knobBox.getWidth(), knobBox.getHeight()) - 4.0f;
    auto circle = juce::Rectangle<float>(diameter, diameter)
                      .withCentre(knobBox.getCentre());

    g.setColour(panelLight);
    g.fillEllipse(circle);
    g.setColour(ink);
    g.drawEllipse(circle, 1.5f);

    // Indicator line in stage colour. -135° (min) to +135° (max), with 0° = up.
    const float r  = diameter * 0.5f;
    const float cx = circle.getCentreX();
    const float cy = circle.getCentreY();

    const double range = getMaximum() - getMinimum();
    const double dispV = currentDisplayValue();
    const double norm  = range > 0.0 ? (dispV - getMinimum()) / range : 0.0;
    const float angle  = juce::MathConstants<float>::pi * (-0.75f + 1.5f * (float)norm);

    const float indLen = r * 0.78f;
    const float ix = cx + std::sin(angle) * indLen;
    const float iy = cy - std::cos(angle) * indLen;

    // Reduced opacity when displaying a hi-fi-derived value.
    const float alpha = overrideActive ? 0.55f : 1.0f;

    g.setColour(stageColour.withMultipliedAlpha(alpha));
    g.drawLine(cx, cy, ix, iy, 2.5f);

    // Label + value
    const auto textInk = ink.withMultipliedAlpha(alpha);
    g.setColour(textInk);
    g.setFont(BigDawgLookAndFeel::makeMonoFont(9.5f, true));
    g.drawFittedText(nameLabel.toUpperCase(),
                     labelBox.toNearestInt(),
                     juce::Justification::centred, 1);

    g.setColour(textInk);
    g.setFont(BigDawgLookAndFeel::makeMonoTabularFont(9.5f));
    g.drawFittedText(formatValue(),
                     valueBox.toNearestInt(),
                     juce::Justification::centred, 1);
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

    g.setFont(BigDawgLookAndFeel::makeMonoFont(9.5f, true));

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
            g.setColour(panelLight);
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
// SignalDiagram
// ---------------------------------------------------------------------------
SignalDiagram::SignalDiagram(DemarcoToneProcessor& p) : proc(p)
{
    startTimerHz(30);
}

SignalDiagram::~SignalDiagram() = default;

void SignalDiagram::timerCallback()
{
    auto& apvts = proc.getAPVTS();
    auto isOff = [&apvts](const juce::String& id) -> bool
    {
        if (auto* raw = apvts.getRawParameterValue(id))
            return raw->load() < 0.5f; // < 0.5 = OFF / bypassed
        return false;
    };

    std::array<bool, 8> now {
        false,                            // 01 Drive       (no APVTS bypass)
        isOff(ParamID::detuneOn),         // 02 Detune
        false,                            // 03 Bitcrush    (transparent-values bypass; not tracked)
        isOff(ParamID::chorusOn),         // 04 Chorus
        isOff(ParamID::vibratoOn),        // 05 Vibrato
        false,                            // 06 Wow
        isOff(ParamID::reverbOn),         // 07 Spring
        false                             // 08 EQ          (placeholder)
    };

    if (now != bypassedLastSeen)
    {
        bypassedLastSeen = now;
        repaint();
    }
}

void SignalDiagram::paint(juce::Graphics& g)
{
    const float marginX       = 16.0f;
    const float endpointSize  = 12.0f;
    const float lineThickness = 10.0f;
    const float tickHeight    = 16.0f;   // 3 above + 10 line + 3 below

    auto bounds = getLocalBounds().toFloat();

    // Line vertical position — leaves room below for sub-numbers + labels.
    const float lineTop    = bounds.getY() + 14.0f;
    const float lineCenter = lineTop + lineThickness * 0.5f;

    const float xStart = bounds.getX() + marginX;
    const float xEnd   = bounds.getRight() - marginX;
    const float spanW  = xEnd - xStart;
    const float segW   = spanW / 8.0f;

    // 12×12 ink endpoint squares (IN, OUT) centered on the line.
    g.setColour(ink);
    g.fillRect(xStart - endpointSize * 0.5f,
               lineCenter - endpointSize * 0.5f,
               endpointSize, endpointSize);
    g.fillRect(xEnd - endpointSize * 0.5f,
               lineCenter - endpointSize * 0.5f,
               endpointSize, endpointSize);

    // 8 colored segments meeting at boundaries.
    const std::array<juce::Colour, 8> stageColours = {
        stDrive, stDetune, stBitcrush, stChorus,
        stVibrato, stWow, stSpring, stEQ
    };
    const std::array<juce::String, 8> stageLabels = {
        "DRIVE", "DETUNE", "BITCRUSH", "CHORUS",
        "VIBRATO", "WOW", "SPRING", "EQ"
    };
    const std::array<juce::String, 8> stageNumbers = {
        "01", "02", "03", "04", "05", "06", "07", "08"
    };

    for (size_t i = 0; i < 8; ++i)
    {
        const float segX = xStart + (float)i * segW;
        const auto col = bypassedLastSeen[i] ? bypass : stageColours[i];
        g.setColour(col);
        g.fillRect(segX, lineTop, segW, lineThickness);
    }

    // 7 hairline ticks at segment boundaries.
    g.setColour(ink);
    for (int i = 1; i < 8; ++i)
    {
        const float tx = xStart + (float)i * segW;
        g.fillRect(tx - 0.5f, lineTop - 3.0f, 1.0f, tickHeight);
    }

    // Sub-numbers (9px) at 50% opacity, then stage labels (9.5px bold) below.
    const float subY   = lineTop + lineThickness + 6.0f;
    const float labelY = subY + 11.0f;

    for (size_t i = 0; i < 8; ++i)
    {
        const float segCx    = xStart + (float)i * segW + segW * 0.5f;
        const float dimAlpha = bypassedLastSeen[i] ? 0.4f : 1.0f;

        g.setColour(ink.withMultipliedAlpha(0.5f * dimAlpha));
        g.setFont(BigDawgLookAndFeel::makeMonoFont(8.0f, false));
        g.drawFittedText(stageNumbers[i],
                         juce::Rectangle<float>(segCx - 30.0f, subY, 60.0f, 10.0f).toNearestInt(),
                         juce::Justification::centred, 1);

        g.setColour(ink.withMultipliedAlpha(dimAlpha));
        g.setFont(BigDawgLookAndFeel::makeMonoFont(9.5f, true));
        g.drawFittedText(stageLabels[i],
                         juce::Rectangle<float>(segCx - 60.0f, labelY, 120.0f, 12.0f).toNearestInt(),
                         juce::Justification::centred, 1);
    }
}

// ---------------------------------------------------------------------------
// PresetSelector
// ---------------------------------------------------------------------------
PresetSelector::PresetSelector(DemarcoToneProcessor& p) : proc(p)
{
    setInterceptsMouseClicks(true, false);
}

void PresetSelector::refresh() { repaint(); }

juce::Rectangle<int> PresetSelector::leftChevronRect() const
{
    // < character sits left of the name; total span ≈ width of bounds.
    auto b = getLocalBounds();
    const int chevW = 18;
    return { b.getX() + 110, b.getY(), chevW, b.getHeight() };
}

juce::Rectangle<int> PresetSelector::rightChevronRect() const
{
    auto b = getLocalBounds();
    const int chevW = 18;
    return { b.getRight() - chevW, b.getY(), chevW, b.getHeight() };
}

juce::Rectangle<int> PresetSelector::nameRect() const
{
    auto b = getLocalBounds();
    return { b.getX() + 132, b.getY(), b.getWidth() - 132 - 22, b.getHeight() };
}

void PresetSelector::paint(juce::Graphics& g)
{
    const int curIdx = proc.getCurrentPresetIndex();
    const auto names = proc.getPresetNames();
    const int total = juce::jmax(1, names.size());

    auto b = getLocalBounds();

    // Left text block: V0.1   PRESET 03/05
    g.setColour(ink);
    g.setFont(BigDawgLookAndFeel::makeMonoFont(10.0f, false));
    g.drawFittedText("V0.1",
                     juce::Rectangle<int>(b.getX(), b.getY(), 28, b.getHeight()),
                     juce::Justification::centredLeft, 1);

    g.setFont(BigDawgLookAndFeel::makeMonoFont(10.0f, true));
    g.drawFittedText("PRESET",
                     juce::Rectangle<int>(b.getX() + 32, b.getY(), 48, b.getHeight()),
                     juce::Justification::centredLeft, 1);

    g.setFont(BigDawgLookAndFeel::makeMonoTabularFont(10.0f));
    juce::String idx;
    idx << juce::String(curIdx + 1).paddedLeft('0', (size_t)2)
        << "/"
        << juce::String(total).paddedLeft('0', (size_t)2);
    g.drawFittedText(idx,
                     juce::Rectangle<int>(b.getX() + 82, b.getY(), 28, b.getHeight()),
                     juce::Justification::centredLeft, 1);

    // Chevrons (Plex Mono)
    g.setFont(BigDawgLookAndFeel::makeMonoFont(13.0f, false));
    g.drawFittedText("<", leftChevronRect(),  juce::Justification::centred, 1);
    g.drawFittedText(">", rightChevronRect(), juce::Justification::centred, 1);

    // Preset name (Archivo Black, all caps)
    g.setFont(BigDawgLookAndFeel::makeDisplayFont(13.0f));
    juce::String name = names[juce::jlimit(0, total - 1, curIdx)].toUpperCase();
    g.drawFittedText(name, nameRect(), juce::Justification::centred, 1);
}

void PresetSelector::mouseDown(const juce::MouseEvent& e)
{
    const auto names = proc.getPresetNames();
    if (names.isEmpty()) return;
    const int curIdx = proc.getCurrentPresetIndex();
    const int total = names.size();

    if (leftChevronRect().contains(e.getPosition()))
    {
        const int newIdx = (curIdx - 1 + total) % total;
        proc.loadPreset(newIdx);
        repaint();
        return;
    }
    if (rightChevronRect().contains(e.getPosition()))
    {
        const int newIdx = (curIdx + 1) % total;
        proc.loadPreset(newIdx);
        repaint();
        return;
    }
    if (nameRect().contains(e.getPosition()))
    {
        juce::PopupMenu menu;
        for (int i = 0; i < names.size(); ++i)
            menu.addItem(i + 1, names[i], true, i == curIdx);

        juce::Component::SafePointer<PresetSelector> safe(this);
        menu.showMenuAsync(
            juce::PopupMenu::Options().withTargetComponent(this),
            [safe](int result)
            {
                if (safe == nullptr || result <= 0) return;
                safe->proc.loadPreset(result - 1);
                safe->repaint();
            });
    }
}

// ---------------------------------------------------------------------------
// DemarcoToneEditor
// ---------------------------------------------------------------------------
DemarcoToneEditor::DemarcoToneEditor(DemarcoToneProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&lnf);

    auto addKnob = [this](Knob& k)
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
    addAndMakeVisible(chorusOnToggle);
    addAndMakeVisible(chorusModeToggle);
    addAndMakeVisible(chorusShapeToggle);
    addAndMakeVisible(vibratoOnToggle);
    addAndMakeVisible(reverbOnToggle);
    addAndMakeVisible(reverbModeToggle);
    addAndMakeVisible(hifiToggle);

    addAndMakeVisible(presetSelector);
    addAndMakeVisible(diagram);

    detuneKnob.setDoubleClickReturnValue(true, 0.0);

    // Parameter attachments — unchanged from v0.1.x; APVTS bindings preserved.
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

    detuneOnAtt  = std::make_unique<BoolToggleAttachment>(apvts, ParamID::detuneOn,  detuneOnToggle);
    chorusOnAtt  = std::make_unique<BoolToggleAttachment>(apvts, ParamID::chorusOn,  chorusOnToggle);
    vibratoOnAtt = std::make_unique<BoolToggleAttachment>(apvts, ParamID::vibratoOn, vibratoOnToggle);
    reverbOnAtt  = std::make_unique<BoolToggleAttachment>(apvts, ParamID::reverbOn,  reverbOnToggle);
    hifiAtt      = std::make_unique<BoolToggleAttachment>(apvts, ParamID::hifiMode,  hifiToggle);

    chorusShapeAtt = std::make_unique<ChoiceToggleAttachment>(apvts, ParamID::chorusShape, chorusShapeToggle);
    chorusModeAtt  = std::make_unique<ChoiceToggleAttachment>(apvts, ParamID::chorusMode,  chorusModeToggle);
    reverbModeAtt  = std::make_unique<ChoiceToggleAttachment>(apvts, ParamID::reverbMode,  reverbModeToggle);

    setSize(1000, 350);
    setResizable(false, false);

    startTimerHz(30); // hi-fi visual sync
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
// Hi-fi visual sync (carries the v0.1.3 behavior into v0.2.0 knobs).
// ---------------------------------------------------------------------------
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

    auto applyOverride = [hifi](Knob& k, float scaled)
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

    presetSelector.refresh();
}

// ---------------------------------------------------------------------------
// Layout — 1000×350, three-row stack
// ---------------------------------------------------------------------------
namespace { // local layout constants
    constexpr int kWindowW   = 1000;
    constexpr int kWindowH   = 350;
    constexpr int kHeaderH   = 50;
    constexpr int kDiagramH  = 70;
    // stage grid = 230

    constexpr int kSideMargin = 12;

    // Stage stripe colours, by index (0..8)
    inline juce::Colour stageStripeColour(int idx)
    {
        using namespace BigDawgColors;
        switch (idx)
        {
            case 0: return stDrive;
            case 1: return stDetune;
            case 2: return stBitcrush;
            case 3: return stChorus;
            case 4: return stVibrato;
            case 5: return stWow;
            case 6: return stSpring;
            case 7: return stEQ;
            case 8: return stOut;
            default: return BigDawgColors::ink;
        }
    }

    inline const char* stageNumber(int idx)
    {
        static const char* nums[] = { "01","02","03","04","05","06","07","08","" };
        return idx >= 0 && idx < 9 ? nums[idx] : "";
    }
    inline const char* stageName(int idx)
    {
        static const char* nms[] = { "DRIVE","DETUNE","BITCRUSH","CHORUS",
                                      "VIBRATO","WOW","SPRING","EQ","OUT" };
        return idx >= 0 && idx < 9 ? nms[idx] : "";
    }
}

// ---------------------------------------------------------------------------
void DemarcoToneEditor::paint(juce::Graphics& g)
{
    g.fillAll(paper);

    // ── Header: wordmark + bottom rule ───────────────────────────────────
    auto displayFont = BigDawgLookAndFeel::makeDisplayFont(22.0f);
    g.setFont(displayFont);
    const juce::String brand = "BIG DAWG";
    const int brandW = (int)std::ceil(juce::GlyphArrangement::getStringWidth(displayFont, brand));

    g.setColour(ink);
    g.drawFittedText(brand,
                     juce::Rectangle<int>(kSideMargin + 2, 0, brandW, kHeaderH),
                     juce::Justification::centredLeft, 1);

    g.setColour(stDrive); // Olivetti red period
    g.drawFittedText(".",
                     juce::Rectangle<int>(kSideMargin + 2 + brandW, 0, 14, kHeaderH),
                     juce::Justification::centredLeft, 1);

    // 1.5px ink rule under the header
    g.setColour(ink);
    g.fillRect(juce::Rectangle<float>(0.0f, (float)kHeaderH - 1.0f,
                                      (float)kWindowW, 1.5f));

    // ── Stage grid: 9 columns, top stripe + section header per column ────
    const int yStage = kHeaderH + kDiagramH;          // 120
    const int gridX  = kSideMargin;
    const int gridW  = kWindowW - 2 * kSideMargin;    // 976
    const int colW   = gridW / 9;                     // 108
    const int stripeH = 5;

    auto numFont  = BigDawgLookAndFeel::makeMonoFont(9.5f, false);
    auto nameFont = BigDawgLookAndFeel::makeDisplayFont(11.0f);

    for (int i = 0; i < 9; ++i)
    {
        const int colX = gridX + i * colW;

        // Top stripe — stage colour
        g.setColour(stageStripeColour(i));
        g.fillRect(juce::Rectangle<int>(colX + 2, yStage,
                                        colW - 4, stripeH));

        // Section number (small, half opacity)
        g.setColour(ink.withMultipliedAlpha(0.55f));
        g.setFont(numFont);
        g.drawFittedText(stageNumber(i),
                         juce::Rectangle<int>(colX + 8, yStage + stripeH + 2,
                                              colW - 16, 12),
                         juce::Justification::topLeft, 1);

        // Section name (Archivo Black)
        g.setColour(ink);
        g.setFont(nameFont);
        g.drawFittedText(stageName(i),
                         juce::Rectangle<int>(colX + 8, yStage + stripeH + 14,
                                              colW - 16, 14),
                         juce::Justification::topLeft, 1);

        // EQ placeholder text — v0.2.1 wiring
        if (i == 7)
        {
            g.setColour(ink.withMultipliedAlpha(0.4f));
            g.setFont(BigDawgLookAndFeel::makeMonoFont(8.5f, false));
            g.drawFittedText("VOICED IN",
                             juce::Rectangle<int>(colX + 4, yStage + 80,
                                                  colW - 8, 12),
                             juce::Justification::centred, 1);
            g.drawFittedText("v0.2.1",
                             juce::Rectangle<int>(colX + 4, yStage + 92,
                                                  colW - 8, 12),
                             juce::Justification::centred, 1);
        }
    }
}

void DemarcoToneEditor::resized()
{
    // ── Header ────────────────────────────────────────────────────────────
    {
        const int hifiW = 88;
        const int hifiH = 18;
        hifiToggle.setBounds(kWindowW - kSideMargin - hifiW,
                              (kHeaderH - hifiH) / 2,
                              hifiW, hifiH);

        const int psW = 320;
        presetSelector.setBounds((kWindowW - psW) / 2,
                                 0, psW, kHeaderH);
    }

    // ── Signal diagram ────────────────────────────────────────────────────
    diagram.setBounds(0, kHeaderH, kWindowW, kDiagramH);

    // ── Stage grid ────────────────────────────────────────────────────────
    const int yStage = kHeaderH + kDiagramH;
    const int hStage = kWindowH - yStage;
    const int gridX  = kSideMargin;
    const int gridW  = kWindowW - 2 * kSideMargin;
    const int colW   = gridW / 9;
    const int stripeH = 5;
    const int headerBandH = 30;            // stripe + numeral + name
    const int toggleBandH = 22;            // OFF/ON + (mode) row
    const int knobYTop = yStage + stripeH + headerBandH + toggleBandH + 4;
    const int knobAreaH = (yStage + hStage) - knobYTop - 4;

    auto colRect = [&](int i) -> juce::Rectangle<int>
    {
        return { gridX + i * colW, yStage, colW, hStage };
    };

    // Helper: place an OFF/ON-style 2-cell toggle in the top of the toggle band.
    auto placeOnOff = [&](BoxToggle& t, int colIndex, int width = 44, int height = 16)
    {
        const auto cr = colRect(colIndex);
        t.setBounds(cr.getRight() - width - 6,
                    yStage + stripeH + 2,         // align with section number
                    width, height);
    };

    // Helper: place a mode toggle centered in the toggle band.
    auto placeMode = [&](BoxToggle& t, int colIndex, int width)
    {
        const auto cr = colRect(colIndex);
        t.setBounds(cr.getCentreX() - width / 2,
                    yStage + stripeH + headerBandH - 6,
                    width, 16);
    };

    // Helper: lay out 1..3 knobs as a row inside a column. For 4 knobs, row 1
    // takes the first three and row 2 takes the fourth at column 1.
    auto placeKnobsSingleRow = [&](int colIndex, std::initializer_list<Knob*> knobs)
    {
        const auto cr = colRect(colIndex);
        const int n = (int)knobs.size();
        const int innerL = cr.getX() + 4;
        const int innerR = cr.getRight() - 4;
        const int innerW = innerR - innerL;
        const int cellW  = innerW / juce::jmax(1, n);
        int idx = 0;
        for (auto* k : knobs)
        {
            const int x = innerL + idx * cellW;
            const int knobBoxY = knobYTop;
            k->setBounds(x + 2, knobBoxY, cellW - 4, knobAreaH);
            ++idx;
        }
    };

    // 01 DRIVE — single knob, centered
    placeKnobsSingleRow(0, { &driveKnob });

    // 02 DETUNE — OFF/ON, single knob (CENTS)
    placeOnOff(detuneOnToggle, 1);
    placeKnobsSingleRow(1, { &detuneKnob });

    // 03 BITCRUSH — two knobs (no OFF/ON: bypass is implicit at transparent values)
    placeKnobsSingleRow(2, { &bitcrushBitsKnob, &bitcrushRateKnob });

    // 04 CHORUS — OFF/ON, mode toggle (CHO/FLG), shape toggle (SINE/TRI),
    //            row 1: RATE / DEPTH / MIX, row 2: WIDTH centered.
    placeOnOff(chorusOnToggle, 3);
    {
        // Two mode-band toggles side by side, centered.
        const int gap = 6;
        const int wMode = 50;   // CHO/FLG
        const int wShape = 50;  // SINE/TRI
        const int totalW = wMode + gap + wShape;
        const auto cr = colRect(3);
        const int bandY = yStage + stripeH + headerBandH - 6;
        const int xLeft = cr.getCentreX() - totalW / 2;
        chorusModeToggle.setBounds(xLeft, bandY, wMode, 16);
        chorusShapeToggle.setBounds(xLeft + wMode + gap, bandY, wShape, 16);
    }
    {
        const auto cr = colRect(3);
        const int innerL = cr.getX() + 4;
        const int innerR = cr.getRight() - 4;
        const int innerW = innerR - innerL;
        const int rowH   = knobAreaH / 2 - 2;
        const int cellW  = innerW / 3;

        // Row 1: RATE / DEPTH / MIX
        Knob* row1[] = { &chorusRate, &chorusDepth, &chorusMix };
        for (int i = 0; i < 3; ++i)
            row1[i]->setBounds(innerL + i * cellW + 1, knobYTop,
                               cellW - 2, rowH);

        // Row 2: WIDTH at column 1 (left), centered horizontally for visual balance
        chorusWidth.setBounds(innerL + cellW + 1, knobYTop + rowH + 4,
                              cellW - 2, rowH);
    }

    // 05 VIBRATO — OFF/ON, two knobs (RATE, DEPTH)
    placeOnOff(vibratoOnToggle, 4);
    placeKnobsSingleRow(4, { &vibratoRate, &vibratoDepth });

    // 06 WOW — single knob (AMOUNT), no OFF/ON
    placeKnobsSingleRow(5, { &wowFlutter });

    // 07 SPRING — OFF/ON, mode (SPR/SLP/PLT), two knobs (MIX, TONE)
    placeOnOff(reverbOnToggle, 6);
    placeMode(reverbModeToggle, 6, 78);
    placeKnobsSingleRow(6, { &reverbMix, &reverbTone });

    // 08 EQ — placeholder; nothing to position.

    // OUT — single knob (TRIM)
    placeKnobsSingleRow(8, { &outputTrim });
}

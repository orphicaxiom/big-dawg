#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

// ---------------------------------------------------------------------------
DemarcoToneProcessor::DemarcoToneProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input",   juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
    loadPreset(0);
}

// ---------------------------------------------------------------------------
APVTS::ParameterLayout DemarcoToneProcessor::createParameterLayout()
{
    using Range = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // 01 DRIVE
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamID::drive, 1 }, "Drive",
        Range(0.0f, 1.0f, 0.001f), 0.25f));

    // 02 DETUNE — cents, centered at 0
    p.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParamID::detuneOn, 1 }, "Detune On", true));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamID::detune, 1 }, "Detune",
        Range(-50.0f, 50.0f, 0.5f), -15.0f));

    // 03 BITCRUSH — bits and rate. Both default to max = transparent.
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamID::bitcrushBits, 1 }, "Bitcrush Bits",
        Range(4.0f, 16.0f, 0.1f), 16.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamID::bitcrushRate, 1 }, "Bitcrush Rate",
        Range(0.1f, 1.0f, 0.001f), 1.0f));

    // 04 CHORUS / FLANGER
    p.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParamID::chorusOn, 1 }, "Chorus On", true));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ ParamID::chorusMode, 1 }, "Chorus Mode",
        juce::StringArray{ "Chorus", "Flanger" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamID::chorusRate, 1 }, "Chorus Rate",
        Range(0.1f, 2.0f, 0.01f), 0.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamID::chorusDepth, 1 }, "Chorus Depth",
        Range(0.0f, 1.0f, 0.001f), 0.6f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamID::chorusMix, 1 }, "Chorus Mix",
        Range(0.0f, 1.0f, 0.001f), 0.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamID::chorusWidth, 1 }, "Chorus Width",
        Range(0.0f, 1.0f, 0.001f), 0.8f));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ ParamID::chorusShape, 1 }, "Chorus Shape",
        juce::StringArray{ "Sine", "Triangle" }, 0));

    // 04 VIBRATO
    p.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParamID::vibratoOn, 1 }, "Vibrato On", false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamID::vibratoRate, 1 }, "Vibrato Rate",
        Range(1.0f, 8.0f, 0.01f), 5.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamID::vibratoDepth, 1 }, "Vibrato Depth",
        Range(0.0f, 1.0f, 0.001f), 0.15f));

    // 05 WOW / FLUTTER
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamID::wowFlutter, 1 }, "Wow/Flutter",
        Range(0.0f, 1.0f, 0.001f), 0.20f));

    // 07 REVERB (SPRING / SLAP / PLATE)
    p.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParamID::reverbOn, 1 }, "Reverb On", true));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ ParamID::reverbMode, 1 }, "Reverb Mode",
        juce::StringArray{ "Spring", "Slap", "Plate" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamID::reverbMix, 1 }, "Reverb Mix",
        Range(0.0f, 1.0f, 0.001f), 0.15f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamID::reverbTone, 1 }, "Reverb Tone",
        Range(-1.0f, 1.0f, 0.001f), -0.2f));

    // OUT
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamID::outputTrim, 1 }, "Output Trim",
        Range(-24.0f, 12.0f, 0.1f), 0.0f));

    // Global mode (persists across preset changes).
    p.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParamID::hifiMode, 1 }, "Hi-Fi Mode", false));

    return { p.begin(), p.end() };
}

// ---------------------------------------------------------------------------
void DemarcoToneProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize  = samplesPerBlock;

    const uint32_t numChannels = (uint32_t)juce::jmax(1, getTotalNumOutputChannels());

    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        (int)numChannels,
        1,                                             // log2 factor — 1 = 2x
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true                                           // integer latency
    );
    oversampler->initProcessing((size_t)samplesPerBlock);
    oversampler->reset();
    setLatencySamples((int)oversampler->getLatencyInSamples());

    // Upsampled stages: sample rate is 2x, max block is 2x.
    juce::dsp::ProcessSpec upSpec {
        sampleRate * 2.0,
        (uint32_t)(samplesPerBlock * 2),
        numChannels
    };
    softClip.prepare(upSpec);
    detune.prepare(upSpec);
    bitcrusher.prepare(upSpec);
    chorus.prepare(upSpec);

    // Base-rate stages.
    juce::dsp::ProcessSpec baseSpec {
        sampleRate,
        (uint32_t)samplesPerBlock,
        numChannels
    };
    vibrato.prepare(baseSpec);
    wowFlutter.prepare(baseSpec);
    springReverb.prepare(baseSpec);
    slapDelay.prepare(baseSpec);
    plateReverb.prepare(baseSpec);

    smoothedTrim.reset(sampleRate, 0.05);
    smoothedTrim.setCurrentAndTargetValue(1.0f);

    // Push current parameter values into the DSP so the first block
    // doesn't snap from defaults to user values.
    syncParametersToDsp();
    softClip.reset();
    detune.reset();
    bitcrusher.reset();
    chorus.reset();
    vibrato.reset();
    wowFlutter.reset();
    springReverb.reset();
    slapDelay.reset();
    plateReverb.reset();
}

void DemarcoToneProcessor::releaseResources()
{
    if (oversampler != nullptr)
        oversampler->reset();
}

bool DemarcoToneProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void DemarcoToneProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);
    juce::ScopedNoDenormals noDenormals;

    const int totalIn  = getTotalNumInputChannels();
    const int totalOut = getTotalNumOutputChannels();
    for (int ch = totalIn; ch < totalOut; ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    syncParametersToDsp();

    if (oversampler == nullptr)
        return;

    juce::dsp::AudioBlock<float> block(buffer);

    // === Upsampled stages =========================================
    auto upBlock = oversampler->processSamplesUp(block);
    softClip.process(upBlock);
    detune.process(upBlock);
    bitcrusher.process(upBlock);
    chorus.process(upBlock);
    oversampler->processSamplesDown(block);

    // === Base-rate stages =========================================
    vibrato.process(block);
    wowFlutter.process(block);

    // Reverb dispatch — exactly one of the three runs per block.
    switch (currentReverbMode)
    {
        case ReverbMode::Spring: springReverb.process(block); break;
        case ReverbMode::Slap:   slapDelay.process(block);    break;
        case ReverbMode::Plate:  plateReverb.process(block);  break;
    }

    // === Output trim ==============================================
    const auto numSamples  = block.getNumSamples();
    const auto numChannels = block.getNumChannels();
    for (size_t i = 0; i < numSamples; ++i)
    {
        const float g = smoothedTrim.getNextValue();
        for (size_t ch = 0; ch < numChannels; ++ch)
            block.getChannelPointer(ch)[i] *= g;
    }
}

void DemarcoToneProcessor::syncParametersToDsp()
{
    auto getF = [this](const juce::String& id) -> float
    {
        if (auto* raw = apvts.getRawParameterValue(id))
            return raw->load();
        return 0.0f;
    };

    // Hi-Fi mode reduces all lo-fi-character stages at the DSP boundary
    // while preserving the spring reverb tone and the core preset structure.
    // Stored APVTS values are untouched; only the values handed to the DSP
    // stages are reduced. Spring reverb stays unscaled — its tone is part
    // of the preset's identity, not its lo-fi-ness.
    //
    // Scale factors live in HifiScale (PluginProcessor.h) so the editor's
    // visual sync can read the same constants.
    const bool hifi = getF(ParamID::hifiMode) >= 0.5f;

    const float driveEff = hifi ? getF(ParamID::drive) * HifiScale::drive
                                : getF(ParamID::drive);
    softClip.setDrive(driveEff);

    detune.setBypassed(getF(ParamID::detuneOn) < 0.5f);
    const float detuneEff = hifi ? getF(ParamID::detune) * HifiScale::detune
                                 : getF(ParamID::detune);
    detune.setCents(detuneEff);

    // Bitcrush in hi-fi: full bypass to transparent values. Bitcrush
    // perceptually doesn't scale linearly — partial reduction sounds
    // worse, not cleaner.
    if (hifi)
    {
        bitcrusher.setBits(HifiScale::bitcrushBitsBypass);
        bitcrusher.setRate(HifiScale::bitcrushRateBypass);
    }
    else
    {
        bitcrusher.setBits(getF(ParamID::bitcrushBits));
        bitcrusher.setRate(getF(ParamID::bitcrushRate));
    }

    chorus.setBypassed(getF(ParamID::chorusOn) < 0.5f);
    chorus.setMode (getF(ParamID::chorusMode) < 0.5f
                    ? Chorus::Mode::Chorus : Chorus::Mode::Flanger);
    chorus.setRate (getF(ParamID::chorusRate));
    chorus.setDepth(getF(ParamID::chorusDepth));
    const float chorusMixEff = hifi ? getF(ParamID::chorusMix) * HifiScale::chorusMix
                                    : getF(ParamID::chorusMix);
    chorus.setMix  (chorusMixEff);
    chorus.setWidth(getF(ParamID::chorusWidth));
    chorus.setShape(getF(ParamID::chorusShape) < 0.5f
                    ? Chorus::Shape::Sine : Chorus::Shape::Triangle);

    vibrato.setBypassed(getF(ParamID::vibratoOn) < 0.5f);
    vibrato.setRate (getF(ParamID::vibratoRate));
    const float vibratoDepthEff = hifi ? getF(ParamID::vibratoDepth) * HifiScale::vibratoDepth
                                       : getF(ParamID::vibratoDepth);
    vibrato.setDepth(vibratoDepthEff);

    const float wowEff = hifi ? getF(ParamID::wowFlutter) * HifiScale::wow
                              : getF(ParamID::wowFlutter);
    wowFlutter.setAmount(wowEff);

    // Reverb: one param set pushed to all three classes; dispatcher picks
    // which one runs. Keeps shared state (mix/tone/bypass) coherent across
    // mode switches.
    const bool  reverbBypassed = getF(ParamID::reverbOn) < 0.5f;
    const float reverbMix01    = getF(ParamID::reverbMix);
    const float reverbToneBip  = getF(ParamID::reverbTone);
    springReverb.setBypassed(reverbBypassed);
    springReverb.setMix (reverbMix01);
    springReverb.setTone(reverbToneBip);
    slapDelay.setBypassed(reverbBypassed);
    slapDelay.setMix (reverbMix01);
    slapDelay.setTone(reverbToneBip);
    plateReverb.setBypassed(reverbBypassed);
    plateReverb.setMix (reverbMix01);
    plateReverb.setTone(reverbToneBip);

    const float modeF = getF(ParamID::reverbMode);
    currentReverbMode = modeF < 0.5f ? ReverbMode::Spring
                      : modeF < 1.5f ? ReverbMode::Slap
                                     : ReverbMode::Plate;

    const float trimDb = getF(ParamID::outputTrim);
    smoothedTrim.setTargetValue(juce::Decibels::decibelsToGain(trimDb));
}

// ---------------------------------------------------------------------------
juce::AudioProcessorEditor* DemarcoToneProcessor::createEditor()
{
    return new DemarcoToneEditor(*this);
}

// ---------------------------------------------------------------------------
void DemarcoToneProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void DemarcoToneProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

// ---------------------------------------------------------------------------
// Preset dispatcher
// ---------------------------------------------------------------------------
void DemarcoToneProcessor::loadPreset(int index)
{
    switch (index)
    {
        case 4:
            loadBaselinePreset();
            currentPresetIndex = 4;
            break;
        case 3:
            loadSlowcorePreset();
            currentPresetIndex = 3;
            break;
        case 2:
            loadPalthPreset();
            currentPresetIndex = 2;
            break;
        case 1:
            loadElvisPreset();
            currentPresetIndex = 1;
            break;
        case 0:
        default:
            loadViceroyPreset();
            currentPresetIndex = 0;
            break;
    }
}

static void setParam(APVTS& apvts, const juce::String& id, float value)
{
    if (auto* p = apvts.getParameter(id))
        p->setValueNotifyingHost(p->convertTo0to1(value));
}

void DemarcoToneProcessor::loadViceroyPreset()
{
    setParam(apvts, ParamID::drive,        0.25f);
    setParam(apvts, ParamID::detuneOn,      1.0f); // on
    setParam(apvts, ParamID::detune,      -15.0f);
    // v1 additions — explicit bypass/neutral so VICEROY is behaviorally unchanged.
    setParam(apvts, ParamID::bitcrushBits, 16.0f); // transparent
    setParam(apvts, ParamID::bitcrushRate,  1.0f); // transparent
    setParam(apvts, ParamID::chorusMode,    0.0f); // Chorus (not Flanger)
    setParam(apvts, ParamID::chorusOn,      1.0f); // on
    setParam(apvts, ParamID::chorusRate,    0.5f);
    setParam(apvts, ParamID::chorusDepth,   0.6f);
    setParam(apvts, ParamID::chorusMix,     0.5f);
    setParam(apvts, ParamID::chorusWidth,   0.8f);
    setParam(apvts, ParamID::chorusShape,   1.0f); // Triangle — more pronounced warble for Viceroy
    setParam(apvts, ParamID::vibratoOn,     1.0f); // on
    setParam(apvts, ParamID::vibratoRate,   5.0f);
    setParam(apvts, ParamID::vibratoDepth,  0.15f);
    setParam(apvts, ParamID::wowFlutter,    0.20f);
    setParam(apvts, ParamID::reverbOn,      1.0f); // on
    setParam(apvts, ParamID::reverbMode,    0.0f); // Spring
    setParam(apvts, ParamID::reverbMix,     0.15f);
    setParam(apvts, ParamID::reverbTone,   -0.2f);
    setParam(apvts, ParamID::outputTrim,    0.0f);
}

// ELVIS (THE SAD ONE) — Coma Cinema / Elvis Depressedly (Mat Cothran).
// Lightly over-driven, bedroom-tape feel, "flang voice" era vibrato.
// NOTE: chorusRate=0.0 is a placeholder — will be clamped to min (0.1 Hz) by
// the APVTS. Tune once rate mapping is verified post-DSP-voicing (aim ~0.7..1.0 Hz).
void DemarcoToneProcessor::loadElvisPreset()
{
    setParam(apvts, ParamID::drive,         0.22f);
    setParam(apvts, ParamID::detuneOn,      1.0f);
    setParam(apvts, ParamID::detune,      -25.0f);
    setParam(apvts, ParamID::bitcrushBits, 14.0f); // mild digital grain, tape-adjacent
    setParam(apvts, ParamID::bitcrushRate,  0.90f);
    setParam(apvts, ParamID::chorusMode,    0.0f); // Chorus
    setParam(apvts, ParamID::chorusOn,      1.0f);
    setParam(apvts, ParamID::chorusRate,    0.0f); // TBD — placeholder, clamps to 0.1 Hz
    setParam(apvts, ParamID::chorusDepth,   0.45f);
    setParam(apvts, ParamID::chorusMix,     0.35f);
    setParam(apvts, ParamID::chorusWidth,   0.30f);
    setParam(apvts, ParamID::chorusShape,   1.0f); // Triangle
    setParam(apvts, ParamID::vibratoOn,     0.0f); // off — Jim's call; rate/depth kept for experimentation
    setParam(apvts, ParamID::vibratoRate,   3.0f);
    setParam(apvts, ParamID::vibratoDepth,  0.40f);
    setParam(apvts, ParamID::wowFlutter,    0.55f); // handheld tape, heavy
    setParam(apvts, ParamID::reverbOn,      1.0f);
    setParam(apvts, ParamID::reverbMode,    0.0f); // Spring
    setParam(apvts, ParamID::reverbMix,     0.25f);
    setParam(apvts, ParamID::reverbTone,   -0.50f); // darker than Viceroy
    setParam(apvts, ParamID::outputTrim,    0.0f);
}

// PALTH — salvia palth / melanchole. Hot-interface-clip drive, slow wide chorus,
// heavy plate reverb. No tape, no intentional bitcrush.
// NOTE: drive 0.75 character depends on how SoftClip is voiced. Flag for v1.1
// on whether a tube/digital character toggle should be added.
void DemarcoToneProcessor::loadPalthPreset()
{
    setParam(apvts, ParamID::drive,         0.75f); // hot interface clip — signature
    setParam(apvts, ParamID::detuneOn,      1.0f);
    setParam(apvts, ParamID::detune,      -10.0f); // mild, not warble-forward
    setParam(apvts, ParamID::bitcrushBits, 16.0f); // transparent
    setParam(apvts, ParamID::bitcrushRate,  1.0f); // transparent
    setParam(apvts, ParamID::chorusMode,    0.0f); // Chorus
    setParam(apvts, ParamID::chorusOn,      1.0f);
    setParam(apvts, ParamID::chorusRate,    0.0f); // TBD — aim ~0.3 Hz, slow and dreamy
    setParam(apvts, ParamID::chorusDepth,   0.75f); // deep
    setParam(apvts, ParamID::chorusMix,     0.50f);
    setParam(apvts, ParamID::chorusWidth,   0.85f); // wide shoegaze stereo
    setParam(apvts, ParamID::chorusShape,   0.0f); // Sine — smoother for dreamy LFO
    setParam(apvts, ParamID::vibratoOn,     0.0f);
    setParam(apvts, ParamID::vibratoRate,   2.0f);
    setParam(apvts, ParamID::vibratoDepth,  0.0f);
    setParam(apvts, ParamID::wowFlutter,    0.10f); // minimal — no tape in his setup
    setParam(apvts, ParamID::reverbOn,      1.0f);
    setParam(apvts, ParamID::reverbMode,    2.0f); // Plate — shoegaze move
    setParam(apvts, ParamID::reverbMix,     0.70f); // reverb-soaked, signature (bumped from 0.60)
    setParam(apvts, ParamID::reverbTone,    0.0f);
    setParam(apvts, ParamID::outputTrim,   -0.10f); // duck for wet mix + hot drive
}

// SLOWCORE — genre-spanning slowcore atmosphere (Birth Day / Low / Bedhead /
// Duster / Red House Painters territory). Cleaner-leaning drive, atmospheric
// spring reverb, mild wow/flutter. Was previously named INHALANT and aimed
// specifically at Duster; broadened here to serve the genre rather than the
// single band.
void DemarcoToneProcessor::loadSlowcorePreset()
{
    setParam(apvts, ParamID::drive,         0.40f); // coloured drive, Blues-Driver-adjacent
    setParam(apvts, ParamID::detuneOn,      0.0f); // tuning stable by default
    setParam(apvts, ParamID::detune,        0.0f);
    setParam(apvts, ParamID::bitcrushBits, 16.0f); // transparent
    setParam(apvts, ParamID::bitcrushRate,  1.0f); // transparent
    setParam(apvts, ParamID::chorusMode,    0.0f); // Chorus
    setParam(apvts, ParamID::chorusOn,      1.0f);
    setParam(apvts, ParamID::chorusRate,    0.0f); // TBD — aim ~0.15..0.2 Hz, very slow
    setParam(apvts, ParamID::chorusDepth,   0.20f);
    setParam(apvts, ParamID::chorusMix,     0.15f); // minimal — not chorus-forward
    setParam(apvts, ParamID::chorusWidth,   0.40f);
    setParam(apvts, ParamID::chorusShape,   1.0f); // Triangle
    setParam(apvts, ParamID::vibratoOn,     0.0f);
    setParam(apvts, ParamID::vibratoRate,   2.0f);
    setParam(apvts, ParamID::vibratoDepth,  0.0f);
    setParam(apvts, ParamID::wowFlutter,    0.30f); // 4-track tape feel approximation
    setParam(apvts, ParamID::reverbOn,      1.0f);
    setParam(apvts, ParamID::reverbMode,    0.0f); // Spring
    setParam(apvts, ParamID::reverbMix,     0.35f); // atmospheric
    setParam(apvts, ParamID::reverbTone,   -0.30f); // warm, dark
    setParam(apvts, ParamID::outputTrim,    0.0f);
}

// Everything neutral — starting point for dialing in a new tone from scratch.
void DemarcoToneProcessor::loadBaselinePreset()
{
    setParam(apvts, ParamID::drive,        0.0f);
    setParam(apvts, ParamID::detuneOn,     0.0f);
    setParam(apvts, ParamID::detune,       0.0f);
    setParam(apvts, ParamID::bitcrushBits, 16.0f); // transparent
    setParam(apvts, ParamID::bitcrushRate,  1.0f); // transparent
    setParam(apvts, ParamID::chorusMode,    0.0f); // Chorus
    setParam(apvts, ParamID::chorusOn,     0.0f);
    setParam(apvts, ParamID::chorusRate,   0.5f);
    setParam(apvts, ParamID::chorusDepth,  0.0f);
    setParam(apvts, ParamID::chorusMix,    0.0f);
    setParam(apvts, ParamID::chorusWidth,  0.0f);
    setParam(apvts, ParamID::chorusShape,  0.0f);
    setParam(apvts, ParamID::vibratoOn,    0.0f);
    setParam(apvts, ParamID::vibratoRate,  5.0f);
    setParam(apvts, ParamID::vibratoDepth, 0.0f);
    setParam(apvts, ParamID::wowFlutter,   0.0f);
    setParam(apvts, ParamID::reverbOn,     0.0f);
    setParam(apvts, ParamID::reverbMode,   0.0f); // Spring
    setParam(apvts, ParamID::reverbMix,    0.0f);
    setParam(apvts, ParamID::reverbTone,   0.0f);
    setParam(apvts, ParamID::outputTrim,   0.0f);
}

// ---------------------------------------------------------------------------
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DemarcoToneProcessor();
}

#pragma once
#include <JuceHeader.h>

// 07 REVERB — SLAP mode. Single slapback delay, 80..120ms, 1-2 repeats, dark.
// Matches the SpringReverb interface so the reverb dispatcher can route to
// any of spring/slap/plate without special-casing.
//
// Runs at BASE RATE.
//
// Voicing TODO: short delay line + low feedback + dark EQ on wet.
class SlapDelay
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setBypassed(bool b);
    void setMix(float normalised01);
    void setTone(float bipolarMinus1to1);

    void process(juce::dsp::AudioBlock<float>& block);

private:
    double sr = 44100.0;
    bool   bypassed = false;
    float  mix = 0.0f;
    float  tone = 0.0f;
};

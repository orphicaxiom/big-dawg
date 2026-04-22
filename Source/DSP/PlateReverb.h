#pragma once
#include <JuceHeader.h>

// 07 REVERB — PLATE mode. Longer decay (~1-4s), denser early reflections,
// suitable for shoegaze / dream-pop. Matches the SpringReverb interface so
// the dispatcher can route to any of spring/slap/plate without special-casing.
//
// Runs at BASE RATE.
//
// Voicing TODO: dense FDN or many-stage Schroeder with longer comb delays.
class PlateReverb
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

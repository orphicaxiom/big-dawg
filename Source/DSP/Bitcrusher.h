#pragma once
#include <JuceHeader.h>

// 03 BITCRUSH — bit-depth and sample-rate reduction, between Detune and Chorus.
// Nonlinear, aliasing-prone: runs inside the oversampled bracket.
//
// Bypasses at extremes:
//   - bits >= ~15.9 → transparent (no quantization)
//   - rate >= ~0.999 → transparent (no downsampling)
//
// Voicing TODO: round to 2^bits quantum, hold-last-sample on downsample ratio.
class Bitcrusher
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setBits(float bits);   // 4..16
    void setRate(float rate);   // 0.1..1.0 (fraction of sample rate)

    void process(juce::dsp::AudioBlock<float>& block);

private:
    double sr = 44100.0;
    float  bits = 16.0f;
    float  rate = 1.0f;
};

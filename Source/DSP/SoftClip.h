#pragma once
#include <JuceHeader.h>

// 01 DRIVE — light tube-style soft clipping, Fender Twin / low-gain combo
// character. Input stage warmth, not distortion.
//
// Asymmetric tanh: positive half at full strength, negative half scaled
// down ~15% to introduce even harmonics ("tube" character).
// Makeup gain via 1/tanh(drive) keeps output level roughly constant as
// drive increases.
//
// Runs at 2x oversampled rate to kill aliasing from the nonlinearity.
class SoftClip
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void setDrive(float normalised01);      // 0..1  →  0..24 dB of input gain
    void process(juce::dsp::AudioBlock<float>& block);

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDrive;
    static constexpr float maxDriveDb = 24.0f;
    static constexpr float asymFactor = 0.85f;   // negative-half scalar
};

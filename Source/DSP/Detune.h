#pragma once
#include <JuceHeader.h>

// 02 DETUNE — static pitch offset, -50..+50 cents. Models "guitar tuned flat."
// Placed before chorus so the chorus modulates the already-detuned signal.
//
// Technique: classic two-tap pitch shifter. A single delay buffer with two
// read taps advancing at a rate that differs from the write pointer by
// (1 - pitchRatio). A sin/cos crossfade between the taps hides the seam
// where each tap wraps. The small amount of warble from the crossfade is
// on-brand for the tape-ish target.
//
// Runs at 2x oversampled rate (grouped with soft-clip and chorus).
class Detune
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void setBypassed(bool b);
    void setCents(float cents);             // -50..+50
    void process(juce::dsp::AudioBlock<float>& block);

private:
    bool bypassed = false;
    float readOneTap(size_t ch, float delaySamples) const;

    static constexpr float bufferMs = 100.0f;   // 100 ms of buffer, plenty for ±50c
    static constexpr int   maxChannels = 2;

    double sr = 44100.0;
    int    bufLen = 0;
    std::array<std::vector<float>, maxChannels> buffers;
    std::array<int, maxChannels> writeIdx {};

    // State shared across channels (phase, ratio)
    float  cents = 0.0f;
    double pitchRatio = 1.0;
    double delay0 = 0.0;                     // sample-level delay of tap 0
    double delay1 = 0.0;                     // sample-level delay of tap 1
};

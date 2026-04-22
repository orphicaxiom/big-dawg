#pragma once
#include <JuceHeader.h>

// 04 VIBRATO — pitch-modulated, warblier than the chorus. Bypassable.
//
// Single LFO modulating a short delay line. 100% wet when engaged (no
// dry blend — that's what makes it sound like vibrato and not chorus).
//
// Runs at BASE RATE. The LFO is slow (1..8 Hz) so aliasing is minimal.
class Vibrato
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setBypassed(bool b);
    void setRate(float hz);
    void setDepth(float normalised01);

    void process(juce::dsp::AudioBlock<float>& block);

private:
    float readTap(size_t ch, float delaySamples) const;

    static constexpr float maxDelayMs  = 15.0f;
    static constexpr float baseDelayMs = 6.0f;
    static constexpr int   maxChannels = 2;

    double sr = 44100.0;
    int    bufLen = 0;
    std::array<std::vector<float>, maxChannels> buffers;
    std::array<int, maxChannels> writeIdx {};

    bool   bypassed = true;
    float  rate = 5.0f;
    float  depth = 0.15f;

    double phase = 0.0;
};

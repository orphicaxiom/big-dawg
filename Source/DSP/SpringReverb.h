#pragma once
#include <JuceHeader.h>

// 06 REVERB (SPRING) — short-decay Schroeder reverb, dark-leaning.
//
// 4 parallel comb filters (delay + damping lowpass in feedback) into 2
// series allpass filters. Tilt EQ on the wet path (tone -1 = dark, +1 =
// bright). Always a tad of spring baked in even at low mix.
//
// Runs at BASE RATE.
class SpringReverb
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setBypassed(bool b);
    void setMix(float normalised01);
    void setTone(float bipolarMinus1to1);

    void process(juce::dsp::AudioBlock<float>& block);

private:
    static constexpr int numCombs    = 4;
    static constexpr int numAllpass  = 2;
    static constexpr int maxChannels = 2;

    struct Comb {
        std::vector<float> buf;
        int  idx = 0;
        float lpState = 0.0f;
    };
    struct Allpass {
        std::vector<float> buf;
        int  idx = 0;
    };

    float processOneChannel(size_t ch, float input);

    double sr = 44100.0;
    bool   bypassed = false;
    float  mix = 0.15f;
    float  tone = -0.2f;

    // Per-channel filter networks for stereo decorrelation.
    std::array<std::array<Comb, numCombs>, maxChannels>       combs;
    std::array<std::array<Allpass, numAllpass>, maxChannels>  allpasses;

    // Tilt EQ state (one-pole lowpass + highpass, tone mixes between)
    std::array<float, maxChannels> tiltLpState {};
};

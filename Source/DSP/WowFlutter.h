#pragma once
#include <JuceHeader.h>
#include <random>

// 05 WOW/FLUTTER — tape-style pitch instability. Signature Viceroy element.
//
// "Wow" = slow random walk around ~0.5 Hz, lowpassed. Big, lazy pitch sway.
// "Flutter" = ~6 Hz sine with small phase jitter. Faster, nervous warble.
// Both sum into a single delay-line modulation.
//
// Depth maps 0..1 → 0.05%..1.5% peak pitch deviation. Default 0.2 ≈ 0.3%.
//
// Runs at BASE RATE. At high depth (≥ ~1% modulation) some aliasing is
// possible; if audible, move this into the upsampled block.
class WowFlutter
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setAmount(float normalised01);

    void process(juce::dsp::AudioBlock<float>& block);

private:
    float readTap(size_t ch, float delaySamples) const;

    static constexpr float  maxDelayMs     = 20.0f;
    static constexpr float  baseDelayMs    = 10.0f;    // center position
    static constexpr float  minPctDev      = 0.0005f;  // 0.05%
    static constexpr float  maxPctDev      = 0.015f;   // 1.5%
    static constexpr double wowHz          = 0.5;
    static constexpr double flutterHz      = 6.0;
    static constexpr int    maxChannels    = 2;

    double sr = 44100.0;
    int    bufLen = 0;
    std::array<std::vector<float>, maxChannels> buffers;
    std::array<int, maxChannels> writeIdx {};

    float  amount = 0.2f;

    // LFO state
    double wowPhase = 0.0;         // slow random walk uses lowpass on noise
    double wowState = 0.0;
    double flutterPhase = 0.0;
    std::mt19937 rng { 0xB16D4465u };
    std::uniform_real_distribution<float> dist { -1.0f, 1.0f };
};

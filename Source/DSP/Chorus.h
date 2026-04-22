#pragma once
#include <JuceHeader.h>

// 03 CHORUS — the core Viceroy sound. CE-2 / MXR Analog Chorus voicing.
//
// Per-channel modulated delay, L/R LFOs 180° out of phase when width=1.
// Wet path gets a one-pole lowpass around 4 kHz to mimic BBD darkness.
// Dry/wet crossfade on Mix.
//
// Runs at 2x oversampled rate.
class Chorus
{
public:
    enum class Shape { Sine = 0, Triangle = 1 };
    enum class Mode  { Chorus = 0, Flanger = 1 };

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setBypassed(bool b);
    void setRate(float hz);
    void setDepth(float normalised01);
    void setMix(float normalised01);
    void setWidth(float normalised01);
    void setShape(Shape s);
    void setMode(Mode m);                       // stored only; DSP voicing TBD

    void process(juce::dsp::AudioBlock<float>& block);

private:
    bool bypassed = false;
    float readTap(size_t ch, float delaySamples) const;
    float lfo(double phase) const;

    static constexpr float minDelayMs  = 3.0f;
    static constexpr float maxDelayMs  = 25.0f;
    static constexpr int   maxChannels = 2;

    double sr = 44100.0;
    int    bufLen = 0;
    std::array<std::vector<float>, maxChannels> buffers;
    std::array<int, maxChannels> writeIdx {};

    // BBD-style one-pole lowpass on the wet path (one state per channel)
    std::array<float, maxChannels> lpState {};
    float lpAlpha = 0.0f;

    float  rate = 0.5f, depth = 0.6f, mix = 0.5f, width = 0.8f;
    Shape  shape = Shape::Sine;
    Mode   mode  = Mode::Chorus;

    double phaseL = 0.0;
    double phaseR = juce::MathConstants<double>::pi;    // start 180° apart
};

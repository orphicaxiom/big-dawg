#include "Chorus.h"

void Chorus::prepare(const juce::dsp::ProcessSpec& spec)
{
    sr     = spec.sampleRate;
    bufLen = juce::jmax(8, (int)std::round(maxDelayMs * 0.001 * sr * 2.0));
    for (auto& b : buffers) b.assign((size_t)bufLen, 0.0f);
    for (auto& w : writeIdx) w = 0;
    for (auto& s : lpState)  s = 0.0f;
    phaseL = 0.0;
    phaseR = juce::MathConstants<double>::pi;

    // One-pole lowpass at ~4 kHz. alpha for a simple RC smoother.
    const double fc = 4000.0;
    lpAlpha = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * (float)fc / (float)sr);
}

void Chorus::reset()
{
    for (auto& b : buffers) std::fill(b.begin(), b.end(), 0.0f);
    for (auto& w : writeIdx) w = 0;
    for (auto& s : lpState)  s = 0.0f;
    phaseL = 0.0;
    phaseR = juce::MathConstants<double>::pi;
}

void Chorus::setBypassed(bool b) { bypassed = b; }
void Chorus::setRate(float v)   { rate  = juce::jlimit(0.1f, 2.0f, v); }
void Chorus::setDepth(float v)  { depth = juce::jlimit(0.0f, 1.0f, v); }
void Chorus::setMix(float v)    { mix   = juce::jlimit(0.0f, 1.0f, v); }
void Chorus::setWidth(float v)  { width = juce::jlimit(0.0f, 1.0f, v); }
void Chorus::setShape(Shape s)  { shape = s; }
void Chorus::setMode(Mode m)    { mode  = m; }  // voicing differentiation TBD

float Chorus::readTap(size_t ch, float delaySamples) const
{
    const int L = bufLen;
    double read = (double)writeIdx[ch] - (double)delaySamples;
    while (read < 0.0)  read += L;
    while (read >= L)   read -= L;

    const int i0 = (int)read;
    const int i1 = (i0 + 1) % L;
    const float frac = (float)(read - i0);
    const auto& buf = buffers[ch];
    return buf[(size_t)i0] + frac * (buf[(size_t)i1] - buf[(size_t)i0]);
}

float Chorus::lfo(double p) const
{
    if (shape == Shape::Sine)
        return std::sin((float)p);
    // Triangle: map phase 0..2π to -1..+1..-1
    const double norm = p / juce::MathConstants<double>::twoPi;   // 0..1
    return (float)(4.0 * std::abs(norm - std::floor(norm + 0.5)) - 1.0);
}

void Chorus::process(juce::dsp::AudioBlock<float>& block)
{
    if (bypassed || mix <= 0.0001f) return;

    const size_t numSamples  = block.getNumSamples();
    const size_t numChannels = juce::jmin((size_t)maxChannels, block.getNumChannels());

    // Delay range: centered around ~10ms, swings with depth up to ±7ms
    const float center     = (float)(12.0 * 0.001 * sr);
    const float swing      = depth * (float)(7.0 * 0.001 * sr);
    const float minSamples = (float)(minDelayMs * 0.001 * sr);

    const double phaseInc = 2.0 * juce::MathConstants<double>::pi * rate / sr;

    // Width=0: mono LFO. Width=1: 180° stereo. Interpolate phase offset.
    for (size_t i = 0; i < numSamples; ++i)
    {
        const float modL = lfo(phaseL);
        // Right LFO: phase follows left but shifted by (width * π)
        const double phaseRight = phaseL + juce::MathConstants<double>::pi * (double)width;
        const float modR = lfo(phaseRight);

        phaseL += phaseInc;
        if (phaseL > juce::MathConstants<double>::twoPi)
            phaseL -= juce::MathConstants<double>::twoPi;

        const float dL = juce::jmax(minSamples, center + swing * modL);
        const float dR = juce::jmax(minSamples, center + swing * modR);

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* p = block.getChannelPointer(ch);
            const float in = p[i];
            buffers[ch][(size_t)writeIdx[ch]] = in;

            const float delay = (ch == 0 ? dL : dR);
            float wet = readTap(ch, delay);

            // BBD-style lowpass on wet
            lpState[ch] += lpAlpha * (wet - lpState[ch]);
            wet = lpState[ch];

            p[i] = in * (1.0f - mix) + wet * mix;
            writeIdx[ch] = (writeIdx[ch] + 1) % bufLen;
        }
    }
}

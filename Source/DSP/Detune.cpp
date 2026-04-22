#include "Detune.h"

void Detune::prepare(const juce::dsp::ProcessSpec& spec)
{
    sr     = spec.sampleRate;
    bufLen = juce::jmax(8, (int)std::round(bufferMs * 0.001 * sr));

    for (auto& b : buffers)
        b.assign((size_t)bufLen, 0.0f);

    for (auto& w : writeIdx) w = 0;

    delay0 = bufLen * 0.25;
    delay1 = bufLen * 0.75;
}

void Detune::reset()
{
    for (auto& b : buffers) std::fill(b.begin(), b.end(), 0.0f);
    for (auto& w : writeIdx) w = 0;
    delay0 = bufLen * 0.25;
    delay1 = bufLen * 0.75;
}

void Detune::setBypassed(bool b) { bypassed = b; }

void Detune::setCents(float c)
{
    cents = juce::jlimit(-50.0f, 50.0f, c);
    pitchRatio = std::pow(2.0, (double)cents / 1200.0);
}

float Detune::readOneTap(size_t ch, float delaySamples) const
{
    const int L = bufLen;
    double read = (double)writeIdx[ch] - (double)delaySamples;
    while (read < 0.0)   read += L;
    while (read >= L)    read -= L;

    const int i0 = (int)read;
    const int i1 = (i0 + 1) % L;
    const float frac = (float)(read - i0);
    const auto& buf = buffers[ch];
    return buf[(size_t)i0] + frac * (buf[(size_t)i1] - buf[(size_t)i0]);
}

void Detune::process(juce::dsp::AudioBlock<float>& block)
{
    const size_t numSamples  = block.getNumSamples();
    const size_t numChannels = juce::jmin((size_t)maxChannels, block.getNumChannels());

    // Bypass: explicit OFF toggle, or if cents ~= 0 (dry tone pristine at center).
    if (bypassed || std::abs(cents) < 0.05f)
        return;

    const double rate = 1.0 - pitchRatio;   // delay change per sample

    for (size_t i = 0; i < numSamples; ++i)
    {
        // Crossfade weights from the CURRENT phase of tap 0 only; tap 1 is
        // always 180° offset and yields cos(...).
        const float phase0 = (float)(delay0 / (double)bufLen);
        const float theta  = juce::MathConstants<float>::pi * phase0;
        const float w0 = std::sin(theta);
        const float w1 = std::cos(theta);

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            const float in = block.getChannelPointer(ch)[i];
            buffers[ch][(size_t)writeIdx[ch]] = in;

            const float s0 = readOneTap(ch, (float)delay0);
            const float s1 = readOneTap(ch, (float)delay1);

            block.getChannelPointer(ch)[i] = s0 * w0 + s1 * w1;

            writeIdx[ch] = (writeIdx[ch] + 1) % bufLen;
        }

        // Advance taps (shared across channels — pitch is coherent stereo).
        delay0 += rate;
        delay1 += rate;
        while (delay0 < 0.0)   delay0 += bufLen;
        while (delay0 >= bufLen) delay0 -= bufLen;
        while (delay1 < 0.0)   delay1 += bufLen;
        while (delay1 >= bufLen) delay1 -= bufLen;
    }
}

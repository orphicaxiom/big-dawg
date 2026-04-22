#include "WowFlutter.h"

void WowFlutter::prepare(const juce::dsp::ProcessSpec& spec)
{
    sr     = spec.sampleRate;
    bufLen = juce::jmax(8, (int)std::round(maxDelayMs * 0.001 * sr * 2.0));

    for (auto& b : buffers)
        b.assign((size_t)bufLen, 0.0f);

    for (auto& w : writeIdx) w = 0;

    wowPhase = 0.0;
    wowState = 0.0;
    flutterPhase = 0.0;
}

void WowFlutter::reset()
{
    for (auto& b : buffers) std::fill(b.begin(), b.end(), 0.0f);
    for (auto& w : writeIdx) w = 0;
    wowPhase = 0.0;
    wowState = 0.0;
    flutterPhase = 0.0;
}

void WowFlutter::setAmount(float v)
{
    amount = juce::jlimit(0.0f, 1.0f, v);
}

float WowFlutter::readTap(size_t ch, float delaySamples) const
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

void WowFlutter::process(juce::dsp::AudioBlock<float>& block)
{
    if (amount <= 0.0001f) return;

    const size_t numSamples  = block.getNumSamples();
    const size_t numChannels = juce::jmin((size_t)maxChannels, block.getNumChannels());

    const float pctDev = juce::jmap(amount, 0.0f, 1.0f, minPctDev, maxPctDev);
    const float baseDelay = (float)(baseDelayMs * 0.001 * sr);

    // Depth in samples: base delay times the percentage deviation, scaled
    // up a bit because the two modulators sum.
    const float depthSamples = baseDelay * pctDev * 8.0f;

    // Wow = lowpassed white noise (one-pole smoother). Cutoff tracks wowHz.
    const double wowAlpha = 1.0 - std::exp(-2.0 * juce::MathConstants<double>::pi * wowHz / sr);

    // Flutter phase jitter: add a small random per-sample phase walk.
    const double flutterInc = 2.0 * juce::MathConstants<double>::pi * flutterHz / sr;
    const double jitterAmt  = flutterInc * 0.08;  // ~8% phase wander

    for (size_t i = 0; i < numSamples; ++i)
    {
        // Update wow (lazy random walk)
        wowState += wowAlpha * ((double)dist(rng) - wowState);

        // Update flutter phase with small jitter
        flutterPhase += flutterInc + jitterAmt * (double)dist(rng);
        if (flutterPhase > juce::MathConstants<double>::twoPi)
            flutterPhase -= juce::MathConstants<double>::twoPi;

        // Combined modulation: 60% wow + 40% flutter, scaled by depth
        const double mod = 0.6 * wowState + 0.4 * std::sin(flutterPhase);
        const float  delaySamples = baseDelay + depthSamples * (float)mod;

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            const float in = block.getChannelPointer(ch)[i];
            buffers[ch][(size_t)writeIdx[ch]] = in;
            block.getChannelPointer(ch)[i] = readTap(ch, delaySamples);
            writeIdx[ch] = (writeIdx[ch] + 1) % bufLen;
        }
    }
}

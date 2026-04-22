#include "Vibrato.h"

void Vibrato::prepare(const juce::dsp::ProcessSpec& spec)
{
    sr     = spec.sampleRate;
    bufLen = juce::jmax(8, (int)std::round(maxDelayMs * 0.001 * sr * 2.0));
    for (auto& b : buffers) b.assign((size_t)bufLen, 0.0f);
    for (auto& w : writeIdx) w = 0;
    phase = 0.0;
}

void Vibrato::reset()
{
    for (auto& b : buffers) std::fill(b.begin(), b.end(), 0.0f);
    for (auto& w : writeIdx) w = 0;
    phase = 0.0;
}

void Vibrato::setBypassed(bool b) { bypassed = b; }
void Vibrato::setRate(float v)    { rate  = juce::jlimit(1.0f, 8.0f, v); }
void Vibrato::setDepth(float v)   { depth = juce::jlimit(0.0f, 1.0f, v); }

float Vibrato::readTap(size_t ch, float delaySamples) const
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

void Vibrato::process(juce::dsp::AudioBlock<float>& block)
{
    if (bypassed) return;

    const size_t numSamples  = block.getNumSamples();
    const size_t numChannels = juce::jmin((size_t)maxChannels, block.getNumChannels());

    const float baseDelay  = (float)(baseDelayMs * 0.001 * sr);
    // depth 0..1 maps to 0..~4ms of pitch swing (about ±50 cents at 5 Hz)
    const float depthSamps = depth * (float)(4.0 * 0.001 * sr);
    const double phaseInc  = 2.0 * juce::MathConstants<double>::pi * rate / sr;

    for (size_t i = 0; i < numSamples; ++i)
    {
        const float mod = std::sin((float)phase);
        const float delaySamples = baseDelay + depthSamps * mod;

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            const float in = block.getChannelPointer(ch)[i];
            buffers[ch][(size_t)writeIdx[ch]] = in;
            block.getChannelPointer(ch)[i] = readTap(ch, delaySamples);
            writeIdx[ch] = (writeIdx[ch] + 1) % bufLen;
        }

        phase += phaseInc;
        if (phase > juce::MathConstants<double>::twoPi)
            phase -= juce::MathConstants<double>::twoPi;
    }
}

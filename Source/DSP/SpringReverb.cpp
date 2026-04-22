#include "SpringReverb.h"

namespace
{
    // Classic Schroeder/Freeverb-ish tunings, scaled per channel.
    // Comb delays in ms (short, for spring-like character).
    constexpr float combMs[4]      = { 29.7f, 37.1f, 41.1f, 43.7f };
    constexpr float combMsRight[4] = { 31.4f, 35.3f, 39.8f, 45.1f };
    constexpr float apMs[2]        = {  5.0f,  1.7f };
    constexpr float apMsRight[2]   = {  4.3f,  1.9f };

    // Comb feedback — sets decay time. ~0.78-0.84 gives short spring-ish tail.
    constexpr float combFeedback = 0.82f;
    // Damping lowpass inside the comb feedback (fixed, dark)
    constexpr float combDamping  = 0.3f;
    // Allpass feedback — fixed for diffusion
    constexpr float apFeedback   = 0.5f;
}

void SpringReverb::prepare(const juce::dsp::ProcessSpec& spec)
{
    sr = spec.sampleRate;

    auto msToSamples = [this](float ms) -> size_t {
        return (size_t)juce::jmax(1, (int)std::round(ms * 0.001 * sr));
    };

    for (size_t ch = 0; ch < (size_t)maxChannels; ++ch)
    {
        for (size_t i = 0; i < (size_t)numCombs; ++i)
        {
            const float ms = (ch == 0 ? combMs[i] : combMsRight[i]);
            combs[ch][i].buf.assign(msToSamples(ms), 0.0f);
            combs[ch][i].idx = 0;
            combs[ch][i].lpState = 0.0f;
        }
        for (size_t i = 0; i < (size_t)numAllpass; ++i)
        {
            const float ms = (ch == 0 ? apMs[i] : apMsRight[i]);
            allpasses[ch][i].buf.assign(msToSamples(ms), 0.0f);
            allpasses[ch][i].idx = 0;
        }
    }
    for (auto& s : tiltLpState) s = 0.0f;
}

void SpringReverb::reset()
{
    for (size_t ch = 0; ch < (size_t)maxChannels; ++ch)
    {
        for (auto& c : combs[ch])     { std::fill(c.buf.begin(), c.buf.end(), 0.0f); c.idx = 0; c.lpState = 0.0f; }
        for (auto& a : allpasses[ch]) { std::fill(a.buf.begin(), a.buf.end(), 0.0f); a.idx = 0; }
    }
    for (auto& s : tiltLpState) s = 0.0f;
}

void SpringReverb::setBypassed(bool b) { bypassed = b; }
void SpringReverb::setMix(float v)  { mix  = juce::jlimit(0.0f, 1.0f, v); }
void SpringReverb::setTone(float v) { tone = juce::jlimit(-1.0f, 1.0f, v); }

float SpringReverb::processOneChannel(size_t ch, float input)
{
    // Parallel combs summed
    float combSum = 0.0f;
    for (auto& c : combs[ch])
    {
        const size_t sz = c.buf.size();
        const size_t idx = (size_t)c.idx;
        const float delayed = c.buf[idx];
        c.lpState = delayed * (1.0f - combDamping) + c.lpState * combDamping;
        c.buf[idx] = input + c.lpState * combFeedback;
        c.idx = (int)((idx + 1) % sz);
        combSum += delayed;
    }
    combSum *= 0.25f;

    // Series allpasses for diffusion
    float y = combSum;
    for (auto& a : allpasses[ch])
    {
        const size_t sz = a.buf.size();
        const size_t idx = (size_t)a.idx;
        const float buffered = a.buf[idx];
        const float v = y - apFeedback * buffered;
        a.buf[idx] = v;
        a.idx = (int)((idx + 1) % sz);
        y = apFeedback * v + buffered;
    }

    // Tilt EQ: one-pole LP at ~900 Hz; tone mixes LP (dark) vs (input - LP) (bright)
    const float cutoff = 900.0f;
    const float alpha = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi
                                         * cutoff / (float)sr);
    tiltLpState[ch] += alpha * (y - tiltLpState[ch]);
    const float lp = tiltLpState[ch];
    const float hp = y - lp;
    // tone < 0 → darker (more LP). tone > 0 → brighter (more HP).
    const float tiltL = 0.5f - 0.5f * tone;   // weight on LP
    const float tiltH = 0.5f + 0.5f * tone;   // weight on HP
    return lp * tiltL * 2.0f + hp * tiltH * 2.0f;
}

void SpringReverb::process(juce::dsp::AudioBlock<float>& block)
{
    if (bypassed || mix <= 0.0001f) return;

    const size_t numSamples  = block.getNumSamples();
    const size_t numChannels = juce::jmin((size_t)maxChannels, block.getNumChannels());

    for (size_t i = 0; i < numSamples; ++i)
    {
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* p = block.getChannelPointer(ch);
            const float dry = p[i];
            const float wet = processOneChannel(ch, dry);
            p[i] = dry * (1.0f - mix) + wet * mix;
        }
    }
}

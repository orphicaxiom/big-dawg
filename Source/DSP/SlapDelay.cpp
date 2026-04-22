#include "SlapDelay.h"

void SlapDelay::prepare(const juce::dsp::ProcessSpec& spec) { sr = spec.sampleRate; }
void SlapDelay::reset() {}

void SlapDelay::setBypassed(bool b) { bypassed = b; }
void SlapDelay::setMix(float v)     { mix  = juce::jlimit(0.0f, 1.0f, v); }
void SlapDelay::setTone(float v)    { tone = juce::jlimit(-1.0f, 1.0f, v); }

void SlapDelay::process(juce::dsp::AudioBlock<float>& block)
{
    juce::ignoreUnused(block);
    // Pass-through stub. DSP lands later.
}

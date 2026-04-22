#include "Bitcrusher.h"

void Bitcrusher::prepare(const juce::dsp::ProcessSpec& spec) { sr = spec.sampleRate; }
void Bitcrusher::reset() {}

void Bitcrusher::setBits(float b) { bits = juce::jlimit(4.0f, 16.0f, b); }
void Bitcrusher::setRate(float r) { rate = juce::jlimit(0.1f, 1.0f, r); }

void Bitcrusher::process(juce::dsp::AudioBlock<float>& block)
{
    juce::ignoreUnused(block);
    // Pass-through stub. DSP lands later.
}

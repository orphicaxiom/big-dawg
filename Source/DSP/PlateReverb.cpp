#include "PlateReverb.h"

void PlateReverb::prepare(const juce::dsp::ProcessSpec& spec) { sr = spec.sampleRate; }
void PlateReverb::reset() {}

void PlateReverb::setBypassed(bool b) { bypassed = b; }
void PlateReverb::setMix(float v)     { mix  = juce::jlimit(0.0f, 1.0f, v); }
void PlateReverb::setTone(float v)    { tone = juce::jlimit(-1.0f, 1.0f, v); }

void PlateReverb::process(juce::dsp::AudioBlock<float>& block)
{
    juce::ignoreUnused(block);
    // Pass-through stub. DSP lands later.
}

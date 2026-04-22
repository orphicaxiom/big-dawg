#include "SoftClip.h"

void SoftClip::prepare(const juce::dsp::ProcessSpec& spec)
{
    smoothedDrive.reset(spec.sampleRate, 0.05); // 50 ms ramp
    smoothedDrive.setCurrentAndTargetValue(1.0f);
}

void SoftClip::reset()
{
    smoothedDrive.setCurrentAndTargetValue(smoothedDrive.getTargetValue());
}

void SoftClip::setDrive(float normalised01)
{
    const float driveDb = juce::jlimit(0.0f, 1.0f, normalised01) * maxDriveDb;
    smoothedDrive.setTargetValue(juce::Decibels::decibelsToGain(driveDb));
}

void SoftClip::process(juce::dsp::AudioBlock<float>& block)
{
    const auto numSamples  = block.getNumSamples();
    const auto numChannels = block.getNumChannels();

    // Pull per-sample drive once per sample, but share across channels so
    // stereo stays coherent.
    for (size_t i = 0; i < numSamples; ++i)
    {
        const float drive = smoothedDrive.getNextValue();
        // Makeup: at drive=1 (unity, 0 dB), no clipping so 1/tanh would blow up.
        // Bound drive away from exactly 1.0 by treating <=1.0 as identity.
        const float makeup = drive > 1.0001f ? 1.0f / std::tanh(drive) : 1.0f;

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* data = block.getChannelPointer(ch);
            const float x = data[i] * drive;
            const float y = (x >= 0.0f)
                ? std::tanh(x)
                : std::tanh(x * asymFactor);
            data[i] = y * makeup;
        }
    }
}

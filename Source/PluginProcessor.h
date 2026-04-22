#pragma once
#include <JuceHeader.h>
#include "DSP/SoftClip.h"
#include "DSP/Detune.h"
#include "DSP/Bitcrusher.h"
#include "DSP/Chorus.h"
#include "DSP/Vibrato.h"
#include "DSP/WowFlutter.h"
#include "DSP/SpringReverb.h"
#include "DSP/SlapDelay.h"
#include "DSP/PlateReverb.h"

// ---------------------------------------------------------------------------
// Big Dawg — an AU/VST3 guitar tone effect in the spirit of late-80s
// Japanese combos, analog chorus pedals, and tape-deck wow.
//
// Chassis: Big Dawg
// Default preset: Viceroy (Ode to Viceroy / Salad Days / 2 era tone)
//
// Signal chain:
//   input -> [oversampled: SoftClip -> Detune -> Chorus]
//         -> [base rate:   Vibrato  -> WowFlutter -> SpringReverb]
//         -> output trim
// ---------------------------------------------------------------------------

namespace ParamID
{
    // Input
    static constexpr auto drive       = "drive";        // 0..1

    // Static detune (pre-chorus, bypassable)
    static constexpr auto detuneOn    = "detuneOn";     // bool
    static constexpr auto detune      = "detune";       // cents, -50..+50

    // Bitcrusher (between detune and chorus; bypasses at max values)
    static constexpr auto bitcrushBits = "bitcrushBits"; // float, 4..16  (16 = transparent)
    static constexpr auto bitcrushRate = "bitcrushRate"; // float, 0.1..1 (1  = transparent)

    // Chorus / Flanger (bypassable)
    static constexpr auto chorusOn    = "chorusOn";     // bool
    static constexpr auto chorusMode  = "chorusMode";   // choice: 0=Chorus, 1=Flanger
    static constexpr auto chorusRate  = "chorusRate";   // Hz, 0.1..2.0
    static constexpr auto chorusDepth = "chorusDepth";  // 0..1
    static constexpr auto chorusMix   = "chorusMix";    // 0..1
    static constexpr auto chorusWidth = "chorusWidth";  // 0..1 (stereo spread)
    static constexpr auto chorusShape = "chorusShape";  // choice: 0=Sine, 1=Triangle

    // Vibrato (post-chorus, bypassable)
    static constexpr auto vibratoOn    = "vibratoOn";    // bool
    static constexpr auto vibratoRate  = "vibratoRate";  // Hz, 1..8
    static constexpr auto vibratoDepth = "vibratoDepth"; // 0..1

    // Tape wow/flutter
    static constexpr auto wowFlutter  = "wowFlutter";   // 0..1 (mapped 0.05%..1.5% internally)

    // Reverb (SPRING / SLAP / PLATE; bypassable)
    static constexpr auto reverbOn    = "reverbOn";     // bool
    static constexpr auto reverbMode  = "reverbMode";   // choice: 0=Spring, 1=Slap, 2=Plate
    static constexpr auto reverbMix   = "reverbMix";    // 0..1
    static constexpr auto reverbTone  = "reverbTone";   // -1..+1 tilt (dark <-> bright)

    // Output
    static constexpr auto outputTrim  = "outputTrim";   // dB, -24..+12
}

class DemarcoToneProcessor : public juce::AudioProcessor
{
public:
    DemarcoToneProcessor();
    ~DemarcoToneProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // --- Preset dispatcher -------------------------------------------------
    // To add a preset: append a name here, add a case in loadPreset(), and
    // write the corresponding load*Preset() in the .cpp.
    // Order: VICEROY is index 0 (default). BASELINE is last (the reset).
    juce::StringArray getPresetNames() const
    {
        return { "VICEROY",
                 "ELVIS (THE SAD ONE)",
                 "PALTH",
                 "INHALANT",
                 "BASELINE" };
    }
    int  getCurrentPresetIndex() const { return currentPresetIndex; }
    void loadPreset(int index);

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Specific preset setters — one function per named preset.
    void loadViceroyPreset();
    void loadElvisPreset();
    void loadPalthPreset();
    void loadInhalantPreset();
    void loadBaselinePreset();

    int currentPresetIndex = 0;

    double currentSampleRate = 44100.0;
    int    currentBlockSize  = 512;

    // Oversampling — 2x via IIR halfband. Soft-clip / detune / chorus run
    // inside the up/down bracket; vibrato / wow / reverb / trim run at
    // base rate after the downsample.
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    // Reverb dispatch — which of the three reverb classes gets called.
    enum class ReverbMode { Spring = 0, Slap = 1, Plate = 2 };
    ReverbMode currentReverbMode = ReverbMode::Spring;

    // DSP stages
    SoftClip     softClip;
    Detune       detune;
    Bitcrusher   bitcrusher;
    Chorus       chorus;
    Vibrato      vibrato;
    WowFlutter   wowFlutter;
    SpringReverb springReverb;
    SlapDelay    slapDelay;
    PlateReverb  plateReverb;

    // Smoothed output trim (applied at base rate after all DSP).
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedTrim;

    // Pull current parameter values and push to DSP stages once per block.
    void syncParametersToDsp();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemarcoToneProcessor)
};

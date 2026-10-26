#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

//==============================================================================
// One band of compression: stereo-linked, soft knee, log-domain smoothing.
struct BandCompressor
{
    void prepare (double sampleRate) { fs = sampleRate; envDb = 0.0f; }
    void reset() { envDb = 0.0f; }

    void setParams (float thresholdDb, float ratio, float attackMs, float releaseMs)
    {
        thr = thresholdDb;
        slope = 1.0f - 1.0f / juce::jmax (1.0f, ratio);
        attCoef = std::exp (-1.0f / (0.001f * juce::jmax (0.05f, attackMs) * (float) fs));
        relCoef = std::exp (-1.0f / (0.001f * juce::jmax (1.0f, releaseMs) * (float) fs));
    }

    // Returns linear gain to apply; `detector` is the linked peak level (linear).
    float process (float detector)
    {
        const float levelDb = juce::Decibels::gainToDecibels (detector, -120.0f);
        const float over = levelDb - thr;
        float targetGr = 0.0f; // positive dB of reduction

        if (over > knee * 0.5f)
            targetGr = slope * over;
        else if (over > -knee * 0.5f)
        {
            const float x = over + knee * 0.5f;
            targetGr = slope * x * x / (2.0f * knee);
        }

        const float coef = targetGr > envDb ? attCoef : relCoef;
        envDb = targetGr + coef * (envDb - targetGr);
        return juce::Decibels::decibelsToGain (-envDb);
    }

    float getGainReductionDb() const { return envDb; }

    double fs = 44100.0;
    float thr = 0.0f, slope = 0.0f, attCoef = 0.0f, relCoef = 0.0f;
    float knee = 6.0f, envDb = 0.0f;
};

//==============================================================================
class Boom8AudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numBands = 4;

    Boom8AudioProcessor();
    ~Boom8AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "8cee Boom-8"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Factory presets live in the host's program list too.
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::array<std::atomic<float>, numBands> grMeters {};
    std::atomic<float> outMeter { 0.0f };

    // Notifies the editor when a preset was loaded from the host side.
    std::function<void()> onPresetChanged;

private:
    using LR = juce::dsp::LinkwitzRileyFilter<float>;

    void updateFilters();

    LR xo1, xo2, xo3;                 // the three splits
    LR apLow2, apLow3, apMid3;        // phase compensation so bands sum flat
    LR apDry1, apDry2, apDry3;        // keeps the parallel (dry) path in phase

    std::array<BandCompressor, numBands> comps;

    // Drive stage DC blocker
    std::array<float, 2> dcX {}, dcY {};

    // Grit (12-bit / 26.04 kHz style sampler crunch)
    double gritPhase = 0.0;
    std::array<float, 2> gritHold {};

    juce::SmoothedValue<float> driveSm, gritSm, mixSm, outSm;
    std::array<juce::SmoothedValue<float>, numBands> makeupSm;

    juce::AudioBuffer<float> dryBuffer;
    double sr = 44100.0;
    int currentPreset = 0;

    JUCE_DECLARE_WEAK_REFERENCEABLE (Boom8AudioProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Boom8AudioProcessor)
};

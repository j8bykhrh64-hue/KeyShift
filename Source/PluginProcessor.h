#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <signalsmith-stretch.h>
#include <atomic>
#include <vector>

class KeyShiftProcessor : public juce::AudioProcessor
{
public:
    KeyShiftProcessor();
    ~KeyShiftProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "KeyShift"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Peak level of the output since the last call (used by the editor's meter)
    float consumeOutputPeak() { return outputPeak.exchange (0.0f); }
    float consumeInputPeak()  { return inputPeak.exchange (0.0f); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    signalsmith::stretch::SignalsmithStretch<float> stretch;

    juce::AudioBuffer<float> wetBuffer;

    // Dry signal delayed by the exact plugin latency, so that
    // 0 st / bypass is a clean pass-through and dry/wet stay time-aligned.
    std::vector<std::vector<float>> dryRing;
    int dryPos = 0;
    int dryLen = 1;

    juce::SmoothedValue<float> wetAmount;   // 0 = all dry, 1 = all shifted
    juce::SmoothedValue<float> outGain;     // linear output gain

    float lastTranspose = 0.0f;
    int numCh = 2;

    std::atomic<float>* pitchParam  = nullptr;
    std::atomic<float>* fineParam   = nullptr;
    std::atomic<float>* mixParam    = nullptr;
    std::atomic<float>* outParam    = nullptr;
    std::atomic<float>* bypassParam = nullptr;

    std::atomic<float> outputPeak { 0.0f };
    std::atomic<float> inputPeak  { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyShiftProcessor)
};

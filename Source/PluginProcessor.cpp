#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

KeyShiftProcessor::KeyShiftProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createParameterLayout())
{
    pitchParam  = apvts.getRawParameterValue ("semitones");
    fineParam   = apvts.getRawParameterValue ("fine");
    mixParam    = apvts.getRawParameterValue ("mix");
    outParam    = apvts.getRawParameterValue ("output");
    bypassParam = apvts.getRawParameterValue ("bypass");
}

juce::AudioProcessorValueTreeState::ParameterLayout KeyShiftProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Pitch: whole semitones
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "semitones", 1 }, "Pitch", -12, 12, 0,
        juce::AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int)
        {
            return juce::String ((v > 0 ? "+" : "") + juce::String (v) + " st");
        })));

    // Fine tune in cents
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "fine", 1 }, "Fine",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 1.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
        {
            const int c = (int) std::round (v);
            return juce::String ((c > 0 ? "+" : "") + juce::String (c) + " ct");
        })));

    // Dry / wet
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
        {
            return juce::String (juce::roundToInt (v * 100.0f)) + " %";
        })));

    // Output trim
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output", 1 }, "Output",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
        {
            return juce::String ((v > 0.05f ? "+" : "") + juce::String (v, 1) + " dB");
        })));

    // Bypass
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    return layout;
}

bool KeyShiftProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void KeyShiftProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    numCh = juce::jmax (1, getTotalNumInputChannels());

    // Default preset = highest-quality real-time setting
    stretch.presetDefault (numCh, (float) sampleRate);
    stretch.reset();

    const int latency = stretch.inputLatency() + stretch.outputLatency();
    setLatencySamples (latency);   // Ableton compensates automatically

    dryLen = juce::jmax (1, latency);
    dryRing.assign ((size_t) numCh, std::vector<float> ((size_t) dryLen, 0.0f));
    dryPos = 0;

    wetBuffer.setSize (numCh, juce::jmax (1, samplesPerBlock), false, true, false);

    lastTranspose = pitchParam->load() + fineParam->load() * 0.01f;
    stretch.setTransposeSemitones (lastTranspose);

    const bool bypassed = bypassParam->load() > 0.5f;
    const bool shifted  = std::abs (lastTranspose) > 1.0e-4f;

    wetAmount.reset (sampleRate, 0.030);
    wetAmount.setCurrentAndTargetValue ((bypassed || ! shifted) ? 0.0f : mixParam->load());

    outGain.reset (sampleRate, 0.020);
    outGain.setCurrentAndTargetValue (bypassed ? 1.0f : juce::Decibels::decibelsToGain (outParam->load()));

    outputPeak.store (0.0f);
}

void KeyShiftProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n  = buffer.getNumSamples();
    const int ch = juce::jmin (buffer.getNumChannels(), numCh);

    for (int c = ch; c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, n);

    if (wetBuffer.getNumSamples() < n)
        wetBuffer.setSize (numCh, n, false, false, true);

    // --- parameters ---
    const float transpose = pitchParam->load() + fineParam->load() * 0.01f;
    if (std::abs (transpose - lastTranspose) > 1.0e-4f)
    {
        stretch.setTransposeSemitones (transpose);
        lastTranspose = transpose;
    }

    const bool bypassed = bypassParam->load() > 0.5f;
    const bool shifted  = std::abs (transpose) > 1.0e-4f;

    wetAmount.setTargetValue ((bypassed || ! shifted) ? 0.0f : mixParam->load());
    outGain.setTargetValue (bypassed ? 1.0f : juce::Decibels::decibelsToGain (outParam->load()));

    // Always run the engine so latency stays constant and the state stays primed
    stretch.process (buffer.getArrayOfReadPointers(), n,
                     wetBuffer.getArrayOfWritePointers(), n);

    constexpr float halfPi = 1.57079632679f;
    float blockPeak = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float m = wetAmount.getNextValue();
        const float g = outGain.getNextValue();

        // equal-power crossfade (dry and shifted signals are uncorrelated)
        const float wetG = std::sin (m * halfPi);
        const float dryG = std::cos (m * halfPi);

        for (int c = 0; c < ch; ++c)
        {
            auto* io   = buffer.getWritePointer (c);
            auto& ring = dryRing[(size_t) c];

            const float in      = io[i];
            const float delayed = ring[(size_t) dryPos];
            ring[(size_t) dryPos] = in;

            const float wet = wetBuffer.getReadPointer (c)[i];
            const float out = (delayed * dryG + wet * wetG) * g;

            io[i] = out;
            blockPeak = juce::jmax (blockPeak, std::abs (out));
        }

        if (++dryPos >= dryLen) dryPos = 0;
    }

    if (blockPeak > outputPeak.load())
        outputPeak.store (blockPeak);
}

juce::AudioProcessorEditor* KeyShiftProcessor::createEditor()
{
    return new KeyShiftEditor (*this);
}

void KeyShiftProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void KeyShiftProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KeyShiftProcessor();
}

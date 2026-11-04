#include "PluginProcessor.h"
#include "PluginEditor.h"

KeyShiftProcessor::KeyShiftProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createParameterLayout())
{
    semitonesParam = apvts.getRawParameterValue ("semitones");
}

juce::AudioProcessorValueTreeState::ParameterLayout KeyShiftProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto attrs = juce::AudioParameterIntAttributes()
                     .withStringFromValueFunction ([] (int v, int)
                     {
                         return (v > 0 ? "+" : "") + juce::String (v) + " st";
                     });

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "semitones", 1 }, "Pitch", -12, 12, 0, attrs));

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

    // Default preset = highest-quality real-time setting (large window, good transient handling)
    stretch.presetDefault (numCh, (float) sampleRate);
    stretch.reset();

    const int latency = stretch.inputLatency() + stretch.outputLatency();
    setLatencySamples (latency);   // Ableton automatically compensates for this

    dryLen = juce::jmax (1, latency);
    dryRing.assign ((size_t) numCh, std::vector<float> ((size_t) dryLen, 0.0f));
    dryPos = 0;

    wetBuffer.setSize (numCh, juce::jmax (1, samplesPerBlock), false, true, false);

    lastSemitones = (int) std::round (semitonesParam->load());
    stretch.setTransposeSemitones ((float) lastSemitones);

    wetMix.reset (sampleRate, 0.030);   // 30 ms crossfade between dry / shifted
    wetMix.setCurrentAndTargetValue (lastSemitones != 0 ? 1.0f : 0.0f);
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

    const int semis = (int) std::round (semitonesParam->load());
    if (semis != lastSemitones)
    {
        stretch.setTransposeSemitones ((float) semis);
        lastSemitones = semis;
    }
    wetMix.setTargetValue (semis != 0 ? 1.0f : 0.0f);

    // Always run the engine so it stays primed and latency stays constant
    stretch.process (buffer.getArrayOfReadPointers(), n,
                     wetBuffer.getArrayOfWritePointers(), n);

    for (int i = 0; i < n; ++i)
    {
        const float m = wetMix.getNextValue();

        for (int c = 0; c < ch; ++c)
        {
            auto* io = buffer.getWritePointer (c);
            auto& ring = dryRing[(size_t) c];

            const float in      = io[i];
            const float delayed = ring[(size_t) dryPos];
            ring[(size_t) dryPos] = in;

            const float wet = wetBuffer.getReadPointer (c)[i];
            io[i] = delayed * (1.0f - m) + wet * m;
        }

        if (++dryPos >= dryLen) dryPos = 0;
    }
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

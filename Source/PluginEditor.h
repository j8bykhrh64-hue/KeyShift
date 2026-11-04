#pragma once

#include "PluginProcessor.h"

namespace ks
{
    inline const juce::Colour ink      { 0xff7d85a0 };
    inline const juce::Colour inkSoft  { 0xff9aa2b8 };
    inline const juce::Colour dotLit   { 0xff5db6ea };
    inline const juce::Colour dotOff   { 0xffcdd2e0 };
    inline const juce::Colour pointer  { 0xff6f84c4 };
    inline const juce::Colour cyan     { 0xff48d8e8 };
    inline const juce::Colour amber    { 0xffffb938 };
}

// Light, airy "soft hardware" look: dotted LED rings, thin strokes, raised white caps
class SoftLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;
};

class KeyShiftEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit KeyShiftEditor (KeyShiftProcessor&);
    ~KeyShiftEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob (juce::Slider&, bool big, bool bipolar, double defaultValue);
    void drawModule (juce::Graphics&);

    KeyShiftProcessor& keyShiftProcessor;
    SoftLookAndFeel lnf;

    juce::Slider pitch, fine, mix, output;
    juce::ToggleButton bypass;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    SliderAtt pitchAtt, fineAtt, mixAtt, outAtt;
    ButtonAtt bypassAtt;

    float displayIn  = 0.0f;   // smoothed linear levels for the meters
    float displayOut = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyShiftEditor)
};

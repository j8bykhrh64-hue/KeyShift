#pragma once

#include "PluginProcessor.h"

namespace ks
{
    inline const juce::Colour ink     { 0xff59627a };
    inline const juce::Colour inkSoft { 0xff8a93a8 };
    inline const juce::Colour amberHi { 0xffffe066 };
    inline const juce::Colour amber   { 0xffffb938 };
    inline const juce::Colour amberLo { 0xffff7a35 };
    inline const juce::Colour dotBlue { 0xffa9c8ea };
    inline const juce::Colour cyan    { 0xff48d8e8 };
}

// Soft white "hardware" look: recessed bezel, dark arc, glowing amber value arc
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
    void drawDisplay (juce::Graphics&);
    void drawMeterBar (juce::Graphics&);

    KeyShiftProcessor& processor;
    SoftLookAndFeel lnf;

    juce::Slider pitch, fine, mix, output;
    juce::ToggleButton bypass;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    SliderAtt pitchAtt, fineAtt, mixAtt, outAtt;
    ButtonAtt bypassAtt;

    float displayLevel = 0.0f;   // smoothed linear output level for the meter / display
    float phase = 0.0f;          // gentle idle animation for the display

    juce::Rectangle<float> pitchPill, finePill, mixPill, outPill, bypassPill;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyShiftEditor)
};


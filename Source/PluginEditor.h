#pragma once

#include "PluginProcessor.h"

// Minimal, clean single-knob look
class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static inline const juce::Colour accent { 0xff7c9cff };

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle,
                           juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (8.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre  = bounds.getCentre();
        const float angle  = startAngle + pos * (endAngle - startAngle);
        const float zero   = startAngle + 0.5f * (endAngle - startAngle);

        // Track
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
        g.setColour (juce::Colour (0xff2a2d33));
        g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        // Value arc (grows from centre / 0 st)
        juce::Path val;
        val.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                           juce::jmin (zero, angle), juce::jmax (zero, angle), true);
        g.setColour (accent);
        g.strokePath (val, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        // Knob body
        const float bodyR = radius * 0.76f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b3f47), centre.x, centre.y - bodyR,
                                                 juce::Colour (0xff1b1d21), centre.x, centre.y + bodyR, false));
        g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

        g.setColour (juce::Colour (0xff4a4e57));
        g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

        // Pointer
        juce::Path p;
        p.addRoundedRectangle (-1.75f, -bodyR + 7.0f, 3.5f, bodyR * 0.30f, 1.75f);
        p.applyTransform (juce::AffineTransform::rotation (angle).translated (centre));
        g.setColour (juce::Colours::white);
        g.fillPath (p);
    }
};

class KeyShiftEditor : public juce::AudioProcessorEditor
{
public:
    explicit KeyShiftEditor (KeyShiftProcessor&);
    ~KeyShiftEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    KeyShiftProcessor& processor;
    KnobLookAndFeel lnf;
    juce::Slider knob;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyShiftEditor)
};

#include "PluginEditor.h"

KeyShiftEditor::KeyShiftEditor (KeyShiftProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      attachment (p.apvts, "semitones", knob)
{
    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob.setRotaryParameters (juce::degreesToRadians (225.0f),
                              juce::degreesToRadians (495.0f), true);
    knob.setDoubleClickReturnValue (true, 0.0);   // double-click = reset to 0
    knob.setMouseDragSensitivity (180);
    knob.setLookAndFeel (&lnf);
    knob.onValueChange = [this] { repaint(); };
    addAndMakeVisible (knob);

    setSize (280, 360);
}

KeyShiftEditor::~KeyShiftEditor()
{
    knob.setLookAndFeel (nullptr);
}

void KeyShiftEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff131417));

    // Title
    g.setColour (juce::Colour (0xff8a8f99));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("K E Y S H I F T", getLocalBounds().removeFromTop (50),
                juce::Justification::centred);

    // Value readout
    const int v = (int) std::round (knob.getValue());
    const juce::String text = (v > 0 ? "+" : "") + juce::String (v);

    auto area = getLocalBounds().removeFromBottom (90);

    g.setColour (v == 0 ? juce::Colour (0xff8a8f99) : juce::Colours::white);
    g.setFont (juce::FontOptions (34.0f, juce::Font::plain));
    g.drawText (text, area.removeFromTop (48), juce::Justification::centred);

    g.setColour (juce::Colour (0xff5c616b));
    g.setFont (juce::FontOptions (11.0f, juce::Font::plain));
    g.drawText ("SEMITONES", area.removeFromTop (18), juce::Justification::centred);
}

void KeyShiftEditor::resized()
{
    knob.setBounds (getLocalBounds().withTrimmedTop (55).withTrimmedBottom (95).reduced (25, 0));
}

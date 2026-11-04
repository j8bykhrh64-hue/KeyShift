#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr int kW = 600;
    constexpr int kH = 392;

    const juce::Rectangle<float> kCard   (14.0f, 14.0f, 572.0f, 364.0f);
    const juce::Rectangle<float> kModule (360.0f, 52.0f, 212.0f, 166.0f);

    void drawTextLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r,
                    float size, bool bold, juce::Colour col, float kerning,
                    juce::Justification just)
    {
        juce::Font f (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
        f.setExtraKerningFactor (kerning);
        g.setFont (f);
        g.setColour (col);
        g.drawText (text, r, just, false);
    }

    void drawOrb (juce::Graphics& g, float cx, float cy, float r)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.9f),
                                                 cx - r * 0.35f, cy - r * 0.35f,
                                                 juce::Colour (0xffbcc4de).withAlpha (0.55f),
                                                 cx + r * 0.5f, cy + r * 0.5f, true));
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
    }

    juce::Colour segmentColour (int i)
    {
        if (i >= 14) return juce::Colour (0xffff5b4a);
        if (i >= 12) return juce::Colour (0xffff9a3c);
        if (i >= 10) return juce::Colour (0xffe9d85a);
        return ks::cyan;
    }

    // Vertical 16-segment LED meter, 0 dB at the top, 3 dB per segment
    void drawMeter (juce::Graphics& g, float x, float yBottom, float level)
    {
        const float dB = juce::Decibels::gainToDecibels (level, -80.0f);

        for (int i = 0; i < 16; ++i)
        {
            const float y = yBottom - (float) (i + 1) * 6.5f + 2.0f;
            const juce::Rectangle<float> seg (x, y, 12.0f, 4.5f);
            const bool on = dB >= -45.0f + (float) i * 3.0f;
            const juce::Colour col = on ? segmentColour (i) : juce::Colour (0xff262a33);

            if (on)
            {
                g.setColour (col.withAlpha (0.20f));
                g.fillRoundedRectangle (seg.expanded (2.0f), 3.0f);
            }

            g.setColour (col);
            g.fillRoundedRectangle (seg, 1.5f);
        }
    }
}

//==============================================================================
// Look and feel
//==============================================================================
void SoftLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                        float pos, float startAngle, float endAngle,
                                        juce::Slider& s)
{
    using namespace juce;

    const bool big     = (bool) s.getProperties()["big"];
    const bool bipolar = (bool) s.getProperties()["bipolar"];

    const auto area = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto c    = area.getCentre();

    const float outer = jmin (area.getWidth(), area.getHeight()) * 0.5f - 2.0f;
    const float ringR = outer - (big ? 3.0f : 2.0f);       // dotted LED ring
    const float bodyR = ringR * (big ? 0.80f : 0.72f);     // knob body
    const float capR  = bodyR * (big ? 0.60f : 0.58f);     // raised cap

    const float angle = startAngle + pos * (endAngle - startAngle);

    // --- dotted ring ---
    const int n      = big ? 49 : 25;
    const int mid    = (n - 1) / 2;
    const int active = roundToInt (pos * (float) (n - 1));

    for (int i = 0; i < n; ++i)
    {
        const float a = startAngle + (float) i / (float) (n - 1) * (endAngle - startAngle);
        const juce::Point<float> p (c.x + ringR * std::sin (a), c.y - ringR * std::cos (a));

        bool lit;

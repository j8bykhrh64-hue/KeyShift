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
        const juce::Point<float> point {
            c.x + ringR * std::sin (a),
            c.y - ringR * std::cos (a)
        };

        const bool isLit = bipolar
            ? (i >= jmin (mid, active) && i <= jmax (mid, active))
            : (pos > 0.0f && i <= active);

        const float baseR = big ? 1.5f : 1.1f;
        const float dotR  = (i == mid && bipolar) ? baseR * 1.5f : baseR;

        if (isLit)
        {
            g.setColour (ks::dotLit.withAlpha (0.20f));
            g.fillEllipse (point.x - dotR * 2.6f, point.y - dotR * 2.6f,
                           dotR * 5.2f, dotR * 5.2f);
            g.setColour (ks::dotLit);
            g.fillEllipse (point.x - dotR * 1.2f, point.y - dotR * 1.2f,
                           dotR * 2.4f, dotR * 2.4f);
        }
        else
        {
            g.setColour (ks::dotOff);
            g.fillEllipse (point.x - dotR, point.y - dotR,
                           dotR * 2.0f, dotR * 2.0f);
        }
    }

    // --- soft knob body ---
    {
        Path body;
        body.addEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        DropShadow (Colour (0x2a506088), big ? 20 : 11, juce::Point<int> (0, big ? 8 : 4)).drawForPath (g, body);

        g.setGradientFill (ColourGradient (Colour (0xfffdfdff), c.x, c.y - bodyR,
                                           Colour (0xffe8ebf4), c.x, c.y + bodyR, false));
        g.fillPath (body);

        g.setColour (Colours::white.withAlpha (0.9f));
        g.drawEllipse (c.x - bodyR + 0.5f, c.y - bodyR + 0.5f, bodyR * 2.0f - 1.0f, bodyR * 2.0f - 1.0f, 1.0f);

        // thin concentric line
        g.setColour (Colour (0x10000000));
        g.drawEllipse (c.x - bodyR * 0.90f, c.y - bodyR * 0.90f, bodyR * 1.80f, bodyR * 1.80f, 0.8f);
    }

    // --- raised cap ---
    {
        Path cap;
        cap.addEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
        DropShadow (Colour (0x34506088), big ? 12 : 6, juce::Point<int> (0, big ? 5 : 3)).drawForPath (g, cap);

        g.setGradientFill (ColourGradient (Colours::white, c.x, c.y - capR,
                                           Colour (0xffeceff6), c.x, c.y + capR, false));
        g.fillPath (cap);

        g.setColour (Colour (0x0e000000));
        g.drawEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 0.8f);
    }

    // --- pointer dot on the rim ---
    {
        const float pr   = bodyR * 0.86f;
        const float dotR = big ? 2.8f : 1.9f;
        const juce::Point<float> p (c.x + pr * std::sin (angle), c.y - pr * std::cos (angle));

        g.setColour (ks::pointer.withAlpha (0.22f));
        g.fillEllipse (p.x - dotR * 2.2f, p.y - dotR * 2.2f, dotR * 4.4f, dotR * 4.4f);
        g.setColour (ks::pointer);
        g.fillEllipse (p.x - dotR, p.y - dotR, dotR * 2.0f, dotR * 2.0f);
    }
}

void SoftLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                        bool highlighted, bool down)
{
    using namespace juce;
    ignoreUnused (down);

    const auto r  = b.getLocalBounds().toFloat().reduced (1.5f);
    const bool on = b.getToggleState();   // ON = bypass engaged

    Path p;
    p.addRoundedRectangle (r, r.getHeight() * 0.5f);

    g.setColour (Colours::white.withAlpha (highlighted ? 0.95f : 0.65f));
    g.fillPath (p);
    g.setColour (Colour (0x22000000));
    g.strokePath (p, PathStrokeType (1.0f));

    const juce::Point<float> led (r.getX() + r.getHeight() * 0.5f, r.getCentreY());
    if (on)
    {
        g.setColour (ks::amber.withAlpha (0.30f));
        g.fillEllipse (led.x - 7.0f, led.y - 7.0f, 14.0f, 14.0f);
        g.setColour (ks::amber);
    }
    else
    {
        g.setColour (ks::dotOff);
    }
    g.fillEllipse (led.x - 3.0f, led.y - 3.0f, 6.0f, 6.0f);

    drawTextLabel (g, "BYPASS", r.withTrimmedLeft (r.getHeight() * 0.8f), 9.5f, false,
               on ? Colour (0xff5d6580) : ks::inkSoft, 0.22f, Justification::centred);
}

//==============================================================================
// Editor
//==============================================================================
KeyShiftEditor::KeyShiftEditor (KeyShiftProcessor& p)
    : AudioProcessorEditor (&p),
      keyShiftProcessor (p),
      pitchAtt  (p.apvts, "semitones", pitch),
      fineAtt   (p.apvts, "fine",      fine),
      mixAtt    (p.apvts, "mix",       mix),
      outAtt    (p.apvts, "output",    output),
      bypassAtt (p.apvts, "bypass",    bypass)
{
    setupKnob (pitch,  true,  true,  0.0);
    setupKnob (fine,   false, true,  0.0);
    setupKnob (mix,    false, false, 1.0);
    setupKnob (output, false, true,  0.0);

    bypass.setLookAndFeel (&lnf);
    bypass.onClick = [this] { repaint(); };
    addAndMakeVisible (bypass);

    setSize (kW, kH);
    startTimerHz (30);
}

KeyShiftEditor::~KeyShiftEditor()
{
    stopTimer();
    pitch.setLookAndFeel (nullptr);
    fine.setLookAndFeel (nullptr);
    mix.setLookAndFeel (nullptr);
    output.setLookAndFeel (nullptr);
    bypass.setLookAndFeel (nullptr);
}

void KeyShiftEditor::setupKnob (juce::Slider& s, bool big, bool bipolar, double defaultValue)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (juce::degreesToRadians (225.0f),
                           juce::degreesToRadians (495.0f), true);
    s.setDoubleClickReturnValue (true, defaultValue);   // double-click = reset
    s.setMouseDragSensitivity (big ? 180 : 140);
    s.getProperties().set ("big", big);
    s.getProperties().set ("bipolar", bipolar);
    s.setLookAndFeel (&lnf);
    s.onValueChange = [this] { repaint(); };
    addAndMakeVisible (s);
}

void KeyShiftEditor::timerCallback()
{
    displayIn  = juce::jmax (keyShiftProcessor.consumeInputPeak(),  displayIn  * 0.82f);
    displayOut = juce::jmax (keyShiftProcessor.consumeOutputPeak(), displayOut * 0.82f);

    repaint (kModule.getSmallestIntegerContainer().expanded (30, 30));
}

void KeyShiftEditor::resized()
{
    pitch.setBounds  (64,  52,  236, 236);
    fine.setBounds   (360, 236, 64, 64);
    mix.setBounds    (434, 236, 64, 64);
    output.setBounds (508, 236, 64, 64);
    bypass.setBounds (466, 342, 96, 26);
}

//------------------------------------------------------------------------------
void KeyShiftEditor::drawModule (juce::Graphics& g)
{
    using namespace juce;

    const auto m = kModule;

    Path p;
    p.addRoundedRectangle (m, 18.0f);

    // soft cyan glow + dark shadow
    DropShadow (Colour (0x3048d8e8), 26, juce::Point<int> (0, 12)).drawForPath (g, p);
    DropShadow (Colour (0x55303a5c), 16, juce::Point<int> (0, 8)).drawForPath (g, p);

    g.setGradientFill (ColourGradient (Colour (0xff222630), m.getX(), m.getY(),
                                       Colour (0xff0e1015), m.getX(), m.getBottom(), false));
    g.fillPath (p);

    // faint cyan bloom inside the module
    {
        Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (p, AffineTransform());
        g.setGradientFill (ColourGradient (Colour (0x2848d8e8), m.getX() + 30.0f, m.getBottom(),
                                           Colour (0x0048d8e8), m.getX() + 30.0f, m.getBottom() - 110.0f, true));
        g.fillRect (m);
    }

    g.setColour (Colour (0x14ffffff));
    g.strokePath (p, PathStrokeType (1.0f));

    // --- readout (left) ---
    const double total = pitch.getValue() + fine.getValue() * 0.01;
    const String shiftText = String ((total >= 0.0 ? "+" : "") + String (total, 2));
    const juce::Rectangle<float> valueRect (m.getX() + 18.0f, m.getY() + 36.0f, 100.0f, 32.0f);

    drawTextLabel (g, "SHIFT", { m.getX() + 18.0f, m.getY() + 20.0f, 80.0f, 12.0f }, 9.0f, false,
               ks::cyan, 0.30f, Justification::centredLeft);

    for (float dx : { -1.5f, 1.5f })
        for (float dy : { -1.5f, 1.5f })
            drawTextLabel (g, shiftText, valueRect.translated (dx, dy), 26.0f, false,
                       ks::cyan.withAlpha (0.10f), 0.02f, Justification::centredLeft);

    drawTextLabel (g, shiftText, valueRect, 26.0f, false, Colour (0xffeaf6ff), 0.02f,
               Justification::centredLeft);
    drawTextLabel (g, "SEMITONES", { m.getX() + 18.0f, m.getY() + 68.0f, 90.0f, 12.0f }, 8.0f, false,
               Colour (0xff6b7389), 0.28f, Justification::centredLeft);

    const double ratio = std::pow (2.0, total / 12.0);
    drawTextLabel (g, "x" + String (ratio, 3), { m.getX() + 18.0f, m.getY() + 104.0f, 90.0f, 14.0f },
               12.0f, false, Colour (0xff9fb4c8), 0.08f, Justification::centredLeft);
    drawTextLabel (g, "RATIO", { m.getX() + 18.0f, m.getY() + 118.0f, 90.0f, 12.0f }, 8.0f, false,
               Colour (0xff6b7389), 0.28f, Justification::centredLeft);

    // --- meters (right) ---
    const float yBottom = m.getY() + 128.0f;
    const float xIn  = m.getX() + 108.0f;
    const float xOut = m.getX() + 130.0f;

    drawMeter (g, xIn,  yBottom, displayIn);
    drawMeter (g, xOut, yBottom, displayOut);

    drawTextLabel (g, "IN",  { xIn  - 6.0f, yBottom + 6.0f, 24.0f, 10.0f }, 8.0f, false,
               Colour (0xff6b7389), 0.2f, Justification::centred);
    drawTextLabel (g, "OUT", { xOut - 6.0f, yBottom + 6.0f, 24.0f, 10.0f }, 8.0f, false,
               Colour (0xff6b7389), 0.2f, Justification::centred);

    // dB scale
    struct Mark { const char* text; int seg; };
    const Mark marks[] = { { "0", 15 }, { "-12", 11 }, { "-24", 7 }, { "-36", 3 } };
    for (const auto& mk : marks)
    {
        const float yc = yBottom - (float) (mk.seg + 1) * 6.5f + 2.0f + 2.25f;
        drawTextLabel (g, mk.text, { m.getX() + 150.0f, yc - 6.0f, 40.0f, 12.0f }, 8.0f, false,
                   Colour (0xff6b7389), 0.1f, Justification::centredLeft);
    }
}

void KeyShiftEditor::paint (juce::Graphics& g)
{
    using namespace juce;

    // pale background with soft glass orbs
    g.setGradientFill (ColourGradient (Colour (0xffeceff7), 0.0f, 0.0f,
                                       Colour (0xffd8ddeb), 0.0f, (float) kH, false));
    g.fillAll();
    drawOrb (g, 592.0f, 384.0f, 56.0f);
    drawOrb (g, 8.0f, 10.0f, 34.0f);

    // main card
    Path card;
    card.addRoundedRectangle (kCard, 26.0f);
    DropShadow (Colour (0x30404a78), 28, juce::Point<int> (0, 12)).drawForPath (g, card);
    g.setGradientFill (ColourGradient (Colour (0xfffafbfe), 0.0f, kCard.getY(),
                                       Colour (0xffeef0f7), 0.0f, kCard.getBottom(), false));
    g.fillPath (card);
    g.setColour (Colours::white.withAlpha (0.9f));
    g.strokePath (card, PathStrokeType (1.0f));

    // title
    drawTextLabel (g, "KEYSHIFT", { 36.0f, 28.0f, 200.0f, 14.0f }, 10.5f, false,
               ks::ink, 0.38f, Justification::centredLeft);

    // tiny dot grid, top right
    g.setColour (Colour (0xffc3c8d8));
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 6; ++col)
            g.fillEllipse (kCard.getRight() - 66.0f + (float) col * 6.0f,
                           kCard.getY() + 18.0f + (float) row * 6.0f, 2.0f, 2.0f);

    // knob value labels
    const juce::String bullet = juce::String::fromUTF8 ("\xe2\x80\xa2");

    const int semis = (int) std::round (pitch.getValue());
    const int cents = (int) std::round (fine.getValue());
    const int mixPc = roundToInt ((float) mix.getValue() * 100.0f);
    const float outDb = (float) output.getValue();

    drawTextLabel (g, "-12", { 182.0f - 81.0f - 20.0f, 260.0f, 40.0f, 12.0f }, 9.5f, false,
               ks::inkSoft, 0.1f, Justification::centred);
    drawTextLabel (g, "+12", { 182.0f + 81.0f - 20.0f, 260.0f, 40.0f, 12.0f }, 9.5f, false,
               ks::inkSoft, 0.1f, Justification::centred);

    drawTextLabel (g, "PITCH  " + bullet + "  " + String ((semis > 0 ? "+" : "") + String (semis)) + " st",
               { 62.0f, 292.0f, 240.0f, 22.0f }, 13.0f, false, ks::ink, 0.22f, Justification::centred);

    struct Small { const char* name; juce::String value; float x; };
    const Small smalls[] =
    {
        { "FINE",   String ((cents > 0 ? "+" : "") + String (cents)) + " ct", 360.0f },
        { "MIX",    String (mixPc) + " %",                                    434.0f },
        { "OUTPUT", String ((outDb > 0.05f ? "+" : "") + String (outDb, 1)) + " dB", 508.0f }
    };
    for (const auto& sm : smalls)
    {
        drawTextLabel (g, sm.name,  { sm.x - 5.0f, 302.0f, 74.0f, 12.0f }, 9.0f, false,
                   ks::inkSoft, 0.26f, Justification::centred);
        drawTextLabel (g, sm.value, { sm.x - 5.0f, 314.0f, 74.0f, 12.0f }, 9.5f, false,
                   ks::ink, 0.10f, Justification::centred);
    }

    // footer: thin divider + latency
    g.setColour (Colour (0x10000000));
    g.fillRect (36.0f, 334.0f, 528.0f, 1.0f);
    g.setColour (Colours::white.withAlpha (0.9f));
    g.fillRect (36.0f, 335.0f, 528.0f, 1.0f);

    const double sr = keyShiftProcessor.getSampleRate();
    const String lat = sr > 0.0 ? String (keyShiftProcessor.getLatencySamples() * 1000.0 / sr, 0) + " ms" : String ("--");
    drawTextLabel (g, "LATENCY  " + lat, { 36.0f, 342.0f, 220.0f, 26.0f }, 9.5f, false,
               ks::inkSoft, 0.22f, Justification::centredLeft);

    // black glowing module
    drawModule (g);
}

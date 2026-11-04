#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr int kWidth       = 620;
    constexpr int kHeight      = 480;
    constexpr int kDisplayH    = 132;
    constexpr int kPanelTop    = 124;
    constexpr int kMeterTop    = 440;
    constexpr int kMeterDots   = 40;

    void drawLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r,
                    float size, bool bold, juce::Colour col, float kerning,
                    juce::Justification just)
    {
        juce::Font f (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
        f.setExtraKerningFactor (kerning);
        g.setFont (f);
        g.setColour (col);
        g.drawText (text, r, just, false);
    }

    void drawPill (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text)
    {
        juce::Path p;
        p.addRoundedRectangle (r, r.getHeight() * 0.5f);

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd5dce7), r.getX(), r.getY(),
                                                 juce::Colour (0xffe8edf5), r.getX(), r.getBottom(), false));
        g.fillPath (p);

        // tiny highlight on the lower edge gives an inset look
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.drawLine (r.getX() + r.getHeight() * 0.5f, r.getBottom() - 0.5f,
                    r.getRight() - r.getHeight() * 0.5f, r.getBottom() - 0.5f, 1.0f);

        drawLabel (g, text, r, 10.5f, true, ks::ink, 0.12f, juce::Justification::centred);
    }

    void drawScrew (juce::Graphics& g, float cx, float cy)
    {
        const float r = 4.5f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff4f7fb), cx, cy - r,
                                                 juce::Colour (0xffd2d9e5), cx, cy + r, false));
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
        g.setColour (juce::Colour (0x30000000));
        g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 0.8f);
        g.drawLine (cx - 2.2f, cy + 2.2f, cx + 2.2f, cy - 2.2f, 1.0f);
    }

    juce::Colour meterColour (float t)
    {
        if (t < 0.62f) return ks::cyan;
        if (t < 0.82f) return juce::Colour (0xffe9d85a);
        if (t < 0.93f) return juce::Colour (0xffff9a3c);
        return juce::Colour (0xffff4d4d);
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

    const auto area  = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto c     = area.getCentre();
    const float outer = jmin (area.getWidth(), area.getHeight()) * 0.5f - 3.0f;
    const float arcR  = outer * 0.80f;
    const float thick = big ? 6.0f : 4.0f;
    const float capR  = arcR * 0.76f;

    const float angle     = startAngle + pos * (endAngle - startAngle);
    const float zeroAngle = startAngle + 0.5f * (endAngle - startAngle);

    // --- recessed bezel ---
    {
        Path bz;
        bz.addEllipse (c.x - outer, c.y - outer, outer * 2.0f, outer * 2.0f);
        DropShadow (Colour (0x30506088), big ? 16 : 10, juce::Point<int> (0, big ? 6 : 4)).drawForPath (g, bz);

        g.setGradientFill (ColourGradient (Colour (0xffdbe2ed), c.x, c.y - outer,
                                           Colour (0xfff8fafd), c.x, c.y + outer, false));
        g.fillPath (bz);

        g.setColour (Colours::white.withAlpha (0.85f));
        g.drawEllipse (c.x - outer + 0.5f, c.y - outer + 0.5f, outer * 2.0f - 1.0f, outer * 2.0f - 1.0f, 1.0f);
    }

    // --- dark arc track ---
    Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (Colour (0xff2b3040));
    g.strokePath (track, PathStrokeType (thick, PathStrokeType::curved, PathStrokeType::rounded));

    // --- glowing value arc ---
    const float a0 = bipolar ? jmin (zeroAngle, angle) : startAngle;
    const float a1 = bipolar ? jmax (zeroAngle, angle) : angle;

    if (a1 - a0 > 0.01f)
    {
        Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, a0, a1, true);

        g.setColour (ks::amber.withAlpha (0.10f));
        g.strokePath (val, PathStrokeType (thick + 10.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (ks::amber.withAlpha (0.22f));
        g.strokePath (val, PathStrokeType (thick + 5.0f, PathStrokeType::curved, PathStrokeType::rounded));

        g.setGradientFill (ColourGradient (ks::amberHi, c.x, c.y - arcR,
                                           ks::amberLo, c.x, c.y + arcR, false));
        g.strokePath (val, PathStrokeType (thick, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // --- zero marker at 12 o'clock ---
    if (bipolar)
    {
        const float len = big ? 9.0f : 6.0f;
        const float top = c.y - arcR - thick * 0.5f - 1.0f - len;
        g.setColour (Colour (0xff2b3040));
        g.fillRoundedRectangle (c.x - 1.5f, top, 3.0f, len, 1.5f);
    }

    // --- raised cap ---
    {
        Path cap;
        cap.addEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
        DropShadow (Colour (0x55506088), big ? 14 : 8, juce::Point<int> (0, big ? 7 : 4)).drawForPath (g, cap);

        g.setGradientFill (ColourGradient (Colours::white, c.x, c.y - capR,
                                           Colour (0xffe3e8f1), c.x, c.y + capR, false));
        g.fillPath (cap);

        g.setColour (Colour (0x12000000));
        g.drawEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 1.0f);
    }

    // --- pointer dot ---
    {
        const float pr   = capR * 0.66f;
        const float dotR = big ? 6.5f : 4.2f;
        const juce::Point<float> p (c.x + pr * std::sin (angle), c.y - pr * std::cos (angle));

        g.setColour (ks::dotBlue.withAlpha (0.35f));
        g.fillEllipse (p.x - dotR - 2.0f, p.y - dotR - 2.0f, (dotR + 2.0f) * 2.0f, (dotR + 2.0f) * 2.0f);

        g.setGradientFill (ColourGradient (Colour (0xffcfe0f3), p.x, p.y - dotR,
                                           ks::dotBlue.darker (0.15f), p.x, p.y + dotR, false));
        g.fillEllipse (p.x - dotR, p.y - dotR, dotR * 2.0f, dotR * 2.0f);
    }
}

void SoftLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                        bool highlighted, bool down)
{
    using namespace juce;
    ignoreUnused (down);

    const auto r = b.getLocalBounds().toFloat().reduced (3.0f);
    const bool on = b.getToggleState();   // ON = bypass engaged

    Path p;
    p.addRoundedRectangle (r, 14.0f);
    DropShadow (Colour (0x55506088), 12, juce::Point<int> (0, 5)).drawForPath (g, p);

    const Colour top = highlighted ? Colour (0xff474e63) : Colour (0xff3a4052);
    g.setGradientFill (ColourGradient (top, r.getX(), r.getY(),
                                       Colour (0xff1a1d28), r.getX(), r.getBottom(), false));
    g.fillPath (p);

    // button face
    const auto face = r.reduced (10.0f);
    g.setGradientFill (ColourGradient (Colour (0xff222634), face.getX(), face.getY(),
                                       Colour (0xff30364a), face.getX(), face.getBottom(), false));
    g.fillEllipse (face);
    g.setColour (Colour (0x18ffffff));
    g.drawEllipse (face, 1.0f);

    // LED
    const juce::Point<float> led (r.getRight() - 10.0f, r.getY() + 10.0f);
    if (on)
    {
        g.setColour (ks::amber.withAlpha (0.28f));
        g.fillEllipse (led.x - 8.0f, led.y - 8.0f, 16.0f, 16.0f);
        g.setColour (ks::amberHi);
    }
    else
    {
        g.setColour (Colour (0xff0d0f15));
    }
    g.fillEllipse (led.x - 3.2f, led.y - 3.2f, 6.4f, 6.4f);
}

//==============================================================================
// Editor
//==============================================================================
KeyShiftEditor::KeyShiftEditor (KeyShiftProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
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

    setSize (kWidth, kHeight);
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
    const float peak = processor.consumeOutputPeak();
    displayLevel = juce::jmax (peak, displayLevel * 0.82f);
    phase += 0.09f;

    repaint (0, 0, getWidth(), kDisplayH);
    repaint (0, kMeterTop, getWidth(), getHeight() - kMeterTop);
}

void KeyShiftEditor::resized()
{
    pitch.setBounds  (52,  148, 240, 240);
    fine.setBounds   (352, 150, 96, 96);
    mix.setBounds    (487, 150, 96, 96);
    output.setBounds (352, 288, 96, 96);
    bypass.setBounds (503, 304, 64, 64);

    pitchPill  = { 102.0f, 402.0f, 140.0f, 24.0f };
    finePill   = { 350.0f, 252.0f, 100.0f, 22.0f };
    mixPill    = { 485.0f, 252.0f, 100.0f, 22.0f };
    outPill    = { 350.0f, 390.0f, 100.0f, 22.0f };
    bypassPill = { 485.0f, 390.0f, 100.0f, 22.0f };
}

//------------------------------------------------------------------------------
void KeyShiftEditor::drawDisplay (juce::Graphics& g)
{
    using namespace juce;

    const float W = (float) getWidth();

    // background + soft violet glow
    g.setGradientFill (ColourGradient (Colour (0xff222840), 0.0f, 0.0f,
                                       Colour (0xff12151f), 0.0f, (float) kDisplayH, false));
    g.fillRect (0.0f, 0.0f, W, (float) kDisplayH);

    g.setGradientFill (ColourGradient (Colour (0x44604bd0), 430.0f, 100.0f,
                                       Colour (0x00604bd0), 700.0f, 100.0f, true));
    g.fillRect (0.0f, 0.0f, W, (float) kDisplayH);

    // title
    drawLabel (g, "KEYSHIFT", { 24.0f, 14.0f, 200.0f, 16.0f }, 13.0f, true,
               Colour (0xffe8edfa), 0.32f, Justification::centredLeft);
    drawLabel (g, "PITCH SHIFTER", { 24.0f, 31.0f, 200.0f, 12.0f }, 9.0f, false,
               Colour (0xff7d89a8), 0.28f, Justification::centredLeft);

    // shift readout
    const double total = pitch.getValue() + fine.getValue() * 0.01;
    const String shiftText = String ((total >= 0.0 ? "+" : "") + String (total, 2));
    const juce::Rectangle<float> valueRect (24.0f, 46.0f, 210.0f, 34.0f);

    for (float dx : { -1.5f, 1.5f })
        for (float dy : { -1.5f, 1.5f })
            drawLabel (g, shiftText, valueRect.translated (dx, dy), 28.0f, true,
                       ks::cyan.withAlpha (0.10f), 0.04f, Justification::centredLeft);

    drawLabel (g, shiftText, valueRect, 28.0f, true, Colour (0xfff2f5ff), 0.04f,
               Justification::centredLeft);
    drawLabel (g, "SEMITONES", { 24.0f, 80.0f, 120.0f, 12.0f }, 9.0f, false,
               Colour (0xff7d89a8), 0.28f, Justification::centredLeft);

    // ratio + latency
    const double ratio = std::pow (2.0, total / 12.0);
    drawLabel (g, "RATIO  x" + String (ratio, 3), { W - 220.0f, 16.0f, 196.0f, 14.0f }, 10.0f, false,
               Colour (0xff9aa6c4), 0.18f, Justification::centredRight);

    const double sr = processor.getSampleRate();
    const String lat = sr > 0.0 ? String (processor.getLatencySamples() * 1000.0 / sr, 0) + " ms" : String ("--");
    drawLabel (g, "LATENCY  " + lat, { W - 220.0f, 32.0f, 196.0f, 14.0f }, 10.0f, false,
               Colour (0xff7d89a8), 0.18f, Justification::centredRight);

    // glowing waveform
    const float base = 100.0f;
    const float x0 = 24.0f, x1 = W - 24.0f;
    const float dB  = Decibels::gainToDecibels (displayLevel, -80.0f);
    const float lvl = jlimit (0.0f, 1.0f, (dB + 55.0f) / 55.0f);
    const float amp = 5.0f + 24.0f * lvl;

    const float centres[3] = { 0.36f, 0.58f, 0.82f };
    const float widths[3]  = { 0.07f, 0.09f, 0.08f };

    auto envelope = [&] (float x)
    {
        float e = 0.0f;
        const float t0 = (x - x0) / (x1 - x0);
        for (int k = 0; k < 3; ++k)
        {
            const float t = (t0 - centres[k]) / widths[k];
            e += (0.75f + 0.25f * std::sin (phase + (float) k * 1.7f)) * std::exp (-t * t);
        }
        return e;
    };

    auto horizontalGradient = [&] (float alpha)
    {
        ColourGradient grad (Colour (0xffff8a2a).withAlpha (alpha), x0, 0.0f,
                             Colour (0xff9a63ff).withAlpha (alpha), x1, 0.0f, false);
        grad.addColour (0.5, Colour (0xffff5fa8).withAlpha (alpha));
        return grad;
    };

    // soft glow blobs at the lobes
    for (int k = 0; k < 3; ++k)
    {
        const float cx = x0 + centres[k] * (x1 - x0);
        g.setGradientFill (ColourGradient (Colour (0xffff6fb5).withAlpha (0.10f + 0.30f * lvl), cx, base,
                                           Colour (0xffff6fb5).withAlpha (0.0f), cx + 44.0f, base, true));
        g.fillEllipse (cx - 46.0f, base - 30.0f, 92.0f, 60.0f);
    }

    // fanned curves
    const float mults[6] = { -1.0f, -0.7f, -0.4f, 0.4f, 0.7f, 1.0f };
    g.setGradientFill (horizontalGradient (0.55f));
    for (float m : mults)
    {
        Path curve;
        for (float x = x0; x <= x1; x += 4.0f)
        {
            const float yy = base + envelope (x) * amp * m;
            if (x == x0) curve.startNewSubPath (x, yy);
            else         curve.lineTo (x, yy);
        }
        g.strokePath (curve, PathStrokeType (1.0f));
    }

    // base line with glow
    const Line<float> line (x0, base, x1, base);
    g.setGradientFill (horizontalGradient (0.10f));
    g.drawLine (line, 8.0f);
    g.setGradientFill (horizontalGradient (0.22f));
    g.drawLine (line, 4.0f);
    g.setGradientFill (horizontalGradient (1.0f));
    g.drawLine (line, 1.4f);
}

void KeyShiftEditor::drawMeterBar (juce::Graphics& g)
{
    using namespace juce;

    const float W = (float) getWidth();
    const float top = (float) kMeterTop;
    const float hgt = (float) (getHeight() - kMeterTop);

    g.setGradientFill (ColourGradient (Colour (0xff1d2130), 0.0f, top,
                                       Colour (0xff10131b), 0.0f, top + hgt, false));
    g.fillRect (0.0f, top, W, hgt);
    g.setColour (Colour (0x22ffffff));
    g.fillRect (0.0f, top, W, 1.0f);

    drawLabel (g, "OUTPUT", { 24.0f, top, 70.0f, hgt }, 11.5f, true,
               Colour (0xffe3e8f5), 0.2f, Justification::centredLeft);

    const float dB = Decibels::gainToDecibels (displayLevel, -80.0f);
    const int lit  = jlimit (0, kMeterDots,
                             roundToInt (jmap (dB, -60.0f, 0.0f, 0.0f, (float) kMeterDots)));

    const float mx0 = 100.0f, mx1 = 468.0f, cy = top + hgt * 0.5f;
    for (int i = 0; i < kMeterDots; ++i)
    {
        const float x = mx0 + (float) i * (mx1 - mx0) / (float) (kMeterDots - 1);
        const bool on = i < lit;
        const Colour col = on ? meterColour ((float) i / (float) (kMeterDots - 1))
                              : Colour (0xff2c2f3a);

        if (on)
        {
            g.setColour (col.withAlpha (0.25f));
            g.fillEllipse (x - 5.0f, cy - 5.0f, 10.0f, 10.0f);
        }
        g.setColour (col);
        g.fillEllipse (x - 2.8f, cy - 2.8f, 5.6f, 5.6f);
    }

    const String dbText = dB <= -60.0f ? String ("-inf dB") : String (dB, 1) + " dB";
    drawLabel (g, dbText, { 486.0f, top, 110.0f, hgt }, 12.0f, true,
               Colour (0xffcdd5ea), 0.1f, Justification::centredRight);
}

void KeyShiftEditor::paint (juce::Graphics& g)
{
    using namespace juce;

    const float W = (float) getWidth();

    // dark base + display
    drawDisplay (g);

    // light panel pulled over the display
    Path panel;
    panel.addRoundedRectangle (0.0f, (float) kPanelTop, W, 400.0f, 24.0f);
    DropShadow (Colour (0x66000000), 18, juce::Point<int> (0, -4)).drawForPath (g, panel);

    g.setGradientFill (ColourGradient (Colour (0xfff4f7fb), 0.0f, (float) kPanelTop,
                                       Colour (0xffe2e8f2), 0.0f, (float) kMeterTop, false));
    g.fillPath (panel);
    g.setColour (Colours::white.withAlpha (0.8f));
    g.strokePath (panel, PathStrokeType (1.0f));

    // divider between the big knob and the small ones
    g.setColour (Colour (0x16000000));
    g.fillRect (322.0f, 156.0f, 1.0f, 264.0f);
    g.setColour (Colours::white.withAlpha (0.9f));
    g.fillRect (323.0f, 156.0f, 1.0f, 264.0f);

    // screws
    drawScrew (g, 16.0f,  142.0f);
    drawScrew (g, W - 16.0f, 142.0f);
    drawScrew (g, 16.0f,  422.0f);
    drawScrew (g, W - 16.0f, 422.0f);

    // labels
    const int semis = (int) std::round (pitch.getValue());
    const int cents = (int) std::round (fine.getValue());
    const int mixPc = roundToInt ((float) mix.getValue() * 100.0f);
    const float outDb = (float) output.getValue();

    drawPill (g, pitchPill, "PITCH  " + String ((semis > 0 ? "+" : "") + String (semis)) + " st");
    drawPill (g, finePill,  "FINE  "  + String ((cents > 0 ? "+" : "") + String (cents)) + " ct");
    drawPill (g, mixPill,   "MIX  "   + String (mixPc) + " %");
    drawPill (g, outPill,   "OUT  "   + String ((outDb > 0.05f ? "+" : "") + String (outDb, 1)) + " dB");
    drawPill (g, bypassPill, "BYPASS");

    drawMeterBar (g);
}

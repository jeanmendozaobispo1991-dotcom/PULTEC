#include "PluginEditor.h"
#include <cmath>

using namespace juce;

namespace
{
    const Colour kInk (0xfff1efe6);

    Point<float> polar (Point<float> c, float r, float deg)       // deg: clockwise from 12 o'clock
    {
        const float a = degreesToRadians (deg);
        return { c.x + r * std::sin (a), c.y - r * std::cos (a) };
    }
    Point<float> polarMath (Point<float> c, float r, float deg)   // deg: counter-clockwise from 3 o'clock
    {
        const float a = degreesToRadians (deg);
        return { c.x + r * std::cos (a), c.y - r * std::sin (a) };
    }

    void label (Graphics& g, const String& t, float cx, float cy, float size, bool bold)
    {
        g.setFont (FontOptions (size, bold ? Font::bold : Font::plain));
        Rectangle<float> r (0.f, 0.f, 420.f, size * 1.7f);
        g.setColour (Colours::black.withAlpha (0.45f));
        g.drawText (t, r.withCentre ({ cx + 1.6f, cy + 1.6f }), Justification::centred, false);
        g.setColour (kInk);
        g.drawText (t, r.withCentre ({ cx, cy }), Justification::centred, false);
    }

    void scale (Graphics& g, Point<float> c, float R, const StringArray& labels, float rLab, float size)
    {
        const int n = labels.size();
        for (int i = 0; i < n; ++i)
        {
            const float deg = -135.f + 270.f * (float) i / (float) (n - 1);
            g.setColour (kInk);
            g.drawLine (Line<float> (polar (c, R * 1.13f, deg), polar (c, R * 1.25f, deg)), 2.0f);
            auto p = polar (c, R * rLab, deg);
            label (g, labels[i], p.x, p.y, size, false);
        }
    }

    void leverLabels (Graphics& g, Point<float> c, const StringArray& labels, float a0, float a1, float r, float size)
    {
        const int n = labels.size();
        for (int i = 0; i < n; ++i)
        {
            auto p = polarMath (c, r, a0 + (a1 - a0) * (float) i / (float) (n - 1));
            label (g, labels[i], p.x, p.y, size, false);
        }
    }

    void chromeScrew (Graphics& g, float x, float y, float r)
    {
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillEllipse (x - r + 2, y - r + 3, r * 2, r * 2);
        g.setGradientFill (ColourGradient (Colour (0xfff2f3f2), x - r, y - r, Colour (0xff6b7176), x + r, y + r, false));
        g.fillEllipse (x - r, y - r, r * 2, r * 2);
        g.setColour (Colour (0xff30363b));
        g.drawEllipse (x - r, y - r, r * 2, r * 2, 1.6f);
        g.drawLine (x - r * .7f, y - r * .25f, x + r * .7f, y + r * .25f, 3.2f);
    }

    void slot (Graphics& g, float cx, float cy)
    {
        Rectangle<float> r (62.f, 34.f);
        r.setCentre (cx, cy);
        g.setColour (Colour (0xff10202c));
        g.fillRoundedRectangle (r.expanded (3.f), 17.f);
        g.setGradientFill (ColourGradient (Colour (0xffe4e6e5), cx, r.getY(), Colour (0xff6e777d), cx, r.getBottom(), false));
        g.fillRoundedRectangle (r, 17.f);
        g.setColour (Colour (0xff2a3238));
        g.drawRoundedRectangle (r, 17.f, 1.5f);
    }
}

// ============================================================ look and feel ====
void PultecLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                          float start, float end, Slider& s)
{
    const Point<float> c ((float) x + (float) w * 0.5f, (float) y + (float) h * 0.5f);
    const float radius = (float) jmin (w, h) * 0.5f;

    if (s.getProperties()["kind"].toString() == "lever")
    {
        const float a0 = (float) (double) s.getProperties()["a0"], a1 = (float) (double) s.getProperties()["a1"];
        const float a = a0 + (a1 - a0) * pos;
        const float base = radius * 0.69f, len = radius * 0.95f;
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillEllipse (c.x - base + 3, c.y - base + 5, base * 2, base * 2);
        g.setGradientFill (ColourGradient (Colour (0xff2c2c30), c.x - base, c.y - base, Colour (0xff050506), c.x + base, c.y + base, false));
        g.fillEllipse (c.x - base, c.y - base, base * 2, base * 2);
        g.setColour (Colour (0xff3a3a3f));
        g.drawEllipse (c.x - base, c.y - base, base * 2, base * 2, 1.5f);
        g.setColour (Colour (0xff131315));
        g.fillEllipse (c.x - base * .72f, c.y - base * .72f, base * 1.44f, base * 1.44f);

        auto dir = polarMath ({ 0.f, 0.f }, 1.f, a);
        const Point<float> n (-dir.y, dir.x);
        Path hnd;
        hnd.startNewSubPath (c - n * (base * 0.42f));
        hnd.lineTo (c + n * (base * 0.42f));
        hnd.lineTo (c + dir * len + n * (base * 0.24f));
        hnd.lineTo (c + dir * len - n * (base * 0.24f));
        hnd.closeSubPath();
        g.setGradientFill (ColourGradient (Colour (0xff2a2a2e), c.x, c.y, Colour (0xff050506), c.x + dir.x * len, c.y + dir.y * len, false));
        g.fillPath (hnd.createPathWithRoundedCorners (5.f));
        g.setColour (Colour (0xfff4f1e8));
        g.drawLine (Line<float> (c + dir * (base * 0.35f), c + dir * (len - 4.f)), 2.4f);
        return;
    }

    // ---- fluted black knob ----
    const float R = radius / 1.12f;
    const float ang = start + pos * (end - start);

    for (int i = 0; i < 4; ++i)                                    // soft shadow
    {
        g.setColour (Colours::black.withAlpha (0.09f));
        const float e = R * (1.02f + 0.035f * (float) i);
        g.fillEllipse (c.x - e + R * 0.07f, c.y - e + R * 0.13f, e * 2, e * 2);
    }
    Path skirt;
    const int N = 360;
    for (int i = 0; i <= N; ++i)
    {
        const float th = MathConstants<float>::twoPi * (float) i / (float) N;
        const float rr = R * (1.f + 0.024f * std::cos (30.f * th));
        Point<float> p (c.x + rr * std::sin (th), c.y - rr * std::cos (th));
        i == 0 ? skirt.startNewSubPath (p) : skirt.lineTo (p);
    }
    skirt.closeSubPath();
    g.setGradientFill (ColourGradient (Colour (0xff3a3a3e), c.x - R * .6f, c.y - R * .7f, Colour (0xff060607), c.x + R * .7f, c.y + R * .8f, false));
    g.fillPath (skirt);
    g.setColour (Colour (0xff000000));
    g.strokePath (skirt, PathStrokeType (1.2f));

    g.setGradientFill (ColourGradient (Colour (0xff2a2a2d), c.x - R * .5f, c.y - R * .6f, Colour (0xff0a0a0b), c.x + R * .6f, c.y + R * .7f, false));
    g.fillEllipse (c.x - R * .80f, c.y - R * .80f, R * 1.60f, R * 1.60f);
    g.setColour (Colour (0xff050505));
    g.drawEllipse (c.x - R * .80f, c.y - R * .80f, R * 1.60f, R * 1.60f, 1.4f);
    g.setGradientFill (ColourGradient (Colour (0xff1b1b1e), c.x - R * .4f, c.y - R * .5f, Colour (0xff070708), c.x + R * .5f, c.y + R * .6f, false));
    g.fillEllipse (c.x - R * .63f, c.y - R * .63f, R * 1.26f, R * 1.26f);
    g.setColour (Colour (0xff303034));
    g.drawEllipse (c.x - R * .63f, c.y - R * .63f, R * 1.26f, R * 1.26f, 1.2f);

    g.setColour (Colours::white.withAlpha (0.10f));                 // sheen
    g.fillEllipse (c.x - R * .52f, c.y - R * .56f, R * .72f, R * .34f);

    const auto p0 = polar (c, R * 0.30f, radiansToDegrees (ang));
    const auto p1 = polar (c, R * 0.94f, radiansToDegrees (ang));
    g.setColour (Colours::black.withAlpha (0.5f));
    g.drawLine (Line<float> (p0, p1), R * 0.075f);
    g.setColour (Colour (0xfff7f4ea));
    g.drawLine (Line<float> (p0, p1), R * 0.050f);
}

void PultecLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool, bool)
{
    const auto bounds = b.getLocalBounds().toFloat();
    const Point<float> c = bounds.getCentre();
    const float r = jmin (bounds.getWidth(), bounds.getHeight()) * 0.22f;
    const bool on = b.getToggleState();

    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillEllipse (c.x - r + 2, c.y - r + 4, r * 2, r * 2);
    g.setGradientFill (ColourGradient (Colour (0xfff0f1f0), c.x - r, c.y - r, Colour (0xff6c7479), c.x + r, c.y + r, false));
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    g.setColour (Colour (0xff2b3237));
    g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 1.6f);
    g.setColour (Colour (0xff4b5358));
    g.fillEllipse (c.x - r * .55f, c.y - r * .55f, r * 1.1f, r * 1.1f);

    const float len = bounds.getHeight() * 0.27f * (on ? -1.f : 1.f);
    const Point<float> tip (c.x + 2.f, c.y + len);
    g.setColour (Colour (0xff0c0c0d));
    g.drawLine (Line<float> (c, tip), r * 0.62f);
    g.setColour (Colour (0xff8d9399));
    g.drawLine (Line<float> (c + Point<float> (-1.f, len * 0.15f), tip + Point<float> (-1.f, -len * 0.1f)), r * 0.14f);
    g.setColour (Colour (0xff1d1d1f));
    g.fillEllipse (tip.x - r * .36f, tip.y - r * .36f, r * .72f, r * .72f);
}

void DriveLamp::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const Point<float> c = b.getCentre();
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.30f;
    const float k = jlimit (0.f, 1.f, level);

    for (int i = 4; i >= 1; --i)                                   // glow
    {
        g.setColour (Colour (0xffff2a2a).withAlpha (0.05f + 0.07f * k) .withMultipliedAlpha ((float) (5 - i) * 0.35f + 0.2f));
        const float e = r * (1.f + 0.30f * (float) i * (0.4f + k));
        g.fillEllipse (c.x - e, c.y - e, e * 2, e * 2);
    }
    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillEllipse (c.x - r + 2, c.y - r + 4, r * 2, r * 2);
    g.setGradientFill (ColourGradient (Colour (0xfff2f3f2), c.x - r, c.y - r, Colour (0xff6c7479), c.x + r, c.y + r, false));
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    g.setColour (Colour (0xff2b3237));
    g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 1.6f);

    const float d = r * 0.80f;
    const Colour dark (0xff4a0508), lit (0xffff3535);
    g.setGradientFill (ColourGradient (dark.interpolatedWith (lit, 0.35f + 0.65f * k), c.x - d * .3f, c.y - d * .3f,
                                       dark, c.x + d, c.y + d, true));
    g.fillEllipse (c.x - d, c.y - d, d * 2, d * 2);
    g.setColour (Colours::white.withAlpha (0.30f + 0.35f * k));
    g.fillEllipse (c.x - d * .55f, c.y - d * .70f, d * .62f, d * .36f);
}

// ================================================================== editor ====
PultecEditor::PultecEditor (PultecProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    knob (lfBoost, "lf_boost", aLfBoost);   knob (lfCut, "lf_cut", aLfCut);
    knob (hfBoost, "hf_boost", aHfBoost);   knob (hfCut, "hf_cut", aHfCut);
    knob (hfBw, "hf_bw", aHfBw);
    knob (inGain, "in_gain", aIn);          knob (outGain, "out_gain", aOut);
    knob (sat, "sat", aSat);
    lever (lfFreq, "lf_freq", 187.f, 10.f, aLfFreq);
    lever (hfFreq, "hf_freq", 198.f, -18.f, aHfFreq);
    lever (hfCutFreq, "hf_cut_freq", 165.f, 15.f, aHfCutFreq);
    lever (osSel, "os", 165.f, 15.f, aOs);

    addAndMakeVisible (eqIn);     aEq = std::make_unique<BA> (proc.apvts, "eq_in", eqIn);
    addAndMakeVisible (autoGain); aAuto = std::make_unique<BA> (proc.apvts, "auto_gain", autoGain);
    addAndMakeVisible (lamp);
    lamp.setInterceptsMouseClicks (false, false);

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) (ui::kW / ui::kH));
    getConstrainer()->setSizeLimits (848, 202, 2546, 606);
    setSize (1274, 303);
    startTimerHz (30);
}

PultecEditor::~PultecEditor() { setLookAndFeel (nullptr); }

void PultecEditor::knob (Slider& s, const char* id, std::unique_ptr<SA>& att)
{
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::NoTextBox, true, 0, 0);
    s.setRotaryParameters (degreesToRadians (-135.f), degreesToRadians (135.f), true);
    s.setMouseDragSensitivity (220);
    s.getProperties().set ("kind", "knob");
    addAndMakeVisible (s);
    att = std::make_unique<SA> (proc.apvts, id, s);
}

void PultecEditor::lever (Slider& s, const char* id, float a0, float a1, std::unique_ptr<SA>& att)
{
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::NoTextBox, true, 0, 0);
    s.setRotaryParameters (degreesToRadians (-135.f), degreesToRadians (135.f), true);
    s.setMouseDragSensitivity (90);
    s.getProperties().set ("kind", "lever");
    s.getProperties().set ("a0", (double) a0);
    s.getProperties().set ("a1", (double) a1);
    addAndMakeVisible (s);
    att = std::make_unique<SA> (proc.apvts, id, s);
}

void PultecEditor::place (Component& c, float cx, float cy, float hw, float hh)
{
    const float s = (float) getWidth() / ui::kW;
    c.setBounds (Rectangle<float> ((cx - hw) * s, (cy - hh) * s, 2 * hw * s, 2 * hh * s).toNearestInt());
}

void PultecEditor::resized()
{
    place (lfBoost, 617, 207, 92, 92);  place (lfCut, 887, 207, 92, 92);
    place (hfBoost, 1231, 207, 92, 92); place (hfCut, 1494, 207, 92, 92);
    place (hfBw, 1068, 396, 72, 72);
    place (inGain, 261, 370, 53, 53);   place (outGain, 424, 370, 53, 53);
    place (sat, 1700, 377, 58, 58);
    place (lfFreq, 765, 405, 58, 58);   place (hfFreq, 1362, 405, 58, 58);
    place (hfCutFreq, 1702, 150, 58, 58); place (osSel, 1883, 150, 58, 58);
    place (eqIn, 538, 393, 45, 60);     place (autoGain, 1851, 377, 45, 60);
    place (lamp, 1947, 351, 60, 60);
    renderBackground();
}

void PultecEditor::renderBackground()
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0) return;
    const float s = (float) w / ui::kW;
    background = Image (Image::ARGB, w, h, true);
    {
        Graphics g (background);
        g.addTransform (AffineTransform::scale (s));

        // body
        g.setGradientFill (ColourGradient (Colour (0xff4b82b4), 0, 0, Colour (0xff2c5685), 0, ui::kH, false));
        g.fillRoundedRectangle (0, 0, ui::kW, ui::kH, 14.f);
        // rack ears
        for (float x0 : { 0.f, 1995.f })
        {
            g.setGradientFill (ColourGradient (Colour (0xff3a6fa6), x0, 0, Colour (0xff23487a), x0, ui::kH, false));
            g.fillRect (x0, 6.f, 127.f, ui::kH - 12.f);
        }
        g.setColour (Colour (0x66102844));  g.drawLine (125, 8, 125, ui::kH - 8, 2.f); g.drawLine (1995, 8, 1995, ui::kH - 8, 2.f);
        g.setColour (Colour (0x44c5dcf0));  g.drawLine (127, 8, 127, ui::kH - 8, 1.f); g.drawLine (1997, 8, 1997, ui::kH - 8, 1.f);
    }

    // brushed-metal grain
    {
        Random rnd (42);
        Image::BitmapData bd (background, Image::BitmapData::readWrite);
        std::vector<float> rowBias ((size_t) h);
        float rb = 0.f;
        for (auto& v : rowBias) { rb = rb * 0.85f + (rnd.nextFloat() - 0.5f) * 5.f; v = rb; }
        for (int y = 0; y < h; ++y)
        {
            float streak = 0.f;
            for (int x = 0; x < w; ++x)
            {
                streak = streak * 0.93f + (rnd.nextFloat() - 0.5f) * 5.5f;
                const float d = rowBias[(size_t) y] * 2.2f + streak + (rnd.nextFloat() - 0.5f) * 3.f;
                auto c = bd.getPixelColour (x, y);
                if (c.getAlpha() == 0) continue;
                auto f = [d] (uint8 v) { return (uint8) jlimit (0, 255, (int) std::lround ((float) v + d)); };
                bd.setPixelColour (x, y, Colour::fromRGBA (f (c.getRed()), f (c.getGreen()), f (c.getBlue()), c.getAlpha()));
            }
        }
    }

    Graphics g (background);
    g.addTransform (AffineTransform::scale (s));

    // bevel
    g.setColour (Colour (0xff1c3652)); g.drawRoundedRectangle (1.5f, 1.5f, ui::kW - 3.f, ui::kH - 3.f, 14.f, 3.f);
    g.setColour (Colour (0x77bcd8f0)); g.drawRoundedRectangle (5.f, 5.f, ui::kW - 10.f, ui::kH - 10.f, 12.f, 1.4f);

    for (float x : { 63.f, 2058.f }) for (float y : { 144.f, 372.f }) slot (g, x, y);
    for (float x : { 168.f, 1954.f }) for (float y : { 45.f, 465.f }) chromeScrew (g, x, y, 19.f);

    // nameplate
    label (g, "PULTEC PROTO", 327, 127, 34, true);
    label (g, "PROGRAM EQUALIZER", 327, 165, 22, true);
    label (g, "EQP-1A STYLE  -  v0.2", 327, 199, 18, false);
    label (g, String ("build ") + __DATE__ + " " + __TIME__, 327, 228, 14, false);

    // group headers
    label (g, "LOW FREQUENCY", 752, 23, 26, true);
    label (g, "HIGH FREQUENCY", 1366, 23, 26, true);
    g.setColour (Colour (0xccf1efe6));
    g.drawLine (558, 41, 947, 41, 1.8f); g.drawLine (1172, 41, 1553, 41, 1.8f);

    StringArray n11; for (int i = 0; i <= 10; ++i) n11.add (String (i));
    const Point<float> pLB (617, 207), pLC (887, 207), pHB (1231, 207), pHC (1494, 207);
    for (auto c : { pLB, pLC, pHB, pHC }) scale (g, c, 82.f, n11, 1.37f, 22.f);
    label (g, "BOOST", pLB.x, 64, 25, true); label (g, "ATTEN", pLC.x, 64, 25, true);
    label (g, "BOOST", pHB.x, 64, 25, true); label (g, "ATTEN", pHC.x, 64, 25, true);

    scale (g, { 1068, 396 }, 64.f, { "SHARP", "", "BROAD" }, 1.62f, 17.f);
    label (g, "BANDWIDTH", 1068, 485, 24, true);

    scale (g, { 261, 370 }, 47.f, { "-12", "0", "+12" }, 1.62f, 18.f);  label (g, "INPUT", 261, 457, 24, true);
    scale (g, { 424, 370 }, 47.f, { "-12", "0", "+12" }, 1.62f, 18.f);  label (g, "OUTPUT", 424, 457, 24, true);
    scale (g, { 1700, 377 }, 52.f, { "0", "3", "6" }, 1.60f, 18.f);     label (g, "SATURATION", 1700, 472, 24, true);

    leverLabels (g, { 765, 405 }, { "20", "30", "60", "100" }, 187.f, 10.f, 92.f, 20.f);
    label (g, "CPS", 763, 482, 24, true);
    leverLabels (g, { 1362, 405 }, { "3", "4", "5", "8", "10", "12", "16" }, 198.f, -18.f, 94.f, 20.f);
    label (g, "KCS", 1359, 482, 24, true);
    leverLabels (g, { 1702, 150 }, { "5", "10", "20" }, 165.f, 15.f, 76.f, 20.f);
    label (g, "ATTEN SEL", 1702, 240, 24, true);
    leverLabels (g, { 1883, 150 }, { "2x", "4x", "8x" }, 165.f, 15.f, 76.f, 20.f);
    label (g, "OVERSAMPLING", 1881, 240, 24, true);

    label (g, "IN", 584, 365, 20, false);  label (g, "OUT", 592, 422, 20, false);  label (g, "EQ", 537, 472, 24, true);
    label (g, "ON", 1894, 340, 20, false); label (g, "OFF", 1897, 401, 20, false); label (g, "AUTO GAIN", 1853, 450, 24, true);
    label (g, "DRIVE", 1946, 422, 24, true);
}

void PultecEditor::paint (Graphics& g)
{
    g.fillAll (Colour (0xff1d2630));
    g.drawImageAt (background, 0, 0);
}

void PultecEditor::timerCallback()
{
    lamp.setLevel (proc.driveLevel.load());
}

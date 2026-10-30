#include "PluginEditor.h"
#include "FoldMath.h"

using namespace juce;

namespace
{
    const Colour pinkLight  { 0xffffc2e0 };
    const Colour pinkBase   { 0xffffa6d1 };
    const Colour pinkHot    { 0xffff4f9a };
    const Colour plum       { 0xff3a0b27 };
    const Colour neonCyan   { 0xff29e6ff };
    const Colour neonYellow { 0xffffe94d };
    const Colour neonLime   { 0xffb6ff3c };
    const Colour neonPink   { 0xffff2e93 };

    constexpr int editorW = 760;
    constexpr int editorH = 540;
    const Rectangle<int> screenBounds { 40, 122, 680, 190 };
    const Rectangle<int> panelBounds  { 30, 318, 700, 198 };

    //==========================================================================
    // Organic, slightly irregular blob (used for leopard rosettes and spots)
    Path makeBlob (Point<float> c, float rx, float ry, float jitter, Random& rng, int n, float rot)
    {
        std::vector<Point<float>> pts;
        for (int i = 0; i < n; ++i)
        {
            const float a = MathConstants<float>::twoPi * (float) i / (float) n;
            const float j = 1.0f + (rng.nextFloat() * 2.0f - 1.0f) * jitter;
            pts.push_back ({ std::cos (a) * rx * j, std::sin (a) * ry * j });
        }

        const auto t = AffineTransform::rotation (rot).translated (c.x, c.y);
        for (auto& p : pts)
            t.transformPoint (p.x, p.y);

        auto mid = [&] (int i) { return (pts[(size_t) (i % n)] + pts[(size_t) ((i + 1) % n)]) * 0.5f; };

        Path path;
        path.startNewSubPath (mid (n - 1));
        for (int i = 0; i < n; ++i)
            path.quadraticTo (pts[(size_t) i], mid (i));
        path.closeSubPath();
        return path;
    }

    // One leopard rosette: pink-hot centre ringed by broken dark-plum arcs
    void drawRosette (Graphics& g, Point<float> c, Random& rng, float scale)
    {
        const float r      = (13.0f + rng.nextFloat() * 9.0f) * scale;
        const float rot    = rng.nextFloat() * MathConstants<float>::twoPi;
        const float squash = 0.72f + rng.nextFloat() * 0.28f;

        g.setColour (pinkHot.withAlpha (0.55f));
        g.fillPath (makeBlob (c, r * 0.82f, r * 0.82f * squash, 0.18f, rng, 9, rot));

        g.setColour (plum);
        const int   segs = 3 + rng.nextInt (3);
        const float step = MathConstants<float>::twoPi / (float) segs;
        const float a0   = rng.nextFloat() * MathConstants<float>::twoPi;

        for (int i = 0; i < segs; ++i)
        {
            const float gap  = step * (0.18f + rng.nextFloat() * 0.18f);
            const float from = a0 + (float) i * step;
            Path arc;
            arc.addCentredArc (c.x, c.y, r, r * squash, rot, from, from + step - gap, true);
            g.strokePath (arc, PathStrokeType ((3.0f + rng.nextFloat() * 2.5f) * scale,
                                               PathStrokeType::curved, PathStrokeType::rounded));
        }

        // a couple of stray little spots next to the rosette
        for (int i = 0; i < 2; ++i)
        {
            const float a = rng.nextFloat() * MathConstants<float>::twoPi;
            const auto sc = c + Point<float> (std::cos (a), std::sin (a)) * (r * (1.5f + rng.nextFloat() * 0.4f));
            g.fillPath (makeBlob (sc, 3.0f * scale, 2.2f * scale, 0.3f, rng, 7, a));
        }
    }

    Image makeLeopard (int w, int h)
    {
        Image img (Image::RGB, w, h, true);
        Graphics g (img);

        g.setGradientFill (ColourGradient (pinkLight, 0.0f, 0.0f, pinkBase.darker (0.08f), (float) w, (float) h, false));
        g.fillAll();

        Random rng (1337);
        std::vector<Point<float>> pts;
        const float minDist = 44.0f;

        for (int tries = 0; tries < 6000; ++tries)
        {
            const Point<float> p (rng.nextFloat() * (float) (w + 60) - 30.0f,
                                  rng.nextFloat() * (float) (h + 60) - 30.0f);
            bool ok = true;
            for (auto& q : pts)
                if (p.getDistanceFrom (q) < minDist) { ok = false; break; }
            if (ok) pts.push_back (p);
        }

        for (auto& p : pts)
            drawRosette (g, p, rng, 1.0f);

        return img;
    }

    //==========================================================================
    void drawSticker (Graphics& g, const Path& p, Colour fill)
    {
        g.setColour (Colours::white);
        g.strokePath (p, PathStrokeType (9.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Colours::black);
        g.strokePath (p, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (fill);
        g.fillPath (p);
    }

    Path heartPath (Rectangle<float> r)
    {
        Path h;
        h.startNewSubPath (0.5f, 0.95f);
        h.cubicTo (-0.1f, 0.55f, 0.05f, 0.0f, 0.5f, 0.3f);
        h.cubicTo (0.95f, 0.0f, 1.1f, 0.55f, 0.5f, 0.95f);
        h.closeSubPath();
        h.applyTransform (AffineTransform::scale (r.getWidth(), r.getHeight()).translated (r.getX(), r.getY()));
        return h;
    }

    Path boltPath (Rectangle<float> r, bool mirror)
    {
        Path b;
        b.startNewSubPath (0.55f, 0.0f);
        b.lineTo (0.10f, 0.55f);
        b.lineTo (0.45f, 0.55f);
        b.lineTo (0.30f, 1.00f);
        b.lineTo (0.90f, 0.40f);
        b.lineTo (0.55f, 0.40f);
        b.lineTo (0.85f, 0.0f);
        b.closeSubPath();
        if (mirror)
            b.applyTransform (AffineTransform::scale (-1.0f, 1.0f).translated (1.0f, 0.0f));
        b.applyTransform (AffineTransform::scale (r.getWidth(), r.getHeight()).translated (r.getX(), r.getY()));
        return b;
    }

    Path starPath (Point<float> c, float outer, int points = 5, float rot = 0.0f)
    {
        Path s;
        s.addStar (c, points, outer * (points == 4 ? 0.3f : 0.45f), outer, rot);
        return s;
    }

    Font boldFont (float size) { return Font (FontOptions (size, Font::bold)); }

    //==========================================================================
    Image makeBackground (int w, int h)
    {
        Image img = makeLeopard (w, h);
        Graphics g (img);

        // --- glitter sparkles --------------------------------------------------
        Random rng (99);
        for (int i = 0; i < 30; ++i)
        {
            const Point<float> p (rng.nextFloat() * (float) w, rng.nextFloat() * (float) h);
            if (screenBounds.expanded (6).contains (p.toInt()) || panelBounds.expanded (6).contains (p.toInt()))
                continue;
            const float s = 5.0f + rng.nextFloat() * 6.0f;
            g.setColour (neonCyan.withAlpha (0.35f));
            g.fillEllipse (p.x - s, p.y - s, s * 2.0f, s * 2.0f);
            g.setColour (Colours::white);
            g.fillPath (starPath (p, s, 4));
        }

        // --- knob panel ------------------------------------------------------
        auto panel = panelBounds.toFloat();
        g.setColour (Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (panel.translated (4.0f, 5.0f), 22.0f);
        g.setColour (Colour (0xff1a0a28).withAlpha (0.72f));
        g.fillRoundedRectangle (panel, 22.0f);
        g.setColour (Colours::white);
        g.drawRoundedRectangle (panel.reduced (1.5f), 21.0f, 3.0f);
        g.setColour (neonCyan);
        g.drawRoundedRectangle (panel.reduced (6.0f), 17.0f, 2.0f);

        // --- title ------------------------------------------------------------
        {
            GlyphArrangement ga;
            ga.addFittedText (boldFont (52.0f), "WAVE FOLDING BOSS", 92.0f, 10.0f, 576.0f, 70.0f,
                              Justification::centred, 1);
            Path text;
            ga.createPath (text);

            Path shadow (text);
            shadow.applyTransform (AffineTransform::translation (5.0f, 6.0f));
            g.setColour (neonCyan);
            g.fillPath (shadow);

            g.setColour (Colours::white);
            g.strokePath (text, PathStrokeType (10.0f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (Colours::black);
            g.strokePath (text, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));

            auto tb = text.getBounds();
            g.setGradientFill (ColourGradient (neonPink.brighter (0.25f), 0.0f, tb.getY(),
                                               neonPink.darker (0.1f), 0.0f, tb.getBottom(), false));
            g.fillPath (text);
        }

        // --- tagline pill ------------------------------------------------------
        {
            Rectangle<float> pill (215.0f, 90.0f, 330.0f, 24.0f);
            g.setColour (Colours::black);
            g.fillRoundedRectangle (pill, 12.0f);
            g.setColour (Colours::white);
            g.drawRoundedRectangle (pill.reduced (1.0f), 11.0f, 2.0f);
            g.setColour (neonLime);
            g.setFont (boldFont (12.5f));
            g.drawText (CharPointer_UTF8 ("\xe2\x98\x85 WAVEFOLDING ONLY \xe2\x98\x85 NOTHING ELSE \xe2\x98\x85"),
                        pill, Justification::centred, false);
        }

        // --- stickers around the title ------------------------------------------
        drawSticker (g, boltPath ({ 22.0f, 12.0f, 50.0f, 68.0f }, false), neonYellow);
        drawSticker (g, boltPath ({ 688.0f, 12.0f, 50.0f, 68.0f }, true), neonYellow);
        drawSticker (g, heartPath ({ 16.0f, 88.0f, 34.0f, 30.0f }), neonPink);
        drawSticker (g, heartPath ({ 710.0f, 88.0f, 34.0f, 30.0f }), neonPink);

        // --- checkerboard strip -------------------------------------------------
        const int sq = 18;
        g.setColour (neonCyan);
        g.fillRect (0, h - sq - 6, w, 3);
        for (int y = 0; y < 2; ++y)
            for (int x = 0; x * sq < w; ++x)
            {
                g.setColour (((x + y) % 2 == 0) ? Colours::black : Colours::white);
                g.fillRect (x * sq, h - sq + y * (sq / 2) - 0, sq, sq / 2);
            }

        return img;
    }
}

//==============================================================================
PinkLookAndFeel::PinkLookAndFeel()
{
    setColour (Slider::textBoxTextColourId, neonCyan);
    setColour (Slider::textBoxBackgroundColourId, Colours::black);
    setColour (Slider::textBoxOutlineColourId, Colours::white);
    setColour (TextEditor::backgroundColourId, Colours::black);
    setColour (TextEditor::textColourId, neonCyan);
    setColour (TextEditor::highlightColourId, neonPink);
}

void PinkLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h,
                                        float pos, float a0, float a1, Slider&)
{
    const auto area = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float d   = jmin (area.getWidth(), area.getHeight());
    const auto  sq  = Rectangle<float> (d, d).withCentre (area.getCentre()).reduced (3.0f);
    const auto  face = sq.reduced (d * 0.15f);
    const auto  c   = face.getCentre();
    const float fr  = face.getWidth() * 0.5f;
    const float trackR = sq.getWidth() * 0.5f - 3.0f;
    const float angle  = a0 + pos * (a1 - a0);

    // value ring
    Path track;
    track.addCentredArc (c.x, c.y, trackR, trackR, 0.0f, a0, a1, true);
    g.setColour (Colours::white);
    g.strokePath (track, PathStrokeType (13.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (Colours::black);
    g.strokePath (track, PathStrokeType (9.0f, PathStrokeType::curved, PathStrokeType::rounded));

    if (pos > 0.004f)
    {
        Path val;
        val.addCentredArc (c.x, c.y, trackR, trackR, 0.0f, a0, angle, true);
        g.setColour (neonCyan);
        g.strokePath (val, PathStrokeType (4.5f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // sticker face
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillEllipse (face.translated (3.0f, 4.0f));
    g.setColour (Colours::white);
    g.fillEllipse (face.expanded (3.5f));
    g.setColour (Colours::black);
    g.fillEllipse (face.expanded (1.5f));
    g.setGradientFill (ColourGradient (pinkLight, face.getX(), face.getY(),
                                       pinkHot, face.getRight(), face.getBottom(), false));
    g.fillEllipse (face);

    // mini leopard print on the knob face
    {
        Graphics::ScopedSaveState save (g);
        Path clip;
        clip.addEllipse (face);
        g.reduceClipRegion (clip);

        Random rng (5);
        const Point<float> spots[] = { { -0.55f, -0.45f }, { 0.5f, -0.5f }, { -0.5f, 0.55f },
                                       { 0.55f, 0.5f }, { 0.0f, 0.95f }, { 0.0f, -0.98f } };
        for (auto& s : spots)
            drawRosette (g, c + s * fr, rng, fr / 48.0f);
    }

    g.setColour (Colours::white.withAlpha (0.4f));
    g.fillEllipse (face.getX() + fr * 0.35f, face.getY() + fr * 0.15f, fr * 0.6f, fr * 0.3f);

    // pointer with a little star on the tip
    const Point<float> dir (std::sin (angle), -std::cos (angle));
    const auto tip = c + dir * (fr * 0.80f);
    g.setColour (Colours::black);
    g.drawLine (Line<float> (c + dir * (fr * 0.15f), tip), 5.0f);
    g.setColour (Colours::black);
    g.fillPath (starPath (tip, 10.0f, 5, angle));
    g.setColour (neonYellow);
    g.fillPath (starPath (tip, 7.0f, 5, angle));
}

void PinkLookAndFeel::drawLabel (Graphics& g, Label& l)
{
    auto b = l.getLocalBounds().toFloat().reduced (1.5f);
    g.setColour (Colours::black);
    g.fillRoundedRectangle (b, b.getHeight() * 0.5f);
    g.setColour (Colours::white);
    g.drawRoundedRectangle (b, b.getHeight() * 0.5f, 1.5f);

    if (! l.isBeingEdited())
    {
        g.setColour (neonCyan);
        g.setFont (boldFont (13.0f));
        g.drawFittedText (l.getText(), l.getLocalBounds(), Justification::centred, 1);
    }
}

//==============================================================================
FoldScreen::FoldScreen (AudioProcessorValueTreeState& apvts)
    : driveP    (apvts.getRawParameterValue (wfb::ids::drive)),
      stagesP   (apvts.getRawParameterValue (wfb::ids::stages)),
      shapeP    (apvts.getRawParameterValue (wfb::ids::shape)),
      symmetryP (apvts.getRawParameterValue (wfb::ids::symmetry))
{
    setOpaque (false);
    startTimerHz (30);
}

void FoldScreen::timerCallback()
{
    phase = std::fmod (phase + 0.07f, MathConstants<float>::twoPi);
    repaint();
}

void FoldScreen::paint (Graphics& g)
{
    // sticker frame
    auto b = getLocalBounds().toFloat().reduced (5.0f);
    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (b.translated (3.0f, 4.0f), 18.0f);
    g.setColour (Colours::white);
    g.fillRoundedRectangle (b, 18.0f);
    g.setColour (Colours::black);
    g.fillRoundedRectangle (b.reduced (3.0f), 15.0f);

    auto s = b.reduced (7.0f);
    g.setGradientFill (ColourGradient (Colour (0xff2e0a45), s.getX(), s.getY(),
                                       Colour (0xff12051f), s.getX(), s.getBottom(), false));
    g.fillRoundedRectangle (s, 11.0f);

    Graphics::ScopedSaveState save (g);
    Path clip;
    clip.addRoundedRectangle (s, 11.0f);
    g.reduceClipRegion (clip);

    const float drive = Decibels::decibelsToGain (driveP->load());
    const float bias  = symmetryP->load();
    const int stages  = jlimit (1, 4, (int) std::lround (stagesP->load()));
    const float shape = shapeP->load();

    auto rest = s;
    auto left = rest.removeFromLeft (s.getWidth() * 0.34f).reduced (14.0f);
    auto right = rest.reduced (14.0f);

    auto drawGrid = [&] (Rectangle<float> r)
    {
        g.setColour (Colours::white.withAlpha (0.08f));
        for (int i = 1; i < 4; ++i)
        {
            const float fx = r.getX() + r.getWidth() * (float) i / 4.0f;
            const float fy = r.getY() + r.getHeight() * (float) i / 4.0f;
            g.drawVerticalLine ((int) fx, r.getY(), r.getBottom());
            g.drawHorizontalLine ((int) fy, r.getX(), r.getRight());
        }
        g.setColour (neonPink.withAlpha (0.55f));
        g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight());
        g.drawVerticalLine ((int) r.getCentreX(), r.getY(), r.getBottom());
        g.setColour (neonPink.withAlpha (0.35f));
        g.drawRect (r, 1.0f);
    };
    drawGrid (left);
    drawGrid (right);

    // transfer curve
    {
        Path tp;
        const int n = jmax (2, (int) left.getWidth() * 2);
        for (int i = 0; i < n; ++i)
        {
            const float xn = -1.0f + 2.0f * (float) i / (float) (n - 1);
            const float yv = jlimit (-1.0f, 1.0f, wfb::foldChain (xn, drive, bias, stages, shape));
            const float px = left.getX() + (xn * 0.5f + 0.5f) * left.getWidth();
            const float py = left.getBottom() - (yv * 0.5f + 0.5f) * left.getHeight();
            if (i == 0) tp.startNewSubPath (px, py); else tp.lineTo (px, py);
        }
        g.setColour (neonCyan.withAlpha (0.25f));
        g.strokePath (tp, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (neonCyan);
        g.strokePath (tp, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // scope: sine in (pink), folded sine out (lime)
    {
        Path in, out;
        const int n = jmax (2, (int) right.getWidth());
        const float amp = right.getHeight() * 0.5f;
        for (int i = 0; i < n; ++i)
        {
            const float t = (float) i / (float) (n - 1);
            const float x = std::sin (phase + MathConstants<float>::twoPi * 2.0f * t);
            const float y = jlimit (-1.0f, 1.0f, wfb::foldChain (x, drive, bias, stages, shape));
            const float px = right.getX() + t * right.getWidth();
            const float pyIn  = right.getCentreY() - x * amp * 0.9f;
            const float pyOut = right.getCentreY() - y * amp * 0.9f;
            if (i == 0) { in.startNewSubPath (px, pyIn); out.startNewSubPath (px, pyOut); }
            else        { in.lineTo (px, pyIn);          out.lineTo (px, pyOut); }
        }
        g.setColour (pinkHot.withAlpha (0.7f));
        g.strokePath (in, PathStrokeType (1.5f));
        g.setColour (neonLime.withAlpha (0.25f));
        g.strokePath (out, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (neonLime);
        g.strokePath (out, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // tiny labels
    g.setFont (boldFont (11.0f));
    g.setColour (neonPink);
    g.drawText ("TRANSFER", left.withHeight (14.0f).translated (4.0f, 2.0f), Justification::topLeft, false);
    g.drawText ("FOLDED OUT", right.withHeight (14.0f).translated (4.0f, 2.0f), Justification::topLeft, false);

    // scanlines
    g.setColour (Colours::black.withAlpha (0.16f));
    for (float y = s.getY(); y < s.getBottom(); y += 3.0f)
        g.fillRect (s.getX(), y, s.getWidth(), 1.0f);
}

//==============================================================================
WaveFoldingBossEditor::WaveFoldingBossEditor (WaveFoldingBossProcessor& p)
    : AudioProcessorEditor (&p), proc (p), screen (p.apvts)
{
    setLookAndFeel (&laf);
    addAndMakeVisible (screen);

    struct Spec
    {
        const char* id;
        std::function<String (double)> toText;
        std::function<double (const String&)> fromText;
    };

    auto dbText   = [] (double v) { return String (v, 1) + " dB"; };
    auto numFrom  = [] (const String& t) { return t.getDoubleValue(); };
    auto pctFrom  = [] (const String& t) { return t.getDoubleValue() / 100.0; };
    auto pctText  = [] (double v) { return String (roundToInt (v * 100.0)) + "%"; };
    auto signedPct = [] (double v) { const int p = roundToInt (v * 100.0); return String (p > 0 ? "+" : "") + String (p) + "%"; };
    auto intText  = [] (double v) { return String ((int) std::lround (v)); };

    const Spec specs[numKnobs] = {
        { wfb::ids::drive,    dbText,    numFrom },
        { wfb::ids::stages,   intText,   numFrom },
        { wfb::ids::shape,    pctText,   pctFrom },
        { wfb::ids::symmetry, signedPct, pctFrom },
        { wfb::ids::mix,      pctText,   pctFrom },
        { wfb::ids::output,   dbText,    numFrom },
    };

    for (int i = 0; i < numKnobs; ++i)
    {
        auto& s = sliders[(size_t) i];
        s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        s.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
        s.setTextBoxStyle (Slider::TextBoxBelow, false, 84, 22);
        s.textFromValueFunction = specs[i].toText;
        s.valueFromTextFunction = specs[i].fromText;
        s.setPopupDisplayEnabled (false, false, nullptr);
        addAndMakeVisible (s);

        auto* param = p.apvts.getParameter (specs[i].id);
        attachments.push_back (std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.apvts, specs[i].id, s));
        s.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }

    setSize (editorW, editorH);
    setResizable (false, false);
    background = makeBackground (editorW, editorH);
}

WaveFoldingBossEditor::~WaveFoldingBossEditor()
{
    setLookAndFeel (nullptr);
}

void WaveFoldingBossEditor::resized()
{
    screen.setBounds (screenBounds);

    const int widths[numKnobs] = { 160, 104, 104, 104, 104, 104 };
    int x = 40;
    for (int i = 0; i < numKnobs; ++i)
    {
        sliders[(size_t) i].setBounds (x, 346, widths[i], 166);
        x += widths[i];
    }
}

void WaveFoldingBossEditor::paint (Graphics& g)
{
    g.drawImageAt (background, 0, 0);

    static const char* names[numKnobs] = { "FOLD", "STAGES", "SHAPE", "SYMMETRY", "MIX", "OUTPUT" };
    g.setFont (boldFont (15.0f));
    for (int i = 0; i < numKnobs; ++i)
    {
        auto r = sliders[(size_t) i].getBounds().withY (326).withHeight (20);
        g.setColour (neonPink);
        g.drawText (names[i], r.translated (1, 2), Justification::centred, false);
        g.setColour (Colours::white);
        g.drawText (names[i], r, Justification::centred, false);
    }
}

void WaveFoldingBossEditor::paintOverChildren (Graphics& g)
{
    // stickers slapped on the corners of the screen
    drawSticker (g, starPath ({ 52.0f, 128.0f }, 21.0f, 5, 0.2f), neonYellow);
    drawSticker (g, heartPath ({ 690.0f, 284.0f, 36.0f, 32.0f }), neonPink);
}

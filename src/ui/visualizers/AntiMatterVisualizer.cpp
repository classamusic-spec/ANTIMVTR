#include "AntiMatterVisualizer.h"
#include "core/Random.h"

namespace am::ui
{

AntiMatterVisualizer::AntiMatterVisualizer (Diagnostics& d) : diag (d)
{
    setInterceptsMouseClicks (false, false);
    setOpaque (false);

    Rng rng (0xA11CE);
    for (int i = 0; i < 44; ++i)
    {
        Fragment f;
        f.angle  = rng.nextFloat() * juce::MathConstants<float>::twoPi;
        f.radius = 0.55f + 0.5f * rng.nextFloat();
        f.size   = 0.008f + 0.03f * rng.nextFloat() * rng.nextFloat();
        f.speed  = (0.02f + 0.12f * rng.nextFloat()) * (rng.chance (0.5f) ? 1.0f : -1.0f);
        f.spin   = rng.nextBipolar() * 1.5f;
        f.phase  = rng.nextFloat() * juce::MathConstants<float>::twoPi;
        f.sides  = 3 + rng.nextInt (4);
        fragments.push_back (f);
    }
    for (int i = 0; i < 140; ++i)
        stars.push_back ({ rng.nextFloat(), rng.nextFloat(), 0.4f + 1.4f * rng.nextFloat() * rng.nextFloat(), rng.nextFloat() * 6.28f });

    startTimerHz (fps);
}

AntiMatterVisualizer::~AntiMatterVisualizer() { stopTimer(); }

void AntiMatterVisualizer::setTargetFrameRate (int newFps)
{
    fps = juce::jlimit (10, 60, newFps);
    if (isTimerRunning()) startTimerHz (fps);
}

void AntiMatterVisualizer::visibilityChanged()
{
    if (isShowing()) startTimerHz (fps); else stopTimer();
}

void AntiMatterVisualizer::resized() {}

void AntiMatterVisualizer::parentHierarchyChanged()
{
    if (isShowing() && ! isTimerRunning()) startTimerHz (fps);
}

void AntiMatterVisualizer::timerCallback()
{
    if (! isShowing()) { stopTimer(); return; }   // restarted by parentHierarchyChanged / visibilityChanged
    const float dt = 1.0f / (float) fps;
    time += dt;
    latest = diag.visualSnapshots.latest();
    integrate (dt);
    repaint();
}

void AntiMatterVisualizer::integrate (float dt)
{
    // Mass = inertia: heavier matter eases more slowly toward new parameter values.
    const float ease = juce::jlimit (0.02f, 0.5f, dt * (6.0f - 4.5f * latest.mass));
    auto lerpTo = [ease] (float& v, float target) { v += (target - v) * ease; };

    lerpTo (smooth.density, latest.density); lerpTo (smooth.form, latest.form); lerpTo (smooth.mass, latest.mass);
    lerpTo (smooth.tension, latest.tension); lerpTo (smooth.decay, latest.decay); lerpTo (smooth.surface, latest.surface);
    lerpTo (smooth.bend, latest.bend); lerpTo (smooth.melt, latest.melt); lerpTo (smooth.tear, latest.tear);
    lerpTo (smooth.magnet, latest.magnet); lerpTo (smooth.gravity, latest.gravity); lerpTo (smooth.scatter, latest.scatter);
    lerpTo (smooth.crush, latest.crush); lerpTo (smooth.fractureActivity, latest.fractureOn ? latest.fractureActivity : 0.0f);
    lerpTo (smooth.spaceActivity, latest.spaceActivity);
    smooth.spaceType = latest.spaceType;
    smooth.activeVoices = latest.activeVoices;
    smooth.activeNodes = latest.activeNodes;
    smooth.pitchHz = latest.pitchHz > 0.0f ? latest.pitchHz : smooth.pitchHz;
    smooth.numVisualNodes = latest.numVisualNodes;
    for (int i = 0; i < latest.numVisualNodes; ++i)
    {
        smooth.nodeFrequency[i] = latest.nodeFrequency[i];
        smooth.nodeEnergy[i] += (latest.nodeEnergy[i] - smooth.nodeEnergy[i]) * 0.3f;
        smooth.nodePan[i] = latest.nodePan[i];
        smooth.nodeCluster[i] = latest.nodeCluster[i];
    }

    // Audio pulse: fast attack, release governed by Decay.
    const float level = juce::jlimit (0.0f, 1.0f, latest.rmsL * 4.0f);
    if (level > pulse) pulse += (level - pulse) * 0.6f;
    else               pulse += (level - pulse) * juce::jlimit (0.02f, 0.3f, 0.25f - 0.2f * smooth.decay);
    energyEnvelope += (latest.noteEnergy - energyEnvelope) * 0.15f;
}

juce::Path AntiMatterVisualizer::blobPath (juce::Point<float> centre, float radius, float stretchX, float stretchY, float rotation,
                                          const float* harmonics, int numHarmonics, float phaseOffset) const
{
    juce::Path p;
    const int segments = 96;
    for (int i = 0; i <= segments; ++i)
    {
        const float a = (float) i / (float) segments * juce::MathConstants<float>::twoPi;
        float r = 1.0f;
        for (int h = 0; h < numHarmonics; ++h)
            r += harmonics[h] * std::sin ((float) (h + 2) * a + phaseOffset * (float) (h + 1) + time * (0.3f + 0.17f * (float) h));
        const float x = std::cos (a) * r * radius * stretchX;
        const float y = std::sin (a) * r * radius * stretchY;
        const float rx = x * std::cos (rotation) - y * std::sin (rotation);
        const float ry = x * std::sin (rotation) + y * std::cos (rotation);
        if (i == 0) p.startNewSubPath (centre.x + rx, centre.y + ry); else p.lineTo (centre.x + rx, centre.y + ry);
    }
    p.closeSubPath();
    return p;
}

void AntiMatterVisualizer::drawBackground (juce::Graphics& g, juce::Rectangle<float> area)
{
    // Environmental halo tinted by the Space type.
    static const juce::Colour spaceTints[] = { Theme::violet, juce::Colour (0xff1a1a2a), Theme::amber, Theme::blue, Theme::magenta, Theme::cyan, Theme::ivory, Theme::textSecondary };
    const auto tint = spaceTints[juce::jlimit (0, 7, smooth.spaceType)];
    const float haloAlpha = 0.05f + 0.10f * smooth.spaceActivity + 0.05f * pulse;
    juce::ColourGradient halo (tint.withAlpha (haloAlpha), area.getCentreX(), area.getCentreY(),
                               juce::Colours::transparentBlack, area.getCentreX(), area.getY(), true);
    g.setGradientFill (halo);
    g.fillEllipse (area.expanded (area.getWidth() * 0.15f));

    // Starfield
    for (const auto& s : stars)
    {
        const float tw = 0.5f + 0.5f * std::sin (time * 0.8f + s.twinkle);
        g.setColour (Theme::textPrimary.withAlpha (0.08f + 0.25f * tw * s.size / 1.8f));
        const float x = area.getX() + s.x * area.getWidth();
        const float y = area.getY() + s.y * area.getHeight();
        g.fillEllipse (x, y, s.size, s.size);
    }

    // Thin orbit ring + crosshair (scientific framing)
    const auto ring = area.reduced (area.getWidth() * 0.06f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawEllipse (ring, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.035f));
    g.drawLine (area.getCentreX(), area.getY(), area.getCentreX(), area.getY() + area.getHeight() * 0.08f, 1.0f);
    g.drawLine (area.getCentreX(), area.getBottom() - area.getHeight() * 0.08f, area.getCentreX(), area.getBottom(), 1.0f);
    g.drawLine (area.getX(), area.getCentreY(), area.getX() + area.getWidth() * 0.08f, area.getCentreY(), 1.0f);
    g.drawLine (area.getRight() - area.getWidth() * 0.08f, area.getCentreY(), area.getRight(), area.getCentreY(), 1.0f);
}

void AntiMatterVisualizer::drawObject (juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto c = area.getCentre();
    const float R = area.getWidth() * 0.30f * (1.0f + 0.08f * pulse + 0.06f * smooth.mass);

    // Structure: Form organises the harmonics (low form = smooth/harmonic, high = crystalline/complex).
    const float chaos = 0.35f * smooth.surface + 0.5f * smooth.melt + 0.3f * smooth.scatter;
    float harmonics[6];
    for (int h = 0; h < 6; ++h)
    {
        const float order = (float) h;
        const float organised = (h == 0 || h == 2) ? 0.10f : 0.02f;             // low orders → ordered look
        const float crystalline = 0.03f + 0.09f * std::abs (std::sin (order * 2.1f + smooth.form * 6.0f));
        harmonics[h] = lerp (organised, crystalline, smooth.form) + chaos * 0.06f * std::sin (time * (0.5f + order * 0.2f) + order);
        harmonics[h] *= (0.6f + 0.8f * smooth.density);
    }
    // Bend deforms one side; Magnet aligns (reduces odd harmonics).
    harmonics[1] += 0.18f * smooth.bend * std::sin (time * 0.7f);
    harmonics[3] *= 1.0f - 0.7f * smooth.magnet;
    harmonics[5] *= 1.0f - 0.7f * smooth.magnet;

    const float stretchX = 1.0f + 0.35f * (smooth.tension - 0.5f) * 2.0f * 0.5f;
    const float stretchY = 1.0f / juce::jmax (0.6f, stretchX);
    const float rotation = time * 0.05f * (0.5f + smooth.decay);

    // Tear separates layers into two lobes.
    const float tearOffset = smooth.tear * R * 0.55f;

    struct Layer { juce::Colour colour; float scale; float alpha; float phase; float offsetX; };
    const Layer layers[] = {
        { Theme::violet,  1.18f, 0.16f, 0.0f, -tearOffset * 0.6f },
        { Theme::blue,    1.05f, 0.22f, 1.3f,  tearOffset * 0.5f },
        { Theme::magenta, 0.92f, 0.20f, 2.4f, -tearOffset * 0.3f },
        { Theme::cyan,    0.80f, 0.26f, 3.7f,  tearOffset * 0.7f },
        { Theme::ivory,   0.62f, 0.14f, 5.1f,  tearOffset * 0.1f },
    };

    const float melt = smooth.melt;
    for (const auto& L : layers)
    {
        const juce::Point<float> centre (c.x + L.offsetX, c.y + std::sin (time * 0.9f + L.phase) * R * 0.03f);
        auto path = blobPath (centre, R * L.scale, stretchX, stretchY, rotation + L.phase * 0.1f, harmonics, 6, L.phase);

        // Melt: diffuse fills, weaker edges.
        juce::ColourGradient fill (L.colour.withAlpha (L.alpha * (0.7f + 0.3f * pulse)), centre.x, centre.y,
                                   L.colour.withAlpha (0.0f), centre.x + R * L.scale * 1.05f, centre.y, true);
        g.setGradientFill (fill);
        g.fillPath (path);
        draw::glowPath (g, path, L.colour.withAlpha (juce::jlimit (0.0f, 1.0f, 0.5f - 0.4f * melt)), 1.0f,
                        6.0f + 12.0f * melt, 0.35f + 0.4f * pulse);
    }

    // Filaments: energy threads from the core outward (density → count).
    {
        const int threads = 4 + (int) (smooth.density * 10.0f);
        Rng rng (0xF11A);
        for (int i = 0; i < threads; ++i)
        {
            const float a0 = rng.nextFloat() * juce::MathConstants<float>::twoPi + time * 0.08f * (rng.nextBipolar());
            const float a1 = a0 + rng.nextBipolar() * (0.6f + 1.2f * smooth.surface);
            const float r0 = R * 0.35f, r1 = R * (0.9f + 0.4f * rng.nextFloat());
            juce::Path f;
            f.startNewSubPath (c.x + std::cos (a0) * r0, c.y + std::sin (a0) * r0);
            const float bulge = R * (0.4f + 0.6f * smooth.bend) * rng.nextBipolar();
            f.quadraticTo (c.x + std::cos ((a0 + a1) * 0.5f) * (r0 + r1) * 0.5f + bulge, c.y + std::sin ((a0 + a1) * 0.5f) * (r0 + r1) * 0.5f,
                           c.x + std::cos (a1) * r1, c.y + std::sin (a1) * r1);
            const auto col = (i % 3 == 0) ? Theme::cyan : (i % 3 == 1 ? Theme::magenta : Theme::ivory);
            draw::glowPath (g, f, col.withAlpha (0.35f + 0.4f * energyEnvelope), 0.8f, 4.0f, 0.3f + 0.5f * pulse);
        }
    }

    // The void core — dark matter with a faint rim light.
    {
        const float coreR = R * (0.42f + 0.1f * smooth.mass) * (1.0f - 0.35f * melt);
        auto core = juce::Rectangle<float> (c.x - coreR + tearOffset * 0.15f, c.y - coreR, coreR * 2.0f, coreR * 2.0f);
        juce::ColourGradient dark (juce::Colour (0xff030306), c.x, c.y, juce::Colour (0xff030306).withAlpha (0.0f), c.x + coreR * 1.25f, c.y, true);
        g.setGradientFill (dark);
        g.fillEllipse (core.expanded (coreR * 0.25f));
        g.setColour (Theme::ivory.withAlpha (0.10f + 0.15f * pulse));
        g.drawEllipse (core, 1.0f);
    }
}

void AntiMatterVisualizer::drawFragments (juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto c = area.getCentre();
    const float R = area.getWidth() * 0.30f;
    const int count = juce::jlimit (6, (int) fragments.size(), 8 + (int) (smooth.density * 22.0f) + (int) (smooth.fractureActivity * 14.0f));
    const float speedScale = 0.3f + 1.7f * (1.0f - smooth.decay) + 2.0f * smooth.fractureActivity;

    for (int i = 0; i < count; ++i)
    {
        const auto& f = fragments[(size_t) i];
        const float a = f.angle + time * f.speed * speedScale + smooth.scatter * std::sin (time * 3.0f + f.phase) * 0.6f;
        const float wobble = 1.0f + 0.05f * std::sin (time * 1.3f + f.phase);
        const float r = R * (f.radius + 0.35f * smooth.tear + 0.25f * smooth.fractureActivity) * wobble;
        const juce::Point<float> p (c.x + std::cos (a) * r * (1.0f + 0.2f * (smooth.tension - 0.5f)), c.y + std::sin (a) * r);
        const float s = area.getWidth() * f.size * (1.0f + smooth.fractureActivity);

        juce::Path shard;
        for (int k = 0; k < f.sides; ++k)
        {
            const float ka = (float) k / (float) f.sides * juce::MathConstants<float>::twoPi + time * f.spin + f.phase;
            const float kr = s * (0.7f + 0.3f * std::sin (ka * 3.0f + f.phase));
            juce::Point<float> v (p.x + std::cos (ka) * kr, p.y + std::sin (ka) * kr);
            if (k == 0) shard.startNewSubPath (v); else shard.lineTo (v);
        }
        shard.closeSubPath();
        const auto col = (i % 4 == 0) ? Theme::cyan : (i % 4 == 1 ? Theme::ivory : (i % 4 == 2 ? Theme::violet : Theme::blue));
        g.setColour (col.withAlpha (0.18f));
        g.fillPath (shard);
        g.setColour (col.withAlpha (0.55f));
        g.strokePath (shard, juce::PathStrokeType (0.8f));
    }
}

void AntiMatterVisualizer::drawNodes (juce::Graphics& g, juce::Rectangle<float> area)
{
    if (smooth.numVisualNodes <= 0) return;
    const auto c = area.getCentre();
    const float R = area.getWidth() * 0.30f;
    for (int i = 0; i < smooth.numVisualNodes; ++i)
    {
        const float f = smooth.nodeFrequency[i];
        if (f <= 0.0f) continue;
        const float u = juce::jlimit (0.0f, 1.0f, (std::log2 (f / 30.0f)) / 9.0f);   // 30 Hz .. ~15 kHz
        const float a = -juce::MathConstants<float>::halfPi + u * juce::MathConstants<float>::twoPi;
        const float r = R * (1.35f + 0.12f * smooth.nodePan[i]);
        const float e = juce::jlimit (0.0f, 1.0f, smooth.nodeEnergy[i] * 6.0f);
        const float s = 1.5f + 3.5f * e;
        const auto col = (smooth.nodeCluster[i] % 2 == 0) ? Theme::cyan : Theme::magenta;
        const juce::Point<float> p (c.x + std::cos (a) * r, c.y + std::sin (a) * r);
        draw::glowEllipse (g, juce::Rectangle<float> (p.x - s, p.y - s, s * 2, s * 2), col, s * 2.5f, e);
        g.setColour (col.withAlpha (0.3f + 0.7f * e));
        g.fillEllipse (p.x - s * 0.5f, p.y - s * 0.5f, s, s);
    }
}

void AntiMatterVisualizer::drawCaptions (juce::Graphics& g, juce::Rectangle<float> area)
{
    if (area.getHeight() < 200.0f) return;
    const float h = juce::jlimit (7.5f, 10.0f, area.getHeight() * 0.02f);
    auto tl = area.removeFromTop (30.0f);
    draw::trackedText (g, "INHALE", tl.withWidth (90.0f).withTrimmedLeft (8.0f), juce::Justification::topLeft, Theme::captionFont (h), Theme::textPrimary.withAlpha (0.8f));
    draw::trackedText (g, "IDEA", tl.withWidth (90.0f).withTrimmedLeft (8.0f).withTrimmedTop (h * 1.5f), juce::Justification::topLeft, Theme::captionFont (h - 1.0f), Theme::textDim);
    draw::trackedText (g, "EXHALE", tl.withLeft (tl.getRight() - 90.0f).withTrimmedRight (8.0f), juce::Justification::topRight, Theme::captionFont (h), Theme::textPrimary.withAlpha (0.8f));
    draw::trackedText (g, "EVOLVE", tl.withLeft (tl.getRight() - 90.0f).withTrimmedRight (8.0f).withTrimmedTop (h * 1.5f), juce::Justification::topRight, Theme::captionFont (h - 1.0f), Theme::textDim);

    auto bottom = area.removeFromBottom (44.0f);
    const float titleH = juce::jlimit (12.0f, 22.0f, area.getWidth() * 0.04f);
    draw::trackedText (g, "ANTI-MATR", bottom.removeFromTop (titleH * 1.3f), juce::Justification::centred, Theme::titleFont (titleH), Theme::textPrimary.withAlpha (0.9f));
    draw::trackedText (g, "SOUND BEYOND MATTER", bottom, juce::Justification::centredTop, Theme::captionFont (h), Theme::textSecondary);
}

void AntiMatterVisualizer::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto square = bounds.withSizeKeepingCentre (size, size);

    drawBackground (g, square);
    drawNodes (g, square);
    drawObject (g, square);
    drawFragments (g, square);
    drawCaptions (g, bounds);
}

} // namespace am::ui

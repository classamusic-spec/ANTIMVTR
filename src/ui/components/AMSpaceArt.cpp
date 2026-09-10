#include "AMSpaceArt.h"

namespace am::ui
{

namespace
{
    constexpr float kTwoPi = juce::MathConstants<float>::twoPi;

    void fourPointStar (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour col)
    {
        juce::Path p;
        p.startNewSubPath (c.x, c.y - r);
        p.quadraticTo (c.x, c.y, c.x + r, c.y);
        p.quadraticTo (c.x, c.y, c.x, c.y + r);
        p.quadraticTo (c.x, c.y, c.x - r, c.y);
        p.quadraticTo (c.x, c.y, c.x, c.y - r);
        p.closeSubPath();
        g.setColour (col);
        g.fillPath (p);
    }
}

void SpaceArt::stars (juce::Graphics& g, juce::Rectangle<float> a, int seed, int count, float phase, float alpha)
{
    juce::Random rng (seed);
    for (int i = 0; i < count; ++i)
    {
        const float x = a.getX() + rng.nextFloat() * a.getWidth();
        const float y = a.getY() + rng.nextFloat() * a.getHeight();
        const float s = 0.6f + 1.4f * rng.nextFloat() * rng.nextFloat();
        const float tw = 0.5f + 0.5f * std::sin (phase * (0.6f + rng.nextFloat()) + (float) i);
        g.setColour (Theme::textPrimary.withAlpha (alpha * (0.25f + 0.75f * tw)));
        g.fillEllipse (x, y, s, s);
    }
}

void SpaceArt::draw (juce::Graphics& g, juce::Rectangle<float> area, int type, float phase, float activity, float corner)
{
    juce::Graphics::ScopedSaveState save (g);
    juce::Path clip; clip.addRoundedRectangle (area, corner);
    g.reduceClipRegion (clip);

    juce::ColourGradient bg (juce::Colour (0xff07070d), area.getX(), area.getY(), juce::Colour (0xff030305), area.getX(), area.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRect (area);

    switch (juce::jlimit (0, kNumTypes - 1, type))
    {
        case 0: nebula (g, area, phase, activity); break;
        case 1: voidHole (g, area, phase, activity); break;
        case 2: chamber (g, area, phase, activity); break;
        case 3: orbit (g, area, phase, activity); break;
        case 4: dream (g, area, phase, activity); break;
        case 5: machine (g, area, phase, activity); break;
        case 6: shimmer (g, area, phase, activity); break;
        default: dust (g, area, phase, activity); break;
    }

    // vignette + glass edge
    juce::ColourGradient vignette (juce::Colours::transparentBlack, area.getCentreX(), area.getCentreY(),
                                   juce::Colours::black.withAlpha (0.5f), area.getX(), area.getY(), true);
    g.setGradientFill (vignette);
    g.fillRect (area);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (area.reduced (0.5f), corner, 1.0f);
}

void SpaceArt::nebula (juce::Graphics& g, juce::Rectangle<float> a, float phase, float activity)
{
    const auto c = a.getCentre();
    const float R = juce::jmin (a.getWidth(), a.getHeight()) * 0.5f;
    stars (g, a, 11, 48, phase, 0.5f);

    // soft gas clouds
    draw::softLight (g, { c.x - R * 0.35f, c.y + R * 0.15f }, R * 1.1f, Theme::violet, 0.22f + 0.15f * activity);
    draw::softLight (g, { c.x + R * 0.45f, c.y - R * 0.25f }, R * 0.9f, Theme::magenta, 0.16f + 0.1f * activity);
    draw::softLight (g, { c.x + R * 0.1f, c.y + R * 0.4f }, R * 0.8f, Theme::blue, 0.14f);
    draw::softLight (g, c, R * 0.55f, Theme::ivory, 0.18f + 0.2f * activity);

    // spiral arms of glowing dust
    juce::Random rng (3);
    for (int arm = 0; arm < 2; ++arm)
    {
        for (int i = 0; i < 70; ++i)
        {
            const float u = (float) i / 70.0f;
            const float ang = u * 4.6f + (float) arm * juce::MathConstants<float>::pi + phase * 0.12f;
            const float rad = R * (0.06f + 0.92f * u);
            const float jitter = (rng.nextFloat() * 2.0f - 1.0f) * R * 0.05f * u;
            const juce::Point<float> p (c.x + std::cos (ang) * rad + jitter, c.y + std::sin (ang) * rad * 0.62f + jitter);
            const float s = (0.8f + 2.2f * rng.nextFloat()) * (1.0f - 0.5f * u);
            const auto col = (i % 3 == 0) ? Theme::magenta : (i % 3 == 1 ? Theme::violet : Theme::cyan);
            g.setColour (col.withAlpha (0.25f + 0.6f * (1.0f - u)));
            g.fillEllipse (p.x - s * 0.5f, p.y - s * 0.5f, s, s);
        }
    }
    draw::glowDot (g, c, R * 0.06f, Theme::ivory, 0.7f + 0.3f * activity);
}

void SpaceArt::voidHole (juce::Graphics& g, juce::Rectangle<float> a, float phase, float activity)
{
    const auto c = a.getCentre();
    const float R = juce::jmin (a.getWidth(), a.getHeight()) * 0.34f;
    stars (g, a, 21, 40, phase, 0.4f);
    // accretion disc (tilted ellipse) behind the sphere
    {
        juce::Graphics::ScopedSaveState s (g);
        g.addTransform (juce::AffineTransform::rotation (-0.35f, c.x, c.y));
        for (int i = 0; i < 3; ++i)
        {
            const float rx = R * (1.35f + 0.25f * (float) i), ry = rx * 0.22f;
            g.setColour (Theme::ivory.withAlpha ((0.22f - 0.06f * (float) i) * (0.7f + 0.3f * activity)));
            g.drawEllipse (c.x - rx, c.y - ry, rx * 2.0f, ry * 2.0f, 1.0f);
        }
    }
    draw::softLight (g, c, R * 1.6f, juce::Colour (0xff4a4a6a), 0.25f);
    // the sphere
    g.setColour (juce::Colour (0xff020204));
    g.fillEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    juce::Path rim;
    rim.addCentredArc (c.x, c.y, R, R, 0.0f, -2.6f + 0.2f * std::sin (phase * 0.5f), 0.4f, true);
    draw::glowPath (g, rim, Theme::ivory, 1.2f, 8.0f, 0.5f + 0.5f * activity);
}

void SpaceArt::chamber (juce::Graphics& g, juce::Rectangle<float> a, float phase, float activity)
{
    const auto c = a.getCentre();
    // warm light from the back wall
    draw::softLight (g, { c.x, c.y - a.getHeight() * 0.1f }, a.getWidth() * 0.55f, Theme::amber, 0.18f + 0.15f * activity);
    g.setColour (Theme::amber.withAlpha (0.35f));
    // perspective floor / ceiling lines converging to the centre
    for (int i = -4; i <= 4; ++i)
    {
        const float x = c.x + (float) i * a.getWidth() * 0.16f;
        g.drawLine (x, a.getBottom(), c.x + (float) i * a.getWidth() * 0.03f, c.y, 1.0f);
        g.drawLine (x, a.getY(), c.x + (float) i * a.getWidth() * 0.03f, c.y, 1.0f);
    }
    for (int i = 1; i <= 5; ++i)
    {
        const float t = (float) i / 6.0f;
        const float hw = a.getWidth() * 0.5f * (1.0f - t * 0.8f), hh = a.getHeight() * 0.5f * (1.0f - t * 0.8f);
        g.setColour (Theme::amber.withAlpha (0.12f + 0.2f * t));
        g.drawRect (juce::Rectangle<float> (c.x - hw, c.y - hh, hw * 2.0f, hh * 2.0f), 1.0f);
    }
    const float pulse = 0.5f + 0.5f * std::sin (phase * 1.5f);
    draw::glowDot (g, c, 2.0f, Theme::amber, 0.4f + 0.6f * pulse * activity);
}

void SpaceArt::orbit (juce::Graphics& g, juce::Rectangle<float> a, float phase, float activity)
{
    const auto c = a.getCentre();
    const float R = juce::jmin (a.getWidth(), a.getHeight()) * 0.22f;
    stars (g, a, 31, 40, phase, 0.45f);
    draw::softLight (g, c, R * 2.2f, Theme::blue, 0.15f + 0.15f * activity);
    // planet
    juce::ColourGradient body (Theme::blue.brighter (0.2f), c.x - R * 0.4f, c.y - R * 0.4f, juce::Colour (0xff0a1230), c.x + R * 0.7f, c.y + R * 0.7f, true);
    g.setGradientFill (body);
    g.fillEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    // rings and moons
    for (int i = 0; i < 3; ++i)
    {
        const float rx = R * (1.6f + 0.55f * (float) i), ry = rx * (0.28f + 0.05f * (float) i);
        juce::Graphics::ScopedSaveState s (g);
        g.addTransform (juce::AffineTransform::rotation (-0.3f + 0.15f * (float) i, c.x, c.y));
        g.setColour (Theme::cyan.withAlpha (0.28f - 0.05f * (float) i));
        g.drawEllipse (c.x - rx, c.y - ry, rx * 2.0f, ry * 2.0f, 1.0f);
        const float ang = phase * (0.5f - 0.12f * (float) i) + (float) i * 2.1f;
        const juce::Point<float> m (c.x + std::cos (ang) * rx, c.y + std::sin (ang) * ry);
        draw::glowDot (g, m, 1.6f + 0.6f * (float) i, Theme::ivory, 0.6f + 0.4f * activity);
    }
}

void SpaceArt::dream (juce::Graphics& g, juce::Rectangle<float> a, float phase, float activity)
{
    const auto c = a.getCentre();
    const float R = juce::jmin (a.getWidth(), a.getHeight()) * 0.5f;
    draw::softLight (g, { c.x - R * 0.5f, c.y - R * 0.2f }, R * 1.2f, Theme::magenta, 0.2f + 0.1f * activity);
    draw::softLight (g, { c.x + R * 0.4f, c.y + R * 0.3f }, R * 1.1f, Theme::violet, 0.22f);
    draw::softLight (g, { c.x + R * 0.2f, c.y - R * 0.5f }, R * 0.8f, Theme::ivory, 0.08f);
    stars (g, a, 41, 30, phase, 0.35f);
    juce::Random rng (8);
    for (int i = 0; i < 7; ++i)
    {
        const juce::Point<float> p (a.getX() + rng.nextFloat() * a.getWidth(), a.getY() + rng.nextFloat() * a.getHeight());
        const float tw = 0.5f + 0.5f * std::sin (phase * 0.9f + (float) i * 1.3f);
        fourPointStar (g, p, 3.0f + 5.0f * rng.nextFloat() * (0.6f + 0.4f * tw), Theme::ivory.withAlpha (0.35f + 0.5f * tw * (0.6f + 0.4f * activity)));
    }
}

void SpaceArt::machine (juce::Graphics& g, juce::Rectangle<float> a, float phase, float activity)
{
    const auto c = a.getCentre();
    const float R = juce::jmin (a.getWidth(), a.getHeight()) * 0.42f;
    g.setColour (Theme::cyan.withAlpha (0.07f));
    for (int i = 1; i < 8; ++i)
    {
        g.drawLine (a.getX() + a.getWidth() * (float) i / 8.0f, a.getY(), a.getX() + a.getWidth() * (float) i / 8.0f, a.getBottom(), 1.0f);
        g.drawLine (a.getX(), a.getY() + a.getHeight() * (float) i / 8.0f, a.getRight(), a.getY() + a.getHeight() * (float) i / 8.0f, 1.0f);
    }
    draw::softLight (g, c, R * 1.2f, Theme::cyan, 0.12f + 0.15f * activity);
    for (int ring = 0; ring < 3; ++ring)
    {
        juce::Path hex;
        const float r = R * (0.35f + 0.3f * (float) ring);
        const float rot = phase * (ring % 2 == 0 ? 0.15f : -0.1f);
        for (int k = 0; k < 6; ++k)
        {
            const float ang = (float) k / 6.0f * kTwoPi + rot;
            const juce::Point<float> p (c.x + std::cos (ang) * r, c.y + std::sin (ang) * r);
            if (k == 0) hex.startNewSubPath (p); else hex.lineTo (p);
        }
        hex.closeSubPath();
        draw::glowPath (g, hex, Theme::cyan.withAlpha (0.6f - 0.15f * (float) ring), 1.0f, 4.0f, 0.3f + 0.5f * activity);
    }
    draw::glowDot (g, c, 2.2f, Theme::cyan, 0.8f);
}

void SpaceArt::shimmer (juce::Graphics& g, juce::Rectangle<float> a, float phase, float activity)
{
    const auto c = a.getCentre();
    draw::softLight (g, c, a.getWidth() * 0.45f, Theme::ivory, 0.10f + 0.12f * activity);
    stars (g, a, 51, 90, phase * 2.0f, 0.7f);
    juce::Random rng (9);
    for (int i = 0; i < 9; ++i)
    {
        const juce::Point<float> p (a.getX() + rng.nextFloat() * a.getWidth(), a.getY() + rng.nextFloat() * a.getHeight());
        const float tw = 0.5f + 0.5f * std::sin (phase * 2.0f + (float) i * 0.9f);
        fourPointStar (g, p, 2.5f + 4.0f * tw, Theme::ivory.withAlpha (0.3f + 0.6f * tw));
    }
}

void SpaceArt::dust (juce::Graphics& g, juce::Rectangle<float> a, float phase, float activity)
{
    draw::softLight (g, a.getCentre(), a.getWidth() * 0.4f, Theme::textSecondary, 0.08f + 0.1f * activity);
    juce::Random rng (61);
    for (int i = 0; i < 120; ++i)
    {
        const float drift = std::fmod (phase * (0.01f + 0.02f * rng.nextFloat()) + rng.nextFloat(), 1.0f);
        const float x = a.getX() + std::fmod (rng.nextFloat() + drift, 1.0f) * a.getWidth();
        const float y = a.getY() + rng.nextFloat() * a.getHeight();
        const float s = 0.6f + 1.6f * rng.nextFloat();
        g.setColour (Theme::ivory.withAlpha (0.08f + 0.35f * rng.nextFloat()));
        g.fillEllipse (x, y, s, s);
    }
}

} // namespace am::ui

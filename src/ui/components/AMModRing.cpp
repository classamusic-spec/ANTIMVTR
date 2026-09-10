#include "AMModRing.h"

namespace am::ui
{

AMModRing::AMModRing()
{
    setInterceptsMouseClicks (false, false);
    setOpaque (false);
}

void AMModRing::setAngles (float s, float e) { startAngle = s; endAngle = e; repaint(); }

void AMModRing::setBase (float n)
{
    n = juce::jlimit (0.0f, 1.0f, n);
    if (std::abs (n - base) < 0.0005f) return;
    base = n;
    if (isActive()) repaint();
}

void AMModRing::setRange (float lo, float hi)
{
    lo = juce::jlimit (0.0f, 1.0f, lo); hi = juce::jlimit (0.0f, 1.0f, hi);
    if (lo > hi) std::swap (lo, hi);
    const bool had = hasRange;
    hasRange = (hi - lo) > 0.002f;
    if (std::abs (lo - rangeLo) < 0.0005f && std::abs (hi - rangeHi) < 0.0005f && had == hasRange) return;
    rangeLo = lo; rangeHi = hi;
    repaint();
}

void AMModRing::setCurrent (float n)
{
    n = juce::jlimit (0.0f, 1.0f, n);
    const bool was = isActive();
    hasCurrent = true;
    if (std::abs (n - current) < 0.0025f && was == isActive()) return;
    current = n;
    repaint();
}

void AMModRing::setSourceCount (int n)
{
    n = juce::jmax (0, n);
    if (n == sourceCount) return;
    sourceCount = n;
    repaint();
}

void AMModRing::clear()
{
    hasRange = false; hasCurrent = false; sourceCount = 0; rangeLo = rangeHi = base; current = base;
    repaint();
}

bool AMModRing::isActive() const noexcept
{
    return hasRange || (hasCurrent && std::abs (current - base) > 0.006f);
}

void AMModRing::drawCountBadge (juce::Graphics& g, juce::Rectangle<float> rb, float stroke) const
{
    if (sourceCount < 2) return;
    const float r = juce::jmax (5.0f, stroke * 3.0f);
    if (rb.getWidth() < r * 5.0f) return;

    // Upper-right of the orbit, clear of the arc's own sweep (which opens at the bottom).
    const float a = juce::MathConstants<float>::pi * 0.62f;
    const juce::Point<float> c (rb.getCentreX() + std::sin (a) * rb.getWidth() * 0.5f,
                                rb.getCentreY() - std::cos (a) * rb.getHeight() * 0.5f);
    const auto badge = juce::Rectangle<float> (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

    g.setColour (Theme::background.withAlpha (0.92f));
    g.fillEllipse (badge.expanded (1.4f));
    g.setColour (accent.withAlpha (0.85f));
    g.fillEllipse (badge);
    draw::trackedText (g, juce::String (juce::jmin (9, sourceCount)), badge, juce::Justification::centred,
                       Theme::labelFontStrong (r * 1.25f), Theme::background);
}

void AMModRing::draw (juce::Graphics& g, juce::Rectangle<float> rb, float stroke) const
{
    if (! isActive() || rb.getWidth() < 6.0f) return;
    auto angleFor = [this] (float n) { return startAngle + n * (endAngle - startAngle); };
    const float band = stroke * 1.7f;

    // A dark moat under the whole sweep separates the orbit from the knob's value arc,
    // so the amber ring never blends into the section accent below it.
    {
        auto sweep = draw::arc (rb, angleFor (0.0f), angleFor (1.0f));
        g.setColour (Theme::background.withAlpha (0.85f));
        g.strokePath (sweep, juce::PathStrokeType (band * 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        g.setColour (accent.withAlpha (0.10f));
        g.strokePath (sweep, juce::PathStrokeType (band * 0.5f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    }

    if (hasRange)
    {
        auto range = draw::arc (rb, angleFor (rangeLo), angleFor (rangeHi));
        g.setColour (accent.withAlpha (0.30f));
        g.strokePath (range, juce::PathStrokeType (band, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

        // End brackets: short radial ticks that make the reach of the modulation explicit.
        for (float n : { rangeLo, rangeHi })
        {
            const float a = angleFor (n);
            const float r0 = rb.getWidth() * 0.5f - band * 0.65f, r1 = rb.getWidth() * 0.5f + band * 0.65f;
            const juce::Point<float> p0 (rb.getCentreX() + std::sin (a) * r0, rb.getCentreY() - std::cos (a) * r0);
            const juce::Point<float> p1 (rb.getCentreX() + std::sin (a) * r1, rb.getCentreY() - std::cos (a) * r1);
            g.setColour (accent.withAlpha (0.75f));
            g.drawLine ({ p0, p1 }, juce::jmax (1.0f, stroke * 0.7f));
        }
    }

    // Travel arc from the base value to the current modulated position: the live part.
    if (hasCurrent && std::abs (current - base) > 0.006f)
    {
        auto delta = draw::arc (rb, juce::jmin (angleFor (base), angleFor (current)), juce::jmax (angleFor (base), angleFor (current)));
        draw::glowPath (g, delta, accent, stroke * 1.15f, stroke * 3.4f, 0.8f);
    }

    // Current position marker.
    if (hasCurrent)
    {
        const float a = angleFor (current);
        const float r = rb.getWidth() * 0.5f;
        const juce::Point<float> p (rb.getCentreX() + std::sin (a) * r, rb.getCentreY() - std::cos (a) * r);
        draw::glowDot (g, p, juce::jmax (1.7f, stroke * 1.15f), accent, 0.95f);
    }

    drawCountBadge (g, rb, stroke);
}

void AMModRing::paint (juce::Graphics& g)
{
    // Same geometry as KnobRings::forFootprint so a stand-alone ring lines up with a knob's orbit.
    const auto b = getLocalBounds().toFloat();
    const float d = juce::jmin (b.getWidth(), b.getHeight());
    const float stroke = juce::jmax (1.1f, d * 0.016f);
    draw (g, b.withSizeKeepingCentre (d, d).reduced (stroke * 1.4f), stroke);
}

} // namespace am::ui

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

void AMModRing::clear()
{
    hasRange = false; hasCurrent = false; rangeLo = rangeHi = base; current = base;
    repaint();
}

bool AMModRing::isActive() const noexcept
{
    return hasRange || (hasCurrent && std::abs (current - base) > 0.006f);
}

void AMModRing::draw (juce::Graphics& g, juce::Rectangle<float> rb, float stroke) const
{
    if (! isActive() || rb.getWidth() < 6.0f) return;
    auto angleFor = [this] (float n) { return startAngle + n * (endAngle - startAngle); };

    if (hasRange)
    {
        auto range = draw::arc (rb, angleFor (rangeLo), angleFor (rangeHi));
        g.setColour (accent.withAlpha (0.16f));
        g.strokePath (range, juce::PathStrokeType (stroke * 2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        g.setColour (accent.withAlpha (0.55f));
        g.strokePath (range, juce::PathStrokeType (stroke * 0.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // delta arc from the base value to the current modulated position
    if (hasCurrent && std::abs (current - base) > 0.006f)
    {
        auto delta = draw::arc (rb, juce::jmin (angleFor (base), angleFor (current)), juce::jmax (angleFor (base), angleFor (current)));
        draw::glowPath (g, delta, accent, stroke, stroke * 3.0f, 0.7f);
    }

    // current position marker
    if (hasCurrent)
    {
        const float a = angleFor (current);
        const float r = rb.getWidth() * 0.5f;
        const juce::Point<float> p (rb.getCentreX() + std::sin (a) * r, rb.getCentreY() - std::cos (a) * r);
        draw::glowDot (g, p, juce::jmax (1.6f, stroke * 1.1f), accent, 0.9f);
    }
}

void AMModRing::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float d = juce::jmin (b.getWidth(), b.getHeight());
    const float stroke = juce::jmax (1.2f, d * 0.02f);
    draw (g, b.withSizeKeepingCentre (d, d).reduced (stroke * 1.5f), stroke);
}

} // namespace am::ui

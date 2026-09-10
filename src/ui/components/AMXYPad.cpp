#include "AMXYPad.h"

namespace am::ui
{

AMXYPad::AMXYPad (const juce::String& xl, const juce::String& yl, juce::Colour a)
    : xLabel (xl.toUpperCase()), yLabel (yl.toUpperCase()), accent (a)
{
    setWantsKeyboardFocus (false);
}

juce::Rectangle<float> AMXYPad::field() const
{
    auto b = getLocalBounds().toFloat();
    const float labelH = juce::jlimit (15.0f, 26.0f, juce::jmin (b.getWidth(), b.getHeight()) * 0.075f);
    auto inner = b.withTrimmedBottom (labelH).withTrimmedLeft (labelH);
    const float s = juce::jmin (inner.getWidth(), inner.getHeight());
    return inner.withSizeKeepingCentre (s, s);
}

float AMXYPad::markerRadius() const
{
    return juce::jlimit (3.0f, 7.0f, field().getWidth() * 0.022f);
}

juce::Rectangle<float> AMXYPad::plotArea() const
{
    return field().reduced (markerRadius() + 2.0f);
}

void AMXYPad::setPosition (float x, float y, juce::NotificationType notify)
{
    x = juce::jlimit (0.0f, 1.0f, x); y = juce::jlimit (0.0f, 1.0f, y);
    if (std::abs (x - px) < 0.0005f && std::abs (y - py) < 0.0005f) return;
    px = x; py = y;
    trail[(size_t) trailHead] = { px, py };
    trailHead = (trailHead + 1) % kTrail;
    trailCount = juce::jmin (kTrail, trailCount + 1);
    repaint();
    if (notify != juce::dontSendNotification && onChange) onChange (px, py);
}

void AMXYPad::updateFromMouse (const juce::MouseEvent& e)
{
    const auto f = plotArea();
    if (e.mods.isShiftDown())
    {
        const auto delta = e.position - dragOrigin;
        setPosition (dragValueOrigin.x + delta.x / f.getWidth() * 0.2f, dragValueOrigin.y - delta.y / f.getHeight() * 0.2f);
    }
    else
    {
        setPosition ((e.position.x - f.getX()) / f.getWidth(), 1.0f - (e.position.y - f.getY()) / f.getHeight());
    }
    const juce::String tx = formatX ? formatX (px) : juce::String (px, 2);
    const juce::String ty = formatY ? formatY (py) : juce::String (py, 2);
    tip.showFor (*this, e.getPosition(), xLabel + " " + tx + "   " + yLabel + " " + ty);
}

void AMXYPad::mouseDown (const juce::MouseEvent& e)
{
    dragging = true;
    dragOrigin = e.position;
    dragValueOrigin = { px, py };
    if (onDragStart) onDragStart();
    updateFromMouse (e);
}

void AMXYPad::mouseDrag (const juce::MouseEvent& e) { updateFromMouse (e); }

void AMXYPad::mouseUp (const juce::MouseEvent&)
{
    dragging = false;
    if (onDragEnd) onDragEnd();
    repaint();
}

void AMXYPad::mouseDoubleClick (const juce::MouseEvent&)
{
    if (onDragStart) onDragStart();
    setPosition (dx, dy);
    if (onDragEnd) onDragEnd();
}

void AMXYPad::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const auto f = field();
    const auto plot = plotArea();
    const float corner = juce::jmin (10.0f, f.getHeight() * 0.06f);
    const float lit = juce::jmax (hover.value, dragging ? 1.0f : 0.0f);
    draw::insetSurface (g, f, corner);

    const juce::Point<float> p (plot.getX() + px * plot.getWidth(), plot.getBottom() - py * plot.getHeight());

    {
        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (f.reduced (1.0f).toNearestInt());

        // Grid: eighths as a faint weave, quarters as the readable structure, centre lines strongest.
        for (int i = 1; i < 8; ++i)
        {
            if (i % 2 == 0) continue;
            const float x = f.getX() + f.getWidth() * (float) i / 8.0f, y = f.getY() + f.getHeight() * (float) i / 8.0f;
            g.setColour (juce::Colours::white.withAlpha (0.025f));
            g.drawLine (x, f.getY(), x, f.getBottom(), 1.0f);
            g.drawLine (f.getX(), y, f.getRight(), y, 1.0f);
        }
        for (int i = 1; i < 4; ++i)
        {
            const float x = f.getX() + f.getWidth() * (float) i / 4.0f, y = f.getY() + f.getHeight() * (float) i / 4.0f;
            g.setColour (juce::Colours::white.withAlpha (i == 2 ? 0.085f : 0.045f));
            g.drawLine (x, f.getY(), x, f.getBottom(), 1.0f);
            g.drawLine (f.getX(), y, f.getRight(), y, 1.0f);
        }

        // Where the field has been: the trail fades from old to new.
        if (trailCount > 1)
        {
            for (int i = 1; i < trailCount; ++i)
            {
                const auto& a = trail[(size_t) ((trailHead - trailCount + i - 1 + kTrail * 2) % kTrail)];
                const auto& c = trail[(size_t) ((trailHead - trailCount + i + kTrail * 2) % kTrail)];
                const juce::Point<float> q0 (plot.getX() + a.x * plot.getWidth(), plot.getBottom() - a.y * plot.getHeight());
                const juce::Point<float> q1 (plot.getX() + c.x * plot.getWidth(), plot.getBottom() - c.y * plot.getHeight());
                const float t = (float) i / (float) juce::jmax (1, trailCount - 1);
                g.setColour (accent.withAlpha (0.05f + 0.35f * t * t));
                g.drawLine ({ q0, q1 }, juce::jmax (1.0f, markerRadius() * 0.35f * t));
            }
        }

        // crosshair lines to the axes
        g.setColour (accent.withAlpha (0.22f + 0.22f * lit));
        g.drawLine (f.getX(), p.y, f.getRight(), p.y, 1.0f);
        g.drawLine (p.x, f.getY(), p.x, f.getBottom(), 1.0f);

        // the point
        draw::softLight (g, p, f.getWidth() * 0.18f, accent, 0.18f + 0.2f * lit + 0.2f * energy);
        draw::glowDot (g, p, markerRadius(), accent, 0.7f + 0.3f * lit);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.drawEllipse (juce::Rectangle<float> (markerRadius() * 3.0f, markerRadius() * 3.0f).withCentre (p), 1.0f);
    }

    // Quarter ticks outside the field give the grid a scale.
    {
        const float tick = juce::jlimit (3.0f, 6.0f, f.getWidth() * 0.012f);
        g.setColour (Theme::textDim.withAlpha (0.55f));
        for (int i = 0; i <= 4; ++i)
        {
            const float x = f.getX() + f.getWidth() * (float) i / 4.0f, y = f.getY() + f.getHeight() * (float) i / 4.0f;
            const float len = (i == 0 || i == 4) ? tick : tick * 0.6f;
            g.drawLine (x, f.getBottom(), x, f.getBottom() + len, 1.0f);
            g.drawLine (f.getX() - len, y, f.getX(), y, 1.0f);
        }
    }

    // Axis names and live readouts, at the same scale as the panel's own labels.
    const float labelH = juce::jlimit (15.0f, 26.0f, juce::jmin (b.getWidth(), b.getHeight()) * 0.075f);
    const float h = juce::jlimit (9.0f, 12.5f, labelH * 0.56f);
    auto bottom = juce::Rectangle<float> (f.getX(), f.getBottom(), f.getWidth(), labelH);
    draw::trackedText (g, xLabel, bottom, juce::Justification::centredLeft, Theme::labelFont (h), Theme::textSecondary);
    draw::trackedText (g, formatX ? formatX (px) : juce::String (px, 2), bottom, juce::Justification::centredRight,
                       Theme::valueFont (h + 1.0f), Theme::textValue.interpolatedWith (accent, 0.35f + 0.4f * lit));
    {
        juce::Graphics::ScopedSaveState save (g);
        auto left = juce::Rectangle<float> (f.getX() - labelH, f.getY(), labelH, f.getHeight());
        g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, left.getCentreX(), left.getCentreY()));
        auto rotated = juce::Rectangle<float> (left.getHeight(), left.getWidth()).withCentre (left.getCentre());
        draw::trackedText (g, yLabel, rotated, juce::Justification::centredLeft, Theme::labelFont (h), Theme::textSecondary);
        draw::trackedText (g, formatY ? formatY (py) : juce::String (py, 2), rotated, juce::Justification::centredRight,
                           Theme::valueFont (h + 1.0f), Theme::textValue.interpolatedWith (accent, 0.35f + 0.4f * lit));
    }
}

} // namespace am::ui

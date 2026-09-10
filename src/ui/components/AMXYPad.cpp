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
    const float labelH = juce::jlimit (12.0f, 18.0f, b.getHeight() * 0.1f);
    auto inner = b.withTrimmedBottom (labelH).withTrimmedLeft (labelH);
    const float s = juce::jmin (inner.getWidth(), inner.getHeight());
    return inner.withSizeKeepingCentre (s, s);
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
    const auto f = field();
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
    const float corner = juce::jmin (10.0f, f.getHeight() * 0.06f);
    const float lit = juce::jmax (hover.value, dragging ? 1.0f : 0.0f);
    draw::insetSurface (g, f, corner);

    {
        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (f.reduced (1.0f).toNearestInt());
        // grid
        g.setColour (juce::Colours::white.withAlpha (0.035f));
        for (int i = 1; i < 8; ++i)
        {
            const float x = f.getX() + f.getWidth() * (float) i / 8.0f, y = f.getY() + f.getHeight() * (float) i / 8.0f;
            g.drawLine (x, f.getY(), x, f.getBottom(), 1.0f);
            g.drawLine (f.getX(), y, f.getRight(), y, 1.0f);
        }
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawLine (f.getCentreX(), f.getY(), f.getCentreX(), f.getBottom(), 1.0f);
        g.drawLine (f.getX(), f.getCentreY(), f.getRight(), f.getCentreY(), 1.0f);

        const juce::Point<float> p (f.getX() + px * f.getWidth(), f.getBottom() - py * f.getHeight());

        // trail
        if (trailCount > 1)
        {
            juce::Path t;
            for (int i = 0; i < trailCount; ++i)
            {
                const auto& v = trail[(size_t) ((trailHead - trailCount + i + kTrail * 2) % kTrail)];
                const juce::Point<float> q (f.getX() + v.x * f.getWidth(), f.getBottom() - v.y * f.getHeight());
                if (i == 0) t.startNewSubPath (q); else t.lineTo (q);
            }
            g.setColour (accent.withAlpha (0.25f));
            g.strokePath (t, juce::PathStrokeType (1.0f));
        }

        // crosshair lines to the axes
        g.setColour (accent.withAlpha (0.25f + 0.2f * lit));
        g.drawLine (f.getX(), p.y, f.getRight(), p.y, 1.0f);
        g.drawLine (p.x, f.getY(), p.x, f.getBottom(), 1.0f);

        // the point
        draw::softLight (g, p, f.getWidth() * 0.18f, accent, 0.18f + 0.2f * lit + 0.2f * energy);
        draw::glowDot (g, p, juce::jlimit (3.0f, 6.0f, f.getWidth() * 0.02f), accent, 0.7f + 0.3f * lit);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.drawEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (p), 1.0f);
    }

    // axis labels + readouts
    const float h = juce::jlimit (8.0f, 10.5f, b.getHeight() * 0.05f);
    const float labelH = juce::jlimit (12.0f, 18.0f, b.getHeight() * 0.1f);
    auto bottom = juce::Rectangle<float> (f.getX(), f.getBottom(), f.getWidth(), labelH);
    draw::trackedText (g, xLabel, bottom, juce::Justification::centredLeft, Theme::labelFont (h), Theme::textSecondary);
    draw::trackedText (g, formatX ? formatX (px) : juce::String (px, 2), bottom, juce::Justification::centredRight, Theme::valueFont (h + 1.0f), Theme::textValue);
    {
        juce::Graphics::ScopedSaveState save (g);
        auto left = juce::Rectangle<float> (f.getX() - labelH, f.getY(), labelH, f.getHeight());
        g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, left.getCentreX(), left.getCentreY()));
        auto rotated = juce::Rectangle<float> (left.getHeight(), left.getWidth()).withCentre (left.getCentre());
        draw::trackedText (g, yLabel, rotated, juce::Justification::centredLeft, Theme::labelFont (h), Theme::textSecondary);
        draw::trackedText (g, formatY ? formatY (py) : juce::String (py, 2), rotated, juce::Justification::centredRight, Theme::valueFont (h + 1.0f), Theme::textValue);
    }
}

} // namespace am::ui

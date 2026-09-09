#include "AMSourceSelector.h"

namespace am::ui
{

AMSourceSelector::AMSourceSelector (std::vector<Item> i) : items (std::move (i))
{
    setWantsKeyboardFocus (false);
    startTimerHz (30);
}

void AMSourceSelector::setSelected (int index, juce::NotificationType notify)
{
    index = juce::jlimit (0, (int) items.size() - 1, index);
    if (index == selected) return;
    selected = index;
    repaint();
    if (notify != juce::dontSendNotification && onChange) onChange (selected);
}

int AMSourceSelector::indexAt (juce::Point<int> p) const
{
    if (items.empty()) return -1;
    const int w = getWidth() / (int) items.size();
    return juce::jlimit (0, (int) items.size() - 1, p.x / juce::jmax (1, w));
}

void AMSourceSelector::mouseDown (const juce::MouseEvent& e) { setSelected (indexAt (e.getPosition())); }
void AMSourceSelector::mouseMove (const juce::MouseEvent& e) { const int h = indexAt (e.getPosition()); if (h != hover) { hover = h; repaint(); } }

void AMSourceSelector::drawThumbnail (juce::Graphics& g, const Item& item, juce::Rectangle<float> circle, bool on, int index)
{
    const float r = circle.getWidth() * 0.5f;
    const auto c = circle.getCentre();
    juce::Graphics::ScopedSaveState save (g);
    juce::Path clip; clip.addEllipse (circle);
    g.reduceClipRegion (clip);

    // Dark sphere base
    juce::ColourGradient base (Theme::knobBase.brighter (0.15f), c.x - r * 0.4f, c.y - r * 0.5f, juce::Colour (0xff050508), c.x + r, c.y + r, true);
    g.setGradientFill (base);
    g.fillEllipse (circle);

    const juce::Colour accent = item.accent;
    const float a = on ? 0.95f : 0.45f;
    const float t = phase + (float) index * 1.7f;

    switch (item.icon)
    {
        case Icon::Wave:
        {
            juce::Path p;
            const int n = 40;
            for (int i = 0; i <= n; ++i)
            {
                const float x = circle.getX() + circle.getWidth() * (float) i / (float) n;
                const float u = (float) i / (float) n;
                const float env = std::sin (u * juce::MathConstants<float>::pi);
                const float y = c.y - r * 0.55f * env * std::sin (u * 14.0f + t * (on ? 1.0f : 0.2f)) * (0.6f + 0.4f * energy);
                if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }
            draw::glowPath (g, p, accent.withAlpha (a), 1.3f, 6.0f, on ? 0.8f : 0.3f);
            break;
        }
        case Icon::Dust:
        {
            juce::Random rng (1234 + index);
            for (int i = 0; i < 70; ++i)
            {
                const float ang = rng.nextFloat() * juce::MathConstants<float>::twoPi;
                const float rad = std::sqrt (rng.nextFloat()) * r * 0.9f;
                const float flicker = 0.5f + 0.5f * std::sin (t * 2.0f + (float) i);
                const float s = 0.8f + 1.6f * rng.nextFloat();
                g.setColour (accent.withAlpha (a * (0.25f + 0.75f * flicker)));
                g.fillEllipse (c.x + std::cos (ang) * rad - s * 0.5f, c.y + std::sin (ang) * rad - s * 0.5f, s, s);
            }
            break;
        }
        case Icon::Impact:
        {
            juce::Random rng (77 + index);
            juce::Path p;
            for (int i = 0; i < 14; ++i)
            {
                const float ang = rng.nextFloat() * juce::MathConstants<float>::twoPi;
                const float len = r * (0.35f + 0.6f * rng.nextFloat());
                p.startNewSubPath (c.x + std::cos (ang) * r * 0.1f, c.y + std::sin (ang) * r * 0.1f);
                p.lineTo (c.x + std::cos (ang) * len, c.y + std::sin (ang) * len);
                const float ang2 = ang + 0.5f * rng.nextFloat();
                p.lineTo (c.x + std::cos (ang2) * len * 0.7f, c.y + std::sin (ang2) * len * 0.7f);
            }
            draw::glowPath (g, p, accent.withAlpha (a), 1.0f, 5.0f, on ? 0.7f : 0.3f);
            g.setColour (Theme::textPrimary.withAlpha (a));
            g.fillEllipse (c.x - 2.0f, c.y - 2.0f, 4.0f, 4.0f);
            break;
        }
        case Icon::Sample:
        default:
        {
            juce::Random rng (9 + index);
            juce::Path p;
            const int bars = 26;
            for (int i = 0; i < bars; ++i)
            {
                const float u = (float) i / (float) (bars - 1);
                const float x = circle.getX() + r * 0.2f + (circle.getWidth() - r * 0.4f) * u;
                const float h = r * (0.15f + 0.7f * rng.nextFloat() * std::sin (u * juce::MathConstants<float>::pi));
                p.addRectangle (x, c.y - h * 0.5f, 1.4f, h);
            }
            g.setColour (accent.withAlpha (a * 0.8f));
            g.fillPath (p);
            break;
        }
    }

    // glass rim
    g.setColour (juce::Colours::white.withAlpha (on ? 0.10f : 0.05f));
    g.drawEllipse (circle.reduced (1.0f), 1.0f);
}

void AMSourceSelector::paint (juce::Graphics& g)
{
    if (items.empty()) return;
    const auto b = getLocalBounds().toFloat();
    const float cellW = b.getWidth() / (float) items.size();
    const float labelH = juce::jlimit (10.0f, 18.0f, b.getHeight() * 0.2f);

    for (int i = 0; i < (int) items.size(); ++i)
    {
        auto cell = juce::Rectangle<float> (b.getX() + cellW * (float) i, b.getY(), cellW, b.getHeight());
        auto area = cell.withTrimmedBottom (labelH);
        const float d = juce::jmin (area.getWidth(), area.getHeight()) * 0.78f;
        auto circle = area.withSizeKeepingCentre (d, d);
        const bool on = i == selected;
        const auto& item = items[(size_t) i];

        if (on)
        {
            draw::glowEllipse (g, circle, item.accent, d * 0.22f, 0.8f + 0.2f * energy);
            g.setColour (item.accent.withAlpha (0.9f));
            g.drawEllipse (circle.expanded (2.5f), 1.4f);
        }
        else if (i == hover)
        {
            draw::glowEllipse (g, circle, item.accent, d * 0.12f, 0.35f);
        }

        drawThumbnail (g, item, circle, on, i);

        auto labelArea = cell.withTop (circle.getBottom() + 2.0f);
        const float h = juce::jlimit (8.5f, 12.5f, labelH * 0.7f);
        draw::trackedText (g, item.label.toUpperCase(), labelArea, juce::Justification::centredTop, Theme::labelFont (h),
                           on ? Theme::textPrimary : Theme::textSecondary);
    }
}

} // namespace am::ui

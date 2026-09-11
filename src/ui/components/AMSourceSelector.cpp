#include "AMSourceSelector.h"
#include "ui/UILayout.h"

namespace am::ui
{

AMSourceSelector::AMSourceSelector (std::vector<Item> i) : items (std::move (i))
{
    items.resize (juce::jmin ((int) items.size(), kMaxItems));
    for (auto& it : items) { labels.push_back (it.label.toUpperCase()); captions.push_back (it.caption.toUpperCase()); }
    setWantsKeyboardFocus (false);
    scratch.preallocateSpace (1024);
    if (! items.empty()) lit[0].snap (1.0f);
}

AMSourceSelector::~AMSourceSelector() { stopTimer(); }

void AMSourceSelector::visibilityChanged()   { if (isShowing()) startTimerHz (24); else stopTimer(); }
void AMSourceSelector::parentHierarchyChanged() { if (isShowing() && ! isTimerRunning()) startTimerHz (24); }

void AMSourceSelector::timerCallback()
{
    if (! isShowing()) { stopTimer(); return; }
    phase += 1.0f / 24.0f;
    for (int i = 0; i < (int) items.size(); ++i)
    {
        lit[(size_t) i].target = (i == selected) ? 1.0f : 0.0f;
        hov[(size_t) i].target = (i == hover) ? 1.0f : 0.0f;
        lit[(size_t) i].settle (0.22f);
        hov[(size_t) i].settle (0.25f);
    }
    repaint();
}

void AMSourceSelector::setSelected (int index, juce::NotificationType notify)
{
    index = juce::jlimit (0, (int) items.size() - 1, index);
    if (index == selected) return;
    selected = index;
    repaint();
    if (notify != juce::dontSendNotification && onChange) onChange (selected);
}

int AMSourceSelector::preferredHeight (int width) const noexcept
{
    return layout::sourceSelectorHeight (width, (int) items.size());
}

juce::Rectangle<float> AMSourceSelector::cellBounds (int index) const
{
    const auto b = getLocalBounds().toFloat();
    const int n = juce::jmax (1, (int) items.size());
    if (orientation == Orientation::Column)
    {
        const float h = b.getHeight() / (float) n;
        return { b.getX(), b.getY() + h * (float) index, b.getWidth(), h };
    }
    const float w = b.getWidth() / (float) n;
    return { b.getX() + w * (float) index, b.getY(), w, b.getHeight() };
}

int AMSourceSelector::indexAt (juce::Point<int> p) const
{
    if (items.empty()) return -1;
    const int n = (int) items.size();
    if (orientation == Orientation::Column)
        return juce::jlimit (0, n - 1, p.y / juce::jmax (1, getHeight() / n));
    return juce::jlimit (0, n - 1, p.x / juce::jmax (1, getWidth() / n));
}

void AMSourceSelector::mouseDown (const juce::MouseEvent& e) { setSelected (indexAt (e.getPosition())); }
void AMSourceSelector::mouseMove (const juce::MouseEvent& e) { const int h = indexAt (e.getPosition()); if (h != hover) { hover = h; if (! isTimerRunning()) startTimerHz (24); } }
void AMSourceSelector::mouseExit (const juce::MouseEvent&) { hover = -1; }

void AMSourceSelector::drawThumbnail (juce::Graphics& g, const Item& item, juce::Rectangle<float> circle, float on, int index)
{
    const float r = circle.getWidth() * 0.5f;
    const auto c = circle.getCentre();
    juce::Graphics::ScopedSaveState save (g);
    juce::Path clip; clip.addEllipse (circle);
    g.reduceClipRegion (clip);

    // A dark glass ball: lit from the top-left like everything else on the panel.
    juce::ColourGradient base (juce::Colour (0xff232733), c.x + draw::kLightX * r * 0.55f, c.y + draw::kLightY * r * 0.5f,
                               juce::Colour (0xff040508), c.x - draw::kLightX * r * 1.1f, c.y - draw::kLightY * r * 1.05f, true);
    base.addColour (0.5, juce::Colour (0xff0d1017));
    g.setGradientFill (base);
    g.fillEllipse (circle);
    // ambient tint in the source accent
    draw::softLight (g, c, r * 1.1f, item.accent, 0.06f + 0.14f * on);

    const juce::Colour accent = item.accent;
    const float a = 0.58f + 0.42f * on;
    const float t = phase * (0.35f + 0.65f * on) + (float) index * 1.7f;
    const float e = 0.6f + 0.4f * energy * on;

    switch (item.icon)
    {
        case Icon::Wave:
        {
            // two layered sine traces with a soft reflection
            for (int layer = 1; layer >= 0; --layer)
            {
                scratch.clear();
                const int n = 40;
                const float amp = r * (layer == 0 ? 0.5f : 0.34f) * e;
                for (int i = 0; i <= n; ++i)
                {
                    const float u = (float) i / (float) n;
                    const float x = circle.getX() + circle.getWidth() * u;
                    const float env = std::sin (u * juce::MathConstants<float>::pi);
                    const float y = c.y - amp * env * std::sin (u * 12.0f + t * 1.4f + (float) layer * 1.3f);
                    if (i == 0) scratch.startNewSubPath (x, y); else scratch.lineTo (x, y);
                }
                if (layer == 0) draw::glowPath (g, scratch, accent.withAlpha (a), 1.4f, 7.0f, 0.5f + 0.5f * on);
                else { g.setColour (accent.withAlpha (a * 0.3f)); g.strokePath (scratch, juce::PathStrokeType (1.0f)); }
            }
            break;
        }
        case Icon::Dust:
        {
            juce::Random rng (1234 + index);
            draw::softLight (g, c, r * 0.7f, accent, 0.3f * a);
            for (int i = 0; i < 64; ++i)
            {
                const float ang = rng.nextFloat() * juce::MathConstants<float>::twoPi;
                const float rad = std::pow (rng.nextFloat(), 0.7f) * r * 0.92f;
                const float drift = std::sin (t * 0.8f + (float) i) * r * 0.03f;
                const float flicker = 0.35f + 0.65f * (0.5f + 0.5f * std::sin (t * 2.2f + (float) i * 1.9f));
                const float s = 1.0f + 2.0f * rng.nextFloat() * rng.nextFloat();
                const auto col = (i % 5 == 0) ? Theme::textPrimary : accent;
                g.setColour (col.withAlpha (a * flicker * (0.35f + 0.65f * (1.0f - rad / r))));
                g.fillEllipse (c.x + std::cos (ang) * rad + drift - s * 0.5f, c.y + std::sin (ang) * rad - drift - s * 0.5f, s, s);
            }
            break;
        }
        case Icon::Impact:
        {
            juce::Random rng (77 + index);
            // expanding shock ring
            const float ring = std::fmod (t * 0.5f, 1.0f);
            g.setColour (accent.withAlpha (a * 0.35f * (1.0f - ring)));
            g.drawEllipse (juce::Rectangle<float> (0, 0, 2.0f * r * ring, 2.0f * r * ring).withCentre (c), 1.0f);
            scratch.clear();
            for (int i = 0; i < 16; ++i)
            {
                const float ang = rng.nextFloat() * juce::MathConstants<float>::twoPi;
                const float len = r * (0.3f + 0.62f * rng.nextFloat()) * (0.85f + 0.15f * std::sin (t * 3.0f + (float) i));
                scratch.startNewSubPath (c.x + std::cos (ang) * r * 0.08f, c.y + std::sin (ang) * r * 0.08f);
                scratch.lineTo (c.x + std::cos (ang) * len, c.y + std::sin (ang) * len);
            }
            draw::glowPath (g, scratch, accent.withAlpha (a), 1.0f, 5.0f, 0.4f + 0.5f * on);
            draw::glowDot (g, c, r * 0.07f, Theme::textPrimary, 0.6f + 0.4f * on);
            break;
        }
        case Icon::Gesture:
        {
            scratch.clear();
            const int n = 40;
            for (int i = 0; i <= n; ++i)
            {
                const float u = (float) i / (float) n;
                const float x = circle.getX() + circle.getWidth() * u;
                const float y = c.y + r * 0.45f * std::sin (u * 5.0f + t) * std::sin (u * juce::MathConstants<float>::pi) * e;
                if (i == 0) scratch.startNewSubPath (x, y); else scratch.lineTo (x, y);
            }
            draw::glowPath (g, scratch, accent.withAlpha (a), 1.4f, 7.0f, 0.5f + 0.5f * on);
            break;
        }
        case Icon::Sample:
        default:
        {
            juce::Random rng (9 + index);
            scratch.clear();
            const int bars = 22;
            const float step = (circle.getWidth() - r * 0.5f) / (float) bars;
            for (int i = 0; i < bars; ++i)
            {
                const float u = (float) i / (float) (bars - 1);
                const float x = circle.getX() + r * 0.25f + step * (float) i;
                const float h = r * (0.12f + 0.75f * rng.nextFloat() * std::sin (u * juce::MathConstants<float>::pi)) * e;
                scratch.addRectangle (x, c.y - h * 0.5f, juce::jmax (1.0f, step * 0.45f), h);
            }
            g.setColour (accent.withAlpha (a * 0.75f));
            g.fillPath (scratch);
            // sweeping playhead
            const float px = circle.getX() + r * 0.25f + (circle.getWidth() - r * 0.5f) * std::fmod (t * 0.25f, 1.0f);
            g.setColour (Theme::textPrimary.withAlpha (0.35f * a));
            g.drawLine (px, c.y - r * 0.55f, px, c.y + r * 0.55f, 1.0f);
            break;
        }
    }

    // The glass over the art: thickness toward the shaded edge, a rim light on the
    // lit one, and a small specular dot in the upper left.
    {
        juce::ColourGradient thickness (juce::Colours::transparentBlack, c.x + draw::kLightX * r * 0.5f, c.y + draw::kLightY * r * 0.5f,
                                        juce::Colours::black.withAlpha (0.55f), c.x - draw::kLightX * r * 1.15f, c.y - draw::kLightY * r * 1.15f, true);
        thickness.addColour (0.6, juce::Colours::black.withAlpha (0.06f));
        g.setGradientFill (thickness);
        g.fillEllipse (circle);
    }
    {
        // a broad diagonal sweep across the top-left of the glass
        const juce::Point<float> sc (c.x + draw::kLightX * r * 0.42f, c.y + draw::kLightY * r * 0.40f);
        juce::Graphics::ScopedSaveState sweepState (g);
        g.addTransform (juce::AffineTransform::rotation (-0.62f, sc.x, sc.y).scaled (1.0f, 0.5f, sc.x, sc.y));
        draw::softLight (g, sc, r * 0.72f, juce::Colours::white, 0.10f + 0.05f * on);
    }

    const float toLight = std::atan2 (draw::kLightX, -draw::kLightY);
    juce::Path rim;
    rim.addCentredArc (c.x, c.y, r - 1.2f, r - 1.2f, 0.0f, toLight - 1.35f, toLight + 1.35f, true);
    g.setColour (juce::Colours::white.withAlpha (0.22f + 0.16f * on));
    g.strokePath (rim, juce::PathStrokeType (juce::jmax (0.9f, r * 0.055f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float dot = juce::jmax (1.0f, r * 0.13f);
    const juce::Point<float> spec (c.x + draw::kLightX * r * 0.55f, c.y + draw::kLightY * r * 0.52f);
    draw::softLight (g, spec, dot * 3.2f, juce::Colours::white, 0.16f);
    g.setColour (juce::Colours::white.withAlpha (0.72f));
    g.fillEllipse (spec.x - dot, spec.y - dot * 0.78f, dot * 2.0f, dot * 1.56f);

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (circle.reduced (0.5f), 1.0f);
}

void AMSourceSelector::paint (juce::Graphics& g)
{
    if (items.empty()) return;

    for (int i = 0; i < (int) items.size(); ++i)
    {
        const auto cell = cellBounds (i);
        const auto& item = items[(size_t) i];
        const float on = lit[(size_t) i].value;
        const float hv = hov[(size_t) i].value * (1.0f - on);

        juce::Rectangle<float> circle;
        if (orientation == Orientation::Column)
        {
            const float pad = juce::jlimit (2.0f, 8.0f, cell.getHeight() * 0.09f);
            auto row = cell.reduced (0.0f, pad);
            const float d = juce::jmin (row.getHeight(), cell.getWidth() * 0.34f);

            // The whole row is the target, so a lit entry reads as a selected list
            // item: the sidebar pill of SPEC section 3.
            if (on > 0.01f || hv > 0.01f)
                draw::sidebarPill (g, row.reduced (pad * 0.5f, 0.0f), juce::jlimit (5.0f, 12.0f, row.getHeight() * 0.24f),
                                   item.accent, on, hv, pad * 0.5f);

            auto inner = row.withTrimmedLeft (juce::jlimit (8.0f, 20.0f, cell.getWidth() * 0.06f));
            circle = inner.removeFromLeft (d).withSizeKeepingCentre (d, d);

            auto text = inner.withTrimmedLeft (juce::jlimit (7.0f, 16.0f, d * 0.24f)).withTrimmedRight (6.0f);
            const bool hasCaption = captions[(size_t) i].isNotEmpty() && text.getHeight() > 26.0f;
            const float nameH = juce::jlimit (9.5f, 13.5f, text.getHeight() * (hasCaption ? 0.32f : 0.42f));
            auto nameArea = hasCaption ? text.removeFromTop (text.getHeight() * 0.55f) : text;
            draw::trackedText (g, labels[(size_t) i], nameArea, hasCaption ? juce::Justification::bottomLeft : juce::Justification::centredLeft,
                               draw::fitFont (on > 0.5f ? Theme::labelFontStrong (nameH) : Theme::labelFont (nameH), labels[(size_t) i], nameArea.getWidth()),
                               Theme::textSecondary.interpolatedWith (Theme::textPrimary, juce::jmax (on, hv * 0.5f)));
            if (hasCaption)
            {
                const float capH = juce::jlimit (7.5f, 10.0f, nameH * 0.72f);
                // The caption carries the section colour, but deepened against the
                // pale row: the accent at a third of its strength vanished here.
                draw::trackedText (g, captions[(size_t) i], text, juce::Justification::topLeft,
                                   draw::fitFont (Theme::captionFont (capH), captions[(size_t) i], text.getWidth()),
                                   item.accent.darker (0.35f).interpolatedWith (Theme::textDim, on > 0.5f ? 0.0f : 0.45f));
            }
        }
        else
        {
            const float labelH = juce::jlimit (10.0f, 18.0f, cell.getHeight() * 0.2f);
            auto area = cell.withTrimmedBottom (labelH);
            // The chosen source sits slightly proud of its neighbours.
            const float d = juce::jmin (area.getWidth(), area.getHeight()) * 0.76f * (0.90f + 0.10f * on);
            circle = area.withSizeKeepingCentre (d, d);
        }

        const float d = circle.getWidth();
        draw::contactShadowEllipse (g, circle, juce::jlimit (2.0f, 10.0f, d * 0.12f), 0.85f);
        if (on > 0.01f)
        {
            const auto pair = Theme::accentPair (item.accent);
            draw::glowEllipse (g, circle, pair.second, d * 0.24f, on * (0.75f + 0.25f * energy));
            const auto ring = circle.expanded (juce::jmax (2.5f, d * 0.05f));
            juce::ColourGradient grad (pair.first.withAlpha (0.95f * on), ring.getX(), ring.getCentreY(),
                                       pair.second.withAlpha (0.95f * on), ring.getRight(), ring.getCentreY(), false);
            g.setGradientFill (grad);
            g.drawEllipse (ring, juce::jmax (1.3f, d * 0.028f));
        }
        if (hv > 0.01f)
        {
            draw::glowEllipse (g, circle, item.accent, d * 0.12f, 0.4f * hv);
            g.setColour (item.accent.withAlpha (0.35f * hv));
            g.drawEllipse (circle.expanded (2.5f), 1.0f);
        }

        drawThumbnail (g, item, circle, on, i);

        if (orientation == Orientation::Column) continue;

        const float labelH = juce::jlimit (10.0f, 18.0f, cell.getHeight() * 0.2f);
        auto labelArea = cell.withTop (circle.getBottom() + 3.0f);
        const float h = juce::jlimit (8.5f, 12.0f, labelH * 0.68f);
        const auto font = draw::fitFont (on > 0.5f ? Theme::labelFontStrong (h) : Theme::labelFont (h), labels[(size_t) i], cell.getWidth() - 4.0f);
        draw::trackedText (g, labels[(size_t) i], labelArea, juce::Justification::centredTop, font,
                           Theme::textSecondary.interpolatedWith (Theme::textPrimary, juce::jmax (on, hv * 0.5f)));
    }
}

} // namespace am::ui

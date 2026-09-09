#include "AMSourceSelector.h"

namespace am::ui
{

AMSourceSelector::AMSourceSelector (std::vector<Item> i) : items (std::move (i))
{
    items.resize (juce::jmin ((int) items.size(), kMaxItems));
    for (auto& it : items) labels.push_back (it.label.toUpperCase());
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

int AMSourceSelector::indexAt (juce::Point<int> p) const
{
    if (items.empty()) return -1;
    const int w = getWidth() / (int) items.size();
    return juce::jlimit (0, (int) items.size() - 1, p.x / juce::jmax (1, w));
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

    // Dark glass disc
    juce::ColourGradient base (juce::Colour (0xff1a1a24), c.x - r * 0.4f, c.y - r * 0.5f, juce::Colour (0xff050508), c.x + r * 0.5f, c.y + r * 0.9f, true);
    g.setGradientFill (base);
    g.fillEllipse (circle);
    // ambient tint in the source accent
    draw::softLight (g, c, r * 1.1f, item.accent, 0.06f + 0.14f * on);

    const juce::Colour accent = item.accent;
    const float a = 0.42f + 0.58f * on;
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
            draw::softLight (g, c, r * 0.6f, accent, 0.2f * a);
            for (int i = 0; i < 64; ++i)
            {
                const float ang = rng.nextFloat() * juce::MathConstants<float>::twoPi;
                const float rad = std::pow (rng.nextFloat(), 0.7f) * r * 0.92f;
                const float drift = std::sin (t * 0.8f + (float) i) * r * 0.03f;
                const float flicker = 0.35f + 0.65f * (0.5f + 0.5f * std::sin (t * 2.2f + (float) i * 1.9f));
                const float s = 0.7f + 1.7f * rng.nextFloat() * rng.nextFloat();
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

    // glass rim + vignette
    juce::ColourGradient vignette (juce::Colours::transparentBlack, c.x, c.y, juce::Colours::black.withAlpha (0.45f), c.x, c.y + r, true);
    g.setGradientFill (vignette);
    g.fillEllipse (circle);
    g.setColour (juce::Colours::white.withAlpha (0.06f + 0.08f * on));
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
        const float d = juce::jmin (area.getWidth(), area.getHeight()) * 0.76f;
        auto circle = area.withSizeKeepingCentre (d, d);
        const auto& item = items[(size_t) i];
        const float on = lit[(size_t) i].value;
        const float hv = hov[(size_t) i].value * (1.0f - on);

        if (on > 0.01f)
        {
            draw::glowEllipse (g, circle, item.accent, d * 0.24f, on * (0.75f + 0.25f * energy));
            g.setColour (item.accent.withAlpha (0.9f * on));
            g.drawEllipse (circle.expanded (2.5f), 1.3f);
        }
        if (hv > 0.01f)
        {
            draw::glowEllipse (g, circle, item.accent, d * 0.12f, 0.4f * hv);
            g.setColour (item.accent.withAlpha (0.35f * hv));
            g.drawEllipse (circle.expanded (2.5f), 1.0f);
        }

        drawThumbnail (g, item, circle, on, i);

        auto labelArea = cell.withTop (circle.getBottom() + 3.0f);
        const float h = juce::jlimit (8.5f, 12.0f, labelH * 0.68f);
        draw::trackedText (g, labels[(size_t) i], labelArea, juce::Justification::centredTop, on > 0.5f ? Theme::labelFontStrong (h) : Theme::labelFont (h),
                           Theme::textSecondary.interpolatedWith (Theme::textPrimary, juce::jmax (on, hv * 0.5f)));
    }
}

} // namespace am::ui

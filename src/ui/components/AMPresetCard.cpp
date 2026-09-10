#include "AMPresetCard.h"

namespace am::ui
{

AMPresetCard::AMPresetCard()
{
    setWantsKeyboardFocus (false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void AMPresetCard::setPreset (int i, const juce::String& n, const juce::String& c, const juce::StringArray& t)
{
    index = i; name = n.toUpperCase(); category = c.toUpperCase(); tags = t;
    for (auto& tag : tags) tag = tag.toUpperCase();
    repaint();
}

void AMPresetCard::setCurrent (bool on)
{
    if (on == current) return;
    current = on;
    if (isShowing()) anim.animate (lit, on ? 1.0f : 0.0f); else lit.snap (on ? 1.0f : 0.0f);
    repaint();
}

void AMPresetCard::mouseDown (const juce::MouseEvent&) { if (onClick) onClick (index); }
void AMPresetCard::mouseDoubleClick (const juce::MouseEvent&) { if (onDoubleClick) onDoubleClick (index); }

void AMPresetCard::drawArt (juce::Graphics& g, juce::Rectangle<float> a, const juce::String& category, int seed, float lit, float corner)
{
    juce::Graphics::ScopedSaveState save (g);
    juce::Path clip; clip.addRoundedRectangle (a, corner);
    g.reduceClipRegion (clip);
    g.setColour (juce::Colour (0xff050509));
    g.fillRect (a);

    juce::Random rng (seed);
    const auto c = a.getCentre();
    const float R = juce::jmin (a.getWidth(), a.getHeight()) * 0.5f;
    const juce::String cat = category.toUpperCase();
    juce::Colour tintA = Theme::violet, tintB = Theme::blue;
    if      (cat.contains ("BASS"))                          { tintA = Theme::amber;   tintB = Theme::magenta; }
    else if (cat.contains ("KEY") || cat.contains ("BELL"))  { tintA = Theme::cyan;    tintB = Theme::ivory; }
    else if (cat.contains ("LEAD"))                          { tintA = Theme::magenta; tintB = Theme::cyan; }
    else if (cat.contains ("PERC") || cat.contains ("DRUM")) { tintA = Theme::ivory;   tintB = Theme::blue; }
    else if (cat.contains ("TEX") || cat.contains ("FX") || cat.contains ("DRONE")) { tintA = Theme::blue; tintB = Theme::violet; }
    else if (cat.contains ("INIT"))                          { tintA = Theme::textSecondary; tintB = Theme::textDim; }

    draw::softLight (g, { c.x - R * 0.3f, c.y + R * 0.2f }, R * 1.2f, tintA, 0.22f + 0.15f * lit);
    draw::softLight (g, { c.x + R * 0.4f, c.y - R * 0.3f }, R * 1.0f, tintB, 0.16f + 0.1f * lit);

    // stars
    for (int i = 0; i < 26; ++i)
    {
        const float x = a.getX() + rng.nextFloat() * a.getWidth(), y = a.getY() + rng.nextFloat() * a.getHeight();
        const float s = 0.6f + 1.2f * rng.nextFloat();
        g.setColour (Theme::textPrimary.withAlpha (0.15f + 0.4f * rng.nextFloat()));
        g.fillEllipse (x, y, s, s);
    }

    if (cat.contains ("INIT"))
    {
        g.setColour (Theme::textSecondary.withAlpha (0.4f));
        g.drawEllipse (juce::Rectangle<float> (R * 0.8f, R * 0.8f).withCentre (c), 1.0f);
        draw::glowDot (g, c, 2.0f, Theme::ivory, 0.6f);
        return;
    }
    if (cat.contains ("BASS"))
    {
        // heavy low wave
        juce::Path p;
        for (int i = 0; i <= 48; ++i)
        {
            const float u = (float) i / 48.0f;
            const float y = c.y + R * 0.45f * std::sin (u * 4.2f + (float) seed) * std::sin (u * juce::MathConstants<float>::pi);
            if (i == 0) p.startNewSubPath (a.getX() + u * a.getWidth(), y); else p.lineTo (a.getX() + u * a.getWidth(), y);
        }
        draw::glowPath (g, p, tintA, 2.0f, 10.0f, 0.5f + 0.4f * lit);
        return;
    }
    if (cat.contains ("KEY") || cat.contains ("BELL"))
    {
        // crystal shards
        for (int i = 0; i < 7; ++i)
        {
            juce::Path shard;
            const juce::Point<float> p (c.x + (rng.nextFloat() * 2.0f - 1.0f) * R * 0.7f, c.y + (rng.nextFloat() * 2.0f - 1.0f) * R * 0.5f);
            const float s = R * (0.12f + 0.25f * rng.nextFloat());
            const float rot = rng.nextFloat() * 6.28f;
            for (int k = 0; k < 4; ++k)
            {
                const float ang = rot + (float) k * 1.57f;
                const float kr = s * (k % 2 == 0 ? 1.0f : 0.45f);
                const juce::Point<float> v (p.x + std::cos (ang) * kr, p.y + std::sin (ang) * kr);
                if (k == 0) shard.startNewSubPath (v); else shard.lineTo (v);
            }
            shard.closeSubPath();
            g.setColour (tintA.withAlpha (0.16f));
            g.fillPath (shard);
            g.setColour (tintB.withAlpha (0.55f));
            g.strokePath (shard, juce::PathStrokeType (0.9f));
        }
        return;
    }
    if (cat.contains ("PERC") || cat.contains ("DRUM"))
    {
        juce::Path burst;
        for (int i = 0; i < 18; ++i)
        {
            const float ang = rng.nextFloat() * 6.28f, len = R * (0.3f + 0.6f * rng.nextFloat());
            burst.startNewSubPath (c.x + std::cos (ang) * R * 0.08f, c.y + std::sin (ang) * R * 0.08f);
            burst.lineTo (c.x + std::cos (ang) * len, c.y + std::sin (ang) * len);
        }
        draw::glowPath (g, burst, tintA, 1.0f, 5.0f, 0.5f);
        draw::glowDot (g, c, 2.5f, Theme::textPrimary, 0.8f);
        return;
    }
    // default: PAD / TEXTURE — layered blobs
    for (int layer = 0; layer < 3; ++layer)
    {
        juce::Path blob;
        const float radius = R * (0.75f - 0.18f * (float) layer);
        const float ph = rng.nextFloat() * 6.28f;
        for (int i = 0; i <= 48; ++i)
        {
            const float ang = (float) i / 48.0f * 6.28f;
            const float r = radius * (1.0f + 0.12f * std::sin (ang * 3.0f + ph) + 0.06f * std::sin (ang * 5.0f + ph * 2.0f));
            const juce::Point<float> v (c.x + std::cos (ang) * r * 1.25f, c.y + std::sin (ang) * r * 0.8f);
            if (i == 0) blob.startNewSubPath (v); else blob.lineTo (v);
        }
        blob.closeSubPath();
        const auto col = layer == 0 ? tintA : (layer == 1 ? tintB : Theme::ivory);
        g.setColour (col.withAlpha (0.08f));
        g.fillPath (blob);
        g.setColour (col.withAlpha (0.45f));
        g.strokePath (blob, juce::PathStrokeType (1.0f));
    }
}

void AMPresetCard::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (2.0f);
    const float corner = juce::jmin (12.0f, b.getWidth() * 0.06f);
    const float on = lit.value;
    const float hv = hover.value;

    if (on > 0.02f) draw::glowRoundedRect (g, b, corner, accent, 16.0f, 0.55f * on);
    else if (hv > 0.02f) draw::glowRoundedRect (g, b, corner, accent, 10.0f, 0.25f * hv);
    draw::panelSurface (g, b, corner);
    if (on > 0.02f)
    {
        g.setColour (accent.withAlpha (0.55f * on));
        g.drawRoundedRectangle (b.reduced (0.5f), corner, 1.0f);
    }

    auto area = b.reduced (juce::jmin (12.0f, b.getWidth() * 0.06f));
    auto art = area.removeFromTop (area.getHeight() * 0.5f);
    drawArt (g, art, category, name.hashCode() & 0x7fff, juce::jmax (on, hv * 0.5f), corner * 0.7f);

    area.removeFromTop (juce::jmin (10.0f, area.getHeight() * 0.1f));
    const float nameH = juce::jlimit (9.0f, 14.0f, area.getHeight() * 0.22f);
    auto nameArea = area.removeFromTop (nameH * 1.5f);
    draw::trackedText (g, name, nameArea, juce::Justification::centredLeft, Theme::displayFont (nameH, 0.14f),
                       Theme::textPrimary.interpolatedWith (accent, 0.3f * on));
    if (on > 0.5f)
    {
        auto check = nameArea.removeFromRight (nameH * 1.4f).withSizeKeepingCentre (nameH, nameH);
        draw::glowEllipse (g, check, accent, 6.0f, 0.5f);
        g.setColour (accent);
        g.fillEllipse (check);
        juce::Path tick; tick.startNewSubPath (check.getX() + check.getWidth() * 0.25f, check.getCentreY());
        tick.lineTo (check.getX() + check.getWidth() * 0.45f, check.getBottom() - check.getHeight() * 0.28f);
        tick.lineTo (check.getRight() - check.getWidth() * 0.22f, check.getY() + check.getHeight() * 0.3f);
        g.setColour (Theme::background);
        g.strokePath (tick, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    const float capH = juce::jlimit (7.5f, 10.0f, nameH * 0.72f);
    auto capArea = area.removeFromTop (capH * 1.6f);
    draw::trackedText (g, category, capArea, juce::Justification::centredLeft, Theme::captionFont (capH), Theme::textSecondary);

    // tag chips
    if (area.getHeight() > capH * 1.8f)
    {
        auto chipRow = area.removeFromBottom (juce::jmin (area.getHeight(), capH * 2.2f));
        const auto font = Theme::captionFont (capH * 0.9f);
        float x = chipRow.getX();
        for (const auto& tag : tags)
        {
            const float w = draw::trackedTextWidth (font, tag) + capH * 1.6f;
            if (x + w > chipRow.getRight()) break;
            auto chip = juce::Rectangle<float> (x, chipRow.getY(), w, chipRow.getHeight());
            g.setColour (Theme::glass);
            g.fillRoundedRectangle (chip, chip.getHeight() * 0.5f);
            g.setColour (Theme::border);
            g.drawRoundedRectangle (chip.reduced (0.5f), chip.getHeight() * 0.5f, 1.0f);
            draw::trackedText (g, tag, chip, juce::Justification::centred, font, Theme::textSecondary);
            x += w + 5.0f;
        }
    }
}

} // namespace am::ui

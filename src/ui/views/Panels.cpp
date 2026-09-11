#include "Panels.h"
#include "ui/UILayout.h"

namespace am::ui
{

namespace
{
    constexpr float kTwoPi = juce::MathConstants<float>::twoPi;

    /** One sample of a wavetable frame: a harmonic stack whose weights depend on the table. */
    float tableSample (int table, float frame, float p)
    {
        const float t = juce::jlimit (0.0f, 1.0f, frame);
        const int harmonics = 2 + (int) (10.0f * t);
        float y = 0.0f, norm = 0.0f;
        for (int h = 1; h <= harmonics; ++h)
        {
            const float fh = (float) h;
            float w = 0.0f;
            switch (table % 8)
            {
                case 0: w = h == 1 ? 1.0f : 0.0f; break;                                   // BASIC
                case 1: w = 1.0f / fh; break;                                              // HARMONIC
                case 2: w = std::exp (-std::abs (fh - (1.0f + 5.0f * t)) * 0.7f); break;   // FORMANT
                case 3: w = (h % 2 == 1 ? 1.0f : -0.35f) / fh; break;                      // FOLDED
                case 4: w = (h % 3 == 0 ? 1.0f : 0.25f) / std::sqrt (fh); break;           // METALLIC
                case 5: w = std::sin (fh * (1.0f + 3.0f * t)) / fh; break;                 // SPECTRAL
                case 6: w = ((h * 2654435761u) % 97 > 40 ? 1.0f : -1.0f) / fh; break;      // FRACTURED
                default: w = std::sin (fh * 12.9898f) / std::sqrt (fh); break;             // NOISE
            }
            y += w * std::sin (kTwoPi * p * fh + (float) (h * h) * 0.21f * t);
            norm += std::abs (w);
        }
        return y / juce::jmax (0.35f, norm);
    }

    struct Point3 { float x = 0.0f, y = 0.0f, z = 0.0f; };

    /** A point on the unit sphere for index i of n, spiralled so the nodes spread evenly. */
    Point3 spherePoint (int i, int n)
    {
        const float y = 1.0f - 2.0f * ((float) i + 0.5f) / (float) juce::jmax (1, n);
        const float r = std::sqrt (juce::jmax (0.0f, 1.0f - y * y));
        const float theta = 2.39996323f * (float) i;                      // golden angle
        return { r * std::cos (theta), y, r * std::sin (theta) };
    }
}

//==============================================================================
AMSidebar::AMSidebar (std::vector<Item> entries, juce::Colour accentColour)
    : items (std::move (entries)), accent (accentColour)
{
    setWantsKeyboardFocus (false);
}

juce::Colour AMSidebar::accentFor (int index) const noexcept
{
    if (index < 0 || index >= (int) items.size()) return accent;
    return items[(size_t) index].accent.isTransparent() ? accent : items[(size_t) index].accent;
}

float AMSidebar::rowHeight() const noexcept
{
    const int n = juce::jmax (1, (int) items.size());
    const float natural = juce::jlimit (30.0f, 64.0f, (float) getWidth() * 0.24f);
    return juce::jmin (natural, (float) getHeight() / (float) n);
}

int AMSidebar::preferredHeight (int width) const noexcept
{
    const int n = juce::jmax (1, (int) items.size());
    return juce::roundToInt (juce::jlimit (30.0f, 64.0f, (float) width * 0.24f)) * n;
}

juce::Rectangle<float> AMSidebar::pillBounds (int index) const
{
    const float h = rowHeight();
    const float total = h * (float) items.size();
    const float top = juce::jmax (0.0f, ((float) getHeight() - total) * 0.5f);
    const float pad = juce::jlimit (1.0f, 4.0f, h * 0.07f);
    return { 0.0f, top + h * (float) index + pad, (float) getWidth(), h - pad * 2.0f };
}

int AMSidebar::rowAt (juce::Point<int> p) const
{
    for (int i = 0; i < (int) items.size(); ++i)
        if (pillBounds (i).expanded (0.0f, 1.5f).contains (p.toFloat())) return i;
    return -1;
}

void AMSidebar::setSelected (int index, juce::NotificationType notify)
{
    index = juce::jlimit (0, juce::jmax (0, (int) items.size() - 1), index);
    if (index == selected) return;
    selected = index;
    repaint();
    if (notify != juce::dontSendNotification && onChange) onChange (selected);
}

void AMSidebar::mouseDown (const juce::MouseEvent& e)
{
    const int r = rowAt (e.getPosition());
    if (r >= 0 && r != selected) setSelected (r);
}

void AMSidebar::mouseMove (const juce::MouseEvent& e)
{
    const int r = rowAt (e.getPosition());
    if (r != hovered) { hovered = r; repaint(); }
}

void AMSidebar::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (std::abs (wheel.deltaY) < 0.01f) return;
    setSelected (selected + (wheel.deltaY > 0.0f ? -1 : 1));
}

void AMSidebar::paint (juce::Graphics& g)
{
    if (items.empty() || getWidth() < 8) return;
    for (int i = 0; i < (int) items.size(); ++i)
    {
        const auto pill = pillBounds (i);
        if (pill.getHeight() < 6.0f) continue;
        const bool sel = i == selected, hov = i == hovered;
        const auto tint = accentFor (i);
        const float radius = juce::jmin (pill.getHeight() * 0.34f, 12.0f);

        if (sel)
        {
            draw::contactShadow (g, pill, radius, juce::jmax (4.0f, pill.getHeight() * 0.22f), 0.85f);
            draw::SlabStyle style;
            style.top = Theme::panelTop;
            style.bottom = Theme::panel.brighter (0.25f);
            style.shadow = 0.0f;
            style.brush = 0.35f;
            draw::raisedSlab (g, pill, radius, style);
            // The accent runs down the left edge of the raised slab.
            auto bar = pill.withWidth (juce::jmax (2.5f, pill.getHeight() * 0.075f));
            draw::glowRoundedRect (g, bar, bar.getWidth() * 0.5f, tint, 8.0f, 0.55f);
            g.setColour (tint);
            g.fillRoundedRectangle (bar.reduced (0.0f, radius * 0.25f), bar.getWidth() * 0.5f);
        }
        else if (hov)
        {
            g.setColour (juce::Colours::white.withAlpha (0.55f));
            g.fillRoundedRectangle (pill, radius);
            g.setColour (tint.withAlpha (0.22f));
            g.drawRoundedRectangle (pill.reduced (0.5f), radius, 1.0f);
        }

        // Glyph, then the label (and its caption when the row is tall enough for two lines).
        auto row = pill.reduced (juce::jmax (6.0f, pill.getHeight() * 0.22f), 0.0f);
        const float d = juce::jmin (row.getHeight() * 0.54f, row.getWidth() * 0.32f);
        auto glyph = row.removeFromLeft (d * 1.35f).withSizeKeepingCentre (d, d);
        row.removeFromLeft (juce::jmax (3.0f, d * 0.28f));
        Icons::draw (g, items[(size_t) i].icon, glyph,
                     sel ? tint : Theme::textDim.interpolatedWith (tint, hov ? 0.45f : 0.0f), sel ? 1.05f : 0.85f);

        const bool twoLine = items[(size_t) i].caption.isNotEmpty() && pill.getHeight() > 34.0f && row.getWidth() > 54.0f;
        const float labelH = juce::jlimit (9.0f, 13.0f, pill.getHeight() * (twoLine ? 0.26f : 0.30f));
        auto text = twoLine ? row.removeFromTop (row.getHeight() * 0.56f) : row;
        draw::trackedText (g, items[(size_t) i].label, text, juce::Justification::centredLeft,
                           sel ? Theme::labelFontStrong (labelH) : Theme::labelFont (labelH),
                           sel ? Theme::textPrimary : Theme::textSecondary);
        if (twoLine)
            draw::trackedText (g, items[(size_t) i].caption, row, juce::Justification::centredLeft,
                               Theme::captionFont (juce::jlimit (7.5f, 10.0f, labelH * 0.78f)),
                               sel ? tint.withAlpha (0.9f) : Theme::textDim.withAlpha (0.8f));
    }
}

//==============================================================================
float PageDisplay::corner() const noexcept
{
    const auto b = getLocalBounds().toFloat();
    return juce::jlimit (5.0f, 16.0f, juce::jmin (b.getWidth(), b.getHeight()) * 0.035f);
}

void PageDisplay::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    if (b.getWidth() < 8.0f || b.getHeight() < 8.0f) return;
    const float c = corner();

    if (paintsOwnGround())
    {
        paintArt (g, b);
    }
    else
    {
        // The recess: near-black, cut into the pale chassis, lit from the top left.
        draw::insetWell (g, b, c, Theme::well, 1.0f);
        {
            juce::Graphics::ScopedSaveState save (g);
            juce::Path clip;
            clip.addRoundedRectangle (b.reduced (1.2f), juce::jmax (0.0f, c - 1.2f));
            g.reduceClipRegion (clip);
            paintArt (g, b.reduced (1.2f));

            // A vignette pulls the eye to the middle of the screen.
            juce::ColourGradient vignette (juce::Colours::transparentBlack, b.getCentreX(), b.getCentreY(),
                                           juce::Colours::black.withAlpha (0.55f), b.getX(), b.getY(), true);
            g.setGradientFill (vignette);
            g.fillRect (b);
        }
        draw::screenGlass (g, b, c, 1.0f);
    }

    // Lettering: the screen names itself top-left and captions itself bottom-left.
    const float inset = juce::jmax (8.0f, juce::jmin (b.getWidth(), b.getHeight()) * 0.035f);
    const float h = juce::jlimit (8.0f, 11.5f, b.getHeight() * 0.028f);
    if (title.isNotEmpty())
        draw::trackedText (g, title, b.reduced (inset, inset * 0.8f).removeFromTop (h * 1.6f),
                           juce::Justification::topLeft, Theme::captionFont (h), accent.withAlpha (0.75f));
    if (caption.isNotEmpty())
        draw::trackedText (g, caption, b.reduced (inset, inset * 0.8f).removeFromBottom (h * 1.6f),
                           juce::Justification::bottomLeft, Theme::captionFont (h), Theme::textDim.withAlpha (0.75f));
}

//==============================================================================
void WaveMeshDisplay::setShape (int tableIndex, float pos, float scanAmount, float morphAmount)
{
    table = tableIndex;
    position = juce::jlimit (0.0f, 1.0f, pos);
    scan = juce::jlimit (0.0f, 1.0f, scanAmount);
    morph = juce::jlimit (0.0f, 1.0f, morphAmount);
}

void WaveMeshDisplay::paintArt (juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto pair = Theme::accentPair (accent);
    draw::softLight (g, { area.getCentreX(), area.getCentreY() + area.getHeight() * 0.12f },
                     area.getWidth() * 0.55f, pair.second, 0.14f + 0.10f * energy);

    const int rows = juce::jlimit (9, 20, (int) (area.getHeight() / 26.0f));
    const int cols = juce::jlimit (28, 120, (int) (area.getWidth() / 9.0f));
    const float frontY = area.getBottom() - area.getHeight() * 0.12f;
    const float backY  = area.getY() + area.getHeight() * 0.20f;
    const float travel = position + scan * 0.35f * std::sin (phase * 0.7f);

    std::vector<juce::Path> traces ((size_t) rows);
    std::vector<float> depths ((size_t) rows);

    for (int r = 0; r < rows; ++r)
    {
        const float t = (float) r / (float) (rows - 1);          // 0 back, 1 front
        const float persp = 0.52f + 0.48f * t;
        const float y = backY + (frontY - backY) * (t * t * 0.55f + t * 0.45f);
        const float w = area.getWidth() * 0.86f * persp;
        const float x0 = area.getCentreX() - w * 0.5f;
        const float amp = area.getHeight() * (0.055f + 0.075f * t) * (0.62f + 0.55f * energy);
        const float frame = std::fmod (juce::jmax (0.0f, travel + (1.0f - t) * (0.25f + 0.55f * morph)), 1.0f);
        depths[(size_t) r] = t;

        auto& p = traces[(size_t) r];
        for (int i = 0; i <= cols; ++i)
        {
            const float u = (float) i / (float) cols;
            const float v = tableSample (table, frame, u + phase * 0.06f * (0.4f + t));
            const float px = x0 + w * u;
            const float py = y - v * amp;
            if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
        }
    }

    // Ribs between the frames, so the stack reads as one surface and not as loose lines.
    const int ribs = juce::jlimit (6, 18, cols / 7);
    for (int k = 0; k <= ribs; ++k)
    {
        const float u = (float) k / (float) ribs;
        juce::Path rib;
        for (int r = 0; r < rows; ++r)
        {
            const float t = depths[(size_t) r];
            const float persp = 0.52f + 0.48f * t;
            const float y = backY + (frontY - backY) * (t * t * 0.55f + t * 0.45f);
            const float w = area.getWidth() * 0.86f * persp;
            const float amp = area.getHeight() * (0.055f + 0.075f * t) * (0.62f + 0.55f * energy);
            const float frame = std::fmod (juce::jmax (0.0f, travel + (1.0f - t) * (0.25f + 0.55f * morph)), 1.0f);
            const float v = tableSample (table, frame, u + phase * 0.06f * (0.4f + t));
            const float px = area.getCentreX() - w * 0.5f + w * u;
            const float py = y - v * amp;
            if (r == 0) rib.startNewSubPath (px, py); else rib.lineTo (px, py);
        }
        g.setColour (pair.first.withAlpha (0.13f));
        g.strokePath (rib, juce::PathStrokeType (0.8f));
    }

    for (int r = 0; r < rows; ++r)
    {
        const float t = depths[(size_t) r];
        const auto col = pair.second.interpolatedWith (pair.first, t);
        if (r == rows - 1)
            draw::glowPath (g, traces[(size_t) r], col, 1.7f, 9.0f, 0.7f + 0.3f * energy);
        else
        {
            g.setColour (col.withAlpha (0.16f + 0.55f * t * t));
            g.strokePath (traces[(size_t) r], juce::PathStrokeType (0.6f + 1.1f * t));
        }
    }
}

//==============================================================================
void LatticeDisplay::setMatter (float d, float f, float m, float t, float s, int topologyIndex, int seedValue)
{
    density = juce::jlimit (0.0f, 1.0f, d);
    form = juce::jlimit (0.0f, 1.0f, f);
    mass = juce::jlimit (0.0f, 1.0f, m);
    tension = juce::jlimit (0.0f, 1.0f, t);
    surface = juce::jlimit (0.0f, 1.0f, s);
    topology = topologyIndex;
    seed = seedValue;
}

void LatticeDisplay::paintArt (juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto pair = Theme::accentPair (accent);
    const auto c = area.getCentre();
    const float R = juce::jmin (area.getWidth(), area.getHeight()) * (0.30f + 0.08f * form);

    draw::softLight (g, c, R * 1.7f, pair.first, 0.16f + 0.12f * energy);

    const int n = juce::jlimit (10, 46, 12 + (int) (34.0f * density));
    const float spin = phase * (0.14f + 0.24f * tension);
    const float tilt = 0.42f + 0.22f * std::sin (phase * 0.23f);

    struct Node { juce::Point<float> p; float depth, r; };
    std::vector<Node> nodes ((size_t) n);
    juce::Random rng (seed * 131 + topology * 17 + 5);
    for (int i = 0; i < n; ++i)
    {
        auto v = spherePoint (i, n);
        // Surface roughens the shell; topology squashes it into its own arrangement.
        const float rough = 1.0f + surface * (rng.nextFloat() - 0.5f) * 0.55f;
        const float squash = topology % 6 == 1 ? 0.35f : (topology % 6 == 3 ? 0.8f : 1.0f);
        const float cs = std::cos (spin), sn = std::sin (spin);
        const float x = v.x * cs - v.z * sn;
        const float z = v.x * sn + v.z * cs;
        const float y = v.y * squash;
        const float depth = 0.5f + 0.5f * z;
        nodes[(size_t) i] = { { c.x + x * R * rough, c.y + (y * std::cos (tilt) + z * std::sin (tilt) * 0.35f) * R * rough },
                              depth, juce::jmax (1.0f, R * (0.020f + 0.030f * depth) * (0.7f + 0.8f * mass)) };
    }

    // Couplings: every pair close enough to be bound, drawn back to front.
    const float link = R * (0.55f + 0.55f * form) * (0.65f + 0.6f * tension);
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
        {
            const float dist = nodes[(size_t) i].p.getDistanceFrom (nodes[(size_t) j].p);
            if (dist > link) continue;
            const float strength = 1.0f - dist / link;
            const float depth = 0.5f * (nodes[(size_t) i].depth + nodes[(size_t) j].depth);
            g.setColour (pair.first.interpolatedWith (pair.second, depth)
                             .withAlpha (0.06f + 0.42f * strength * strength * depth));
            g.drawLine (nodes[(size_t) i].p.x, nodes[(size_t) i].p.y, nodes[(size_t) j].p.x, nodes[(size_t) j].p.y,
                        0.5f + 1.2f * strength * depth);
        }

    // The dark core opens with Mass.
    const float core = R * (0.62f - 0.34f * mass);
    if (core > 2.0f)
    {
        juce::ColourGradient hole (juce::Colours::black.withAlpha (0.85f), c.x, c.y,
                                   juce::Colours::transparentBlack, c.x + core, c.y, true);
        g.setGradientFill (hole);
        g.fillEllipse (juce::Rectangle<float> (core * 2.0f, core * 2.0f).withCentre (c));
    }

    for (const auto& node : nodes)
    {
        const auto col = pair.second.interpolatedWith (pair.first, node.depth);
        draw::glowDot (g, node.p, node.r, col, 0.35f + 0.65f * node.depth * (0.6f + 0.6f * energy));
    }
}

//==============================================================================
void RibbonDisplay::setOperators (float b, float m, float t, float mag, float sp)
{
    bend = juce::jlimit (0.0f, 1.0f, b);
    melt = juce::jlimit (0.0f, 1.0f, m);
    tear = juce::jlimit (0.0f, 1.0f, t);
    magnet = juce::jlimit (0.0f, 1.0f, mag);
    speed = juce::jlimit (0.0f, 1.0f, sp);
}

void RibbonDisplay::paintArt (juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto pair = Theme::accentPair (accent);
    draw::softLight (g, area.getCentre(), area.getWidth() * 0.5f, pair.first, 0.15f + 0.10f * energy);

    const int lines = juce::jlimit (10, 26, (int) (area.getHeight() / 20.0f));
    const int cols = juce::jlimit (36, 140, (int) (area.getWidth() / 7.0f));
    const float clock = phase * (0.25f + 1.5f * speed);
    const float midY = area.getCentreY();
    const float band = area.getHeight() * 0.33f;

    auto heightAt = [&] (float u, float v) -> float
    {
        // v: 0..1 across the ribbon's depth. Each operator deforms it its own way.
        float y = std::sin (kTwoPi * (u * 1.3f + v * 0.35f) - clock) * 0.5f;
        y += 0.32f * std::sin (kTwoPi * (u * 2.7f - v * 0.6f) + clock * 0.62f);
        y *= 1.0f - 0.45f * melt;                                                  // MELT flattens and blurs
        y += bend * 1.15f * (u - 0.5f) * (u - 0.5f) * 2.4f - bend * 0.35f;         // BEND folds it over
        if (tear > 0.02f)                                                          // TEAR splits it in two
        {
            const float side = u < 0.5f ? -1.0f : 1.0f;
            y += side * tear * 0.55f * (0.4f + 0.6f * std::abs (u - 0.5f) * 2.0f);
        }
        if (magnet > 0.02f)                                                        // MAGNET snaps it to steps
        {
            const float steps = 5.0f;
            y = y * (1.0f - magnet) + magnet * std::round (y * steps) / steps;
        }
        return y;
    };

    std::vector<juce::Path> ribs ((size_t) lines);
    for (int r = 0; r < lines; ++r)
    {
        const float v = (float) r / (float) (lines - 1);
        const float persp = 0.55f + 0.45f * v;
        const float w = area.getWidth() * 0.9f * persp;
        const float x0 = area.getCentreX() - w * 0.5f;
        const float baseY = midY + (v - 0.5f) * band * 1.5f;
        auto& p = ribs[(size_t) r];
        for (int i = 0; i <= cols; ++i)
        {
            const float u = (float) i / (float) cols;
            const float px = x0 + w * u;
            const float py = baseY - heightAt (u, v) * band * (0.55f + 0.45f * v) * (0.7f + 0.5f * energy);
            if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
        }
    }

    // Fill between neighbours so the lines read as one surface, then stroke them.
    for (int r = 0; r + 1 < lines; ++r)
    {
        juce::Path band2 (ribs[(size_t) r]);
        juce::Path back (ribs[(size_t) r + 1]);
        back.applyTransform (juce::AffineTransform::scale (-1.0f, 1.0f, area.getCentreX(), 0.0f));
        band2.addPath (back);
        band2.closeSubPath();
        g.setColour (pair.first.withAlpha (0.035f));
        g.fillPath (band2);
    }
    for (int r = 0; r < lines; ++r)
    {
        const float v = (float) r / (float) (lines - 1);
        const auto col = pair.second.interpolatedWith (pair.first, v);
        if (r == lines - 1 || r == 0)
            draw::glowPath (g, ribs[(size_t) r], col, 1.4f, 8.0f, 0.45f + 0.35f * energy);
        else
        {
            g.setColour (col.withAlpha (0.14f + 0.46f * v));
            g.strokePath (ribs[(size_t) r], juce::PathStrokeType (0.6f + 0.9f * v));
        }
    }
}

//==============================================================================
void ShardDisplay::setFracture (int frags, float amt, float spr, float rnd, bool isOn, int seedValue)
{
    fragments = juce::jlimit (2, 64, frags);
    amount = juce::jlimit (0.0f, 1.0f, amt);
    spread = juce::jlimit (0.0f, 1.0f, spr);
    random = juce::jlimit (0.0f, 1.0f, rnd);
    on = isOn;
    seed = seedValue;
}

void ShardDisplay::paintArt (juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto pair = Theme::accentPair (accent);
    const auto c = area.getCentre();
    const float R = juce::jmin (area.getWidth(), area.getHeight()) * 0.34f;
    const float live = on ? 1.0f : 0.34f;

    draw::softLight (g, c, R * 1.9f, pair.first, (0.10f + 0.12f * energy) * live);

    const int n = juce::jlimit (6, 48, fragments);
    juce::Random rng (seed * 977 + n);
    const float burst = (0.10f + 0.95f * amount * spread) * live;

    for (int i = 0; i < n; ++i)
    {
        const float a0 = kTwoPi * (float) i / (float) n;
        const float wedge = kTwoPi / (float) n;
        const float jitter = (rng.nextFloat() - 0.5f) * random * wedge * 1.6f;
        const float drift = 0.55f + 0.9f * rng.nextFloat();
        // Each shard flies out along its own angle, breathing on the Evolve clock.
        const float push = burst * drift * (0.72f + 0.28f * std::sin (phase * (0.5f + drift) + (float) i));
        const float inner = R * (0.30f + 0.22f * rng.nextFloat()) * (1.0f - 0.35f * amount);
        const float outer = R * (0.82f + 0.35f * rng.nextFloat());
        const float ox = std::cos (a0 + jitter) * R * push;
        const float oy = std::sin (a0 + jitter) * R * push;

        juce::Path shard;
        const float a1 = a0 - wedge * 0.44f, a2 = a0 + wedge * 0.44f;
        shard.startNewSubPath (c.x + ox + std::cos (a0) * inner, c.y + oy + std::sin (a0) * inner);
        shard.lineTo (c.x + ox + std::cos (a1) * outer, c.y + oy + std::sin (a1) * outer);
        shard.lineTo (c.x + ox + std::cos (a0) * outer * (1.0f + 0.22f * rng.nextFloat()),
                      c.y + oy + std::sin (a0) * outer * (1.0f + 0.22f * rng.nextFloat()));
        shard.lineTo (c.x + ox + std::cos (a2) * outer, c.y + oy + std::sin (a2) * outer);
        shard.closeSubPath();

        const float t = (float) i / (float) n;
        const auto col = pair.first.interpolatedWith (pair.second, t);
        g.setColour (col.withAlpha (0.06f * live + 0.10f * energy));
        g.fillPath (shard);
        g.setColour (col.withAlpha ((0.30f + 0.45f * (1.0f - push)) * live));
        g.strokePath (shard, juce::PathStrokeType (juce::jmax (0.7f, R * 0.012f)));

        // The trail back to where the shard came from.
        if (push > 0.05f)
        {
            juce::Path trail;
            trail.startNewSubPath (c.x + std::cos (a0) * inner, c.y + std::sin (a0) * inner);
            trail.lineTo (c.x + ox + std::cos (a0) * inner, c.y + oy + std::sin (a0) * inner);
            g.setColour (col.withAlpha (0.16f * live));
            g.strokePath (trail, juce::PathStrokeType (0.7f));
        }
    }

    // What is left of the object at the centre.
    const float coreR = R * (0.24f - 0.15f * amount) + 2.0f;
    draw::glowEllipse (g, juce::Rectangle<float> (coreR * 2.0f, coreR * 2.0f).withCentre (c),
                       pair.second, coreR * 2.2f, (0.5f + 0.5f * energy) * live);
}

//==============================================================================
void SpaceDisplay::paintArt (juce::Graphics& g, juce::Rectangle<float> area)
{
    SpaceArt::draw (g, area, type, phase, energy, corner());
}

//==============================================================================
AMModuleTile::AMModuleTile (const juce::String& text, Icon icon, juce::Colour accentColour)
    : label (text.toUpperCase()), glyph (icon), accent (accentColour)
{
    setWantsKeyboardFocus (false);
}

juce::Rectangle<float> AMModuleTile::pipBounds() const
{
    const auto b = getLocalBounds().toFloat();
    const float d = juce::jlimit (5.0f, 10.0f, juce::jmin (b.getWidth(), b.getHeight()) * 0.19f);
    return juce::Rectangle<float> (d, d).withCentre ({ b.getRight() - d * 0.95f, b.getY() + d * 0.95f });
}

void AMModuleTile::mouseMove (const juce::MouseEvent& e)
{
    const bool pip = hasPower && pipBounds().expanded (3.0f).contains (e.position);
    if (pip != hoverPip) { hoverPip = pip; repaint(); }
}

void AMModuleTile::mouseDown (const juce::MouseEvent& e)
{
    if (hasPower && pipBounds().expanded (3.0f).contains (e.position))
    {
        if (onPower) onPower (! powered);
        return;
    }
    if (onSelect) onSelect();
}

void AMModuleTile::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    if (b.getWidth() < 8.0f || b.getHeight() < 8.0f) return;
    const float radius = juce::jmin (b.getWidth(), b.getHeight()) * 0.22f;
    const float labelH = juce::jlimit (7.0f, 10.0f, b.getHeight() * 0.22f);

    draw::insetWell (g, b, radius, Theme::panelInset, 0.7f);
    if (selected)
    {
        draw::contactShadow (g, b, radius, b.getHeight() * 0.16f, 0.7f);
        draw::SlabStyle style;
        style.top = Theme::panelTop;
        style.bottom = Theme::panel.brighter (0.2f);
        style.shadow = 0.0f;
        draw::raisedSlab (g, b, radius, style);
        g.setColour (accent.withAlpha (0.55f));
        g.drawRoundedRectangle (b.reduced (0.6f), radius, 1.0f);
    }
    else if (hovered)
    {
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.fillRoundedRectangle (b, radius);
    }

    auto icon = b.withTrimmedBottom (labelH + 2.0f).reduced (b.getWidth() * 0.26f, b.getHeight() * 0.14f);
    const float d = juce::jmin (icon.getWidth(), icon.getHeight());
    icon = icon.withSizeKeepingCentre (d, d);
    if (powered) draw::glowEllipse (g, icon, accent, d * 0.55f, 0.35f);
    Icons::draw (g, glyph, icon, powered ? accent : Theme::textDim, powered ? 1.0f : 0.8f);

    draw::trackedText (g, label, b.removeFromBottom (labelH + 1.0f), juce::Justification::centredTop,
                       powered ? Theme::labelFontStrong (labelH * 0.84f) : Theme::labelFont (labelH * 0.84f),
                       powered ? Theme::textPrimary : Theme::textDim);

    if (hasPower)
    {
        const auto pip = pipBounds();
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillEllipse (pip.expanded (1.0f));
        if (powered) draw::glowDot (g, pip.getCentre(), pip.getWidth() * 0.5f, Theme::amber, 1.0f);
        else
        {
            g.setColour (Theme::textDim.withAlpha (hoverPip ? 0.85f : 0.45f));
            g.drawEllipse (pip.reduced (0.5f), 1.0f);
        }
    }
}


//==============================================================================
SourcePanel::SourcePanel (AntiMatrProcessor& p)
    : AMPanel ("Source", "Choose your energy", Theme::blue),
      processor (p),
      selector ({ { "Wave", Icon::Wave, Theme::violet }, { "Dust", Icon::Dust, Theme::blue },
                  { "Impact", Icon::Impact, Theme::cyan }, { "Sample", Icon::Sample, Theme::ivory },
                  { "Gesture", Icon::Gesture, Theme::magenta } })
{
    addAndMakeVisible (selector);
    addAndMakeVisible (wave);
    selector.setTooltip ("Source: what creates the energy");

    auto* param = processor.parameters().getParameter (ParameterRegistry::get (Param::sourceSelected).id);
    selectorAttachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
    {
        selector.setSelected ((int) std::lround (v), juce::dontSendNotification);
        rebuildKnobs ((int) std::lround (v));
    });
    selector.onChange = [this] (int i) { selectorAttachment->setValueAsCompleteGesture ((float) i); };
    selectorAttachment->sendInitialUpdate();
    if (currentSource < 0) rebuildKnobs (0);

    wave.onArrow = [this] (int dir)
    {
        static const Param modeParams[] = { Param::waveTable, Param::dustMode, Param::impactMode, Param::sampleMode, Param::gestureMode };
        const Param mp = modeParams[juce::jlimit (0, 4, currentSource)];
        auto* modeParam = processor.parameters().getParameter (ParameterRegistry::get (mp).id);
        const int n = ParameterRegistry::get (mp).numChoices();
        const int cur = (int) std::lround (modeParam->convertFrom0to1 (modeParam->getValue()));
        const int next = ((cur + dir) % n + n) % n;
        modeParam->setValueNotifyingHost (modeParam->convertTo0to1 ((float) next));
    };

    startTimerHz (30);
}

void SourcePanel::rebuildKnobs (int sourceIndex)
{
    if (sourceIndex == currentSource) return;
    currentSource = sourceIndex;
    knobs.clear();
    auto& apvts = processor.parameters();
    const juce::Colour accents[] = { Theme::blue, Theme::blue, Theme::cyan, Theme::violet };
    int idx = 0;
    auto add = [&] (Param p, const juce::String& label = {}) { knobs.push_back (std::make_unique<BoundKnob> (apvts, p, accents[idx++ % 4], label)); addAndMakeVisible (knobs.back()->knob); };
    switch (sourceIndex)
    {
        case 1:  add (Param::dustDensity); add (Param::dustColor); add (Param::dustGrain); add (Param::dustSpread); break;
        case 2:  add (Param::impactHardness); add (Param::impactBrightness); add (Param::impactLength); add (Param::impactRandom); break;
        case 3:  add (Param::sampleStart); add (Param::sampleEnd); add (Param::samplePitch); add (Param::sampleSpread); break;
        case 4:  add (Param::gesturePressure); add (Param::gestureSpeed); add (Param::gestureRoughness); add (Param::gesturePosition); break;
        default: add (Param::wavePosition); add (Param::waveScan); add (Param::waveDetune); add (Param::waveSpread); break;
    }
    static const juce::Colour waveAccents[] = { Theme::violet, Theme::blue, Theme::cyan, Theme::ivory, Theme::magenta };
    wave.setAccent (waveAccents[juce::jlimit (0, 4, sourceIndex)]);
    resized();
}

void SourcePanel::resized()
{
    auto area = contentBounds();
    const int gap = juce::jmax (4, area.getHeight() / 40);
    auto selectorArea = area.removeFromTop (juce::jmin (juce::roundToInt ((float) area.getHeight() * 0.34f),
                                                        selector.preferredHeight (area.getWidth())));
    area.removeFromTop (gap);
    auto knobArea = area.removeFromBottom (juce::roundToInt ((float) area.getHeight() * 0.42f));
    area.removeFromBottom (gap);
    selector.setBounds (selectorArea);
    wave.setBounds (area);
    std::vector<juce::Component*> comps;
    for (auto& k : knobs) comps.push_back (&k->knob);
    layoutGrid (knobArea, comps, 4);
}

void SourcePanel::timerCallback()
{
    if (! isShowing()) return;
    auto& diag = processor.diagnostics();
    const auto& vs = diag.visualSnapshots.latest();
    diag.taps[(int) Stage::Source].readLatest (tapL.data(), tapR.data(), (int) tapL.size());
    wave.setSamples (tapL.data(), (int) tapL.size());
    wave.setEnergy (juce::jlimit (0.0f, 1.0f, vs.sourceRms * 4.0f));
    selector.setEnergy (juce::jlimit (0.0f, 1.0f, vs.sourceRms * 4.0f));
    setActivity (juce::jlimit (0.0f, 1.0f, vs.sourceRms * 2.0f));

    // caption: the selected source's mode / table name (real parameter state)
    static const Param modeParams[] = { Param::waveTable, Param::dustMode, Param::impactMode, Param::sampleMode, Param::gestureMode };
    const Param mp = modeParams[juce::jlimit (0, 4, currentSource)];
    const int mode = paramChoice (processor.currentParamValues(), mp);
    const auto choices = paramChoices (mp);
    wave.setCaption (choices[juce::jlimit (0, choices.size() - 1, mode)]);

    const auto& mod = latestModulation (processor);
    for (auto& k : knobs) k->refreshModRing (mod);
}

//==============================================================================
ShapePanel::ShapePanel (AntiMatrProcessor& p)
    : AMPanel ("Shape", "Turn matter into sound", Theme::cyan), processor (p)
{
    auto& apvts = processor.parameters();
    const juce::Colour accents[] = { Theme::cyan, Theme::blue, Theme::violet, Theme::cyan, Theme::magenta, Theme::ivory };
    const Param simple[] = { Param::shapeDensity, Param::shapeForm, Param::shapeMass, Param::shapeTension, Param::shapeDecay, Param::shapeSurface };
    for (int i = 0; i < 6; ++i)
    {
        simpleKnobs.push_back (std::make_unique<BoundKnob> (apvts, simple[i], accents[i]));
        simpleKnobs.back()->knob.setHero (true);
        addAndMakeVisible (simpleKnobs.back()->knob);
    }
    const Param adv[] = { Param::shapeMaterialA, Param::shapeMaterialB, Param::shapeBlend, Param::shapeTopology,
                          Param::shapeCoupling, Param::shapeDistribution, Param::shapeExcite, Param::shapeStrike,
                          Param::shapeStereo, Param::shapeMix, Param::shapeKeytrack, Param::shapePitch };
    for (int i = 0; i < 12; ++i)
    {
        advancedControls.push_back (std::make_unique<BoundControl> (apvts, adv[i], accents[i % 6]));
        addChildComponent (advancedControls.back()->component());
    }
    addAndMakeVisible (mode);
    mode.setTooltip ("SIMPLE: the six Matter macros. ADVANCED: materials, topology, coupling.");
    mode.onChange = [this] (int i) { setAdvanced (i == 1); };
    startTimerHz (20);
}

void ShapePanel::setAdvanced (bool a)
{
    advanced = a;
    for (auto& k : simpleKnobs) k->knob.setVisible (! advanced);
    for (auto& c : advancedControls) c->component().setVisible (advanced);
    resized();
}

void ShapePanel::resized()
{
    auto area = contentBounds();
    auto modeArea = area.removeFromTop (juce::jlimit (26, 36, area.getHeight() / 10));
    mode.setBounds (modeArea.reduced (juce::jmax (4, area.getWidth() / 24), 0));
    area.removeFromTop (juce::jmax (4, area.getHeight() / 26));
    if (! advanced)
    {
        std::vector<juce::Component*> c;
        for (auto& k : simpleKnobs) c.push_back (&k->knob);
        layoutGrid (area, c, 3);
    }
    else
    {
        std::vector<juce::Component*> c;
        for (auto& k : advancedControls) c.push_back (&k->component());
        layoutGrid (area, c, 4, 4, 4);
    }
}

void ShapePanel::timerCallback()
{
    if (! isShowing()) return;
    const auto& mod = latestModulation (processor);
    if (advanced)
    {
        for (auto& c : advancedControls) c->refreshModRing (mod);
        return;
    }
    // The ring shows the routed modulation; without any, it falls back to where the engine's
    // effective (smoothed) value sits relative to the knob.
    const auto& vs = processor.diagnostics().visualSnapshots.latest();
    const float values[] = { vs.density, vs.form, vs.mass, vs.tension, vs.decay, vs.surface };
    for (int i = 0; i < 6; ++i)
        if (! simpleKnobs[(size_t) i]->refreshModRing (mod))
            simpleKnobs[(size_t) i]->knob.modRing().setCurrent (values[i]);
}

//==============================================================================
EvolvePanel::OperatorCell::OperatorCell (const juce::String& label, Icon icon) : name (label.toUpperCase()), glyph (icon)
{
    setWantsKeyboardFocus (false);
}

void EvolvePanel::OperatorCell::setSelected (bool on)
{
    if (on == selected) return;
    selected = on;
    if (isShowing()) anim.animate (lit, on ? 1.0f : 0.0f); else lit.snap (on ? 1.0f : 0.0f);
    repaint();
}

void EvolvePanel::OperatorCell::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float on = lit.value;
    const float hv = hover.value * (1.0f - on);
    const float labelH = juce::jlimit (10.0f, 16.0f, b.getHeight() * 0.24f);
    auto tile = b.reduced (b.getWidth() * 0.06f, 2.0f);
    const float corner = juce::jmin (10.0f, tile.getWidth() * 0.12f);
    auto iconArea = tile.withTrimmedBottom (labelH + 6.0f).reduced (tile.getWidth() * 0.19f, tile.getHeight() * 0.10f);
    const float d = juce::jmin (iconArea.getWidth(), iconArea.getHeight());
    iconArea = iconArea.withSizeKeepingCentre (d, d);
    const auto pair = Theme::accentPair (Theme::violet);

    // Every operator has a seat cut into the panel; the chosen one is a key raised
    // out of it and lit, so the row reads as four switches rather than three drawings.
    draw::insetWell (g, tile, corner, Theme::panelInset, 0.85f);
    if (hv > 0.02f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.045f * hv));
        g.fillRoundedRectangle (tile, corner);
        g.setColour (pair.second.withAlpha (0.18f * hv));
        g.drawRoundedRectangle (tile.reduced (0.5f), corner, 1.0f);
    }
    if (on > 0.02f)
    {
        auto key = tile.reduced (1.6f);
        const float kc = juce::jmax (1.0f, corner - 1.6f);
        draw::glowRoundedRect (g, key, kc, pair.second, 12.0f, 0.45f * on);

        draw::SlabStyle style;
        style.top    = Theme::panelTop.brighter (0.14f);
        style.bottom = Theme::panel;
        style.shadow = 0.5f * on;
        style.brush  = 0.6f;
        draw::raisedSlab (g, key, kc, style);

        draw::gradientCapsule (g, key.reduced (0.8f), kc - 0.8f, pair.first, pair.second, 0.26f * on);
        g.setColour (pair.second.withAlpha (0.5f * on));
        g.drawRoundedRectangle (key.reduced (0.8f), kc - 0.8f, 1.0f);
    }

    // icon with a glow proportional to the operator amount (real value) and selection
    const float glow = 0.35f * amount + 0.45f * on + 0.15f * hv;
    if (glow > 0.03f) draw::glowEllipse (g, iconArea, Theme::violet, d * 0.45f, glow);
    const auto col = Theme::textSecondary.interpolatedWith (Theme::textPrimary, juce::jmax (on, hv * 0.6f, amount * 0.6f));
    Icons::draw (g, glyph, iconArea, col, 0.85f);

    // amount bar beneath the icon
    {
        auto bar = juce::Rectangle<float> (tile.getX() + tile.getWidth() * 0.25f, iconArea.getBottom() + 3.0f, tile.getWidth() * 0.5f, 2.0f);
        g.setColour (Theme::knobTrack);
        g.fillRoundedRectangle (bar, 1.0f);
        if (amount > 0.01f)
        {
            auto litBar = bar.withWidth (bar.getWidth() * amount);
            draw::glowRoundedRect (g, litBar, 1.0f, Theme::violet, 4.0f, 0.5f);
            g.setColour (Theme::violet.withAlpha (0.9f));
            g.fillRoundedRectangle (litBar, 1.0f);
        }
    }

    const float h = juce::jlimit (8.5f, 11.5f, labelH * 0.66f);
    draw::trackedText (g, name, b.withTop (b.getBottom() - labelH), juce::Justification::centredTop,
                       on > 0.5f ? Theme::labelFontStrong (h) : Theme::labelFont (h), col);
}

Param EvolvePanel::operatorParam (int index)
{
    switch (index) { case 1: return Param::evolveMelt; case 2: return Param::evolveTear; case 3: return Param::evolveMagnet; default: return Param::evolveBend; }
}

EvolvePanel::EvolvePanel (AntiMatrProcessor& p)
    : AMPanel ("Evolve", "Movement & change", Theme::violet), processor (p)
{
    const juce::String names[] = { "Bend", "Melt", "Tear", "Magnet" };
    const Icon icons[] = { Icon::Bend, Icon::Melt, Icon::Tear, Icon::Magnet };
    const char* tips[] = { "BEND: deform the partial structure", "MELT: diffuse and blur the matter", "TEAR: separate the object into parts", "MAGNET: align partials to a target" };
    for (int i = 0; i < 4; ++i)
    {
        cells[(size_t) i] = std::make_unique<OperatorCell> (names[i], icons[i]);
        cells[(size_t) i]->onClick = [this, i] { selectOperator (i, false); };
        cells[(size_t) i]->setTooltip (tips[i]);
        addAndMakeVisible (*cells[(size_t) i]);
    }
    addAndMakeVisible (amount);
    addAndMakeVisible (speed);
    speed.setTooltip (paramTooltip (Param::evolveSpeed));
    auto& apvts = processor.parameters();
    speedAttachment = std::make_unique<SliderAttachment> (apvts, ParameterRegistry::get (Param::evolveSpeed).id, speed);
    speed.setDoubleClickReturnValue (true, ParameterRegistry::get (Param::evolveSpeed).defaultValue);

    auto* selParam = apvts.getParameter (ParameterRegistry::get (Param::evolveSelected).id);
    selectedAttachment = std::make_unique<juce::ParameterAttachment> (*selParam, [this] (float v) { selectOperator ((int) std::lround (v), true); });
    selectedAttachment->sendInitialUpdate();
    if (amountAttachment == nullptr) selectOperator (0, true);
    startTimerHz (20);
}

void EvolvePanel::selectOperator (int index, bool fromParameter)
{
    index = juce::jlimit (0, 3, index);
    if (! fromParameter)
    {
        selectedAttachment->setValueAsCompleteGesture ((float) index);
        return; // the attachment callback re-enters with fromParameter = true
    }
    selectedOperator = index;
    for (int i = 0; i < 4; ++i) cells[(size_t) i]->setSelected (i == index);
    amountAttachment.reset();
    amountAttachment = std::make_unique<SliderAttachment> (processor.parameters(), ParameterRegistry::get (operatorParam (index)).id, amount);
    amount.setDoubleClickReturnValue (true, ParameterRegistry::get (operatorParam (index)).defaultValue);
    amount.setTooltip (paramTooltip (operatorParam (index)));
    amount.setLabel ("Amount");
}

void EvolvePanel::timerCallback()
{
    if (! isShowing()) return;
    const auto values = processor.currentParamValues();
    float total = 0.0f;
    for (int i = 0; i < 4; ++i)
    {
        const float v = paramValue (values, operatorParam (i));
        cells[(size_t) i]->setAmount (v);
        total += v;
    }
    setActivity (juce::jlimit (0.0f, 1.0f, total * 0.5f));
}

void EvolvePanel::resized()
{
    auto area = contentBounds();
    const int gap = juce::jmax (4, area.getHeight() / 30);
    auto cellArea = area.removeFromTop (juce::roundToInt ((float) area.getHeight() * 0.58f));
    layoutKnobRow (cellArea, { cells[0].get(), cells[1].get(), cells[2].get(), cells[3].get() });
    area.removeFromTop (gap);
    const int sliderH = juce::jmax (18, area.getHeight() / 2 - gap / 2);
    amount.setBounds (area.removeFromTop (sliderH));
    area.removeFromTop (gap);
    speed.setBounds (area.removeFromTop (sliderH));
}

//==============================================================================
FracturePanel::FracturePanel (AntiMatrProcessor& p)
    : AMPanel ("Fracture", "Break into new realities", Theme::magenta), processor (p)
{
    addAndMakeVisible (onOff);
    addAndMakeVisible (spectrum);
    onOff.setTooltip ("Fracture on / off");
    auto& apvts = processor.parameters();
    auto* onParam = apvts.getParameter (ParameterRegistry::get (Param::fractureOn).id);
    onAttachment = std::make_unique<juce::ParameterAttachment> (*onParam, [this] (float v) { onOff.setSelected (v >= 0.5f ? 1 : 0, juce::dontSendNotification); });
    onOff.onChange = [this] (int i) { onAttachment->setValueAsCompleteGesture (i == 1 ? 1.0f : 0.0f); };
    onAttachment->sendInitialUpdate();

    const juce::Colour accents[] = { Theme::magenta, Theme::cyan, Theme::violet, Theme::ivory };
    const Param params[] = { Param::fractureAmount, Param::fractureSpread, Param::fractureSequence, Param::fractureRandom };
    for (int i = 0; i < 4; ++i)
    {
        knobs.push_back (std::make_unique<BoundKnob> (apvts, params[i], accents[i]));
        addAndMakeVisible (knobs.back()->knob);
    }
    startTimerHz (30);
}

void FracturePanel::resized()
{
    auto header = headerRightBounds();
    const int w = juce::jlimit (84, 130, header.getWidth() / 2);
    onOff.setBounds (header.removeFromRight (w).withSizeKeepingCentre (w, juce::jlimit (22, 30, header.getHeight() - 8)));

    auto area = contentBounds();
    const int gap = juce::jmax (4, area.getHeight() / 30);
    auto knobArea = area.removeFromBottom (juce::roundToInt ((float) area.getHeight() * 0.42f));
    area.removeFromBottom (gap);
    spectrum.setBounds (area);
    layoutKnobRow (knobArea, { &knobs[0]->knob, &knobs[1]->knob, &knobs[2]->knob, &knobs[3]->knob });
}

void FracturePanel::timerCallback()
{
    if (! isShowing()) return;
    auto& diag = processor.diagnostics();
    const auto& vs = diag.visualSnapshots.latest();
    const auto stage = vs.fractureOn ? Stage::PostFracture : Stage::PostMatter;
    diag.taps[(int) stage].readLatest (tapL.data(), tapR.data(), SpectrumAnalyzer::kSize);
    for (int i = 0; i < SpectrumAnalyzer::kSize; ++i) tapL[(size_t) i] = 0.5f * (tapL[(size_t) i] + tapR[(size_t) i]);
    analyzer.compute (tapL.data(), processor.engine().sampleRate(), bands.data(), AMSpectrumView::kBands);
    spectrum.setFragmentCount (8 << juce::jlimit (0, 2, paramChoice (processor.currentParamValues(), Param::fractureFragments)));
    spectrum.setActivity (vs.fractureOn ? vs.fractureActivity : 0.0f);
    spectrum.setMagnitudes (bands.data(), AMSpectrumView::kBands);
    setActivity (vs.fractureOn ? vs.fractureActivity * 0.8f : 0.0f);

    const auto& mod = latestModulation (processor);
    for (auto& k : knobs) k->refreshModRing (mod);
}

//==============================================================================
SpacePanel::SpacePicker::SpacePicker() { setWantsKeyboardFocus (false); }

juce::Rectangle<float> SpacePanel::SpacePicker::nameBounds() const
{
    const auto b = getLocalBounds().toFloat();
    if (artOnly) return juce::Rectangle<float> (juce::jmin (b.getWidth() - 20.0f, 220.0f), 34.0f).withCentre ({ b.getCentreX(), b.getBottom() - 30.0f });
    auto left = b.withWidth (b.getWidth() * 0.6f).reduced (10.0f, 0.0f);
    return left.withSizeKeepingCentre (left.getWidth(), juce::jlimit (26.0f, 40.0f, b.getHeight() * 0.42f));
}

int SpacePanel::SpacePicker::zoneAt (juce::Point<int> p) const
{
    const auto n = nameBounds();
    if (! n.contains (p.toFloat())) return 0;
    const float zone = juce::jmin (n.getWidth() * 0.25f, 34.0f);
    if (p.x < n.getX() + zone) return -1;
    if (p.x > n.getRight() - zone) return 1;
    return 2;
}

void SpacePanel::SpacePicker::mouseMove (const juce::MouseEvent& e) { const int z = zoneAt (e.getPosition()); if (z != hoverZone) { hoverZone = z; repaint(); } }

void SpacePanel::SpacePicker::mouseDown (const juce::MouseEvent& e)
{
    const int z = zoneAt (e.getPosition());
    if (z == -1 || z == 1) { if (onArrow) onArrow (z); return; }
    if (z == 2 && onSelect)
    {
        juce::PopupMenu m;
        m.addSectionHeader ("SPACE");
        for (int i = 0; i < SpaceArt::kNumTypes; ++i) m.addItem (i + 1, SpaceArt::name (i), true, i == type);
        juce::Component::SafePointer<SpacePicker> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int r) { if (safe != nullptr && r > 0 && safe->onSelect) safe->onSelect (r - 1); });
    }
}

void SpacePanel::SpacePicker::paint (juce::Graphics& g)
{
    const int t = juce::jlimit (0, SpaceArt::kNumTypes - 1, type);
    const auto b = getLocalBounds().toFloat();
    const float corner = juce::jmin (10.0f, b.getHeight() * 0.15f);
    const auto tint = SpaceArt::tint (t);

    if (artOnly)
    {
        SpaceArt::draw (g, b, t, phase, activity, corner);
    }
    else
    {
        draw::insetSurface (g, b, corner);
        auto artArea = b.withLeft (b.getWidth() * 0.6f + 4.0f).reduced (4.0f);
        SpaceArt::draw (g, artArea, t, phase, activity, corner - 2.0f);
    }

    // Name pill with chevrons
    auto nameArea = nameBounds();
    const float pillCorner = nameArea.getHeight() * 0.5f;
    if (hoverZone != 0) draw::glowRoundedRect (g, nameArea, pillCorner, tint, 8.0f, 0.3f);
    g.setColour (artOnly ? Theme::panel.withAlpha (0.85f) : Theme::panel);
    g.fillRoundedRectangle (nameArea, pillCorner);
    g.setColour (Theme::border.withMultipliedAlpha (hoverZone != 0 ? 1.8f : 1.0f));
    g.drawRoundedRectangle (nameArea.reduced (0.5f), pillCorner, 1.0f);
    const float zone = juce::jmin (nameArea.getWidth() * 0.25f, 34.0f);
    draw::chevron (g, nameArea.withWidth (zone).reduced (zone * 0.3f, nameArea.getHeight() * 0.3f), -1, hoverZone == -1 ? tint : Theme::textSecondary);
    draw::chevron (g, nameArea.withLeft (nameArea.getRight() - zone).reduced (zone * 0.3f, nameArea.getHeight() * 0.3f), 1, hoverZone == 1 ? tint : Theme::textSecondary);
    const float h = juce::jlimit (9.0f, 13.0f, nameArea.getHeight() * 0.36f);
    draw::trackedText (g, SpaceArt::name (t), nameArea.reduced (zone, 0.0f), juce::Justification::centred, Theme::labelFontStrong (h),
                       hoverZone == 2 ? Theme::textPrimary.interpolatedWith (tint, 0.4f) : Theme::textPrimary);
}

SpacePanel::SpacePanel (AntiMatrProcessor& p)
    : AMPanel ("Space", "Place it anywhere", Theme::ivory), processor (p)
{
    addAndMakeVisible (picker);
    picker.setTooltip ("Space preset: the environment the sound lives in");
    auto& apvts = processor.parameters();
    auto* typeParam = apvts.getParameter (ParameterRegistry::get (Param::spaceType).id);
    typeAttachment = std::make_unique<juce::ParameterAttachment> (*typeParam, [this] (float v) { picker.type = (int) std::lround (v); picker.repaint(); });
    picker.onArrow = [this] (int dir) { const int n = SpaceArt::kNumTypes; typeAttachment->setValueAsCompleteGesture ((float) (((picker.type + dir) % n + n) % n)); };
    picker.onSelect = [this] (int i) { typeAttachment->setValueAsCompleteGesture ((float) i); };
    typeAttachment->sendInitialUpdate();

    const juce::Colour accents[] = { Theme::magenta, Theme::cyan, Theme::violet, Theme::ivory };
    const Param params[] = { Param::spaceMix, Param::spaceSize, Param::spaceTone, Param::spaceFeedback };
    for (int i = 0; i < 4; ++i)
    {
        knobs.push_back (std::make_unique<BoundKnob> (apvts, params[i], accents[i]));
        addAndMakeVisible (knobs.back()->knob);
    }
    startTimerHz (24);
}

void SpacePanel::resized()
{
    auto area = contentBounds();
    const int gap = juce::jmax (4, area.getHeight() / 30);
    auto knobArea = area.removeFromBottom (juce::roundToInt ((float) area.getHeight() * 0.42f));
    area.removeFromBottom (gap);
    picker.setBounds (area);
    layoutKnobRow (knobArea, { &knobs[0]->knob, &knobs[1]->knob, &knobs[2]->knob, &knobs[3]->knob });
}

void SpacePanel::timerCallback()
{
    if (! isShowing()) return;
    picker.phase += 1.0f / 24.0f;
    const auto& vs = processor.diagnostics().visualSnapshots.latest();
    picker.activity = juce::jlimit (0.0f, 1.0f, vs.rmsL * 3.0f);
    picker.repaint();
    setActivity (vs.spaceActivity * 0.5f);

    const auto& mod = latestModulation (processor);
    for (auto& k : knobs) k->refreshModRing (mod);
}

} // namespace am::ui

#include "AMLogo.h"

namespace am::ui
{

void AMLogo::drawWordmark (juce::Graphics& g, juce::Rectangle<float> b, float energy, bool withTagline, juce::Justification just)
{
    float titleH = withTagline ? juce::jlimit (14.0f, 36.0f, b.getHeight() * 0.58f) : juce::jlimit (12.0f, 36.0f, b.getHeight() * 0.8f);
    const juce::String rest ("NTI-MATR");
    auto font = Theme::displayFont (titleH, 0.22f);
    float restW = draw::trackedTextWidth (font, rest);
    float glyphW = titleH * 0.86f;
    float gap = titleH * 0.08f;
    float totalW = glyphW + gap + restW;
    // Fit the wordmark to the available width (the display face is wide).
    if (totalW > b.getWidth() && totalW > 0.0f)
    {
        titleH *= juce::jmax (0.3f, b.getWidth() / totalW);
        font = Theme::displayFont (titleH, 0.22f);
        restW = draw::trackedTextWidth (font, rest);
        glyphW = titleH * 0.86f;
        gap = titleH * 0.08f;
        totalW = glyphW + gap + restW;
    }

    float x = b.getX();
    if (just.testFlags (juce::Justification::horizontallyCentred)) x = b.getCentreX() - totalW * 0.5f;
    else if (just.testFlags (juce::Justification::right)) x = b.getRight() - totalW;

    auto titleArea = juce::Rectangle<float> (x, b.getY(), totalW, titleH * 1.2f);
    if (! withTagline) titleArea = titleArea.withSizeKeepingCentre (totalW, titleH * 1.2f).withCentre ({ x + totalW * 0.5f, b.getCentreY() });

    // The Λ glyph: matches the display face's cap height and stroke weight.
    const float capH = font.getAscent() * 0.98f;
    const float baseline = titleArea.getY() + (titleArea.getHeight() + capH) * 0.5f;
    auto glyphArea = juce::Rectangle<float> (x, baseline - capH, glyphW, capH);
    const float stroke = juce::jmax (1.3f, titleH * 0.085f);
    const float bevel = juce::jmax (0.5f, titleH * 0.05f);

    juce::Path glyph;
    glyph.startNewSubPath (glyphArea.getX() + stroke * 0.5f, glyphArea.getBottom());
    glyph.lineTo (glyphArea.getCentreX(), glyphArea.getY() + stroke * 0.4f);
    glyph.lineTo (glyphArea.getRight() - stroke * 0.5f, glyphArea.getBottom());

    // crossbar hint (short, slightly lower than a regular A — the "anti" mark)
    juce::Path bar;
    bar.startNewSubPath (glyphArea.getX() + glyphArea.getWidth() * 0.36f, glyphArea.getY() + capH * 0.68f);
    bar.lineTo (glyphArea.getX() + glyphArea.getWidth() * 0.64f, glyphArea.getY() + capH * 0.68f);

    // createStrokedPath clears its destination, so the two strokes are built apart
    // and joined: one engraved shape means one bevel across the whole glyph.
    //
    // The wordmark is cut into the pale ground rather than cast in metal (SPEC
    // section 7). A metal fill runs from a light half to a dark one, and on pearl
    // its light half is the same value as the chassis — the mark half-disappeared.
    juce::Path metal, barMetal;
    juce::PathStrokeType (stroke, juce::PathStrokeType::mitered, juce::PathStrokeType::butt).createStrokedPath (metal, glyph);
    juce::PathStrokeType (stroke * 0.7f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt).createStrokedPath (barMetal, bar);
    metal.addPath (barMetal);
    draw::engravedShape (g, metal, Theme::textPrimary, bevel);

    // Small glowing particle beneath the left foot of the Λ.
    const float dotR = juce::jmax (1.4f, titleH * 0.06f);
    const juce::Point<float> dot (glyphArea.getX() + glyphArea.getWidth() * 0.16f, glyphArea.getBottom() + dotR * 2.6f);
    draw::glowDot (g, dot, dotR, Theme::blue, 0.6f + 0.4f * energy);

    // The rest of the wordmark, cut the same way.
    const float trailing = font.getExtraKerningFactor() * font.getHeight() + 2.0f;
    draw::engravedText (g, rest, titleArea.withLeft (glyphArea.getRight() + gap).withWidth (restW + trailing),
                        juce::Justification::centredLeft, font, Theme::textPrimary);

    if (withTagline)
    {
        auto tagArea = b.withTop (titleArea.getBottom() + titleH * 0.05f);
        const float tagH = juce::jlimit (7.0f, 10.0f, tagArea.getHeight() * 0.55f);
        draw::trackedText (g, "SOUND BEYOND MATTER", tagArea.withLeft (glyphArea.getRight() + gap + titleH * 0.04f).withHeight (tagH * 1.5f),
                           juce::Justification::centredLeft, Theme::captionFont (tagH), Theme::textDim);
    }
}

void AMLogo::paint (juce::Graphics& g)
{
    drawWordmark (g, getLocalBounds().toFloat(), energy, tagline);
}

} // namespace am::ui

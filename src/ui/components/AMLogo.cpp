#include "AMLogo.h"

namespace am::ui
{

void AMLogo::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float titleH = juce::jlimit (16.0f, 40.0f, b.getHeight() * 0.56f);
    auto titleArea = b.withHeight (titleH * 1.15f);
    auto tagArea   = b.withTop (titleArea.getBottom()).withHeight (juce::jmax (8.0f, b.getHeight() - titleArea.getHeight()));

    // The "A" glyph is drawn as a stylised lambda-like shape to give the brand its mark.
    const float glyphW = titleH * 0.78f;
    auto glyphArea = titleArea.removeFromLeft (glyphW).reduced (0.0f, titleH * 0.08f);
    juce::Path glyph;
    glyph.startNewSubPath (glyphArea.getX(), glyphArea.getBottom());
    glyph.lineTo (glyphArea.getCentreX(), glyphArea.getY());
    glyph.lineTo (glyphArea.getRight(), glyphArea.getBottom());
    glyph.startNewSubPath (glyphArea.getX() + glyphArea.getWidth() * 0.3f, glyphArea.getY() + glyphArea.getHeight() * 0.62f);
    glyph.lineTo (glyphArea.getX() + glyphArea.getWidth() * 0.55f, glyphArea.getY() + glyphArea.getHeight() * 0.62f);
    const float stroke = juce::jmax (1.4f, titleH * 0.06f);
    g.setColour (Theme::textPrimary);
    g.strokePath (glyph, juce::PathStrokeType (stroke, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));

    // Small glowing dot under the glyph: the "particle" of the brand.
    const float dotR = juce::jmax (1.5f, titleH * 0.07f);
    auto dot = juce::Rectangle<float> (glyphArea.getX() + glyphArea.getWidth() * 0.12f - dotR, glyphArea.getBottom() + dotR * 1.4f, dotR * 2, dotR * 2);
    draw::glowEllipse (g, dot, Theme::cyan, dotR * 4.0f, 0.6f + 0.4f * energy);
    g.setColour (Theme::cyan);
    g.fillEllipse (dot);

    draw::trackedText (g, "NTI-MATR", titleArea.withTrimmedLeft (titleH * 0.05f), juce::Justification::centredLeft, Theme::titleFont (titleH), Theme::textPrimary);

    const float tagH = juce::jlimit (7.0f, 10.5f, tagArea.getHeight() * 0.62f);
    draw::trackedText (g, "SOUND BEYOND MATTER", tagArea.withTrimmedLeft (glyphW + titleH * 0.05f), juce::Justification::centredLeft, Theme::captionFont (tagH), Theme::textSecondary);
}

} // namespace am::ui

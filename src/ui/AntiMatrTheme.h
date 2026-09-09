#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace am::ui
{

/**
    ANTI-MATR visual system: colours, typography and reference metrics.

    Dark near-black background, graphite panels, subtle glass, thin borders,
    large negative space. Luminous accents communicate energy, selection,
    modulation, focus and activity — never decoration for its own sake.
*/
struct Theme
{
    // Surfaces
    static inline const juce::Colour background   { 0xff07070b };
    static inline const juce::Colour backgroundTop { 0xff0b0b11 };
    static inline const juce::Colour panel        { 0xff0e0e14 };
    static inline const juce::Colour panelTop     { 0xff12121a };
    static inline const juce::Colour panelInset   { 0xff0a0a0f };
    static inline const juce::Colour border       { 0x14ffffff };   // ~8% white
    static inline const juce::Colour borderSoft   { 0x0affffff };
    static inline const juce::Colour glass        { 0x08ffffff };

    // Text
    static inline const juce::Colour textPrimary   { 0xffeaeaf2 };
    static inline const juce::Colour textSecondary { 0xff8f8f9e };
    static inline const juce::Colour textDim       { 0xff55556a };

    // Luminous accents
    static inline const juce::Colour blue    { 0xff4f8dff };
    static inline const juce::Colour cyan    { 0xff67e6ff };
    static inline const juce::Colour violet  { 0xff8f63ff };
    static inline const juce::Colour magenta { 0xffe455cf };
    static inline const juce::Colour ivory   { 0xfff1e7d3 };
    static inline const juce::Colour amber   { 0xffffb46b };

    static inline const juce::Colour knobBase  { 0xff15151d };
    static inline const juce::Colour knobTrack { 0xff262633 };

    /** Accent for a section, used to colour knobs and glows consistently. */
    enum class Section { Source, Shape, Evolve, Fracture, Space, Mod, Neutral };

    static juce::Colour accentFor (Section s) noexcept
    {
        switch (s)
        {
            case Section::Source:   return blue;
            case Section::Shape:    return cyan;
            case Section::Evolve:   return violet;
            case Section::Fracture: return magenta;
            case Section::Space:    return ivory;
            case Section::Mod:      return amber;
            default:                return textSecondary;
        }
    }

    // Typography — every size is derived from a component's own bounds so
    // the interface scales without bitmaps.
    static juce::Font font (float height, bool bold = false, float tracking = 0.0f)
    {
        auto f = juce::Font (juce::FontOptions().withHeight (height).withStyle (bold ? "Bold" : "Regular"));
        if (tracking != 0.0f) f = f.withExtraKerningFactor (tracking);
        return f;
    }

    /** Wide-tracked uppercase title (panel headers, logo). */
    static juce::Font titleFont (float height) { return font (height, false, 0.28f); }
    /** Small tracked label under knobs / tabs. */
    static juce::Font labelFont (float height) { return font (height, false, 0.16f); }
    /** Compact secondary text (subtitles, tags). */
    static juce::Font captionFont (float height) { return font (height, false, 0.22f); }
    /** Numeric readouts. */
    static juce::Font valueFont (float height) { return font (height, false, 0.02f); }

    // Reference layout: the design is authored at 1600 x 1000 logical units.
    static constexpr float kReferenceWidth  = 1600.0f;
    static constexpr float kReferenceHeight = 1000.0f;
    static constexpr float kPanelRadius     = 14.0f;
};

/** Converts reference-design units to actual pixels for the current editor size. */
struct Scale
{
    float factor = 1.0f;
    float operator() (float referencePx) const noexcept { return referencePx * factor; }
    int   i (float referencePx) const noexcept { return juce::roundToInt (referencePx * factor); }

    static Scale forSize (int width, int height) noexcept
    {
        Scale s;
        s.factor = juce::jmin ((float) width / Theme::kReferenceWidth, (float) height / Theme::kReferenceHeight);
        return s;
    }
};

} // namespace am::ui

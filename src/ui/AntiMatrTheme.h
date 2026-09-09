#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace am::ui
{

/**
    ANTI-MATR visual system: colours, typography and reference metrics.

    Dark near-black background, graphite panels, subtle glass, thin borders,
    large negative space. Luminous accents communicate energy, selection,
    modulation, focus and activity — never decoration for its own sake.

    Typography (embedded OFL typefaces, see assets/fonts):
      Michroma        — wordmark and titles (wide geometric display face)
      Be Vietnam Pro  — labels, captions, UI text (Regular / Medium / SemiBold)
      Space Mono      — numeric readouts
    When a build has no embedded fonts (ANTIMATR_HAS_FONTS == 0) every helper
    silently falls back to the platform sans-serif / monospace faces.
*/
struct Theme
{
    // Surfaces
    static inline const juce::Colour background   { 0xff07070b };
    static inline const juce::Colour backgroundTop { 0xff0b0b11 };
    static inline const juce::Colour panel        { 0xff0e0e14 };
    static inline const juce::Colour panelTop     { 0xff12121a };
    static inline const juce::Colour panelInset   { 0xff0a0a0f };
    static inline const juce::Colour panelEdge    { 0xff050508 };   // dark line outside panels (depth)
    static inline const juce::Colour border       { 0x14ffffff };   // ~8% white hairline
    static inline const juce::Colour borderSoft   { 0x0affffff };
    static inline const juce::Colour glass        { 0x08ffffff };
    static inline const juce::Colour glassStrong  { 0x14ffffff };

    // Text
    static inline const juce::Colour textPrimary   { 0xffeaeaf2 };
    static inline const juce::Colour textSecondary { 0xff8f8f9e };
    static inline const juce::Colour textDim       { 0xff55556a };
    static inline const juce::Colour textValue     { 0xffc9c9d6 };

    // Luminous accents
    static inline const juce::Colour blue    { 0xff4f8dff };
    static inline const juce::Colour cyan    { 0xff67e6ff };
    static inline const juce::Colour violet  { 0xff8f63ff };
    static inline const juce::Colour magenta { 0xffe455cf };
    static inline const juce::Colour ivory   { 0xfff1e7d3 };
    static inline const juce::Colour amber   { 0xffffb46b };

    static inline const juce::Colour knobBase  { 0xff15151d };
    static inline const juce::Colour knobTrack { 0xff232330 };

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

    //==========================================================================
    /** The embedded typefaces (each may be nullptr in a build without assets). */
    struct Typefaces
    {
        juce::Typeface::Ptr display;        ///< Michroma
        juce::Typeface::Ptr label;          ///< Be Vietnam Pro Regular
        juce::Typeface::Ptr labelMedium;    ///< Be Vietnam Pro Medium
        juce::Typeface::Ptr labelSemiBold;  ///< Be Vietnam Pro SemiBold
        juce::Typeface::Ptr mono;           ///< Space Mono
        bool embedded = false;
    };

    /** Loads the typefaces once (thread-safe, message thread expected). */
    static const Typefaces& typefaces();

    /** Builds a font from a typeface with a JUCE height and extra tracking (fraction of the height). */
    static juce::Font make (const juce::Typeface::Ptr& typeface, float height, float tracking, bool boldFallback = false, bool monoFallback = false);

    // Typography — every size is derived from a component's own bounds so
    // the interface scales without bitmaps.

    /** General UI text (Be Vietnam Pro Regular / SemiBold). */
    static juce::Font font (float height, bool bold = false, float tracking = 0.0f)
    {
        const auto& t = typefaces();
        return make (bold ? t.labelSemiBold : t.label, height, tracking, bold);
    }

    /** Wordmark / hero text in the display face (Michroma). */
    static juce::Font displayFont (float height, float tracking = 0.18f) { return make (typefaces().display, height * 0.94f, tracking); }
    /** Wide-tracked uppercase title (panel headers). */
    static juce::Font titleFont (float height)   { return make (typefaces().display, height * 0.9f, 0.16f); }
    /** Small tracked label under knobs / tabs (Be Vietnam Pro Medium). */
    static juce::Font labelFont (float height)   { return make (typefaces().labelMedium, height, 0.14f); }
    /** Stronger tracked label (buttons, selected states). */
    static juce::Font labelFontStrong (float height) { return make (typefaces().labelSemiBold, height, 0.16f, true); }
    /** Compact secondary text (subtitles, tags, captions). */
    static juce::Font captionFont (float height) { return make (typefaces().label, height, 0.22f); }
    /** Numeric readouts (Space Mono). */
    static juce::Font valueFont (float height)   { return make (typefaces().mono, height, 0.0f, false, true); }
    /** Body copy without tracking. */
    static juce::Font bodyFont (float height)    { return make (typefaces().label, height, 0.0f); }

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

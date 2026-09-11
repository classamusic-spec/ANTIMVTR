#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace am::ui
{

/**
    ANTI-MATR visual system: colours, typography and reference metrics.

    A pale pearl chassis carrying panels of frosted white glass, near-black
    recessed displays, thin borders and large negative space. Luminous accents
    communicate energy, selection, modulation, focus and activity — never
    decoration for its own sake.

    Typography (embedded OFL typefaces, see assets/fonts):
      Michroma        — wordmark and titles (wide geometric display face)
      Be Vietnam Pro  — labels, captions, UI text (Regular / Medium / SemiBold)
      Space Mono      — numeric readouts
    When a build has no embedded fonts (ANTIMATR_HAS_FONTS == 0) every helper
    silently falls back to the platform sans-serif / monospace faces.
*/
struct Theme
{
    // Surfaces — a pearl chassis carrying frosted white glass panels.
    //
    // The instrument is lit from above and slightly left, as it always was; what
    // changed is the ground. On a light chassis depth cannot come from a panel
    // being brighter than its background, because it is not — it comes from the
    // shadow a panel casts and from a hairline of white along its top edge. So
    // `panelTop` sits *above* white and `panel` just below it, the difference is
    // small, and the shadow does the work.
    static inline const juce::Colour background    { 0xffd3d5da };   // chassis, cool pearl
    static inline const juce::Colour backgroundTop { 0xffe6e8ec };   // lighter toward the top
    static inline const juce::Colour panel         { 0xffeef0f3 };   // bottom of a frosted slab
    static inline const juce::Colour panelTop      { 0xfffcfdfe };   // top of a frosted slab (lit)
    static inline const juce::Colour panelInset    { 0xffc9ccd3 };   // a light capsule cut into a panel
    static inline const juce::Colour panelEdge     { 0x2a2c3242 };   // the shadow a slab sits in
    static inline const juce::Colour border        { 0x1a2a3044 };   // hairline, now dark on light
    static inline const juce::Colour borderSoft    { 0x0d2a3044 };
    static inline const juce::Colour glass         { 0x66ffffff };   // frost: white veil over the ground
    static inline const juce::Colour glassStrong   { 0xa8ffffff };
    static inline const juce::Colour metal         { 0xff9aa0ab };   // chrome rims and screw heads
    static inline const juce::Colour metalLight    { 0xfff4f6f8 };   // the lit face of a turned cap
    static inline const juce::Colour metalDark     { 0xff6c727d };   // its shadowed face
    static inline const juce::Colour well          { 0xff14161c };   // behind the glass: the object's ground

    // The displays. These are the one dark element on the page, and the contrast
    // between a pale chassis and a near-black screen is the signature of the
    // design — so they are genuinely black, not a dark grey.
    static inline const juce::Colour screen        { 0xff0e1016 };   // the face of a recessed display
    static inline const juce::Colour screenDeep    { 0xff05060a };   // under its top lip
    static inline const juce::Colour screenInk     { 0xffe8ecf4 };   // luminous line art on one

    // Text — dark on light now, so the weights invert. Every one of these is
    // checked against the pale chassis, not only against a white panel: a caption
    // that reads on the panel can vanish on the navigation rail.
    static inline const juce::Colour textPrimary   { 0xff15181f };
    static inline const juce::Colour textSecondary { 0xff555b69 };
    static inline const juce::Colour textDim       { 0xff787e8d };
    static inline const juce::Colour textValue     { 0xff23262f };

    // Luminous accents — one pair per section (SPEC section 9). Deepened, because a
    // colour that glowed against charcoal washes out against pearl.
    static inline const juce::Colour blue    { 0xff2f6ae0 };
    static inline const juce::Colour cyan    { 0xff1ba8cf };
    static inline const juce::Colour violet  { 0xff6f3fdc };
    static inline const juce::Colour indigo  { 0xff3f41c4 };
    static inline const juce::Colour magenta { 0xffc22fa8 };
    static inline const juce::Colour ivory   { 0xffb59a63 };
    static inline const juce::Colour amber   { 0xffe8862a };   // the lit LED in the dot ring

    // The knob is the one thing that stays dark: a moulded body with a turned
    // metal cap, sitting on a light panel the way hardware actually does.
    static inline const juce::Colour knobBase  { 0xff2b2e34 };
    static inline const juce::Colour knobTrack { 0xffb9bdc5 };

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

    /** The second colour of a section's accent pair (SPEC section 9). */
    static juce::Colour accentPartner (juce::Colour c) noexcept
    {
        const auto argb = c.getARGB();
        if (argb == blue.getARGB())    return cyan;
        if (argb == cyan.getARGB())    return violet;
        if (argb == violet.getARGB())  return indigo;
        if (argb == indigo.getARGB())  return violet;
        if (argb == magenta.getARGB()) return violet;
        if (argb == ivory.getARGB())   return blue;
        if (argb == amber.getARGB())   return magenta;
        // Anything else pairs with a brighter, slightly rotated version of itself.
        return c.withRotatedHue (0.055f).brighter (0.22f);
    }

    /** Both halves of a section's accent pair, in sweep order. */
    static std::pair<juce::Colour, juce::Colour> accentPair (juce::Colour c) noexcept { return { c, accentPartner (c) }; }

    static std::pair<juce::Colour, juce::Colour> accentPair (Section s) noexcept { return accentPair (accentFor (s)); }

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
    /** Wide-tracked uppercase title (panel headers). One family, four sizes (SPEC section 8). */
    static juce::Font titleFont (float height)   { return make (typefaces().labelSemiBold, height, 0.30f, true); }
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
    static constexpr float kPanelRadius     = 20.0f;
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

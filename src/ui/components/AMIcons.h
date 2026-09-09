#pragma once

#include "ui/AntiMatrTheme.h"

namespace am::ui
{

/**
    Procedural vector icons. Every glyph is drawn from geometry so it scales
    without bitmaps and can take any accent colour.
*/
enum class Icon
{
    Wave, Dust, Impact, Sample, Gesture,
    Bend, Melt, Tear, Magnet, Gravity, Scatter, Freeze, Crush,
    Main, Source, Shape, Evolve, Fracture, Space, Mod, Lab,
    Settings, ChevronLeft, ChevronRight, Sparkle, Copy
};

struct Icons
{
    /** Draws an icon centred in `bounds` with the given colour and stroke scale. */
    static void draw (juce::Graphics& g, Icon icon, juce::Rectangle<float> bounds, juce::Colour colour, float strokeScale = 1.0f);

    /** Builds the icon's path in a unit square (0..1). */
    static juce::Path path (Icon icon);
};

} // namespace am::ui

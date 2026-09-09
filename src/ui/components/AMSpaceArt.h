#pragma once

#include "AMDrawing.h"

namespace am::ui
{

/**
    Procedural environment art for the eight SPACE types (NEBULA, VOID,
    CHAMBER, ORBIT, DREAM, MACHINE, SHIMMER, DUST). Pure vector drawing,
    deterministic per type, animated with `phase` (seconds) and brightened by
    `activity` (0..1). Used by the Space picker, the Space page and preset cards.
*/
struct SpaceArt
{
    static constexpr int kNumTypes = 8;

    static const char* name (int type) noexcept
    {
        static const char* names[] = { "NEBULA", "VOID", "CHAMBER", "ORBIT", "DREAM", "MACHINE", "SHIMMER", "DUST" };
        return names[juce::jlimit (0, kNumTypes - 1, type)];
    }

    static juce::Colour tint (int type) noexcept
    {
        static const juce::Colour tints[] = { Theme::violet, juce::Colour (0xff6a6a88), Theme::amber, Theme::blue, Theme::magenta, Theme::cyan, Theme::ivory, Theme::textSecondary };
        return tints[juce::jlimit (0, kNumTypes - 1, type)];
    }

    /** Draws the art clipped to a rounded rectangle. */
    static void draw (juce::Graphics& g, juce::Rectangle<float> area, int type, float phase, float activity, float corner = 8.0f);

private:
    static void stars (juce::Graphics& g, juce::Rectangle<float> a, int seed, int count, float phase, float alpha);
    static void nebula (juce::Graphics& g, juce::Rectangle<float> a, float phase, float activity);
    static void voidHole (juce::Graphics& g, juce::Rectangle<float> a, float phase, float activity);
    static void chamber (juce::Graphics& g, juce::Rectangle<float> a, float phase, float activity);
    static void orbit (juce::Graphics& g, juce::Rectangle<float> a, float phase, float activity);
    static void dream (juce::Graphics& g, juce::Rectangle<float> a, float phase, float activity);
    static void machine (juce::Graphics& g, juce::Rectangle<float> a, float phase, float activity);
    static void shimmer (juce::Graphics& g, juce::Rectangle<float> a, float phase, float activity);
    static void dust (juce::Graphics& g, juce::Rectangle<float> a, float phase, float activity);
};

} // namespace am::ui

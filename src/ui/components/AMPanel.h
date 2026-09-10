#pragma once

#include "AMDrawing.h"

namespace am::ui
{

/**
    Graphite panel with a wide-tracked title, a small subtitle and a short
    glowing accent line. Children are placed inside contentBounds().
*/
class AMPanel : public juce::Component
{
public:
    AMPanel (const juce::String& title, const juce::String& subtitle, juce::Colour accent);

    void paint (juce::Graphics& g) override;

    /** Area below the header for child components. */
    juce::Rectangle<int> contentBounds() const;
    juce::Rectangle<int> headerBounds() const;

    /** Space on the right of the header for controls (toggles etc.). */
    juce::Rectangle<int> headerRightBounds() const;

    /** Inner padding used by the layout helpers. */
    int padding() const;

    void setTitle (const juce::String& t) { title = t.toUpperCase(); repaint(); }
    void setSubtitle (const juce::String& s) { subtitle = s.toUpperCase(); repaint(); }
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    juce::Colour getAccent() const noexcept { return accent; }
    void setShowAccentLine (bool b) { showAccentLine = b; }
    /** Compact panels use a single-line header (title only, smaller). */
    void setCompact (bool c) { compact = c; resized(); repaint(); }
    void setActivity (float a) { if (std::abs (a - activity) > 0.02f) { activity = a; repaint(); } }

private:
    juce::String title, subtitle;
    juce::Colour accent;
    bool showAccentLine = true;
    bool compact = false;
    mutable bool headerRightBoundsUsed = false;
    float activity = 0.0f;
};

} // namespace am::ui

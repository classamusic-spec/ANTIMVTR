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

    void setAccent (juce::Colour c) { accent = c; repaint(); }
    juce::Colour getAccent() const noexcept { return accent; }
    void setShowAccentLine (bool b) { showAccentLine = b; }
    void setActivity (float a) { if (std::abs (a - activity) > 0.02f) { activity = a; repaint(); } }

private:
    juce::String title, subtitle;
    juce::Colour accent;
    bool showAccentLine = true;
    float activity = 0.0f;
};

} // namespace am::ui

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

    /** The slab itself: the component's bounds less the margin its shadow falls into. */
    juce::Rectangle<float> slabBounds() const;

    /** Space on the right of the header for controls (toggles etc.). */
    juce::Rectangle<int> headerRightBounds() const;

    /** Inner padding used by the layout helpers. */
    int padding() const;

    void setTitle (const juce::String& t) { title = t.toUpperCase(); invalidateChrome(); }
    void setSubtitle (const juce::String& s) { subtitle = s.toUpperCase(); invalidateChrome(); }
    void setAccent (juce::Colour c) { accent = c; invalidateChrome(); }
    juce::Colour getAccent() const noexcept { return accent; }
    void setShowAccentLine (bool b) { showAccentLine = b; invalidateChrome(); }
    /** Compact panels use a single-line header (title only, smaller). */
    void setCompact (bool c) { compact = c; resized(); invalidateChrome(); }
    void setActivity (float a) { if (std::abs (a - activity) > 0.02f) { activity = a; repaint(); } }

    void resized() override { invalidateChrome(); }

private:
    /**
        The slab, its screws and its lettering never change between frames, so they
        are drawn once into an image and blitted after that. Only the activity glow
        is painted live, which keeps a panel that follows the audio nearly free.
    */
    void paintChrome (juce::Graphics& g) const;
    void renderChrome (float scale);
    void invalidateChrome() { chrome = juce::Image(); repaint(); }

    juce::String title, subtitle;
    juce::Colour accent;
    bool showAccentLine = true;
    bool compact = false;
    mutable bool headerRightBoundsUsed = false;
    float activity = 0.0f;
    juce::Image chrome;
    float chromeScale = 0.0f;
};

} // namespace am::ui

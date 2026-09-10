#pragma once

#include "AMDrawing.h"
#include "AMAnimator.h"
#include "AMIcons.h"

namespace am::ui
{

/**
    Circular source selectors (WAVE, DUST, IMPACT, SAMPLE, GESTURE). Each cell
    draws a procedural "energy" thumbnail, animated subtly; the selected one
    carries a glowing accent ring. Selection and hover transitions are eased.

    A row (compact placements such as the MAIN panel) or a column, where each
    entry gets its thumbnail, name and tagline side by side — the deep SOURCE
    page uses the column so choosing an energy fills its own panel.
*/
class AMSourceSelector : public juce::Component,
                         public juce::SettableTooltipClient,
                         private juce::Timer
{
public:
    struct Item { juce::String label; Icon icon; juce::Colour accent; juce::String caption; };
    static constexpr int kMaxItems = 5;
    enum class Orientation { Row, Column };

    explicit AMSourceSelector (std::vector<Item> items);
    ~AMSourceSelector() override;

    void setSelected (int index, juce::NotificationType notify = juce::sendNotification);
    void setOrientation (Orientation o) { orientation = o; resized(); repaint(); }

    /** Height at which a row's thumbnails fill their cells: taller than this only adds empty space. */
    int preferredHeight (int width) const noexcept;
    int getSelected() const noexcept { return selected; }
    std::function<void (int)> onChange;

    /** Energy 0..1 animates the selected thumbnail. */
    void setEnergy (float e) { energy = e; }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent&) override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

private:
    void timerCallback() override;
    juce::Rectangle<float> cellBounds (int index) const;
    int indexAt (juce::Point<int> p) const;
    void drawThumbnail (juce::Graphics& g, const Item& item, juce::Rectangle<float> circle, float on, int index);

    std::vector<Item> items;
    std::vector<juce::String> labels, captions;
    Orientation orientation = Orientation::Row;
    int selected = 0;
    int hover = -1;
    float energy = 0.0f;
    float phase = 0.0f;
    std::array<Eased, kMaxItems> lit {};
    std::array<Eased, kMaxItems> hov {};
    juce::Path scratch;
};

} // namespace am::ui

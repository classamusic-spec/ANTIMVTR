#pragma once

#include "AMDrawing.h"
#include "AMIcons.h"

namespace am::ui
{

/**
    Row of circular source selectors (WAVE, DUST, IMPACT, SAMPLE). Each cell
    draws a procedural "energy" thumbnail; the selected one has a glowing ring.
*/
class AMSourceSelector : public juce::Component,
                         private juce::Timer
{
public:
    struct Item { juce::String label; Icon icon; juce::Colour accent; };

    explicit AMSourceSelector (std::vector<Item> items);

    void setSelected (int index, juce::NotificationType notify = juce::sendNotification);
    int getSelected() const noexcept { return selected; }
    std::function<void (int)> onChange;

    /** Energy 0..1 animates the selected thumbnail. */
    void setEnergy (float e) { energy = e; }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }

private:
    void timerCallback() override { phase += 0.02f; if (isShowing()) repaint(); }
    int indexAt (juce::Point<int> p) const;
    void drawThumbnail (juce::Graphics& g, const Item& item, juce::Rectangle<float> circle, bool on, int index);

    std::vector<Item> items;
    int selected = 0;
    int hover = -1;
    float energy = 0.0f;
    float phase = 0.0f;
};

} // namespace am::ui

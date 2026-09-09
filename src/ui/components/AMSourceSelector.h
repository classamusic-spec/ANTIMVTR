#pragma once

#include "AMDrawing.h"
#include "AMAnimator.h"
#include "AMIcons.h"

namespace am::ui
{

/**
    Row of circular source selectors (WAVE, DUST, IMPACT, SAMPLE). Each cell
    draws a procedural "energy" thumbnail, animated subtly; the selected one
    carries a glowing accent ring. Selection and hover transitions are eased.
*/
class AMSourceSelector : public juce::Component,
                         private juce::Timer
{
public:
    struct Item { juce::String label; Icon icon; juce::Colour accent; };
    static constexpr int kMaxItems = 5;

    explicit AMSourceSelector (std::vector<Item> items);
    ~AMSourceSelector() override;

    void setSelected (int index, juce::NotificationType notify = juce::sendNotification);
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
    int indexAt (juce::Point<int> p) const;
    void drawThumbnail (juce::Graphics& g, const Item& item, juce::Rectangle<float> circle, float on, int index);

    std::vector<Item> items;
    std::vector<juce::String> labels;
    int selected = 0;
    int hover = -1;
    float energy = 0.0f;
    float phase = 0.0f;
    std::array<Eased, kMaxItems> lit {};
    std::array<Eased, kMaxItems> hov {};
    juce::Path scratch;
};

} // namespace am::ui

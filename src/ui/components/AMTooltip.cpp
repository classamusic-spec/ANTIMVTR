#include "AMTooltip.h"

namespace am::ui
{

AMTooltip::AMTooltip()
{
    setInterceptsMouseClicks (false, false);
    setAlwaysOnTop (true);
    setOpaque (false);
}

AMTooltip::~AMTooltip()
{
    stopTimer();
    if (auto* parent = getParentComponent()) parent->removeChildComponent (this);
}

void AMTooltip::showFor (juce::Component& control, juce::Point<int> localPoint, const juce::String& newText, int holdMs)
{
    auto* host = control.getTopLevelComponent();
    if (host == nullptr) return;
    text = newText;
    font = Theme::valueFont (12.0f);
    const int w = juce::roundToInt (juce::GlyphArrangement::getStringWidth (font, text)) + 22;
    const int h = 24;
    auto p = host->getLocalPoint (&control, localPoint);
    auto area = juce::Rectangle<int> (p.x + 14, p.y - h - 10, w, h);
    if (area.getRight() > host->getWidth() - 4) area.setX (p.x - w - 14);
    if (area.getY() < 4) area.setY (p.y + 14);
    if (getParentComponent() != host) host->addAndMakeVisible (this);
    setBounds (area);
    setVisible (true);
    toFront (false);
    repaint();
    startTimer (holdMs);
}

void AMTooltip::hide()
{
    stopTimer();
    setVisible (false);
}

void AMTooltip::paint (juce::Graphics& g)
{
    getLookAndFeel().drawTooltip (g, text, getWidth(), getHeight());
}

} // namespace am::ui

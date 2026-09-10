#include "Controls.h"

namespace am::ui
{

juce::StringArray paramChoices (Param p)
{
    juce::StringArray items;
    items.addTokens (juce::String (ParameterRegistry::get (p).choices), "|", "");
    return items;
}

juce::String paramTooltip (Param p)
{
    const auto& d = ParameterRegistry::get (p);
    juce::String s (d.name);
    if (d.unit[0] != 0) s << "  (" << d.unit << ")";
    s << "\nDouble-click resets, shift-drag for fine control, right-click for options.";
    return s;
}

BoundKnob::BoundKnob (juce::AudioProcessorValueTreeState& apvts, Param p, juce::Colour accent, const juce::String& labelOverride)
    : param (p), knob (labelOverride.isNotEmpty() ? labelOverride : juce::String (ParameterRegistry::get (p).name), accent)
{
    const auto& d = ParameterRegistry::get (p);
    attachment = std::make_unique<SliderAttachment> (apvts, d.id, knob);
    knob.setDoubleClickReturnValue (true, d.defaultValue);
    if (d.min < 0.0f && d.max > 0.0f) knob.setBipolar (true);
    knob.setTooltip (paramTooltip (p));
}

BoundControl::BoundControl (juce::AudioProcessorValueTreeState& apvts, Param p, juce::Colour accent, const juce::String& labelOverride)
    : parameter (p), kind (ParameterRegistry::get (p).kind)
{
    const auto& d = ParameterRegistry::get (p);
    const juce::String label = labelOverride.isNotEmpty() ? labelOverride : juce::String (d.name);
    auto* param = apvts.getParameter (d.id);

    if (kind == ParamKind::Bool)
    {
        auto t = std::make_unique<AMToggle> (label, accent);
        auto* raw = t.get();
        paramAttachment = std::make_unique<juce::ParameterAttachment> (*param, [raw] (float v) { raw->setToggleState (v >= 0.5f, juce::dontSendNotification); });
        raw->onChange = [this] (bool on) { paramAttachment->setValueAsCompleteGesture (on ? 1.0f : 0.0f); };
        paramAttachment->sendInitialUpdate();
        comp = std::move (t);
    }
    else if (kind == ParamKind::Choice)
    {
        auto c = std::make_unique<AMChoice> (label, paramChoices (p), accent);
        auto* raw = c.get();
        paramAttachment = std::make_unique<juce::ParameterAttachment> (*param, [raw] (float v) { raw->setSelected ((int) std::lround (v), juce::dontSendNotification); });
        raw->onChange = [this] (int i) { paramAttachment->setValueAsCompleteGesture ((float) i); };
        paramAttachment->sendInitialUpdate();
        comp = std::move (c);
    }
    else
    {
        auto k = std::make_unique<AMKnob> (label, accent);
        sliderAttachment = std::make_unique<SliderAttachment> (apvts, d.id, *k);
        k->setDoubleClickReturnValue (true, d.defaultValue);
        if (d.min < 0.0f && d.max > 0.0f) k->setBipolar (true);
        comp = std::move (k);
    }
    if (auto* tc = dynamic_cast<juce::SettableTooltipClient*> (comp.get())) tc->setTooltip (paramTooltip (p));
}

void layoutKnobRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> comps, int rows)
{
    std::vector<juce::Component*> v (comps);
    const int n = (int) v.size();
    if (n == 0) return;
    layoutGrid (area, v, (n + rows - 1) / rows);
}

void layoutGrid (juce::Rectangle<int> area, const std::vector<juce::Component*>& comps, int columns, int gapX, int gapY)
{
    const int n = (int) comps.size();
    if (n == 0 || columns <= 0) return;
    const int rows = (n + columns - 1) / columns;
    const int cellW = (area.getWidth() - gapX * (columns - 1)) / columns;
    const int cellH = (area.getHeight() - gapY * (rows - 1)) / rows;
    for (int i = 0; i < n; ++i)
    {
        const int r = i / columns, c = i % columns;
        comps[(size_t) i]->setBounds (area.getX() + c * (cellW + gapX), area.getY() + r * (cellH + gapY), cellW, cellH);
    }
}

} // namespace am::ui

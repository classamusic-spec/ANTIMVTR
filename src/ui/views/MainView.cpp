#include "MainView.h"

namespace am::ui
{

MainView::MainView (AntiMatrProcessor& p)
    : processor (p), source (p), shape (p), evolve (p), fracture (p), space (p), object (p.diagnostics())
{
    addAndMakeVisible (object);
    addAndMakeVisible (source);
    addAndMakeVisible (shape);
    addAndMakeVisible (evolve);
    addAndMakeVisible (fracture);
    addAndMakeVisible (space);
}

void MainView::resized()
{
    // Proportional layout derived from the 1600 x 1000 reference design.
    const auto b = getLocalBounds();
    const float W = (float) b.getWidth(), H = (float) b.getHeight();
    // The page occupies the 810 reference units between the top bar and the navigation bar.
    constexpr float kPageHeight = 810.0f;
    auto rel = [&] (float x, float y, float w, float h)
    {
        return juce::Rectangle<int> (b.getX() + juce::roundToInt (x / Theme::kReferenceWidth * W),
                                     b.getY() + juce::roundToInt (y / kPageHeight * H),
                                     juce::roundToInt (w / Theme::kReferenceWidth * W),
                                     juce::roundToInt (h / kPageHeight * H));
    };

    // Reference geometry (in design units, relative to the page area below the top bar).
    source.setBounds   (rel (22.0f,   8.0f,  458.0f, 470.0f));
    object.setBounds   (rel (488.0f,  0.0f,  592.0f, 486.0f));
    shape.setBounds    (rel (1084.0f, 8.0f,  494.0f, 470.0f));
    evolve.setBounds   (rel (22.0f,  496.0f, 458.0f, 300.0f));
    fracture.setBounds (rel (500.0f, 496.0f, 540.0f, 300.0f));
    space.setBounds    (rel (1060.0f, 496.0f, 518.0f, 300.0f));
}

//==============================================================================
GroupPage::GroupPage (AntiMatrProcessor& p, const juce::String& title, const juce::String& subtitle,
                      std::vector<ParamGroup> groups, juce::Colour accentColour)
    : accent (accentColour)
{
    juce::ignoreUnused (title, subtitle);
    for (auto g : groups)
    {
        Section s;
        s.panel = std::make_unique<AMPanel> (ParameterRegistry::groupName (g), subtitle, accent);
        content.addAndMakeVisible (*s.panel);
        for (auto param : ParameterRegistry::inGroup (g))
        {
            s.knobs.push_back (std::make_unique<BoundKnob> (p.parameters(), param, accent));
            s.panel->addAndMakeVisible (s.knobs.back()->knob);
        }
        sections.push_back (std::move (s));
    }
    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);
}

void GroupPage::resized()
{
    viewport.setBounds (getLocalBounds());
    const int width = getWidth() - 16;
    const int pad = juce::jmax (8, getWidth() / 70);
    const int knobSize = juce::jlimit (70, 110, getWidth() / 12);
    const int perRow = juce::jmax (1, (width - pad * 4) / knobSize);
    int y = pad;
    for (auto& s : sections)
    {
        const int rows = ((int) s.knobs.size() + perRow - 1) / perRow;
        const int header = juce::jlimit (40, 64, knobSize * 6 / 10);
        const int h = header + rows * knobSize + pad * 2;
        s.panel->setBounds (pad, y, width - pad * 2, h);
        auto area = s.panel->contentBounds();
        int i = 0;
        for (auto& k : s.knobs)
        {
            const int r = i / perRow, c = i % perRow;
            k->knob.setBounds (area.getX() + c * knobSize, area.getY() + r * knobSize, knobSize, knobSize);
            ++i;
        }
        y += h + pad;
    }
    content.setSize (width, y);
}

} // namespace am::ui

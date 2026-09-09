#include "PresetBrowser.h"

namespace am::ui
{

//==============================================================================
class PresetBrowser::CategoryList : public juce::Component
{
public:
    struct Entry { juce::String name; int count; };
    std::vector<Entry> entries;
    juce::String selected { "ALL" };
    std::function<void (const juce::String&)> onSelect;

    int rowHeight() const { return juce::jlimit (26, 36, getHeight() / 14); }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float h = juce::jlimit (8.5f, 11.5f, (float) rowHeight() * 0.34f);
        draw::trackedText (g, "CATEGORIES", b.withHeight ((float) rowHeight()), juce::Justification::centredLeft, Theme::captionFont (h * 0.85f), Theme::textDim);
        float y = b.getY() + (float) rowHeight();
        for (int i = 0; i < (int) entries.size(); ++i)
        {
            auto row = juce::Rectangle<float> (b.getX(), y, b.getWidth(), (float) rowHeight());
            const bool on = entries[(size_t) i].name == selected;
            const bool hv = i == hoverRow;
            if (on)
            {
                juce::ColourGradient grad (Theme::blue.withAlpha (0.18f), row.getX(), row.getY(), Theme::blue.withAlpha (0.0f), row.getRight(), row.getY(), false);
                g.setGradientFill (grad);
                g.fillRoundedRectangle (row, 6.0f);
                juce::Path bar; bar.startNewSubPath (row.getX() + 1.0f, row.getY() + 6.0f); bar.lineTo (row.getX() + 1.0f, row.getBottom() - 6.0f);
                draw::glowPath (g, bar, Theme::blue, 2.0f, 8.0f, 0.8f);
            }
            else if (hv)
            {
                g.setColour (juce::Colours::white.withAlpha (0.03f));
                g.fillRoundedRectangle (row, 6.0f);
            }
            const auto col = on ? Theme::textPrimary : (hv ? Theme::textSecondary.brighter (0.4f) : Theme::textSecondary);
            draw::trackedText (g, entries[(size_t) i].name, row.withTrimmedLeft (12.0f), juce::Justification::centredLeft, on ? Theme::labelFontStrong (h) : Theme::labelFont (h), col);
            draw::trackedText (g, juce::String (entries[(size_t) i].count), row.withTrimmedRight (8.0f), juce::Justification::centredRight, Theme::valueFont (h), Theme::textDim);
            y += (float) rowHeight();
        }
    }

    int rowAt (juce::Point<int> p) const
    {
        const int r = (p.y - rowHeight()) / rowHeight();
        return (r >= 0 && r < (int) entries.size()) ? r : -1;
    }
    void mouseMove (const juce::MouseEvent& e) override { const int r = rowAt (e.getPosition()); if (r != hoverRow) { hoverRow = r; repaint(); } }
    void mouseExit (const juce::MouseEvent&) override { hoverRow = -1; repaint(); }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int r = rowAt (e.getPosition());
        if (r < 0) return;
        selected = entries[(size_t) r].name;
        repaint();
        if (onSelect) onSelect (selected);
    }

private:
    int hoverRow = -1;
};

//==============================================================================
PresetBrowser::PresetBrowser (AntiMatrProcessor& p) : processor (p)
{
    setWantsKeyboardFocus (true);
    setOpaque (false);
    categories = std::make_unique<CategoryList>();
    categories->onSelect = [this] (const juce::String& c) { currentCategory = c; refilter(); };
    addAndMakeVisible (*categories);

    close.setTooltip ("Close (Esc)");
    close.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (close);

    search.setMultiLine (false);
    search.setReturnKeyStartsNewLine (false);
    search.setSelectAllWhenFocused (true);
    search.setFont (Theme::bodyFont (14.0f));
    search.setTextToShowWhenEmpty ("SEARCH PRESETS", Theme::textDim);
    search.setIndents (12, 0);
    search.setJustification (juce::Justification::centredLeft);
    search.addListener (this);
    addAndMakeVisible (search);

    viewport.setViewedComponent (&grid, false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (6);
    addAndMakeVisible (viewport);

    rebuildCatalogue();
    processor.addChangeListener (this);
}

PresetBrowser::~PresetBrowser()
{
    processor.removeChangeListener (this);
}

void PresetBrowser::visibilityChanged()
{
    if (isVisible()) { rebuildCatalogue(); grabKeyboardFocus(); }
}

void PresetBrowser::parentSizeChanged()
{
    if (auto* parent = getParentComponent()) setBounds (parent->getLocalBounds());
}

bool PresetBrowser::keyPressed (const juce::KeyPress& key)
{
    if (key.getKeyCode() == juce::KeyPress::escapeKey) { if (onClose) onClose(); return true; }
    if (key.getKeyCode() == juce::KeyPress::leftKey)  { processor.loadNextPreset (-1); return true; }
    if (key.getKeyCode() == juce::KeyPress::rightKey) { processor.loadNextPreset (1); return true; }
    return false;
}

void PresetBrowser::changeListenerCallback (juce::ChangeBroadcaster*)
{
    const int cur = processor.currentPresetIndex();
    for (auto& c : cards) c->setCurrent (c->getIndex() == cur);
    repaint();
}

void PresetBrowser::select (int presetIndex)
{
    if (presetIndex >= 0 && presetIndex < processor.presets().numFactoryPresets())
        processor.loadFactoryPreset (presetIndex);
}

void PresetBrowser::rebuildCatalogue()
{
    auto& presets = processor.presets();
    const int n = presets.numFactoryPresets();
    if ((int) cards.size() != n)
    {
        cards.clear();
        for (int i = 0; i < n; ++i)
        {
            auto card = std::make_unique<AMPresetCard>();
            card->onClick = [this] (int idx) { select (idx); };
            card->onDoubleClick = [this] (int idx) { select (idx); if (onClose) onClose(); };
            grid.addAndMakeVisible (*card);
            cards.push_back (std::move (card));
        }
    }
    std::map<juce::String, int> counts;
    allTags.clear();
    for (int i = 0; i < n; ++i)
    {
        const auto& f = presets.factoryPreset (i);
        cards[(size_t) i]->setPreset (i, f.name, f.category, f.tags);
        cards[(size_t) i]->setCurrent (i == processor.currentPresetIndex());
        counts[f.category.toUpperCase()]++;
        for (const auto& t : f.tags) allTags.addIfNotAlreadyThere (t.toUpperCase());
    }
    allTags.sort (true);
    categories->entries.clear();
    categories->entries.push_back ({ "ALL", n });
    for (auto& [name, count] : counts) categories->entries.push_back ({ name, count });
    categories->repaint();

    tagChips.clear();
    for (const auto& t : allTags)
    {
        auto chip = std::make_unique<AMButton> (t, Theme::cyan);
        chip->setChip (true);
        chip->setClickingTogglesState (true);
        chip->onClick = [this] { refilter(); };
        addAndMakeVisible (*chip);
        tagChips.push_back (std::move (chip));
    }
    refilter();
}

void PresetBrowser::refilter()
{
    activeTags.clear();
    for (auto& c : tagChips) if (c->getToggleState()) activeTags.add (c->getButtonText().toUpperCase());
    const auto query = search.getText().trim().toUpperCase();
    auto& presets = processor.presets();
    visibleCards.clear();
    for (int i = 0; i < (int) cards.size(); ++i)
    {
        const auto& f = presets.factoryPreset (i);
        bool ok = currentCategory == "ALL" || f.category.equalsIgnoreCase (currentCategory);
        if (ok && query.isNotEmpty())
            ok = f.name.toUpperCase().contains (query) || f.category.toUpperCase().contains (query) || f.tags.joinIntoString (" ").toUpperCase().contains (query);
        for (const auto& t : activeTags) if (ok && ! f.tags.contains (t, true)) ok = false;
        cards[(size_t) i]->setVisible (ok);
        if (ok) visibleCards.push_back (i);
    }
    layoutGrid();
    repaint();
}

void PresetBrowser::layoutGrid()
{
    const int width = viewport.getMaximumVisibleWidth();
    if (width <= 0) return;
    const int gap = juce::jmax (8, width / 80);
    const int columns = juce::jlimit (2, 6, width / 230);
    const int cardW = (width - gap * (columns - 1)) / columns;
    const int cardH = juce::roundToInt ((float) cardW * 0.82f);
    int i = 0;
    for (int idx : visibleCards)
    {
        const int r = i / columns, c = i % columns;
        cards[(size_t) idx]->setBounds (c * (cardW + gap), r * (cardH + gap), cardW, cardH);
        ++i;
    }
    const int rows = ((int) visibleCards.size() + columns - 1) / columns;
    grid.setSize (width, juce::jmax (1, rows * (cardH + gap)));
}

void PresetBrowser::resized()
{
    auto area = getLocalBounds();
    const int pad = juce::jlimit (20, 60, getWidth() / 26);
    area.reduce (pad, pad * 2 / 3);

    headerArea = area.removeFromTop (juce::jlimit (44, 64, getHeight() / 14));
    close.setBounds (headerArea.removeFromRight (headerArea.getHeight()).reduced (6));
    area.removeFromTop (pad / 2);

    auto left = area.removeFromLeft (juce::jlimit (150, 220, getWidth() / 7));
    area.removeFromLeft (pad);
    categories->setBounds (left);

    auto searchArea = area.removeFromTop (juce::jlimit (32, 40, getHeight() / 22));
    search.setBounds (searchArea.withWidth (juce::jmin (searchArea.getWidth(), 420)));
    area.removeFromTop (pad / 3);

    chipArea = area.removeFromTop (juce::jlimit (22, 28, getHeight() / 32));
    {
        auto row = chipArea;
        const auto font = Theme::captionFont (9.0f);
        for (auto& chip : tagChips)
        {
            const int w = juce::roundToInt (draw::trackedTextWidth (font, chip->getButtonText().toUpperCase())) + 26;
            if (row.getWidth() < w) { chip->setBounds (0, 0, 0, 0); continue; }
            chip->setBounds (row.removeFromLeft (w));
            row.removeFromLeft (6);
        }
    }
    area.removeFromTop (pad / 2);
    viewport.setBounds (area);
    layoutGrid();
}

void PresetBrowser::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (Theme::background.withAlpha (0.965f));
    g.fillRect (b);
    juce::ColourGradient vignette (juce::Colours::transparentBlack, b.getCentreX(), b.getCentreY(), juce::Colours::black.withAlpha (0.45f), b.getX(), b.getY(), true);
    g.setGradientFill (vignette);
    g.fillRect (b);
    draw::softLight (g, { b.getWidth() * 0.2f, b.getHeight() * 0.1f }, b.getWidth() * 0.35f, Theme::blue, 0.05f);
    draw::softLight (g, { b.getWidth() * 0.85f, b.getHeight() * 0.9f }, b.getWidth() * 0.3f, Theme::violet, 0.05f);

    const auto h = headerArea.toFloat();
    const float titleH = juce::jlimit (14.0f, 22.0f, h.getHeight() * 0.42f);
    draw::trackedText (g, "BROWSER", h.withHeight (titleH * 1.4f), juce::Justification::centredLeft, Theme::titleFont (titleH), Theme::textPrimary);
    const juce::String caption = "FACTORY LIBRARY   \xC2\xB7   " + juce::String (visibleCards.size()) + " OF " + juce::String (cards.size()) + " PRESETS";
    draw::trackedText (g, juce::String::fromUTF8 (caption.toRawUTF8()), h.withTop (h.getY() + titleH * 1.4f), juce::Justification::topLeft, Theme::captionFont (juce::jlimit (7.5f, 10.0f, titleH * 0.5f)), Theme::textSecondary);
    juce::Path line; line.startNewSubPath (h.getX(), h.getBottom() + 4.0f); line.lineTo (h.getX() + 84.0f, h.getBottom() + 4.0f);
    draw::glowPath (g, line, Theme::blue, 1.2f, 7.0f, 0.6f);
    g.setColour (Theme::borderSoft);
    g.drawLine (h.getX() + 84.0f, h.getBottom() + 4.0f, h.getRight(), h.getBottom() + 4.0f, 1.0f);
}

} // namespace am::ui

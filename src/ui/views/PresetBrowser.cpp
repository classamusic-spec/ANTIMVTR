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

    /** Rows shrink so the whole list always fits: a browser that hides
        categories below the fold is a browser you cannot navigate. */
    int rowHeight() const { return browser::categoryRowHeight (getHeight(), (int) entries.size() + 1); }

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
            // Same sidebar pill as the page selectors, so the browser belongs to the
            // instrument rather than to a dialog.
            if (on || hv) draw::sidebarPill (g, row.reduced (0.0f, 1.0f), 6.0f, Theme::blue, on ? 1.0f : 0.0f, hv ? 1.0f : 0.0f);
            const auto col = on ? Theme::textPrimary : (hv ? Theme::textPrimary.withAlpha (0.75f) : Theme::textSecondary);
            draw::trackedText (g, entries[(size_t) i].name, row.withTrimmedLeft (12.0f), juce::Justification::centredLeft, on ? Theme::labelFontStrong (h) : Theme::labelFont (h), col);
            draw::trackedText (g, juce::String (entries[(size_t) i].count), row.withTrimmedRight (8.0f), juce::Justification::centredRight, Theme::valueFont (h), Theme::textDim);
            y += (float) rowHeight();
        }
    }

    int rowAt (juce::Point<int> p) const
    {
        const int r = (p.y - rowHeight()) / juce::jmax (1, rowHeight());
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
/** A viewport that says when it has scrolled, so the card pool can follow it. */
class PresetBrowser::GridViewport : public juce::Viewport
{
public:
    std::function<void()> onScroll;
    void visibleAreaChanged (const juce::Rectangle<int>&) override { if (onScroll) onScroll(); }
};

//==============================================================================
PresetBrowser::PresetBrowser (AntiMatrProcessor& p) : processor (p)
{
    setWantsKeyboardFocus (true);
    setOpaque (false);
    categories = std::make_unique<CategoryList>();
    categories->onSelect = [this] (const juce::String& c) { filter.category = c; refilter(); };
    addAndMakeVisible (*categories);

    close.setTooltip ("Close (Esc)");
    close.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (close);

    clearFilters.setTooltip ("Clear the search and every tag");
    clearFilters.setOutlined (true);
    clearFilters.onClick = [this]
    {
        search.setText ({}, juce::dontSendNotification);
        for (auto& chip : tagChips) chip->setToggleState (false, juce::dontSendNotification);
        refilter();
    };
    addChildComponent (clearFilters);

    search.setMultiLine (false);
    search.setReturnKeyStartsNewLine (false);
    search.setSelectAllWhenFocused (true);
    search.setFont (Theme::bodyFont (14.0f));
    search.setTextToShowWhenEmpty ("SEARCH PRESETS", Theme::textDim);
    search.setIndents (12, 0);
    search.setJustification (juce::Justification::centredLeft);
    search.addListener (this);
    addAndMakeVisible (search);

    chipViewport.setViewedComponent (&chipStrip, false);
    chipViewport.setScrollBarsShown (true, false, true, false);
    chipViewport.setScrollBarThickness (5);
    addAndMakeVisible (chipViewport);

    viewport = std::make_unique<GridViewport>();
    viewport->setViewedComponent (&grid, false);
    viewport->setScrollBarsShown (true, false);
    viewport->setScrollBarThickness (6);
    viewport->onScroll = [this] { updateCardWindow (false); };
    addAndMakeVisible (*viewport);

    rebuildCatalogue();
    processor.addChangeListener (this);
}

PresetBrowser::~PresetBrowser()
{
    processor.removeChangeListener (this);
}

void PresetBrowser::visibilityChanged()
{
    if (! isVisible()) return;
    rebuildCatalogue();
    // Open on the patch you are playing: at 300 presets the alternative is
    // landing at the top of the bank and hunting for where you already were.
    // The overlay is made visible before it is given bounds, so the grid has no
    // size yet and the scroll has to wait for the layout that follows.
    pendingScrollTo = processor.currentPresetIndex();
    scrollToPreset (pendingScrollTo);
    grabKeyboardFocus();
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

    // Just start typing. At 300 presets that is how you find one, and making a
    // player aim at the search box first would be the browser's slowest moment.
    // The arrow keys still audition, because they only mean something here.
    const auto character = key.getTextCharacter();
    if (character != 0 && ! juce::CharacterFunctions::isWhitespace (character)
        && ! search.hasKeyboardFocus (true))
    {
        search.grabKeyboardFocus();
        search.setText (search.getText() + juce::String::charToString (character), juce::sendNotificationSync);
        search.moveCaretToEnd();
        return true;
    }
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

//==============================================================================
void PresetBrowser::rebuildCatalogue()
{
    auto& presets = processor.presets();
    const int n = presets.numFactoryPresets();
    if (catalogue.size() == n && ! catalogue.entries.empty()) return;   // nothing new to read

    catalogue.clear();
    for (int i = 0; i < n; ++i)
    {
        const auto& f = presets.factoryPreset (i);
        catalogue.add (f.name, f.category, f.tags);
    }
    catalogue.finish();

    categories->entries.clear();
    for (int i = 0; i < catalogue.categories.size(); ++i)
        categories->entries.push_back ({ catalogue.categories[i], catalogue.categoryCounts[(size_t) i] });
    if (! catalogue.categories.contains (filter.category, true)) filter.category = "ALL";
    categories->selected = filter.category;
    categories->repaint();

    // One chip per distinct tag, in the vocabulary's own order so the strip
    // reads as CHARACTER, MOTION, SPACE, REGISTER, GESTURE, USE.
    tagChips.clear();
    for (const auto& t : catalogue.tags)
    {
        auto chip = std::make_unique<AMButton> (t, Theme::cyan);
        chip->setChip (true);
        chip->setClickingTogglesState (true);
        const auto group = browser::tagGroupOf (t);
        chip->setTooltip (group.isNotEmpty() ? group + " · " + t : t);
        chip->onClick = [this] { refilter(); };
        chipStrip.addAndMakeVisible (*chip);
        tagChips.push_back (std::move (chip));
    }
    layoutChips();
    refilter();
}

void PresetBrowser::refilter()
{
    filter.tags.clear();
    for (auto& c : tagChips)
        if (c->getToggleState()) filter.tags.add (c->getButtonText().toUpperCase());
    filter.query = search.getText().trim();

    browser::applyFilter (catalogue, filter, filtered);

    clearFilters.setVisible (filter.query.isNotEmpty() || ! filter.tags.isEmpty());
    layoutGrid();
    repaint();
}

//==============================================================================
void PresetBrowser::layoutGrid()
{
    const int width = viewport != nullptr ? viewport->getMaximumVisibleWidth() : 0;
    if (width <= 0) return;

    geometry = browser::gridGeometry (width, (int) filtered.size());
    grid.setSize (width, geometry.contentHeight);

    // The pool covers the viewport plus a row above and below, so a scroll
    // never shows an empty slot and never allocates a component.
    const int wanted = juce::jmin ((int) filtered.size(),
                                   browser::gridPoolSize (geometry, viewport->getMaximumVisibleHeight(), 1));
    while ((int) cards.size() > wanted) cards.pop_back();
    while ((int) cards.size() < wanted)
    {
        auto card = std::make_unique<AMPresetCard>();
        card->onClick = [this] (int idx) { select (idx); };
        card->onDoubleClick = [this] (int idx) { select (idx); if (onClose) onClose(); };
        grid.addAndMakeVisible (*card);
        cards.push_back (std::move (card));
    }

    updateCardWindow (true);
}

void PresetBrowser::updateCardWindow (bool force)
{
    if (viewport == nullptr || cards.empty())
    {
        windowValid = false;
        return;
    }

    const int scrollY = viewport->getViewPositionY();
    const auto next = browser::gridWindow (geometry, scrollY, viewport->getMaximumVisibleHeight(), 1, (int) filtered.size());
    if (! force && windowValid && next.first == window.first && next.count == window.count) return;

    window = next;
    windowValid = true;

    const int current = processor.currentPresetIndex();
    auto& presets = processor.presets();

    for (size_t k = 0; k < cards.size(); ++k)
    {
        const int slot = window.first + (int) k;
        auto& card = *cards[k];
        if (slot >= window.last() || slot >= (int) filtered.size())
        {
            card.setVisible (false);
            continue;
        }
        const int presetIndex = filtered[(size_t) slot];
        const auto& f = presets.factoryPreset (presetIndex);
        card.setPreset (presetIndex, f.name, f.category, f.tags);
        card.setCurrent (presetIndex == current);
        const auto slotBounds = browser::cardBounds (geometry, slot);
        card.setBounds (slotBounds.x, slotBounds.y, slotBounds.width, slotBounds.height);
        card.setVisible (true);
    }
}

void PresetBrowser::scrollToPreset (int presetIndex)
{
    if (viewport == nullptr || viewport->getMaximumVisibleHeight() <= 0) return;
    pendingScrollTo = -1;
    const auto at = std::find (filtered.begin(), filtered.end(), presetIndex);
    if (at == filtered.end()) return;

    const int slot = (int) std::distance (filtered.begin(), at);
    const auto bounds = browser::cardBounds (geometry, slot);
    const int centred = bounds.y - juce::jmax (0, (viewport->getMaximumVisibleHeight() - bounds.height) / 2);
    viewport->setViewPosition (0, juce::jmax (0, centred));
    updateCardWindow (true);
}

//==============================================================================
void PresetBrowser::layoutChips()
{
    if (chipArea.isEmpty()) return;

    const auto font = Theme::captionFont (9.0f);
    std::vector<int> widths;
    widths.reserve (tagChips.size());
    for (auto& chip : tagChips)
        widths.push_back (juce::roundToInt (draw::trackedTextWidth (font, chip->getButtonText().toUpperCase())) + 26);

    const auto strip = browser::chipStripGeometry (getHeight());
    const int stripWidth = juce::jmax (1, chipArea.getWidth() - 8);
    const auto flow = browser::flowChips (widths, stripWidth, strip.chipHeight, strip.gapX, strip.gapY);
    for (size_t i = 0; i < tagChips.size() && i < flow.bounds.size(); ++i)
    {
        const auto& b = flow.bounds[i];
        tagChips[i]->setBounds (b.x, b.y, b.width, b.height);
    }

    chipStrip.setSize (stripWidth, juce::jmax (strip.chipHeight, flow.height));
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
    search.setBounds (searchArea.removeFromLeft (juce::jmin (searchArea.getWidth(), 420)));
    searchArea.removeFromLeft (10);
    clearFilters.setBounds (searchArea.removeFromLeft (juce::jmin (78, juce::jmax (0, searchArea.getWidth()))).reduced (0, 3));
    area.removeFromTop (pad / 3);

    // Two rows of chips, scrollable: fifty tags do not fit on one line at any
    // window size the instrument opens at.
    chipArea = area.removeFromTop (browser::chipStripGeometry (getHeight()).height);
    chipViewport.setBounds (chipArea);
    layoutChips();
    area.removeFromTop (pad / 2);

    viewport->setBounds (area);
    layoutGrid();
    if (pendingScrollTo >= 0) scrollToPreset (pendingScrollTo);
}

void PresetBrowser::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    // A frosted sheet laid over the instrument: the chassis, not a dark scrim, and
    // no vignette — the page under it should read as out of focus, not switched off.
    g.setColour (Theme::background.withAlpha (0.972f));
    g.fillRect (b);
    juce::ColourGradient sheet (Theme::backgroundTop.withAlpha (0.85f), b.getCentreX(), b.getY(),
                                Theme::background.withAlpha (0.85f), b.getCentreX(), b.getBottom(), false);
    g.setGradientFill (sheet);
    g.fillRect (b);
    draw::grain (g, b, 0.5f);
    draw::softLight (g, { b.getWidth() * 0.2f, b.getHeight() * 0.1f }, b.getWidth() * 0.35f, Theme::blue, 0.045f);
    draw::softLight (g, { b.getWidth() * 0.85f, b.getHeight() * 0.9f }, b.getWidth() * 0.3f, Theme::violet, 0.045f);

    const auto h = headerArea.toFloat();
    const float titleH = juce::jlimit (14.0f, 22.0f, h.getHeight() * 0.42f);
    draw::trackedText (g, "BROWSER", h.withHeight (titleH * 1.4f), juce::Justification::centredLeft, Theme::titleFont (titleH), Theme::textPrimary);
    const juce::String caption = juce::String (juce::CharPointer_UTF8 ("FACTORY LIBRARY   \xC2\xB7   ")) + juce::String ((int) filtered.size()) + " OF " + juce::String (catalogue.size()) + " PRESETS";
    draw::trackedText (g, caption, h.withTop (h.getY() + titleH * 1.4f), juce::Justification::topLeft, Theme::captionFont (juce::jlimit (7.5f, 10.0f, titleH * 0.5f)), Theme::textSecondary);
    juce::Path line; line.startNewSubPath (h.getX(), h.getBottom() + 4.0f); line.lineTo (h.getX() + 84.0f, h.getBottom() + 4.0f);
    draw::glowPath (g, line, Theme::blue, 1.2f, 7.0f, 0.6f);
    g.setColour (Theme::borderSoft);
    g.drawLine (h.getX() + 84.0f, h.getBottom() + 4.0f, h.getRight(), h.getBottom() + 4.0f, 1.0f);

    // "Nothing matched" is a real state at 300 presets with three tags active.
    if (filtered.empty() && catalogue.size() > 0 && viewport != nullptr)
    {
        const auto empty = viewport->getBounds().toFloat().withHeight (60.0f);
        draw::trackedText (g, "NO PRESETS MATCH THIS FILTER", empty, juce::Justification::centredTop,
                           Theme::labelFont (11.0f), Theme::textDim);
    }
}

} // namespace am::ui

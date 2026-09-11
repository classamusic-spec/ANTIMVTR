#pragma once

#include "PresetBrowserModel.h"
#include "ui/components/AMPresetCard.h"
#include "ui/components/AMButton.h"
#include "plugin/AntiMatrProcessor.h"

namespace am::ui
{

/**
    Full-page preset BROWSER overlay: category list, search field, tag chips
    and a grid of AMPresetCards for the factory library. Clicking a card
    loads it (the browser stays open for auditioning); ESC or the close
    button dismisses it. Sized to its parent automatically.

    Built to hold a library of hundreds. Two things about it are deliberate:

      * The card grid is VIRTUAL. Cards are a small recycled pool covering the
        viewport plus a row of overscan, so the component count depends on the
        size of the window and not on the size of the bank — 300 presets cost
        the same as 36 to filter, lay out and scroll.

      * The tag chips WRAP into their own scrolling strip. The closed tag
        vocabulary is about fifty tags; one row holds eight, and the previous
        single-row layout silently dropped whatever did not fit, which made
        most of the vocabulary impossible to filter by.

    The arithmetic behind both lives in PresetBrowserModel.h, where it is unit
    tested without a screen.
*/
class PresetBrowser : public juce::Component,
                      private juce::ChangeListener,
                      private juce::TextEditor::Listener
{
public:
    explicit PresetBrowser (AntiMatrProcessor& p);
    ~PresetBrowser() override;

    std::function<void()> onClose;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void parentSizeChanged() override;
    bool keyPressed (const juce::KeyPress& key) override;
    void visibilityChanged() override;
    void mouseDown (const juce::MouseEvent&) override {}   // swallow clicks on the scrim

    /** Live card components — the recycled pool, never the size of the bank. */
    int numCardComponents() const noexcept { return (int) cards.size(); }
    /** How many presets survive the current filter. */
    int numFilteredPresets() const noexcept { return (int) filtered.size(); }

private:
    class CategoryList;
    class GridViewport;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void textEditorTextChanged (juce::TextEditor&) override { refilter(); }
    void textEditorEscapeKeyPressed (juce::TextEditor&) override { if (onClose) onClose(); }
    void rebuildCatalogue();
    void refilter();
    void layoutGrid();
    void updateCardWindow (bool force);
    void layoutChips();
    void scrollToPreset (int presetIndex);
    void select (int presetIndex);

    AntiMatrProcessor& processor;
    AMIconButton close { Icon::Close, Theme::textSecondary };
    AMButton clearFilters { "Clear", Theme::textSecondary };
    juce::TextEditor search;
    std::unique_ptr<CategoryList> categories;

    browser::Catalogue catalogue;
    browser::Filter    filter;
    std::vector<int>   filtered;          ///< catalogue indices surviving the filter

    std::vector<std::unique_ptr<AMButton>> tagChips;
    juce::Viewport  chipViewport;
    juce::Component chipStrip;

    std::unique_ptr<GridViewport> viewport;
    juce::Component grid;
    std::vector<std::unique_ptr<AMPresetCard>> cards;   ///< recycled pool, one per visible slot
    browser::GridGeometry geometry;
    browser::GridWindow   window;
    bool windowValid = false;
    int  pendingScrollTo = -1;   ///< preset to reveal once the grid has a size

    juce::Rectangle<int> headerArea, chipArea;
};

} // namespace am::ui

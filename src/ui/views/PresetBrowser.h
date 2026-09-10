#pragma once

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

private:
    class CategoryList;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void textEditorTextChanged (juce::TextEditor&) override { refilter(); }
    void textEditorEscapeKeyPressed (juce::TextEditor&) override { if (onClose) onClose(); }
    void rebuildCatalogue();
    void refilter();
    void layoutGrid();
    void select (int presetIndex);

    AntiMatrProcessor& processor;
    AMIconButton close { Icon::Close, Theme::textSecondary };
    juce::TextEditor search;
    std::unique_ptr<CategoryList> categories;
    std::vector<std::unique_ptr<AMButton>> tagChips;
    juce::StringArray allTags, activeTags;
    juce::Viewport viewport;
    juce::Component grid;
    std::vector<std::unique_ptr<AMPresetCard>> cards;
    std::vector<int> visibleCards;
    juce::String currentCategory { "ALL" };
    juce::Rectangle<int> headerArea, chipArea;
};

} // namespace am::ui

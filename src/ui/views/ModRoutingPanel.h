#pragma once

#include "Controls.h"
#include "ui/components/AMModAssign.h"

namespace am::ui
{

/**
    Column geometry of one routing row.

    Pure maths so that the table header and the rows are laid out by exactly
    the same code and can never drift apart. All rectangles are in the row's
    own coordinate space; `forRow` takes the row bounds (or, for the header,
    a rectangle of the same width and row height placed where the header is).
*/
struct ModRowLayout
{
    juce::Rectangle<int> chip;          ///< destination-coloured bar at the left edge
    juce::Rectangle<int> source;        ///< source name
    juce::Rectangle<int> arrow;         ///< the marker between source and destination
    juce::Rectangle<int> destination;   ///< destination name
    juce::Rectangle<int> depth;         ///< depth bar
    juce::Rectangle<int> value;         ///< depth readout
    juce::Rectangle<int> power;         ///< enable toggle
    juce::Rectangle<int> polarity;      ///< BI / UNI
    juce::Rectangle<int> remove;        ///< remove button

    static ModRowLayout forRow (juce::Rectangle<int> row) noexcept;

    /** Width of the source-to-destination block, used to align the header captions. */
    int namesWidth() const noexcept { return destination.getRight() - source.getX(); }
};

/**
    Number of columns for the source-scope grid: as square as the count allows,
    never so many that a card falls below a readable width, and preferring a
    full last row over a ragged one.
*/
int modScopeColumns (int count, int width) noexcept;

//==============================================================================
/**
    ROUTINGS — the modulation matrix on the MOD page.

    One row per routing: source to destination, a bipolar depth bar in the
    target's own units, a BI / UNI polarity switch, an on/off toggle and a
    remove button. Every row is tinted with its *destination's* section
    colour, so the matrix reads at a glance: cyan rows shape Matter, magenta
    rows drive Fracture, and so on. The depth bar carries a live overlay
    showing what the source is contributing right now.

    Below the list, one scope per source in use plots that source's recent
    output and how many destinations it drives, so the panel shows the
    modulation moving instead of leaving the space empty.

    Two ways to make a connection:
      * pick a source, press ASSIGN and click any knob in the plug-in, or
      * pick a source and choose the destination from the parameter menu.

    Everything goes through `AntiMatrProcessor::setModRoutings()`, which
    publishes the table to the audio thread and stores it in the patch.
*/
class ModRoutingPanel : public AMPanel,
                        private juce::Timer,
                        private juce::ChangeListener
{
public:
    explicit ModRoutingPanel (AntiMatrProcessor& p);
    ~ModRoutingPanel() override;

    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    class Row;
    class SourceScope;

    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void rebuildRows();
    void rebuildScopes();
    void refreshRows();
    void showSourceMenu();
    void showTargetMenu();
    bool addRouting (ModSource source, Param target);
    void removeRouting (int index);
    void setDepth (int index, float depth);
    void setBipolar (int index, bool bipolar);
    void setEnabled (int index, bool on);
    void updateAssignButton();

    void paintTableHeader (juce::Graphics& g);
    void paintEmptyState (juce::Graphics& g);

    int rowHeight() const;

    AntiMatrProcessor& processor;
    ModRoutingTable table;

    AMButton sourceButton { "LFO 1", Theme::amber };
    AMButton assignButton { "ASSIGN", Theme::amber };
    AMButton targetButton { "CHOOSE TARGET", Theme::textSecondary };
    AMButton clearButton  { "CLEAR ALL", Theme::textSecondary };

    juce::Rectangle<int> headerArea, listArea, scopeArea;
    juce::Viewport viewport;
    juce::Component rowHolder;
    std::vector<std::unique_ptr<Row>> rows;
    std::vector<std::unique_ptr<SourceScope>> scopes;

    ModSource pendingSource = ModSource::LFO1;
    float assignPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModRoutingPanel)
};

} // namespace am::ui

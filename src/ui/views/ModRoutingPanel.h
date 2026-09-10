#pragma once

#include "Controls.h"
#include "ui/components/AMModAssign.h"

namespace am::ui
{

/**
    ROUTINGS — the modulation matrix on the MOD page.

    One row per routing: source ▸ destination, a depth slider in the target's
    own units, a BI / UNI polarity switch, an on/off dot and a remove button.
    A live bar behind each row shows what the source is doing right now.

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

    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void rebuildRows();
    void refreshRows();
    void showSourceMenu();
    void showTargetMenu();
    bool addRouting (ModSource source, Param target);
    void removeRouting (int index);
    void setDepth (int index, float depth);
    void setBipolar (int index, bool bipolar);
    void setEnabled (int index, bool on);
    void updateAssignButton();

    int rowHeight() const;

    AntiMatrProcessor& processor;
    ModRoutingTable table;

    AMButton sourceButton { "LFO 1", Theme::amber };
    AMButton assignButton { "ASSIGN", Theme::amber };
    AMButton targetButton { "CHOOSE TARGET", Theme::textSecondary };
    AMButton clearButton  { "CLEAR ALL", Theme::textSecondary };

    juce::Rectangle<int> headerArea;
    juce::Viewport viewport;
    juce::Component rowHolder;
    std::vector<std::unique_ptr<Row>> rows;

    ModSource pendingSource = ModSource::LFO1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModRoutingPanel)
};

} // namespace am::ui

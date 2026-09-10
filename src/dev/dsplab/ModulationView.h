#pragma once

#include "LabWidgets.h"
#include "dsp/mod/ModulationSnapshot.h"
#include "state/ModRouting.h"

namespace am::dev
{

/**
    MODULATION (§77)

    What every modulation source is doing right now — a rolling scope and a
    numeric readout per source — beside the live routing matrix: source,
    destination, depth in the destination's own units, polarity, whether the
    routing runs per voice or once for the whole engine, and the contribution
    it is making this block with the range it has covered recently.

    Everything comes from the `ModulationSnapshot` the engine publishes each
    block plus the message-thread routing table; the view never touches the
    audio thread.
*/
class ModulationView : public LabView
{
public:
    ModulationView();
    ~ModulationView() override;

    void updateFrame (const LabFrame& f) override;
    void resized() override;

private:
    class SourceGrid;

    KeyValueTable facts { "Modulation engine" };
    LabPanel sourcePanel { "Sources  (rolling scope, current value)" };
    std::unique_ptr<SourceGrid> grid;
    LabPanel routingPanel { "Routings" };
    LabTable routingTable;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModulationView)
};

} // namespace am::dev

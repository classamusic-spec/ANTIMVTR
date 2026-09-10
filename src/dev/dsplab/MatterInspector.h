#pragma once

#include "LabWidgets.h"
#include "MatterCapture.h"
#include "ResonanceDistributionView.h"
#include "TopologyView.h"

namespace am::dev
{

/**
    MATTER INSPECTOR (§71–73)

    The MATTER tab: a sortable table of every published node (index,
    frequency, target, ratio to the fundamental, energy, weight, damping,
    cluster, pan, nonlinearity, excitation, coupling count), the target vs
    actual frequency plot, the coupling graph, the ratio histogram, the
    harmonicity deviation and the summary statistics — plus the BEFORE /
    AFTER capture buttons the EVOLVE tab compares.
*/
class MatterInspector : public LabView
{
public:
    explicit MatterInspector (MatterCaptureStore& captures);

    void updateFrame (const LabFrame& f) override;
    void resized() override;

private:
    void rebuildTable (const DiagnosticSnapshot& s);
    void rebuildSummary (const DiagnosticSnapshot& s);
    void applySource();

    MatterCaptureStore& captures;
    DiagnosticSnapshot latest;
    bool haveSnapshot = false;

    /** Which node set the plots show: the live engine or one of the captures. */
    enum class Source { Live = 0, Before, After };
    Source source = Source::Live;

    juce::ComboBox sourceBox;
    juce::ToggleButton clusterMode { "Cluster graph" };
    juce::TextButton captureBefore { "CAPTURE BEFORE" }, captureAfter { "CAPTURE AFTER" }, clearCaptures { "CLEAR" };

    KeyValueTable summary { "Matter summary" };
    LabTable nodeTable;
    LabPanel nodePanel { "Nodes  (click a header to sort)" };
    FrequencyDistributionView distribution;
    TopologyView topology;
    RatioHistogramView histogram;
    HarmonicityView harmonicity;
};

} // namespace am::dev

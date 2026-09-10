#pragma once

#include "LabWidgets.h"
#include "MatterCapture.h"
#include "ResonanceDistributionView.h"
#include "dev/diagnostics/MatterAnalysis.h"

namespace am::dev
{

/**
    EVOLVE VIEW (§75)

    Compares the BEFORE and AFTER Matter captures taken on the MATTER tab:
    the two distributions above each other, how far each node moved (in
    cents), how the cluster and energy balance changed, and an A/B between
    RAW (Evolve bypassed) and EVOLVED using the DevControls bypass.

    The per-operator toggles write DevControls::evolveBypassMask (one bit per
    EvolveOperator), and the live table shows what EvolveEngine did to the
    focus voice in the last block (DiagnosticSnapshot::evolve).
*/
class EvolveView : public LabView
{
public:
    explicit EvolveView (MatterCaptureStore& captures);

    void updateFrame (const LabFrame& f) override;
    void resized() override;

private:
    void applyBypass (bool raw);
    void applyOperatorBypass();

    MatterCaptureStore& captures;
    Diagnostics* diagnostics = nullptr;

    FrequencyDistributionView beforePlot, afterPlot;
    KeyValueTable delta { "Distribution delta" };
    KeyValueTable evolveParams { "Evolve parameters (effective)" };
    KeyValueTable engineTable { "Evolve engine (focus voice, last block)" };
    LabPanel abPanel { "A / B  raw vs evolved" };
    LabPanel operatorPanel { "Per-operator bypass" };
    juce::TextButton rawButton { "RAW  (bypass)" }, evolvedButton { "EVOLVED" };
    std::vector<std::unique_ptr<juce::ToggleButton>> operatorToggles;
};

} // namespace am::dev

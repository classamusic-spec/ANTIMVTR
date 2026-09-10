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

    Per-operator bypass is laid out but disabled: the engine publishes a
    single `bypassEvolve` flag, and adding per-operator flags is the Evolve
    owner's call, not DSP LAB's.
*/
class EvolveView : public LabView
{
public:
    explicit EvolveView (MatterCaptureStore& captures);

    void updateFrame (const LabFrame& f) override;
    void resized() override;

private:
    void applyBypass (bool raw);

    MatterCaptureStore& captures;
    Diagnostics* diagnostics = nullptr;

    FrequencyDistributionView beforePlot, afterPlot;
    KeyValueTable delta { "Distribution delta" };
    KeyValueTable evolveParams { "Evolve parameters (effective)" };
    LabPanel abPanel { "A / B  raw vs evolved" };
    LabPanel operatorPanel { "Per-operator bypass" };
    juce::TextButton rawButton { "RAW  (bypass)" }, evolvedButton { "EVOLVED" };
    std::vector<std::unique_ptr<juce::ToggleButton>> operatorToggles;
};

} // namespace am::dev

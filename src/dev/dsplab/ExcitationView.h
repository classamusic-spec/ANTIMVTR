#pragma once

#include "SignalInspector.h"

namespace am::dev
{

/** Energy per cluster and per node, with the strongest exciters listed (§74). */
class ExcitationEnergyView : public LabPanel
{
public:
    ExcitationEnergyView();
    void setSnapshot (const DiagnosticSnapshot& s);
    void paint (juce::Graphics& g) override;

private:
    struct ClusterBar { uint8_t cluster = 0; float energy = 0.0f; float excitation = 0.0f; int nodes = 0; };
    std::vector<ClusterBar> clusters;
    float maxEnergy = 0.0f;
    int numNodes = 0;
};

//==============================================================================
/**
    EXCITATION VIEW (§74) — the SOURCE tab.

    Dry-mode selection (SOURCE ONLY / MATTER ONLY / MATTER + EVOLVE / FULL
    PATH) written straight into DevControls, the raw-audio inspector pinned to
    the source tap, the per-stage level meters and the energy the excitation
    deposits per cluster and per node.
*/
class ExcitationView : public LabView
{
public:
    ExcitationView();

    void updateFrame (const LabFrame& f) override;
    void resized() override;

private:
    void applyDryMode (int mode, Diagnostics* diag);

    juce::TextButton dryButtons[(int) DryMode::Count];
    LabPanel dryPanel { "Dry mode  (DevControls.dryMode)" };
    SignalInspector inspector;
    StageMeters meters;
    ExcitationEnergyView energy;
    KeyValueTable sourceInfo { "Source stage" };
    Diagnostics* diagnostics = nullptr;
    int currentDryMode = 0;
};

} // namespace am::dev

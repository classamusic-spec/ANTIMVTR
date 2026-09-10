#pragma once

#include "LabWidgets.h"
#include "state/ParameterRegistry.h"

namespace am::dev
{

/**
    MODULATION / PARAMETER TRACE (§77)

    Every parameter with its base value (what the host owns), the modulation
    contribution added this block and the effective value the engine uses,
    read from ControlGraph — plus the descriptor facts that explain the
    mapping: group, unit, range, skew, smoothing kind and whether the
    parameter is modulatable at all.

    A search box filters the table; selecting a row opens the end-to-end
    trace of that parameter, updating live.
*/
class ParameterTraceView : public LabView
{
public:
    ParameterTraceView();
    ~ParameterTraceView() override;

    void updateFrame (const LabFrame& f) override;
    void resized() override;

private:
    void rebuildRows (const LabFrame& f);
    void refreshTrace (const LabFrame& f);
    bool matchesFilter (const ParamDesc& d) const;

    LabPanel tablePanel { "Parameters  (search, then click a row to trace)" };
    LabTable table;
    juce::TextEditor search;
    juce::ToggleButton onlyChanged { "Only non-default" }, onlyModulated { "Only modulated" };

    KeyValueTable trace { "Parameter trace" };
    LabPanel historyPanel { "Effective value history" };

    std::vector<int> visibleParams;      ///< model row -> parameter index
    int selectedParam = -1;
    bool selectionSynced = false;
    juce::String filterText;

    static constexpr int kHistory = 300;
    std::array<float, kHistory> history {};
    int historyWrite = 0;
    bool historyFilled = false;
    float historyMin = 0.0f, historyMax = 1.0f;

    class HistoryPlot;
    std::unique_ptr<HistoryPlot> historyPlot;
};

} // namespace am::dev

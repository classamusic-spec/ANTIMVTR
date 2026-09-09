#include "ParameterTraceView.h"

#include "plugin/AntiMatrProcessor.h"

namespace am::dev
{

using namespace am::ui;

namespace
{
    const char* smoothingName (SmoothingKind k) noexcept
    {
        switch (k)
        {
            case SmoothingKind::None:   return "none";
            case SmoothingKind::Fast:   return "fast";
            case SmoothingKind::Medium: return "medium";
            case SmoothingKind::Slow:   return "slow";
            default:                    return "?";
        }
    }

    const char* kindName (ParamKind k) noexcept
    {
        switch (k)
        {
            case ParamKind::Float:  return "float";
            case ParamKind::Bool:   return "bool";
            case ParamKind::Choice: return "choice";
            case ParamKind::Int:    return "int";
            default:                return "?";
        }
    }
}

//==============================================================================
/** Rolling plot of the effective value of the traced parameter. */
class ParameterTraceView::HistoryPlot : public juce::Component
{
public:
    void setData (const float* values, int count, int writePos, bool filled, float lo, float hi, juce::String label)
    {
        data.assign (values, values + count);
        write = writePos;
        wrapped = filled;
        minValue = lo;
        maxValue = hi;
        caption = std::move (label);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat().reduced (2.0f);
        if (data.empty() || (! wrapped && write < 2))
        {
            plot::emptyState (g, area, "select a parameter");
            return;
        }

        const float span = juce::jmax (1.0e-6f, maxValue - minValue);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        for (int i = 0; i <= 4; ++i)
        {
            const float y = area.getY() + area.getHeight() * (float) i / 4.0f;
            g.drawLine (area.getX(), y, area.getRight(), y, 1.0f);
        }

        const int count = wrapped ? (int) data.size() : write;
        juce::Path p;
        for (int i = 0; i < count; ++i)
        {
            const int index = wrapped ? (write + i) % (int) data.size() : i;
            const float x = area.getX() + area.getWidth() * (float) i / (float) juce::jmax (1, count - 1);
            const float v = juce::jlimit (0.0f, 1.0f, (data[(size_t) index] - minValue) / span);
            const float y = area.getBottom() - v * area.getHeight();
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (Theme::amber);
        g.strokePath (p, juce::PathStrokeType (1.2f));

        plot::caption (g, juce::String (maxValue, 3), area.removeFromTop (10.0f), Theme::textDim, 8.0f);
        plot::caption (g, juce::String (minValue, 3),
                       juce::Rectangle<float> (area.getX(), area.getBottom() - 10.0f, 60.0f, 10.0f), Theme::textDim, 8.0f);
        plot::caption (g, caption, juce::Rectangle<float> (area.getX(), area.getY(), area.getWidth(), 10.0f),
                       Theme::textDim, 8.0f, juce::Justification::centredRight);
    }

private:
    std::vector<float> data;
    int write = 0;
    bool wrapped = false;
    float minValue = 0.0f, maxValue = 1.0f;
    juce::String caption;
};

//==============================================================================
ParameterTraceView::ParameterTraceView()
{
    search.setTextToShowWhenEmpty ("search id, name or group...", Theme::textDim);
    search.setFont (Theme::font (11.5f));
    search.setColour (juce::TextEditor::backgroundColourId, Theme::panelInset);
    search.setColour (juce::TextEditor::outlineColourId, Theme::border);
    search.setColour (juce::TextEditor::textColourId, Theme::textPrimary);
    search.onTextChange = [this] { filterText = search.getText().trim().toLowerCase(); };
    addAndMakeVisible (search);

    styleToggle (onlyChanged, Theme::amber);
    styleToggle (onlyModulated, Theme::violet);
    addAndMakeVisible (onlyChanged);
    addAndMakeVisible (onlyModulated);

    table.setColumns ({
        { "ID",       190, false }, { "GROUP",  70, false }, { "BASE",   66, true },
        { "MOD",       62, true },  { "EFFECT", 66, true },  { "MIN",    58, true },
        { "MAX",       58, true },  { "SMOOTH", 58, false }, { "KIND",   50, false },
        { "MODULATABLE", 44, true }
    });
    table.onSelectionChanged = [this] (int row)
    {
        selectedParam = (row >= 0 && row < (int) visibleParams.size()) ? visibleParams[(size_t) row] : -1;
        historyWrite = 0;
        historyFilled = false;
        history.fill (0.0f);
    };
    tablePanel.addAndMakeVisible (table);
    addAndMakeVisible (tablePanel);

    trace.setRowHeight (14.0f);
    trace.setLabelWidthFraction (0.46f);
    addAndMakeVisible (trace);

    historyPlot = std::make_unique<HistoryPlot>();
    historyPanel.addAndMakeVisible (*historyPlot);
    addAndMakeVisible (historyPanel);
}

bool ParameterTraceView::matchesFilter (const ParamDesc& d) const
{
    if (filterText.isEmpty())
        return true;
    return juce::String (d.id).toLowerCase().contains (filterText)
        || juce::String (d.name).toLowerCase().contains (filterText)
        || juce::String (ParameterRegistry::groupName (d.group)).toLowerCase().contains (filterText);
}

void ParameterTraceView::rebuildRows (const LabFrame& f)
{
    const auto& control = f.processor.engine().control();
    const auto& base = control.baseValues();
    const auto& effective = control.values();

    std::vector<LabTable::Row> rows;
    std::vector<int> ids;
    rows.reserve ((size_t) kNumParams);
    ids.reserve ((size_t) kNumParams);

    for (const auto& d : ParameterRegistry::all())
    {
        if (! matchesFilter (d))
            continue;

        const int index = paramIndex (d.param);
        const float b = base[(size_t) index];
        const float e = effective[(size_t) index];
        const float m = control.modulationOf (d.param);

        if (onlyChanged.getToggleState() && std::abs (b - d.defaultValue) < 1.0e-6f)
            continue;
        if (onlyModulated.getToggleState() && std::abs (m) < 1.0e-6f)
            continue;

        const auto modColour = std::abs (m) > 1.0e-6f ? Theme::violet : Theme::textDim;
        const auto baseColour = std::abs (b - d.defaultValue) > 1.0e-6f ? Theme::amber : Theme::textPrimary;

        rows.push_back ({
            { juce::String (d.id), 0.0, Theme::textSecondary },
            { juce::String (ParameterRegistry::groupName (d.group)), 0.0, Theme::textDim },
            { juce::String (b, 4), b, baseColour },
            { juce::String (m, 4), m, modColour },
            { juce::String (e, 4), e, Theme::cyan },
            { juce::String (d.min, 2), d.min, Theme::textDim },
            { juce::String (d.max, 2), d.max, Theme::textDim },
            { juce::String (smoothingName (d.smoothing)), 0.0, Theme::textDim },
            { juce::String (kindName (d.kind)), 0.0, Theme::textDim },
            { d.modulatable ? "yes" : "-", d.modulatable ? 1.0 : 0.0, d.modulatable ? Theme::textSecondary : Theme::textDim }
        });
        ids.push_back (index);
    }

    visibleParams = std::move (ids);
    table.setRows (std::move (rows));
    tablePanel.setSubtitle (juce::String ((int) visibleParams.size()) + " / " + juce::String (kNumParams) + " PARAMETERS");
}

void ParameterTraceView::refreshTrace (const LabFrame& f)
{
    if (selectedParam < 0 || selectedParam >= kNumParams)
    {
        trace.setRows ({ { "No parameter selected", "" },
                         { "", "" },
                         { "Click a row to trace it", "" },
                         { "id -> base -> modulation", "" },
                         { "-> effective -> group", "" } });
        historyPlot->setData (history.data(), kHistory, 0, false, 0.0f, 1.0f, {});
        return;
    }

    const auto param = paramFromIndex (selectedParam);
    const auto& d = ParameterRegistry::get (param);
    const auto& control = f.processor.engine().control();
    const float base = control.baseValues()[(size_t) selectedParam];
    const float effective = control.values()[(size_t) selectedParam];
    const float mod = control.modulationOf (param);

    history[(size_t) historyWrite] = effective;
    historyWrite = (historyWrite + 1) % kHistory;
    if (historyWrite == 0) historyFilled = true;

    const int count = historyFilled ? kHistory : historyWrite;
    historyMin = d.min;
    historyMax = d.max;
    if (! (historyMax > historyMin))
    {
        historyMin = 0.0f;
        historyMax = juce::jmax (1.0f, (float) juce::jmax (1, d.numChoices() - 1));
    }
    historyPlot->setData (history.data(), kHistory, historyWrite, historyFilled, historyMin, historyMax,
                          juce::String (count) + " FRAMES");

    juce::String hostValue = "-";
    if (auto* hostParam = f.processor.parameters().getParameter (d.id))
        hostValue = juce::String (hostParam->getValue(), 4) + " norm";

    trace.setRows ({
        { "id",              juce::String (d.id) },
        { "name",            juce::String (d.name) },
        { "group",           juce::String (ParameterRegistry::groupName (d.group)) },
        { "kind",            juce::String (kindName (d.kind)) },
        { "", "" },
        { "1. host (APVTS)", hostValue },
        { "2. base",         juce::String (base, 5) + (d.unit[0] != 0 ? " " + juce::String (d.unit) : juce::String()) },
        { "3. modulation",   juce::String (mod, 5) },
        { "4. smoothing",    juce::String (smoothingName (d.smoothing)) + "  ("
                             + juce::String (ParameterRegistry::smoothingMs (d.smoothing), 0) + " ms)" },
        { "5. effective",    juce::String (effective, 5) + (d.unit[0] != 0 ? " " + juce::String (d.unit) : juce::String()) },
        { "6. formatted",    d.formatValue (effective) },
        { "", "" },
        { "range",           juce::String (d.min, 3) + " .. " + juce::String (d.max, 3) },
        { "default",         juce::String (d.defaultValue, 4) },
        { "skew",            juce::String (d.skew, 3) },
        { "modulatable",     d.modulatable ? "yes" : "no" },
        { "mutation",        juce::String (ParameterRegistry::mutationName (d.mutation)) },
    });

    trace.clearRowColours();
    trace.setRowColour (6, Theme::amber);
    trace.setRowColour (7, std::abs (mod) > 1.0e-6f ? Theme::violet : Theme::textDim);
    trace.setRowColour (9, Theme::cyan);
}

void ParameterTraceView::updateFrame (const LabFrame& f)
{
    rebuildRows (f);
    refreshTrace (f);
}

void ParameterTraceView::resized()
{
    auto area = getLocalBounds();

    auto right = area.removeFromRight (juce::jmax (250, area.getWidth() * 30 / 100));
    area.removeFromRight (5);

    trace.setBounds (right.removeFromTop (juce::jmax (250, right.getHeight() * 68 / 100)));
    right.removeFromTop (5);
    historyPanel.setBounds (right);
    historyPlot->setBounds (historyPanel.contentBounds());

    auto searchRow = area.removeFromTop (24);
    search.setBounds (searchRow.removeFromLeft (juce::jmax (180, searchRow.getWidth() / 3)));
    searchRow.removeFromLeft (10);
    onlyChanged.setBounds (searchRow.removeFromLeft (130));
    onlyModulated.setBounds (searchRow.removeFromLeft (130));
    area.removeFromTop (5);

    tablePanel.setBounds (area);
    table.setBounds (tablePanel.contentBounds());
}

} // namespace am::dev

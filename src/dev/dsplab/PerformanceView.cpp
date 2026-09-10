#include "PerformanceView.h"

#include "plugin/AntiMatrProcessor.h"

namespace am::dev
{

using namespace am::ui;

namespace
{
    juce::Colour loadColour (float percent) noexcept
    {
        if (percent >= 90.0f) return Theme::magenta;
        if (percent >= 60.0f) return Theme::amber;
        if (percent >= 25.0f) return Theme::cyan;
        return Theme::textPrimary;
    }

    juce::String bytes (size_t n)
    {
        if (n >= 1024 * 1024) return juce::String ((double) n / (1024.0 * 1024.0), 2) + " MB";
        if (n >= 1024)        return juce::String ((double) n / 1024.0, 1) + " KB";
        return juce::String ((int) n) + " B";
    }
}

//==============================================================================
CpuHistoryView::CpuHistoryView() : LabPanel ("CPU history  (last 300 frames)") {}

void CpuHistoryView::push (float movingPercent, float peakPercent)
{
    moving[(size_t) write] = std::isfinite (movingPercent) ? movingPercent : 0.0f;
    peak[(size_t) write] = std::isfinite (peakPercent) ? peakPercent : 0.0f;
    write = (write + 1) % kHistory;
    if (write == 0) filled = true;
    repaint();
}

void CpuHistoryView::paint (juce::Graphics& g)
{
    LabPanel::paint (g);

    auto area = contentBoundsF();
    const int count = filled ? kHistory : write;
    if (count < 2)
    {
        plot::emptyState (g, area, "collecting");
        return;
    }

    float top = 10.0f;
    for (int i = 0; i < count; ++i)
        top = juce::jmax (top, moving[(size_t) i]);
    top = juce::jmin (200.0f, std::ceil (top * 1.25f / 10.0f) * 10.0f);

    // Budget line at 100 %.
    if (top >= 100.0f)
    {
        const float y = area.getBottom() - (100.0f / top) * area.getHeight();
        g.setColour (Theme::magenta.withAlpha (0.35f));
        g.drawLine (area.getX(), y, area.getRight(), y, 1.0f);
        plot::caption (g, "100% BUDGET", juce::Rectangle<float> (area.getRight() - 80.0f, y - 10.0f, 78.0f, 10.0f),
                       Theme::magenta.withAlpha (0.6f), 8.0f, juce::Justification::centredRight);
    }

    for (int i = 1; i < 4; ++i)
    {
        const float y = area.getY() + area.getHeight() * (float) i / 4.0f;
        g.setColour (juce::Colours::white.withAlpha (0.045f));
        g.drawLine (area.getX(), y, area.getRight(), y, 1.0f);
    }

    juce::Path p;
    for (int i = 0; i < count; ++i)
    {
        const int index = filled ? (write + i) % kHistory : i;
        const float x = area.getX() + area.getWidth() * (float) i / (float) (count - 1);
        const float y = area.getBottom() - juce::jlimit (0.0f, 1.0f, moving[(size_t) index] / top) * area.getHeight();
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }

    juce::Path filledPath (p);
    filledPath.lineTo (area.getRight(), area.getBottom());
    filledPath.lineTo (area.getX(), area.getBottom());
    filledPath.closeSubPath();
    g.setColour (Theme::cyan.withAlpha (0.14f));
    g.fillPath (filledPath);
    g.setColour (Theme::cyan);
    g.strokePath (p, juce::PathStrokeType (1.2f));

    plot::caption (g, juce::String (top, 0) + "%", area.removeFromTop (10.0f), Theme::textDim, 8.0f);
    plot::caption (g, "NOW", juce::Rectangle<float> (area.getRight() - 34.0f, area.getBottom() - 10.0f, 32.0f, 10.0f),
                   Theme::textDim, 8.0f, juce::Justification::centredRight);
}

//==============================================================================
PerformanceView::PerformanceView()
{
    subsystems.setColumns ({
        { "SUBSYSTEM", 110, false }, { "MOVING %", 74, true }, { "AVG %", 68, true },
        { "PEAK %", 68, true }, { "BAR", 120, true }
    });
    tablePanel.addAndMakeVisible (subsystems);
    addAndMakeVisible (tablePanel);
    addAndMakeVisible (history);

    engineInfo.setRowHeight (14.0f);
    engineInfo.setLabelWidthFraction (0.56f);
    memory.setRowHeight (14.0f);
    memory.setLabelWidthFraction (0.6f);
    addAndMakeVisible (engineInfo);
    addAndMakeVisible (memory);

    styleToggle (profiling);
    profiling.setToggleState (true, juce::dontSendNotification);
    profiling.onClick = [this]
    {
        if (diagnostics != nullptr)
            diagnostics->dev.profiling.store (profiling.getToggleState(), std::memory_order_relaxed);
    };
    styleButton (resetProfiler);
    resetProfiler.onClick = [this]
    {
        if (diagnostics != nullptr)
            diagnostics->profiler.reset();
    };
    controls.addAndMakeVisible (profiling);
    controls.addAndMakeVisible (resetProfiler);
    addAndMakeVisible (controls);
}

void PerformanceView::rebuildSubsystems (const DiagnosticSnapshot& s)
{
    std::vector<LabTable::Row> rows;
    rows.reserve ((size_t) PerformanceProfiler::kNumSubsystems + 1);

    float largest = 0.1f;
    for (int i = 0; i < PerformanceProfiler::kNumSubsystems; ++i)
        largest = juce::jmax (largest, s.perf.movingPercent[i]);

    auto barText = [largest] (float value)
    {
        const int width = juce::jlimit (0, 18, (int) std::lround (value / largest * 18.0f));
        return juce::String::repeatedString ("|", width);
    };

    for (int i = 0; i < PerformanceProfiler::kNumSubsystems; ++i)
    {
        const float moving = s.perf.movingPercent[i];
        rows.push_back ({
            { juce::String (subsystemName ((Subsystem) i)), 0.0, Theme::textSecondary },
            { juce::String (moving, 3), moving, loadColour (moving) },
            { juce::String (s.perf.avgPercent[i], 3), s.perf.avgPercent[i], Theme::textPrimary },
            { juce::String (s.perf.peakPercent[i], 3), s.perf.peakPercent[i], loadColour (s.perf.peakPercent[i]) },
            { barText (moving), moving, Theme::cyan.withAlpha (0.75f) }
        });
    }

    rows.push_back ({
        { "TOTAL", 0.0, Theme::amber },
        { juce::String (s.perf.totalMovingPercent, 3), s.perf.totalMovingPercent, loadColour (s.perf.totalMovingPercent) },
        { juce::String (s.perf.totalAvgPercent, 3), s.perf.totalAvgPercent, Theme::amber },
        { juce::String (s.perf.totalPeakPercent, 3), s.perf.totalPeakPercent, loadColour (s.perf.totalPeakPercent) },
        { "", s.perf.totalMovingPercent, Theme::amber }
    });

    subsystems.setRows (std::move (rows));
}

void PerformanceView::rebuildEngine (const LabFrame& f)
{
    const auto& s = f.snapshot;
    static const char* qualityNames[] = { "ECO", "NORMAL", "HIGH", "ULTRA" };
    const double budgetMs = s.perf.budgetMicros * 0.001;
    const double lastMs = s.perf.lastBlockMicros * 0.001;

    engineInfo.setRows ({
        { "Sample rate",     juce::String (s.sampleRate, 0) + " Hz" },
        { "Block size",      juce::String (s.blockSize) + " smp" },
        { "Block budget",    juce::String (s.perf.budgetMicros, 1) + " us  (" + juce::String (budgetMs, 3) + " ms)" },
        { "Last block",      juce::String (s.perf.lastBlockMicros, 1) + " us  (" + juce::String (lastMs, 3) + " ms)" },
        { "Quality",         qualityNames[juce::jlimit (0, 3, (int) s.quality)] },
        { "Voices",          juce::String (s.activeVoices) + " / " + juce::String (s.maxVoices) },
        { "Profiler voices", juce::String (s.perf.activeVoices) },
        { "Latency",         juce::String (s.latencySamples) + " smp" },
        { "Blocks measured", juce::String (s.perf.blocksMeasured) },
        { "Overruns",        juce::String (s.perf.overruns) },
        { "Matter nodes",    juce::String (s.activeNodes) + " / " + juce::String (s.numNodes) },
        { "Sample clock",    juce::String (s.sampleTime) },
    });

    engineInfo.clearRowColours();
    if (s.perf.overruns > 0)
        engineInfo.setRowColour (9, Theme::magenta);
    if (s.perf.totalMovingPercent >= 60.0f)
        engineInfo.setRowColour (3, Theme::amber);
}

void PerformanceView::rebuildMemory (const LabFrame& f)
{
    // sizeof-based estimates of the preallocated structures. These are exact
    // for the fixed-size members; heap buffers inside the voices are counted
    // through their owning types where those are fixed-size arrays.
    const size_t tapBytes = sizeof (Diagnostics::taps);
    const size_t snapshotBytes = sizeof (TripleBuffer<DiagnosticSnapshot>) + sizeof (TripleBuffer<VisualStateSnapshot>);
    const size_t diagnosticsBytes = sizeof (Diagnostics);
    const size_t engineBytes = sizeof (SynthEngine);
    const size_t voiceBytes = sizeof (AntiMatrVoice);
    const size_t voicesTotal = voiceBytes * (size_t) juce::jmax (1, f.snapshot.maxVoices);
    const size_t nodeBytes = sizeof (MatterEngine::Node) * (size_t) kMaxMatterNodes;

    memory.setRows ({
        { "SIZEOF ESTIMATES", "" },
        { "SynthEngine",        bytes (engineBytes) },
        { "AntiMatrVoice",      bytes (voiceBytes) },
        { "Voices (allocated)", bytes (voicesTotal) + "  x" + juce::String (f.snapshot.maxVoices) },
        { "Matter nodes/voice", bytes (nodeBytes) },
        { "Diagnostics",        bytes (diagnosticsBytes) },
        { "Audio taps",         bytes (tapBytes) + "  (" + juce::String ((int) Stage::Count) + " x "
                                + juce::String (Diagnostics::kTapSize) + " smp)" },
        { "Snapshot buffers",   bytes (snapshotBytes) },
        { "DiagnosticSnapshot", bytes (sizeof (DiagnosticSnapshot)) },
        { "VisualStateSnapshot",bytes (sizeof (VisualStateSnapshot)) },
        { "Estimated total",    bytes (engineBytes + voicesTotal + diagnosticsBytes) },
        { "RUNTIME", "" },
        { "Events dropped",     juce::String (f.snapshot.eventsDropped) },
        { "Events pending",     juce::String ((int) f.diagnostics.events.pending()) },
        { "ALLOCATIONS", "" },
        { "In audio callback",  "no hook" },
    });

    memory.clearRowColours();
    if (f.snapshot.eventsDropped > 0)
        memory.setRowColour (12, Theme::amber);
    memory.setRowColour (15, Theme::textDim);
}

void PerformanceView::updateFrame (const LabFrame& f)
{
    diagnostics = &f.diagnostics;

    const bool enabled = f.diagnostics.dev.profiling.load (std::memory_order_relaxed);
    if (enabled != profiling.getToggleState())
        profiling.setToggleState (enabled, juce::dontSendNotification);

    rebuildSubsystems (f.snapshot);
    rebuildEngine (f);
    rebuildMemory (f);
    history.push (f.snapshot.perf.totalMovingPercent, f.snapshot.perf.totalPeakPercent);
}

void PerformanceView::resized()
{
    auto area = getLocalBounds();

    auto right = area.removeFromRight (juce::jmax (235, area.getWidth() * 30 / 100));
    area.removeFromRight (5);

    controls.setBounds (right.removeFromTop (34));
    {
        auto inner = controls.contentBounds();
        profiling.setBounds (inner.removeFromLeft (inner.getWidth() / 2));
        resetProfiler.setBounds (inner.reduced (2, 0));
    }
    right.removeFromTop (5);
    engineInfo.setBounds (right.removeFromTop (juce::jmax (200, right.getHeight() * 46 / 100)));
    right.removeFromTop (5);
    memory.setBounds (right);

    history.setBounds (area.removeFromBottom (juce::jmax (130, area.getHeight() * 38 / 100)));
    area.removeFromBottom (5);
    tablePanel.setBounds (area);
    subsystems.setBounds (tablePanel.contentBounds());
}

} // namespace am::dev

#include "ParameterSweepTool.h"

#include "plugin/AntiMatrProcessor.h"

namespace am::dev
{

using namespace am::ui;

//==============================================================================
SweepPlotView::SweepPlotView() : LabPanel ("Sweep curves") {}

void SweepPlotView::paint (juce::Graphics& g)
{
    LabPanel::paint (g);

    auto area = contentBoundsF();
    if (points == nullptr || points->size() < 2)
    {
        plot::emptyState (g, area, "run a sweep");
        return;
    }

    auto legend = area.removeFromTop (12.0f);
    auto axis = area.removeFromBottom (11.0f);

    float maxCpu = 1.0f, maxCentroid = 100.0f;
    for (const auto& p : *points)
    {
        maxCpu = juce::jmax (maxCpu, p.cpuPercent);
        maxCentroid = juce::jmax (maxCentroid, p.centroidHz);
    }

    // Dangerous zones behind everything else.
    const float stepWidth = area.getWidth() / (float) points->size();
    for (size_t i = 0; i < points->size(); ++i)
        if ((*points)[i].dangerous())
        {
            g.setColour (Theme::magenta.withAlpha (0.14f));
            g.fillRect (area.getX() + (float) i * stepWidth, area.getY(), stepWidth, area.getHeight());
        }

    for (int i = 0; i <= 4; ++i)
    {
        const float y = area.getY() + area.getHeight() * (float) i / 4.0f;
        g.setColour (juce::Colours::white.withAlpha (0.045f));
        g.drawLine (area.getX(), y, area.getRight(), y, 1.0f);
    }

    auto curve = [&] (auto valueFn, juce::Colour colour, float scale)
    {
        juce::Path p;
        for (size_t i = 0; i < points->size(); ++i)
        {
            const float x = area.getX() + area.getWidth() * (float) i / (float) juce::jmax<size_t> (1, points->size() - 1);
            const float v = juce::jlimit (0.0f, 1.0f, valueFn ((*points)[i]) / scale);
            const float y = area.getBottom() - v * area.getHeight();
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (1.3f));
    };

    curve ([] (const SweepPoint& p) { return p.peak; }, Theme::amber, 1.0f);
    curve ([] (const SweepPoint& p) { return p.rms; }, Theme::cyan, 1.0f);
    curve ([&] (const SweepPoint& p) { return p.cpuPercent; }, Theme::violet, maxCpu);
    curve ([&] (const SweepPoint& p) { return p.centroidHz; }, Theme::blue, maxCentroid);

    float x = legend.getX();
    auto legendItem = [&] (const juce::String& text, juce::Colour colour)
    {
        const float w = Theme::captionFont (9.0f).getStringWidthFloat (text) + 24.0f;
        g.setColour (colour);
        g.fillRect (x, legend.getCentreY() - 1.0f, 10.0f, 2.0f);
        plot::caption (g, text, legend.withX (x + 13.0f).withWidth (w), colour, 9.0f);
        x += w;
    };
    legendItem ("PEAK 0-1", Theme::amber);
    legendItem ("RMS 0-1", Theme::cyan);
    legendItem ("CPU 0-" + juce::String (maxCpu, 0) + "%", Theme::violet);
    legendItem ("CENTROID 0-" + juce::String (maxCentroid / 1000.0f, 1) + "k", Theme::blue);

    plot::caption (g, "0.0", axis, Theme::textDim, 8.0f);
    plot::caption (g, "PARAMETER 0 -> 1", axis, Theme::textDim, 8.0f, juce::Justification::centred);
    plot::caption (g, "1.0", axis, Theme::textDim, 8.0f, juce::Justification::centredRight);
}

//==============================================================================
ParameterSweepTool::ParameterSweepTool (AntiMatrProcessor& p) : processor (p)
{
    populateParameterList();
    styleCombo (parameterBox);
    controls.addAndMakeVisible (parameterBox);

    auto setupSlider = [this] (juce::Slider& s, double lo, double hi, double value, double interval)
    {
        s.setSliderStyle (juce::Slider::IncDecButtons);
        s.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 46, 20);
        s.setRange (lo, hi, interval);
        s.setValue (value, juce::dontSendNotification);
        s.setColour (juce::Slider::textBoxBackgroundColourId, Theme::panelInset);
        s.setColour (juce::Slider::textBoxOutlineColourId, Theme::border);
        s.setColour (juce::Slider::textBoxTextColourId, Theme::textPrimary);
        controls.addAndMakeVisible (s);
    };
    setupSlider (seconds, 1.0, 60.0, 8.0, 1.0);
    setupSlider (steps, 5.0, 129.0, 33.0, 1.0);
    setupSlider (noteSlider, 24.0, 96.0, 60.0, 1.0);

    auto setupLabel = [this] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (Theme::captionFont (9.0f));
        l.setColour (juce::Label::textColourId, Theme::textDim);
        controls.addAndMakeVisible (l);
    };
    setupLabel (secondsLabel, "SECONDS");
    setupLabel (stepsLabel, "STEPS");
    setupLabel (noteLabel, "NOTE");

    styleButton (startButton, Theme::cyan);
    styleButton (stopButton, Theme::magenta);
    startButton.onClick = [this] { startSweep(); };
    stopButton.onClick = [this] { stopSweep (true); };
    stopButton.setEnabled (false);
    controls.addAndMakeVisible (startButton);
    controls.addAndMakeVisible (stopButton);
    addAndMakeVisible (controls);

    table.setColumns ({
        { "STEP", 44, true }, { "NORM", 56, true }, { "VALUE", 72, true },
        { "PEAK", 62, true }, { "RMS", 66, true }, { "CREST", 56, true },
        { "CPU %", 60, true }, { "CENTROID Hz", 84, true }, { "NODES", 54, true },
        { "NaN", 46, true }, { "SAFETY", 58, true }, { "FLAG", 60, false }
    });
    table.setRowColourFn ([this] (int row) -> juce::Colour
    {
        if (row < 0 || row >= (int) points.size()) return juce::Colours::transparentBlack;
        return points[(size_t) row].dangerous() ? Theme::magenta.withAlpha (0.10f) : juce::Colours::transparentBlack;
    });
    tablePanel.addAndMakeVisible (table);
    addAndMakeVisible (tablePanel);

    plotView.setPoints (&points);
    addAndMakeVisible (plotView);

    summary.setRowHeight (14.0f);
    summary.setLabelWidthFraction (0.6f);
    addAndMakeVisible (summary);
    refreshSummary();
}

ParameterSweepTool::~ParameterSweepTool()
{
    stopTimer();
}

void ParameterSweepTool::populateParameterList()
{
    int id = 1;
    ParamGroup lastGroup = ParamGroup::Count;
    for (const auto& d : ParameterRegistry::all())
    {
        if (d.group != lastGroup)
        {
            parameterBox.addSectionHeading (juce::String (ParameterRegistry::groupName (d.group)).toUpperCase());
            lastGroup = d.group;
        }
        parameterBox.addItem (juce::String (d.id), id);
        ++id;
    }
    parameterBox.setSelectedId (paramIndex (Param::shapeDensity) + 1, juce::dontSendNotification);
}

void ParameterSweepTool::startSweep()
{
    stopSweep (true);

    sweptParameter = parameterBox.getSelectedId() - 1;
    if (sweptParameter < 0 || sweptParameter >= kNumParams)
        return;

    const auto& desc = ParameterRegistry::get (paramFromIndex (sweptParameter));
    auto* hostParam = processor.parameters().getParameter (desc.id);
    if (hostParam == nullptr)
        return;

    originalNormalised = hostParam->getValue();
    totalSteps = juce::jlimit (2, 129, (int) steps.getValue());
    dwellSeconds = juce::jmax (0.05, seconds.getValue() / (double) totalSteps);
    testNote = juce::jlimit (0, 127, (int) noteSlider.getValue());

    points.clear();
    points.reserve ((size_t) totalSteps);
    currentStep = 0;
    running = true;

    processor.injectMidi (juce::MidiMessage::noteOn (1, testNote, 0.85f));
    hostParam->setValueNotifyingHost (0.0f);

    safetyAtStepStart = processor.diagnostics().safety.snapshot().total;
    stepStartMs = juce::Time::getMillisecondCounterHiRes();

    startButton.setEnabled (false);
    stopButton.setEnabled (true);
    startTimer (10);
    refreshSummary();
}

void ParameterSweepTool::stopSweep (bool restoreParameter)
{
    stopTimer();

    if (running)
    {
        processor.injectMidi (juce::MidiMessage::noteOff (1, testNote));
        if (restoreParameter && sweptParameter >= 0 && sweptParameter < kNumParams)
        {
            const auto& desc = ParameterRegistry::get (paramFromIndex (sweptParameter));
            if (auto* hostParam = processor.parameters().getParameter (desc.id))
                hostParam->setValueNotifyingHost (originalNormalised);
        }
    }

    running = false;
    startButton.setEnabled (true);
    stopButton.setEnabled (false);
    summaryData = SweepSummary::of (points);
    refreshSummary();
}

void ParameterSweepTool::timerCallback()
{
    if (! running)
    {
        stopTimer();
        return;
    }

    const double elapsed = (juce::Time::getMillisecondCounterHiRes() - stepStartMs) * 0.001;
    if (elapsed < dwellSeconds)
        return;

    measureCurrentStep();
    ++currentStep;

    if (currentStep >= totalSteps)
    {
        rebuildTable();
        stopSweep (true);
        return;
    }

    if (sweptParameter >= 0 && sweptParameter < kNumParams)
    {
        const auto& desc = ParameterRegistry::get (paramFromIndex (sweptParameter));
        if (auto* hostParam = processor.parameters().getParameter (desc.id))
            hostParam->setValueNotifyingHost ((float) currentStep / (float) (totalSteps - 1));
    }

    safetyAtStepStart = processor.diagnostics().safety.snapshot().total;
    stepStartMs = juce::Time::getMillisecondCounterHiRes();
    rebuildTable();
}

void ParameterSweepTool::measureCurrentStep()
{
    if (diagnostics == nullptr || sweptParameter < 0)
        return;

    const auto& desc = ParameterRegistry::get (paramFromIndex (sweptParameter));
    const float normalised = (float) currentStep / (float) juce::jmax (1, totalSteps - 1);

    diagnostics->taps[(int) Stage::Master].readLatest (tapL.data(), tapR.data(), (int) tapL.size());
    for (size_t i = 0; i < tapMono.size(); ++i)
        tapMono[i] = 0.5f * (tapL[i] + tapR[i]);

    const auto metrics = SignalMetrics::measure (tapL.data(), tapR.data(), (int) tapL.size());
    const auto safetyNow = diagnostics->safety.snapshot().total;
    const auto perf = diagnostics->profiler.snapshot();

    SweepPoint point;
    point.normalised = normalised;
    point.value = desc.fromNormalised (normalised);
    point.peak = metrics.peak;
    point.rms = metrics.rms;
    point.crestFactor = metrics.crestFactor;
    point.nonFinite = metrics.nonFinite;
    point.cpuPercent = perf.totalMovingPercent;
    point.centroidHz = spectrum.spectralCentroid (tapMono.data(), (int) tapMono.size(), sampleRate);
    point.activeNodes = latest != nullptr ? latest->activeNodes : 0;
    point.safetyDelta = safetyNow > safetyAtStepStart ? safetyNow - safetyAtStepStart : 0;

    points.push_back (point);
}

void ParameterSweepTool::rebuildTable()
{
    std::vector<LabTable::Row> rows;
    rows.reserve (points.size());

    for (size_t i = 0; i < points.size(); ++i)
    {
        const auto& p = points[i];
        const bool danger = p.dangerous();
        const auto tint = danger ? Theme::magenta : Theme::textPrimary;

        rows.push_back ({
            { juce::String ((int) i), (double) i, Theme::textSecondary },
            { juce::String (p.normalised, 3), p.normalised, Theme::textSecondary },
            { juce::String (p.value, 4), p.value, Theme::textPrimary },
            { juce::String (p.peak, 4), p.peak, p.peak >= 0.99f ? Theme::magenta : tint },
            { juce::String (p.rms, 5), p.rms, tint },
            { juce::String (p.crestFactor, 2), p.crestFactor, tint },
            { juce::String (p.cpuPercent, 2), p.cpuPercent, p.cpuPercent >= 80.0f ? Theme::magenta : tint },
            { juce::String (p.centroidHz, 0), p.centroidHz, Theme::blue },
            { juce::String (p.activeNodes), (double) p.activeNodes, Theme::textSecondary },
            { juce::String (p.nonFinite), (double) p.nonFinite, p.nonFinite > 0 ? Theme::magenta : Theme::textDim },
            { juce::String ((int) p.safetyDelta), (double) p.safetyDelta, p.safetyDelta > 0 ? Theme::magenta : Theme::textDim },
            { danger ? "DANGER" : "ok", danger ? 1.0 : 0.0, danger ? Theme::magenta : Theme::textDim }
        });
    }

    table.setRows (std::move (rows));
    plotView.repaint();
}

void ParameterSweepTool::refreshSummary()
{
    const auto s = points.empty() ? SweepSummary() : SweepSummary::of (points);
    juce::String parameterId = "-";
    if (sweptParameter >= 0 && sweptParameter < kNumParams)
        parameterId = ParameterRegistry::get (paramFromIndex (sweptParameter)).id;

    summary.setRows ({
        { "Parameter",     parameterId },
        { "State",         running ? "SWEEPING " + juce::String (currentStep) + " / " + juce::String (totalSteps)
                                   : (points.empty() ? "idle" : "complete") },
        { "Dwell per step",juce::String (dwellSeconds, 3) + " s" },
        { "Test note",     juce::String (testNote) },
        { "", "" },
        { "Points",        juce::String (s.numPoints) },
        { "Dangerous",     juce::String (s.dangerousPoints) },
        { "Max peak",      juce::String (s.maxPeak, 4) },
        { "Max CPU",       juce::String (s.maxCpuPercent, 2) + " %" },
        { "Centroid range",juce::String (s.minCentroidHz, 0) + " - " + juce::String (s.maxCentroidHz, 0) + " Hz" },
        { "Non-finite",    juce::String (s.totalNonFinite) },
        { "Safety events", juce::String ((int) s.totalSafety) },
        { "", "" },
        { "Verdict",       s.numPoints == 0 ? "-" : (s.dangerousPoints > 0 ? "DANGEROUS ZONES" : "SAFE") },
    });

    summary.clearRowColours();
    summary.setRowColour (1, running ? Theme::cyan : Theme::textSecondary);
    if (s.dangerousPoints > 0) summary.setRowColour (6, Theme::magenta);
    if (s.maxPeak >= 0.99f)    summary.setRowColour (7, Theme::magenta);
    if (s.totalNonFinite > 0)  summary.setRowColour (10, Theme::magenta);
    if (s.totalSafety > 0)     summary.setRowColour (11, Theme::magenta);
    summary.setRowColour (13, s.numPoints == 0 ? Theme::textDim
                             : (s.dangerousPoints > 0 ? Theme::magenta : Theme::cyan));
}

void ParameterSweepTool::updateFrame (const LabFrame& f)
{
    latest = &f.snapshot;
    diagnostics = &f.diagnostics;
    sampleRate = f.sampleRate;
    if (running)
        refreshSummary();
}

void ParameterSweepTool::resized()
{
    auto area = getLocalBounds();

    controls.setBounds (area.removeFromTop (56));
    {
        auto inner = controls.contentBounds();
        auto row = inner.removeFromTop (24);
        parameterBox.setBounds (row.removeFromLeft (juce::jmax (200, row.getWidth() / 3)));
        row.removeFromLeft (10);
        secondsLabel.setBounds (row.removeFromLeft (58));
        seconds.setBounds (row.removeFromLeft (96));
        row.removeFromLeft (6);
        stepsLabel.setBounds (row.removeFromLeft (42));
        steps.setBounds (row.removeFromLeft (96));
        row.removeFromLeft (6);
        noteLabel.setBounds (row.removeFromLeft (38));
        noteSlider.setBounds (row.removeFromLeft (96));
        row.removeFromLeft (10);
        startButton.setBounds (row.removeFromLeft (104).reduced (0, 1));
        row.removeFromLeft (4);
        stopButton.setBounds (row.removeFromLeft (60).reduced (0, 1));
    }
    area.removeFromTop (5);

    auto right = area.removeFromRight (juce::jmax (215, area.getWidth() * 26 / 100));
    area.removeFromRight (5);
    summary.setBounds (right);

    plotView.setBounds (area.removeFromTop (juce::jmax (140, area.getHeight() * 42 / 100)));
    area.removeFromTop (5);
    tablePanel.setBounds (area);
    table.setBounds (tablePanel.contentBounds());
}

} // namespace am::dev

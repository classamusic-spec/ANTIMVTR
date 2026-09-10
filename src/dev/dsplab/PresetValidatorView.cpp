#include "PresetValidatorView.h"

namespace am::dev
{

using namespace am::ui;

PresetValidatorView::PresetValidatorView()
    : progressBar (progress)
{
    styleButton (startButton, Theme::cyan);
    styleButton (cancelButton, Theme::magenta);
    startButton.onClick = [this]
    {
        PresetValidator::Options options;
        options.holdSeconds = holdSeconds.getValue();
        options.releaseSeconds = juce::jmax (0.25, holdSeconds.getValue() * 0.6);
        results.clear();
        lastResultCount = 0;
        selectedIndex = -1;
        table.setRows ({});
        validator.startValidation (options);
    };
    cancelButton.onClick = [this] { validator.cancelValidation(); };
    controls.addAndMakeVisible (startButton);
    controls.addAndMakeVisible (cancelButton);

    holdSeconds.setSliderStyle (juce::Slider::IncDecButtons);
    holdSeconds.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 46, 20);
    holdSeconds.setRange (0.25, 5.0, 0.25);
    holdSeconds.setValue (2.0, juce::dontSendNotification);
    holdSeconds.setColour (juce::Slider::textBoxBackgroundColourId, Theme::panelInset);
    holdSeconds.setColour (juce::Slider::textBoxOutlineColourId, Theme::border);
    holdSeconds.setColour (juce::Slider::textBoxTextColourId, Theme::textPrimary);
    controls.addAndMakeVisible (holdSeconds);

    holdLabel.setText ("NOTE HOLD s", juce::dontSendNotification);
    holdLabel.setFont (Theme::captionFont (9.0f));
    holdLabel.setColour (juce::Label::textColourId, Theme::textDim);
    controls.addAndMakeVisible (holdLabel);

    progressBar.setColour (juce::ProgressBar::backgroundColourId, Theme::panelInset);
    progressBar.setColour (juce::ProgressBar::foregroundColourId, Theme::cyan.withAlpha (0.6f));
    controls.addAndMakeVisible (progressBar);
    addAndMakeVisible (controls);

    table.setColumns ({
        { "#", 34, true }, { "PRESET", 150, false }, { "CATEGORY", 90, false },
        { "BOUNDS", 60, false }, { "JSON", 52, false }, { "PEAK", 62, true },
        { "RMS", 66, true }, { "DC", 66, true }, { "TAIL", 62, true },
        { "CENTROID", 74, true }, { "CPU %", 58, true }, { "NaN", 46, true },
        { "SAFETY", 58, true }, { "RESULT", 62, false }
    });
    table.setRowColourFn ([this] (int row) -> juce::Colour
    {
        if (row < 0 || row >= (int) results.size()) return juce::Colours::transparentBlack;
        return results[(size_t) row].passed() ? juce::Colours::transparentBlack : Theme::magenta.withAlpha (0.10f);
    });
    table.onSelectionChanged = [this] (int row)
    {
        selectedIndex = row;
        refreshDetail();
    };
    tablePanel.addAndMakeVisible (table);
    addAndMakeVisible (tablePanel);

    detail.setRowHeight (14.0f);
    detail.setLabelWidthFraction (0.52f);
    addAndMakeVisible (detail);
    refreshDetail();
}

PresetValidatorView::~PresetValidatorView()
{
    validator.cancelValidation();
}

void PresetValidatorView::rebuildTable()
{
    std::vector<LabTable::Row> rows;
    rows.reserve (results.size());

    for (const auto& r : results)
    {
        const bool ok = r.passed();
        const auto tint = ok ? Theme::textPrimary : Theme::magenta;

        rows.push_back ({
            { juce::String (r.index), (double) r.index, Theme::textSecondary },
            { r.name, 0.0, ok ? Theme::textPrimary : Theme::magenta },
            { r.category, 0.0, Theme::textDim },
            { r.parametersInRange ? "ok" : "FAIL", r.parametersInRange ? 1.0 : 0.0,
              r.parametersInRange ? Theme::textSecondary : Theme::magenta },
            { r.jsonRoundTrip ? "ok" : "FAIL", r.jsonRoundTrip ? 1.0 : 0.0,
              r.jsonRoundTrip ? Theme::textSecondary : Theme::magenta },
            { juce::String (r.peak, 4), r.peak, r.peak > 1.0f ? Theme::magenta : tint },
            { juce::String (r.rms, 5), r.rms, r.rms < 1.0e-5f ? Theme::amber : tint },
            { juce::String (r.dc, 5), r.dc, std::abs (r.dc) > 0.02f ? Theme::amber : tint },
            { juce::String (r.tailRms, 6), r.tailRms, Theme::textSecondary },
            { juce::String (r.centroidHz, 0), r.centroidHz, Theme::blue },
            { juce::String (r.cpuAvgPercent, 2), r.cpuAvgPercent, r.cpuAvgPercent > 80.0f ? Theme::magenta : tint },
            { juce::String (r.nonFinite), (double) r.nonFinite, r.nonFinite > 0 ? Theme::magenta : Theme::textDim },
            { juce::String ((int) r.safetyTotal), (double) r.safetyTotal, r.safetyTotal > 0 ? Theme::magenta : Theme::textDim },
            { ok ? "PASS" : "FAIL", ok ? 1.0 : 0.0, ok ? Theme::cyan : Theme::magenta }
        });
    }

    table.setRows (std::move (rows));
    tablePanel.setSubtitle (validator.summary().toUpperCase());
}

void PresetValidatorView::refreshDetail()
{
    if (selectedIndex < 0 || selectedIndex >= (int) results.size())
    {
        detail.setRows ({
            { "No preset selected", "" },
            { "", "" },
            { "VALIDATE ALL builds every", "" },
            { "factory preset, checks its", "" },
            { "parameter bounds and JSON", "" },
            { "round trip, then renders a", "" },
            { "test note offline through a", "" },
            { "private SynthEngine.", "" },
        });
        detail.clearRowColours();
        return;
    }

    const auto& r = results[(size_t) selectedIndex];
    std::vector<KeyValueTable::Row> rows {
        { "Preset",          r.name },
        { "Category",        r.category },
        { "Bounds",          r.parametersInRange ? "ok" : "FAILED" },
        { "JSON round trip", r.jsonRoundTrip ? "ok" : "FAILED" },
        { "", "" },
        { "Peak",            juce::String (r.peak, 5) + "  (" + dbString (r.peak) + " dB)" },
        { "RMS",             juce::String (r.rms, 6) + "  (" + dbString (r.rms) + " dB)" },
        { "Crest factor",    juce::String (r.crestFactor, 2) },
        { "DC offset",       juce::String (r.dc, 6) },
        { "Tail RMS",        juce::String (r.tailRms, 7) },
        { "Centroid",        juce::String (r.centroidHz, 0) + " Hz" },
        { "Non-finite",      juce::String (r.nonFinite) },
        { "Max voices",      juce::String (r.maxActiveVoices) },
        { "CPU avg / peak",  juce::String (r.cpuAvgPercent, 2) + " / " + juce::String (r.cpuPeakPercent, 2) + " %" },
        { "Render time",     juce::String (r.renderSeconds, 3) + " s" },
        { "", "" },
        { "Verdict",         r.passed() ? "PASS" : "FAIL" },
    };

    if (r.safetyTotal > 0)
    {
        rows.push_back ({ "SAFETY", "" });
        for (int i = 0; i < SafetyMonitor::kNumEvents; ++i)
            if (r.safetyCounts[i] > 0)
                rows.push_back ({ juce::String (SafetyMonitor::eventName ((SafetyEvent) i)),
                                  juce::String ((int) r.safetyCounts[i]) });
    }
    if (! r.issues.isEmpty())
    {
        rows.push_back ({ "ISSUES", "" });
        for (const auto& issue : r.issues)
            rows.push_back ({ issue, "" });
    }

    detail.setRows (std::move (rows));
    detail.clearRowColours();
    detail.setRowColour (2, r.parametersInRange ? Theme::textPrimary : Theme::magenta);
    detail.setRowColour (3, r.jsonRoundTrip ? Theme::textPrimary : Theme::magenta);
    detail.setRowColour (11, r.nonFinite > 0 ? Theme::magenta : Theme::textPrimary);
    detail.setRowColour (16, r.passed() ? Theme::cyan : Theme::magenta);
}

void PresetValidatorView::updateFrame (const LabFrame&)
{
    const bool busy = validator.busy();
    progress = busy ? (double) validator.progress() : (results.empty() ? 0.0 : 1.0);

    auto latest = validator.results();
    if (latest.size() != lastResultCount || (wasBusy && ! busy))
    {
        results = std::move (latest);
        lastResultCount = results.size();
        rebuildTable();
        refreshDetail();
    }
    else
    {
        tablePanel.setSubtitle (validator.summary().toUpperCase());
    }

    startButton.setEnabled (! busy);
    cancelButton.setEnabled (busy);
    wasBusy = busy;
}

void PresetValidatorView::resized()
{
    auto area = getLocalBounds();

    controls.setBounds (area.removeFromTop (52));
    {
        auto inner = controls.contentBounds();
        auto row = inner.removeFromTop (22);
        startButton.setBounds (row.removeFromLeft (110).reduced (0, 1));
        row.removeFromLeft (4);
        cancelButton.setBounds (row.removeFromLeft (72).reduced (0, 1));
        row.removeFromLeft (12);
        holdLabel.setBounds (row.removeFromLeft (70));
        holdSeconds.setBounds (row.removeFromLeft (98));
        row.removeFromLeft (12);
        progressBar.setBounds (row.reduced (0, 2));
    }
    area.removeFromTop (5);

    auto right = area.removeFromRight (juce::jmax (235, area.getWidth() * 28 / 100));
    area.removeFromRight (5);
    detail.setBounds (right);

    tablePanel.setBounds (area);
    table.setBounds (tablePanel.contentBounds());
}

} // namespace am::dev

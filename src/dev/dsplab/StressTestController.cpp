#include "StressTestController.h"

#include "plugin/AntiMatrProcessor.h"

namespace am::dev
{

using namespace am::ui;

StressTestController::StressTestController (AntiMatrProcessor& p) : processor (p)
{
    for (int i = 0; i < (int) StressTest::Count; ++i)
        testBox.addItem (StressTestGenerator::name ((StressTest) i), i + 1);
    testBox.setSelectedId (1, juce::dontSendNotification);
    testBox.onChange = [this]
    {
        const auto test = (StressTest) (testBox.getSelectedId() - 1);
        panel.setSubtitle (juce::String (StressTestGenerator::description (test)).toUpperCase());
        if (isRunning()) start (test);
    };
    styleCombo (testBox);
    panel.addAndMakeVisible (testBox);

    styleButton (startButton, Theme::cyan);
    styleButton (stopButton, Theme::magenta);
    startButton.onClick = [this] { start ((StressTest) (testBox.getSelectedId() - 1)); };
    stopButton.onClick = [this] { stop(); };
    panel.addAndMakeVisible (startButton);
    panel.addAndMakeVisible (stopButton);

    for (auto* t : { &runInfo, &measured })
    {
        t->setRowHeight (14.0f);
        t->setLabelWidthFraction (0.54f);
        panel.addAndMakeVisible (t);
    }
    measured.setAccent (Theme::amber);

    addAndMakeVisible (panel);
    panel.setSubtitle (juce::String (StressTestGenerator::description (StressTest::SustainedNote)).toUpperCase());
    refreshStatus();
}

StressTestController::~StressTestController()
{
    stop();
}

void StressTestController::start (StressTest test)
{
    stop();

    StressTestGenerator::Config config;
    config.numFactoryPresets = juce::jmax (1, processor.presets().numFactoryPresets());
    config.maxVoices = juce::jlimit (1, kMaxVoices, processor.diagnostics().diagnosticSnapshots.latest().maxVoices);
    config.seed = 0x51E55u + (uint32_t) test;
    generator.start (test, config);

    verdict = Verdict();
    const auto safety = processor.diagnostics().safety.snapshot();
    verdict.safetyAtStart = safety.total;
    verdict.safetyNow = safety.total;
    verdict.overrunsAtStart = processor.diagnostics().profiler.snapshot().overruns;
    verdict.overrunsNow = verdict.overrunsAtStart;

    startTimeMs = juce::Time::getMillisecondCounterHiRes();
    startTimer (10);
    refreshStatus();
}

void StressTestController::stop()
{
    if (! generator.isRunning())
    {
        stopTimer();
        return;
    }

    pending.clear();
    generator.stop (pending);
    for (const auto& a : pending)
        execute (a);
    pending.clear();

    stopTimer();
    refreshStatus();
}

void StressTestController::execute (const StressAction& a)
{
    switch (a.kind)
    {
        case StressAction::Kind::NoteOn:
            processor.injectMidi (juce::MidiMessage::noteOn (a.channel, a.note, a.velocity));
            break;

        case StressAction::Kind::NoteOff:
            processor.injectMidi (juce::MidiMessage::noteOff (a.channel, a.note));
            break;

        case StressAction::Kind::AllNotesOff:
            for (int ch = 1; ch <= 16; ++ch)
                processor.injectMidi (juce::MidiMessage::allNotesOff (ch));
            break;

        case StressAction::Kind::PitchBend:
            processor.injectMidi (juce::MidiMessage::pitchWheel (a.channel,
                juce::jlimit (0, 16383, (int) std::lround (8192.0f + a.bend * 8191.0f))));
            break;

        case StressAction::Kind::Parameter:
            if (a.paramIndex >= 0 && a.paramIndex < kNumParams)
            {
                const auto& desc = ParameterRegistry::get (paramFromIndex (a.paramIndex));
                if (auto* hostParam = processor.parameters().getParameter (desc.id))
                    hostParam->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, a.normalised));
            }
            break;

        case StressAction::Kind::Preset:
            processor.loadFactoryPreset (a.preset);
            break;
    }
}

void StressTestController::timerCallback()
{
    if (! generator.isRunning())
    {
        stopTimer();
        return;
    }

    const double elapsed = (juce::Time::getMillisecondCounterHiRes() - startTimeMs) * 0.001;

    pending.clear();
    generator.advance (elapsed, pending);
    for (const auto& a : pending)
        execute (a);
    pending.clear();

    const double nominal = generator.duration();
    if (nominal > 0.0 && elapsed >= nominal)
        stop();
}

void StressTestController::sample (const LabFrame& f)
{
    if (generator.isRunning())
    {
        const auto& s = f.snapshot;
        verdict.maxPeak = juce::jmax (verdict.maxPeak, s.stages[(int) Stage::Master].peak);
        verdict.maxCpu = juce::jmax (verdict.maxCpu, s.perf.totalMovingPercent);
        verdict.maxVoices = juce::jmax (verdict.maxVoices, s.activeVoices);
        verdict.maxNodes = juce::jmax (verdict.maxNodes, s.activeNodes);
        verdict.safetyNow = s.safety.total;
        verdict.overrunsNow = s.perf.overruns;
        ++verdict.samples;

        f.diagnostics.taps[(int) Stage::Master].readLatest (tapL.data(), tapR.data(), (int) tapL.size());
        const auto metrics = SignalMetrics::measure (tapL.data(), tapR.data(), (int) tapL.size());
        verdict.nonFinite += metrics.nonFinite;
        verdict.maxPeak = juce::jmax (verdict.maxPeak, metrics.peak);
    }

    refreshStatus();
}

void StressTestController::refreshStatus()
{
    const bool running = generator.isRunning();
    const double elapsed = running ? (juce::Time::getMillisecondCounterHiRes() - startTimeMs) * 0.001 : generator.elapsed();
    const double nominal = generator.duration();
    const uint32_t safetyDelta = verdict.safetyNow > verdict.safetyAtStart ? verdict.safetyNow - verdict.safetyAtStart : 0;
    const uint32_t overrunDelta = verdict.overrunsNow > verdict.overrunsAtStart ? verdict.overrunsNow - verdict.overrunsAtStart : 0;

    if (verdict.samples == 0 && ! running)
        verdictText = "idle";
    else if (verdict.nonFinite > 0 || safetyDelta > 0)
        verdictText = "FAIL";
    else if (verdict.maxPeak >= 0.999f || verdict.maxCpu >= 80.0f || overrunDelta > 0)
        verdictText = "WARN";
    else
        verdictText = running ? "running" : "PASS";

    startButton.setEnabled (! running);
    stopButton.setEnabled (running);

    runInfo.setRows ({
        { "State",      running ? "RUNNING" : "stopped" },
        { "Elapsed",    juce::String (elapsed, 1) + " s" + (nominal > 0.0 ? " / " + juce::String (nominal, 1) + " s" : juce::String()) },
        { "Length",     nominal > 0.0 ? juce::String (nominal, 1) + " s" : juce::String ("until stopped") },
        { "Notes held", juce::String (generator.notesHeld()) },
        { "Max voices", juce::String (verdict.maxVoices) },
        { "Max nodes",  juce::String (verdict.maxNodes) },
        { "Samples",    juce::String (verdict.samples) },
    });
    runInfo.clearRowColours();
    runInfo.setRowColour (0, running ? Theme::cyan : Theme::textDim);

    measured.setRows ({
        { "Max peak",     juce::String (verdict.maxPeak, 4) },
        { "Max CPU",      juce::String (verdict.maxCpu, 1) + " %" },
        { "Overruns",     juce::String ((int) overrunDelta) },
        { "Safety events",juce::String ((int) safetyDelta) },
        { "Non-finite",   juce::String (verdict.nonFinite) },
        { "", "" },
        { "Verdict",      verdictText },
    });
    measured.clearRowColours();
    if (verdict.maxPeak >= 0.999f) measured.setRowColour (0, Theme::magenta);
    if (verdict.maxCpu >= 80.0f)   measured.setRowColour (1, Theme::magenta);
    else if (verdict.maxCpu >= 50.0f) measured.setRowColour (1, Theme::amber);
    if (overrunDelta > 0)          measured.setRowColour (2, Theme::amber);
    if (safetyDelta > 0)           measured.setRowColour (3, Theme::magenta);
    if (verdict.nonFinite > 0)     measured.setRowColour (4, Theme::magenta);
    measured.setRowColour (6, verdictText == "FAIL" ? Theme::magenta
                              : verdictText == "WARN" ? Theme::amber
                              : verdictText == "PASS" ? Theme::cyan : Theme::textSecondary);
}

void StressTestController::resized()
{
    panel.setBounds (getLocalBounds());
    auto inner = panel.contentBounds();

    auto controls = inner.removeFromTop (24);
    testBox.setBounds (controls.removeFromLeft (juce::jmax (150, controls.getWidth() - 130)));
    controls.removeFromLeft (5);
    startButton.setBounds (controls.removeFromLeft (62).reduced (0, 1));
    controls.removeFromLeft (4);
    stopButton.setBounds (controls.removeFromLeft (60).reduced (0, 1));
    inner.removeFromTop (4);

    runInfo.setBounds (inner.removeFromLeft (inner.getWidth() / 2));
    inner.removeFromLeft (5);
    measured.setBounds (inner);
}

} // namespace am::dev

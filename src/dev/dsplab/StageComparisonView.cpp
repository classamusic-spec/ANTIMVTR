#include "StageComparisonView.h"
#include "plugin/AntiMatrProcessor.h"

namespace am::dev
{

using namespace am::ui;

StageComparisonView::StageComparisonView (juce::String title, std::vector<Stage> s,
                                          juce::String bypassText,
                                          std::atomic<bool> DevControls::* bypassMember)
    : stages (std::move (s)), bypassFlag (bypassMember)
{
    spectrum.setTitle (title);
    for (auto stage : stages)
    {
        auto capture = std::make_unique<StageCapture>();
        capture->stage = stage;
        captures.push_back (std::move (capture));
    }

    bypass.setButtonText (bypassText);
    styleToggle (bypass, Theme::magenta);
    bypass.onClick = [this]
    {
        if (diagnostics != nullptr && bypassFlag != nullptr)
            (diagnostics->dev.*bypassFlag).store (bypass.getToggleState(), std::memory_order_relaxed);
    };
    controls.addAndMakeVisible (bypass);

    preWave.setTitle (stages.size() > 1 ? juce::String (stageName (stages.front())) : juce::String ("Pre"));
    postWave.setTitle (juce::String (stageName (stages.back())));

    info.setRowHeight (14.0f);
    info.setLabelWidthFraction (0.6f);

    addAndMakeVisible (spectrum);
    addAndMakeVisible (preWave);
    addAndMakeVisible (postWave);
    addAndMakeVisible (info);
    addAndMakeVisible (controls);
    refreshLayers();
}

void StageComparisonView::refreshLayers()
{
    std::vector<SpectrumView::Layer> layers;
    for (size_t i = 0; i < captures.size(); ++i)
    {
        const bool last = i + 1 == captures.size();
        layers.push_back ({ captures[i].get(), stageColour (captures[i]->stage),
                            juce::String (stageName (captures[i]->stage)).toUpperCase(),
                            last ? 0.16f : 0.05f });
    }
    spectrum.setLayers (std::move (layers));

    if (! captures.empty())
    {
        preWave.setCapture (captures.front().get());
        postWave.setCapture (captures.back().get());
    }
}

void StageComparisonView::updateFrame (const LabFrame& f)
{
    diagnostics = &f.diagnostics;

    if (bypassFlag != nullptr)
    {
        const bool engineState = (f.diagnostics.dev.*bypassFlag).load (std::memory_order_relaxed);
        if (engineState != bypass.getToggleState())
            bypass.setToggleState (engineState, juce::dontSendNotification);
    }

    for (auto& c : captures)
        c->refresh (f.diagnostics, c->stage, f.sampleRate, true, true);

    spectrum.repaint();
    preWave.repaint();
    postWave.repaint();

    std::vector<KeyValueTable::Row> rows;
    for (auto& c : captures)
    {
        const auto& s = f.snapshot.stages[(int) c->stage];
        rows.push_back ({ juce::String (stageName (c->stage)),
                          dbString (s.rms) + " / " + dbString (s.peak) + " dB" });
    }

    if (captures.size() >= 2)
    {
        const auto& a = f.snapshot.stages[(int) captures.front()->stage];
        const auto& b = f.snapshot.stages[(int) captures.back()->stage];
        const float delta = (a.rms > 1.0e-7f && b.rms > 1.0e-7f) ? gainToDb (b.rms / a.rms) : 0.0f;
        rows.push_back ({ "Level change", juce::String (delta, 2) + " dB" });
        rows.push_back ({ "Centroid pre / post",
                          juce::String (captures.front()->centroidHz, 0) + " / "
                        + juce::String (captures.back()->centroidHz, 0) + " Hz" });
        rows.push_back ({ "Flatness pre / post",
                          juce::String (captures.front()->flatness, 4) + " / "
                        + juce::String (captures.back()->flatness, 4) });
        rows.push_back ({ "Crest pre / post",
                          juce::String (captures.front()->metrics.crestFactor, 2) + " / "
                        + juce::String (captures.back()->metrics.crestFactor, 2) });
    }

    rows.push_back ({ "", "" });
    for (auto& extra : extraInfo (f))
        rows.push_back (extra);

    info.setRows (std::move (rows));
}

void StageComparisonView::resized()
{
    auto area = getLocalBounds();

    auto right = area.removeFromRight (juce::jmax (215, area.getWidth() * 26 / 100));
    area.removeFromRight (5);

    controls.setBounds (right.removeFromTop (50));
    bypass.setBounds (controls.contentBounds().reduced (2, 0));
    right.removeFromTop (5);
    info.setBounds (right);

    auto waves = area.removeFromBottom (juce::jmax (110, area.getHeight() * 38 / 100));
    area.removeFromBottom (5);
    spectrum.setBounds (area);

    preWave.setBounds (waves.removeFromLeft (waves.getWidth() / 2));
    waves.removeFromLeft (5);
    postWave.setBounds (waves);
}

//==============================================================================
FractureView::FractureView()
    : StageComparisonView ("Fracture  pre vs post spectrum",
                           { Stage::PostMatter, Stage::PostFracture },
                           "Bypass Fracture", &DevControls::bypassFracture)
{
    setInfoTitle ("Fracture");
    fragments.setAccent (Theme::magenta);
    addAndMakeVisible (fragments);
}

void FractureView::updateFrame (const LabFrame& f)
{
    StageComparisonView::updateFrame (f);
    FractureEngine::FragmentActivity a;
    f.processor.engine().fractureEngine().fillFragmentActivity (a);
    fragments.set (a);
}

void FractureView::resized()
{
    StageComparisonView::resized();
    // Carve a strip for the fragment bars out of the spectrum area (the base class laid out the rest).
    auto area = getLocalBounds();
    area.removeFromRight (juce::jmax (215, area.getWidth() * 26 / 100) + 5);
    area.removeFromBottom (juce::jmax (110, area.getHeight() * 38 / 100) + 5);
    const int stripH = juce::jlimit (56, 90, area.getHeight() / 4);
    fragments.setBounds (area.removeFromBottom (stripH));
}

void FractureView::FragmentPanel::paint (juce::Graphics& g)
{
    LabPanel::paint (g);
    auto area = contentBoundsF().reduced (4.0f, 2.0f);
    const int n = juce::jlimit (0, kMaxFractureFragments, activity.numFragments);
    if (n <= 0 || area.isEmpty())
    {
        g.setColour (Theme::textDim);
        g.setFont (Theme::captionFont (9.0f));
        g.drawText ("FRACTURE IDLE", area, juce::Justification::centred);
        return;
    }
    const float w = area.getWidth() / (float) n;
    for (int f = 0; f < n; ++f)
    {
        auto col = juce::Rectangle<float> (area.getX() + (float) f * w, area.getY(), w, area.getHeight()).reduced (juce::jmin (2.0f, w * 0.15f), 0.0f);
        const float gate = juce::jlimit (0.0f, 1.0f, activity.gain[(size_t) f]);
        const float energy = juce::jlimit (0.0f, 1.0f, activity.energy[(size_t) f]);
        g.setColour (Theme::magenta.withAlpha (0.18f + 0.35f * gate));
        g.fillRect (col.withTop (col.getBottom() - col.getHeight() * gate));
        g.setColour (Theme::cyan.withAlpha (0.85f));
        const float eh = col.getHeight() * energy;
        g.fillRect (col.withTop (col.getBottom() - eh).withWidth (juce::jmax (1.0f, col.getWidth() * 0.35f)).withX (col.getCentreX() - col.getWidth() * 0.175f));
    }
    g.setColour (Theme::textDim);
    g.setFont (Theme::captionFont (8.5f));
    g.drawText ("STEP " + juce::String (activity.currentStep + 1) + "   WET " + juce::String (activity.overall, 3),
                area.removeFromTop (11.0f), juce::Justification::topRight);
}

std::vector<KeyValueTable::Row> FractureView::extraInfo (const LabFrame& f)
{
    const auto& s = f.snapshot;
    const double hopMs = s.fractureHop > 0 && s.sampleRate > 0.0
                       ? (double) s.fractureHop / s.sampleRate * 1000.0 : 0.0;
    const double latencyMs = s.sampleRate > 0.0 ? (double) s.latencySamples / s.sampleRate * 1000.0 : 0.0;
    const int overlap = (s.fractureFFTSize > 0 && s.fractureHop > 0) ? s.fractureFFTSize / s.fractureHop : 0;

    return {
        { "FRACTURE", "" },
        { "FFT size",  s.fractureFFTSize > 0 ? juce::String (s.fractureFFTSize) : juce::String ("not reported") },
        { "Hop",       s.fractureHop > 0 ? juce::String (s.fractureHop) + "  (" + juce::String (hopMs, 2) + " ms)" : juce::String ("not reported") },
        { "Overlap",   overlap > 0 ? juce::String (overlap) + "x" : juce::String ("-") },
        { "Window",    "Hann (engine default)" },
        { "Activity",  juce::String (s.fractureActivity, 4) },
        { "Latency",   juce::String (s.latencySamples) + " smp  (" + juce::String (latencyMs, 2) + " ms)" },
    };
}

//==============================================================================
SpaceView::SpaceView()
    : StageComparisonView ("Space  post-fracture vs post-space vs master",
                           { Stage::PostFracture, Stage::PostSpace, Stage::Master },
                           "Bypass Space", &DevControls::bypassSpace)
{
    setInfoTitle ("Space");
}

std::vector<KeyValueTable::Row> SpaceView::extraInfo (const LabFrame& f)
{
    const auto& s = f.snapshot;
    const auto& space = s.stages[(int) Stage::PostSpace];
    const auto& master = s.stages[(int) Stage::Master];
    const float masterTrim = (space.rms > 1.0e-7f && master.rms > 1.0e-7f) ? gainToDb (master.rms / space.rms) : 0.0f;

    return {
        { "SPACE", "" },
        { "Master trim",  juce::String (masterTrim, 2) + " dB" },
        { "Master peak",  juce::String (master.peak, 4) },
        { "Correlation",  captures.size() >= 2 ? juce::String (captures.back()->metrics.correlation, 3) : juce::String ("-") },
        { "DC (master)",  captures.empty() ? juce::String ("-") : juce::String (captures.back()->metrics.dc, 6) },
        { "Latency",      juce::String (s.latencySamples) + " smp" },
    };
}

} // namespace am::dev

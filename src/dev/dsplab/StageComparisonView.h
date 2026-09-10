#pragma once

#include "SignalInspector.h"
#include "dsp/fracture/FractureEngine.h"

namespace am::dev
{

/**
    Base of the tabs that compare the signal before and after one processor:
    the taps of several stages are captured every frame, overlaid in one
    spectrum, drawn as waveforms and summarised as levels and deltas, with a
    bypass switch written into DevControls.
*/
class StageComparisonView : public LabView
{
public:
    StageComparisonView (juce::String title, std::vector<Stage> stages,
                         juce::String bypassText,
                         std::atomic<bool> DevControls::* bypassMember);

    void updateFrame (const LabFrame& f) override;
    void resized() override;

protected:
    /** Extra rows for the info panel (subclass detail). */
    virtual std::vector<KeyValueTable::Row> extraInfo (const LabFrame&) { return {}; }
    /** Title of the info panel. */
    void setInfoTitle (const juce::String& t) { info.setTitle (t); }

    std::vector<Stage> stages;
    std::vector<std::unique_ptr<StageCapture>> captures;

private:
    void refreshLayers();

    juce::ToggleButton bypass;
    std::atomic<bool> DevControls::* bypassFlag;
    Diagnostics* diagnostics = nullptr;

    SpectrumView spectrum;
    WaveformView preWave, postWave;
    KeyValueTable info { "Stage comparison" };
    LabPanel controls { "Controls" };
};

//==============================================================================
/**
    FRACTURE VIEW (§76)

    FFT size, hop, window, latency and activity read from the snapshot, the
    Post-Matter (pre) and Post-Fracture (post) spectra overlaid so a
    transformation is visible, and the Fracture bypass.
*/
class FractureView : public StageComparisonView
{
public:
    FractureView();

    void updateFrame (const LabFrame& f) override;
    void resized() override;

protected:
    std::vector<KeyValueTable::Row> extraInfo (const LabFrame& f) override;

private:
    /** Per-fragment gate / energy bars and the sequencer step, read wait-free from the engine. */
    class FragmentPanel : public LabPanel
    {
    public:
        FragmentPanel() : LabPanel ("Fragments  gate / energy / step") {}
        void set (const FractureEngine::FragmentActivity& a) { activity = a; repaint(); }
        void paint (juce::Graphics& g) override;
    private:
        FractureEngine::FragmentActivity activity;
    };
    FragmentPanel fragments;
};

//==============================================================================
/** SPACE tab: Post-Fracture vs Post-Space vs Master, with the Space bypass. */
class SpaceView : public StageComparisonView
{
public:
    SpaceView();

protected:
    std::vector<KeyValueTable::Row> extraInfo (const LabFrame& f) override;
};

} // namespace am::dev

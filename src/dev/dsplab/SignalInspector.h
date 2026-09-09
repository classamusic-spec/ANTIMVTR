#pragma once

#include "LabWidgets.h"
#include "dev/diagnostics/SignalMetrics.h"
#include "ui/visualizers/SpectrumAnalyzer.h"

namespace am::dev
{

/** Window of tap samples plus everything derived from it, computed once per
    frame and shared by the views that draw it. */
struct StageCapture
{
    static constexpr int kWindow = 4096;
    static constexpr int kBands  = 320;

    Stage stage = Stage::Master;
    std::array<float, kWindow> left {}, right {}, mono {};
    std::array<float, kBands> bands {};
    SignalMetrics metrics;
    float centroidHz = 0.0f;
    float flatness   = 0.0f;
    double sampleRate = 48000.0;
    bool valid = false;

    /** Reads the newest samples of `s` and recomputes the derived values. */
    void refresh (Diagnostics& diag, Stage s, double sr, bool wantSpectrum, bool wantSpectral);

private:
    ui::SpectrumAnalyzer analyzer;
    SpectrumMeasurement spectrum { 11 };
};

//==============================================================================
/** Stereo waveform of one capture (§81). */
class WaveformView : public LabPanel
{
public:
    WaveformView() : LabPanel ("Waveform") {}
    void setCapture (const StageCapture* c) { capture = c; repaint(); }
    void paint (juce::Graphics& g) override;

private:
    const StageCapture* capture = nullptr;
};

//==============================================================================
/** Log-frequency spectrum; up to four captures can be overlaid (§76, §79). */
class SpectrumView : public LabPanel
{
public:
    SpectrumView() : LabPanel ("Spectrum") {}

    struct Layer
    {
        const StageCapture* capture = nullptr;
        juce::Colour colour { ui::Theme::cyan };
        juce::String label;
        float fillAlpha = 0.16f;
    };

    void setLayers (std::vector<Layer> layers);
    void paint (juce::Graphics& g) override;

private:
    std::vector<Layer> layers;
};

//==============================================================================
/** Rolling spectrogram: one column per refresh, log frequency up the y axis. */
class SpectrogramView : public LabPanel
{
public:
    SpectrogramView();
    void push (const StageCapture& capture);
    void clear();
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Image image;
    int writeColumn = 0;
    bool wrapped = false;
};

//==============================================================================
/** L/R Lissajous phase scope with the correlation readout (§81). */
class PhaseScopeView : public LabPanel
{
public:
    PhaseScopeView() : LabPanel ("Phase scope") {}
    void setCapture (const StageCapture* c) { capture = c; repaint(); }
    void paint (juce::Graphics& g) override;

private:
    const StageCapture* capture = nullptr;
};

//==============================================================================
/**
    RAW AUDIO VIEWS (§81)

    Waveform, spectrum, rolling spectrogram, phase scope and the numeric
    metrics (RMS, peak, crest factor, DC, stereo correlation, non-finite
    count) for any engine stage. The stage selector and the view toggles
    belong to the inspector so every tab that embeds it behaves the same.
*/
class SignalInspector : public LabView
{
public:
    SignalInspector();

    void setStage (Stage s);
    Stage stage() const noexcept { return capture.stage; }

    /** Shows/hides the stage selector row (tabs that pin a stage hide it). */
    void setSelectorVisible (bool shouldBeVisible);
    /** Optional extra spectrum layer (e.g. the "pre" stage of a comparison). */
    void setReferenceCapture (const StageCapture* c, juce::Colour colour, const juce::String& label);

    const StageCapture& currentCapture() const noexcept { return capture; }

    void updateFrame (const LabFrame& f) override;
    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    void refreshLayers();
    void updateMetrics();

    StageCapture capture;
    juce::ComboBox stageBox;
    juce::ToggleButton showSpectrogram { "Spectrogram" }, showPhase { "Phase scope" };

    WaveformView waveform;
    SpectrumView spectrum;
    SpectrogramView spectrogram;
    PhaseScopeView phaseScope;
    KeyValueTable metrics { "Measurements" };

    const StageCapture* reference = nullptr;
    juce::Colour referenceColour { ui::Theme::violet };
    juce::String referenceLabel;
    bool selectorVisible = true;
};

} // namespace am::dev

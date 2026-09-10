#include "SignalInspector.h"

namespace am::dev
{

using namespace am::ui;

namespace
{
    constexpr float kSpectrumFloorDb = -96.0f;
    constexpr float kMinHz = 30.0f;

    /** Blue -> cyan -> amber -> white ramp for the spectrogram. */
    juce::Colour heatColour (float v) noexcept
    {
        v = juce::jlimit (0.0f, 1.0f, v);
        if (v < 0.34f)  return Theme::background.interpolatedWith (Theme::blue, v / 0.34f);
        if (v < 0.62f)  return Theme::blue.interpolatedWith (Theme::cyan, (v - 0.34f) / 0.28f);
        if (v < 0.85f)  return Theme::cyan.interpolatedWith (Theme::amber, (v - 0.62f) / 0.23f);
        return Theme::amber.interpolatedWith (Theme::ivory, (v - 0.85f) / 0.15f);
    }
}

//==============================================================================
void StageCapture::refresh (Diagnostics& diag, Stage s, double sr, bool wantSpectrum, bool wantSpectral)
{
    stage = s;
    sampleRate = sr > 0.0 ? sr : 48000.0;

    diag.taps[(int) s].readLatest (left.data(), right.data(), kWindow);
    for (int i = 0; i < kWindow; ++i)
        mono[(size_t) i] = 0.5f * (left[(size_t) i] + right[(size_t) i]);

    metrics = SignalMetrics::measure (left.data(), right.data(), kWindow);

    if (wantSpectrum)
        analyzer.compute (mono.data(), sampleRate, bands.data(), kBands, kSpectrumFloorDb);
    if (wantSpectral)
    {
        centroidHz = spectrum.spectralCentroid (mono.data(), kWindow, sampleRate);
        flatness   = spectrum.spectralFlatness (mono.data(), kWindow);
    }
    valid = true;
}

//==============================================================================
void WaveformView::paint (juce::Graphics& g)
{
    setSubtitle (capture != nullptr ? juce::String (stageName (capture->stage)).toUpperCase() + "   4096 SMP" : juce::String());
    LabPanel::paint (g);

    auto area = contentBoundsF();
    if (capture == nullptr || ! capture->valid)
    {
        plot::emptyState (g, area, "no capture");
        return;
    }

    plot::waveform (g, area, capture->left.data(), capture->right.data(), StageCapture::kWindow,
                    stageColour (capture->stage).withAlpha (0.95f), Theme::magenta.withAlpha (0.55f));

    plot::caption (g, "L", area.removeFromTop (10.0f).withTrimmedLeft (2.0f), stageColour (capture->stage), 8.5f);
    plot::caption (g, "R", area.removeFromTop (10.0f).withTrimmedLeft (2.0f), Theme::magenta.withAlpha (0.7f), 8.5f);
}

//==============================================================================
void SpectrumView::setLayers (std::vector<Layer> newLayers)
{
    layers = std::move (newLayers);
    repaint();
}

void SpectrumView::paint (juce::Graphics& g)
{
    LabPanel::paint (g);

    auto area = contentBoundsF();
    auto legend = area.removeFromTop (12.0f);

    double maxHz = 18000.0;
    bool any = false;
    for (const auto& l : layers)
        if (l.capture != nullptr && l.capture->valid)
        {
            any = true;
            maxHz = juce::jmin (18000.0, l.capture->sampleRate * 0.45);
        }

    if (! any)
    {
        plot::emptyState (g, area, "no capture");
        return;
    }

    plot::decibelGrid (g, area, kSpectrumFloorDb, true);
    plot::frequencyGrid (g, area, kMinHz, (float) maxHz, true);

    float legendX = legend.getX();
    for (const auto& l : layers)
    {
        if (l.capture == nullptr || ! l.capture->valid) continue;
        plot::spectrumCurve (g, area, l.capture->bands.data(), StageCapture::kBands, l.colour, l.fillAlpha);

        const auto text = l.label.isNotEmpty() ? l.label : juce::String (stageName (l.capture->stage)).toUpperCase();
        const float w = Theme::captionFont (9.0f).getStringWidthFloat (text) + 22.0f;
        g.setColour (l.colour);
        g.fillRect (legendX, legend.getCentreY() - 1.0f, 10.0f, 2.0f);
        plot::caption (g, text, legend.withX (legendX + 13.0f).withWidth (w), l.colour, 9.0f);
        legendX += w;
    }
}

//==============================================================================
SpectrogramView::SpectrogramView() : LabPanel ("Spectrogram") {}

void SpectrogramView::clear()
{
    if (image.isValid())
        image.clear (image.getBounds(), Theme::background);
    writeColumn = 0;
    wrapped = false;
}

void SpectrogramView::resized()
{
    const auto content = contentBounds();
    const int w = juce::jlimit (16, 1024, content.getWidth());
    const int h = juce::jlimit (16, 512, content.getHeight());
    if (! image.isValid() || image.getWidth() != w || image.getHeight() != h)
    {
        image = juce::Image (juce::Image::RGB, w, h, true);
        clear();
    }
}

void SpectrogramView::push (const StageCapture& capture)
{
    if (! image.isValid() || ! capture.valid)
        return;

    const int h = image.getHeight();
    juce::Image::BitmapData pixels (image, juce::Image::BitmapData::writeOnly);

    for (int y = 0; y < h; ++y)
    {
        // Bottom of the image = low frequencies; bands are already log spaced.
        const float u = 1.0f - (float) y / (float) juce::jmax (1, h - 1);
        const int band = juce::jlimit (0, StageCapture::kBands - 1, (int) (u * (StageCapture::kBands - 1)));
        pixels.setPixelColour (writeColumn, y, heatColour (capture.bands[(size_t) band]));
    }

    writeColumn = (writeColumn + 1) % image.getWidth();
    if (writeColumn == 0) wrapped = true;
}

void SpectrogramView::paint (juce::Graphics& g)
{
    LabPanel::paint (g);

    auto area = contentBounds();
    if (! image.isValid())
        return;

    if (! wrapped && writeColumn == 0)
    {
        plot::emptyState (g, area.toFloat(), "collecting");
        return;
    }

    // Draw so the newest column is on the right.
    const int w = image.getWidth();
    const int older = w - writeColumn;
    g.drawImage (image, area.getX(), area.getY(), older, area.getHeight(),
                 writeColumn, 0, older, image.getHeight());
    if (writeColumn > 0)
        g.drawImage (image, area.getX() + older, area.getY(), writeColumn, area.getHeight(),
                     0, 0, writeColumn, image.getHeight());

    plot::caption (g, "20 HZ", juce::Rectangle<float> ((float) area.getX() + 3.0f, (float) area.getBottom() - 11.0f, 44.0f, 10.0f), Theme::textDim, 8.0f);
    plot::caption (g, "18 KHZ", juce::Rectangle<float> ((float) area.getX() + 3.0f, (float) area.getY() + 1.0f, 44.0f, 10.0f), Theme::textDim, 8.0f);
    plot::caption (g, "NOW", juce::Rectangle<float> ((float) area.getRight() - 34.0f, (float) area.getBottom() - 11.0f, 32.0f, 10.0f),
                   Theme::textDim, 8.0f, juce::Justification::centredRight);
}

//==============================================================================
void PhaseScopeView::paint (juce::Graphics& g)
{
    LabPanel::paint (g);

    auto area = contentBoundsF();
    if (capture == nullptr || ! capture->valid)
    {
        plot::emptyState (g, area, "no capture");
        return;
    }

    const float size = juce::jmin (area.getWidth(), area.getHeight());
    auto square = juce::Rectangle<float> (size, size).withCentre (area.getCentre());

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawEllipse (square.reduced (1.0f), 1.0f);
    g.drawLine (square.getCentreX(), square.getY(), square.getCentreX(), square.getBottom(), 1.0f);
    g.drawLine (square.getX(), square.getCentreY(), square.getRight(), square.getCentreY(), 1.0f);

    // Mid/side rotation: x = (L-R), y = (L+R)
    const float radius = size * 0.5f - 2.0f;
    const int step = juce::jmax (1, StageCapture::kWindow / 1024);
    g.setColour (stageColour (capture->stage).withAlpha (0.5f));
    for (int i = 0; i < StageCapture::kWindow; i += step)
    {
        const float l = juce::jlimit (-1.0f, 1.0f, capture->left[(size_t) i]);
        const float r = juce::jlimit (-1.0f, 1.0f, capture->right[(size_t) i]);
        const float x = square.getCentreX() + (l - r) * 0.7071f * radius;
        const float y = square.getCentreY() - (l + r) * 0.7071f * radius;
        g.fillRect (x, y, 1.3f, 1.3f);
    }

    const auto corr = capture->metrics.correlation;
    plot::caption (g, "CORR " + juce::String (corr, 3),
                   area.removeFromBottom (11.0f), corr < 0.0f ? Theme::magenta : Theme::cyan, 9.0f,
                   juce::Justification::centred);
}

//==============================================================================
SignalInspector::SignalInspector()
{
    int id = 1;
    for (int i = 0; i < (int) Stage::Count; ++i)
        stageBox.addItem (juce::String (stageName ((Stage) i)).toUpperCase(), id++);
    stageBox.setSelectedId ((int) Stage::Master + 1, juce::dontSendNotification);
    stageBox.onChange = [this] { setStage ((Stage) (stageBox.getSelectedId() - 1)); };
    styleCombo (stageBox);
    addAndMakeVisible (stageBox);

    styleToggle (showSpectrogram);
    styleToggle (showPhase);
    showSpectrogram.setToggleState (true, juce::dontSendNotification);
    showPhase.setToggleState (true, juce::dontSendNotification);
    showSpectrogram.onClick = [this] { spectrogram.setVisible (showSpectrogram.getToggleState()); resized(); };
    showPhase.onClick = [this] { phaseScope.setVisible (showPhase.getToggleState()); resized(); };
    addAndMakeVisible (showSpectrogram);
    addAndMakeVisible (showPhase);

    capture.stage = Stage::Master;
    waveform.setCapture (&capture);
    phaseScope.setCapture (&capture);
    metrics.setRowHeight (14.0f);

    addAndMakeVisible (waveform);
    addAndMakeVisible (spectrum);
    addAndMakeVisible (spectrogram);
    addAndMakeVisible (phaseScope);
    addAndMakeVisible (metrics);
    refreshLayers();
}

void SignalInspector::setStage (Stage s)
{
    if (capture.stage == s)
        return;
    capture.stage = s;
    capture.valid = false;
    spectrogram.clear();
    stageBox.setSelectedId ((int) s + 1, juce::dontSendNotification);
    refreshLayers();
    repaint();
}

void SignalInspector::setSelectorVisible (bool shouldBeVisible)
{
    selectorVisible = shouldBeVisible;
    stageBox.setVisible (shouldBeVisible);
    resized();
}

void SignalInspector::setReferenceCapture (const StageCapture* c, juce::Colour colour, const juce::String& label)
{
    reference = c;
    referenceColour = colour;
    referenceLabel = label;
    refreshLayers();
}

void SignalInspector::refreshLayers()
{
    std::vector<SpectrumView::Layer> layers;
    if (reference != nullptr)
        layers.push_back ({ reference, referenceColour, referenceLabel, 0.06f });
    layers.push_back ({ &capture, stageColour (capture.stage), juce::String (stageName (capture.stage)).toUpperCase(), 0.16f });
    spectrum.setLayers (std::move (layers));
}

void SignalInspector::updateFrame (const LabFrame& f)
{
    capture.refresh (f.diagnostics, capture.stage, f.sampleRate, true, true);
    if (spectrogram.isVisible())
        spectrogram.push (capture);
    waveform.repaint();
    spectrum.repaint();
    phaseScope.repaint();
    updateMetrics();
}

void SignalInspector::updateMetrics()
{
    const auto& m = capture.metrics;
    metrics.setRows ({
        { "Peak",         juce::String (m.peak, 4) + "  (" + dbString (m.peak) + " dB)" },
        { "RMS",          juce::String (m.rms, 5) + "  (" + dbString (m.rms) + " dB)" },
        { "Crest factor", juce::String (m.crestFactor, 2) },
        { "DC offset",    juce::String (m.dc, 6) },
        { "Correlation",  juce::String (m.correlation, 3) },
        { "Centroid",     juce::String (capture.centroidHz, 0) + " Hz" },
        { "Flatness",     juce::String (capture.flatness, 4) },
        { "Non-finite",   juce::String (m.nonFinite) },
    });
    metrics.clearRowColours();
    if (m.peak >= 0.999f)  metrics.setRowColour (0, Theme::magenta);
    if (std::abs (m.dc) > 0.01f) metrics.setRowColour (3, Theme::amber);
    if (m.nonFinite > 0)   metrics.setRowColour (7, Theme::magenta);
}

void SignalInspector::paint (juce::Graphics&) {}

void SignalInspector::resized()
{
    auto area = getLocalBounds();

    if (selectorVisible)
    {
        auto row = area.removeFromTop (22);
        stageBox.setBounds (row.removeFromLeft (150));
        row.removeFromLeft (10);
        showSpectrogram.setBounds (row.removeFromLeft (110));
        showPhase.setBounds (row.removeFromLeft (110));
        area.removeFromTop (4);
    }
    else
    {
        auto row = area.removeFromTop (20);
        showSpectrogram.setBounds (row.removeFromLeft (110));
        showPhase.setBounds (row.removeFromLeft (110));
        area.removeFromTop (4);
    }

    const bool wantPhase = phaseScope.isVisible();
    const bool wantSpectrogram = spectrogram.isVisible();

    auto bottom = area.removeFromBottom (juce::jmax (92, area.getHeight() / 3));
    area.removeFromBottom (4);

    waveform.setBounds (area.removeFromTop (area.getHeight() / 2));
    area.removeFromTop (4);
    spectrum.setBounds (area);

    auto metricsArea = bottom.removeFromRight (juce::jmax (170, bottom.getWidth() / 4));
    bottom.removeFromRight (4);
    metrics.setBounds (metricsArea);

    if (wantPhase)
    {
        auto phaseArea = bottom.removeFromRight (juce::jmin (bottom.getWidth() / 3, bottom.getHeight() + 20));
        bottom.removeFromRight (4);
        phaseScope.setBounds (phaseArea);
    }
    if (wantSpectrogram)
        spectrogram.setBounds (bottom);
}

} // namespace am::dev

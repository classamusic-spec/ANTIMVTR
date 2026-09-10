#include "SourcePanels.h"
#include "ui/UILayout.h"

namespace am::ui
{

namespace
{
    /** One line per gesture model, shown under the model list. */
    juce::StringArray gestureDescriptions()
    {
        return {
            "Stick and slip locked to the note. Pressure grips, speed drives, roughness roars in the slip.",
            "Dense friction grains. Speed sets how many, roughness how bright the surface is.",
            "Slow, dark, periodic friction with a breathing amplitude.",
            "Turbulent air through a formant body. Pressure opens it, speed stirs the turbulence.",
            "Broad, harsh rasp: hard contact, saturated and torn by roughness.",
            "Buzz and spark: jittered pulse trains, sputtering with pressure."
        };
    }

    juce::String secondsText (double seconds)
    {
        return seconds < 1.0 ? juce::String (juce::roundToInt (seconds * 1000.0)) + " MS"
                             : juce::String (seconds, 2) + " S";
    }
}

//==============================================================================
SampleWaveView::SampleWaveView (juce::Colour c) : accent (c)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTooltip ("Drag the START and END handles to choose the part of the sample that plays. "
                "Click anywhere to move the nearer handle there.");
}

void SampleWaveView::setSample (SampleRef s)
{
    sample = std::move (s);
    peakWidth = 0;
    rebuildPeaks();
    repaint();
}

void SampleWaveView::setRange (float s, float e)
{
    if (std::abs (s - start) < 1.0e-4f && std::abs (e - end) < 1.0e-4f) return;
    start = clamp01 (s);
    end = clamp01 (e);
    repaint();
}

void SampleWaveView::resized()
{
    rebuildPeaks();
}

juce::Rectangle<float> SampleWaveView::rulerArea() const
{
    auto b = getLocalBounds().toFloat().reduced (2.0f, 2.0f);
    if (sample == nullptr || sample->isEmpty()) return {};
    return b.removeFromBottom (juce::jlimit (13.0f, 22.0f, b.getHeight() * 0.09f));
}

juce::Rectangle<float> SampleWaveView::plotArea() const
{
    auto b = getLocalBounds().toFloat().reduced (2.0f, 2.0f);
    const auto ruler = rulerArea();
    return ruler.isEmpty() ? b : b.withTrimmedBottom (ruler.getHeight());
}

void SampleWaveView::rebuildPeaks()
{
    const int width = juce::jmax (1, (int) plotArea().getWidth());
    if (width == peakWidth && ! minPeaks.empty()) return;

    peakWidth = width;
    minPeaks.assign ((size_t) width, 0.0f);
    maxPeaks.assign ((size_t) width, 0.0f);
    rmsPeaks.assign ((size_t) width, 0.0f);
    normalise = 1.0f;
    if (sample == nullptr || sample->isEmpty()) return;

    const int frames = sample->numFrames;
    float loudest = 0.0f;
    for (int x = 0; x < width; ++x)
    {
        const int from = (int) ((int64_t) frames * x / width);
        const int to = juce::jmax (from + 1, (int) ((int64_t) frames * (x + 1) / width));
        float lo = 0.0f, hi = 0.0f, sum = 0.0f;
        int count = 0;
        for (int c = 0; c < sample->numChannels; ++c)
        {
            const float* d = sample->channel (c);
            if (d == nullptr) continue;
            for (int i = from; i < to && i < frames; ++i)
            {
                lo = juce::jmin (lo, d[i]);
                hi = juce::jmax (hi, d[i]);
                sum += d[i] * d[i];
                ++count;
            }
        }
        minPeaks[(size_t) x] = lo;
        maxPeaks[(size_t) x] = hi;
        rmsPeaks[(size_t) x] = count > 0 ? std::sqrt (sum / (float) count) : 0.0f;
        loudest = juce::jmax (loudest, juce::jmax (-lo, hi));
    }
    // Quiet material still fills the view; the gain applied is stated in the corner.
    normalise = loudest > 1.0e-4f ? juce::jlimit (1.0f, 24.0f, 0.96f / loudest) : 1.0f;
}

float SampleWaveView::xForPosition (float position01) const
{
    const auto area = plotArea();
    return area.getX() + clamp01 (position01) * area.getWidth();
}

float SampleWaveView::positionForX (float x) const
{
    const auto area = plotArea();
    return clamp01 ((x - area.getX()) / juce::jmax (1.0f, area.getWidth()));
}

int SampleWaveView::handleAt (juce::Point<int> p) const
{
    const float tolerance = juce::jmax (6.0f, (float) getHeight() * 0.06f);
    const float dStart = std::abs ((float) p.x - xForPosition (start));
    const float dEnd = std::abs ((float) p.x - xForPosition (end));
    if (dStart <= tolerance && dStart <= dEnd) return 0;
    if (dEnd <= tolerance) return 1;
    return -1;
}

void SampleWaveView::mouseMove (const juce::MouseEvent& e)
{
    const int h = handleAt (e.getPosition());
    if (h != hovered) { hovered = h; repaint(); }
}

void SampleWaveView::mouseDown (const juce::MouseEvent& e)
{
    dragging = handleAt (e.getPosition());
    if (dragging < 0)
    {
        // Clicking in the empty part moves the nearer handle there.
        const float pos = positionForX ((float) e.position.x);
        dragging = std::abs (pos - start) <= std::abs (pos - end) ? 0 : 1;
    }
    if (onDragStart) onDragStart();
    mouseDrag (e);
}

void SampleWaveView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging < 0) return;
    const float pos = positionForX ((float) e.position.x);
    if (dragging == 0) start = juce::jmin (pos, end);
    else               end = juce::jmax (pos, start);
    if (onRangeChanged) onRangeChanged (start, end);
    repaint();
}

void SampleWaveView::mouseUp (const juce::MouseEvent&)
{
    dragging = -1;
    if (onDragEnd) onDragEnd();
}

void SampleWaveView::paintEmptyState (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);
    const float unit = juce::jlimit (10.0f, 17.0f, juce::jmin (area.getWidth() * 0.02f, area.getHeight() * 0.09f));

    auto card = juce::Rectangle<float> (juce::jmin (area.getWidth() - unit * 2.0f, unit * 24.0f),
                                        juce::jmin (area.getHeight() - unit * 2.0f, unit * 8.4f)).withCentre (area.getCentre());
    const float corner = juce::jmin (12.0f, card.getHeight() * 0.14f);

    juce::Path outline;
    outline.addRoundedRectangle (card, corner);
    const float dashes[] = { 5.0f, 5.0f };
    juce::Path dashed;
    juce::PathStrokeType (1.0f).createDashedStroke (dashed, outline, dashes, 2);
    g.setColour (accent.withAlpha (0.28f));
    g.fillPath (dashed);

    auto inner = card.reduced (unit, unit * 0.9f);
    Icons::draw (g, Icon::Sample, inner.removeFromTop (unit * 2.4f).withSizeKeepingCentre (unit * 2.2f, unit * 2.2f),
                 accent.withAlpha (0.45f), 1.0f);
    inner.removeFromTop (unit * 0.5f);
    draw::trackedText (g, "NO SAMPLE LOADED", inner.removeFromTop (unit * 1.5f), juce::Justification::centred,
                       Theme::labelFontStrong (unit * 0.9f), Theme::textSecondary);
    draw::trackedText (g, "DROP AN AUDIO FILE HERE, OR PICK A BUILT-IN ENERGY ABOVE",
                       inner.removeFromTop (unit * 1.5f), juce::Justification::centred,
                       draw::fitFont (Theme::captionFont (unit * 0.72f), "DROP AN AUDIO FILE HERE, OR PICK A BUILT-IN ENERGY ABOVE", inner.getWidth()),
                       Theme::textDim);
}

void SampleWaveView::paintRuler (juce::Graphics& g)
{
    const auto ruler = rulerArea();
    const auto area = plotArea();
    if (ruler.isEmpty() || sample == nullptr) return;

    const double seconds = sample->lengthSeconds();
    if (seconds <= 0.0) return;

    const double step = layout::waveRulerStep (seconds);

    const float h = juce::jlimit (7.0f, 9.5f, ruler.getHeight() * 0.58f);
    g.setColour (Theme::borderSoft);
    g.drawLine (area.getX(), ruler.getY(), area.getRight(), ruler.getY(), 1.0f);

    for (int i = 0; (double) i * step <= seconds + 1.0e-6; ++i)
    {
        const double t = (double) i * step;
        const float x = area.getX() + (float) (t / seconds) * area.getWidth();
        g.setColour (Theme::textDim.withAlpha (0.5f));
        g.drawLine (x, ruler.getY(), x, ruler.getY() + ruler.getHeight() * 0.35f, 1.0f);
        if (i == 0) continue;
        const juce::String label = step < 1.0 ? juce::String (juce::roundToInt (t * 1000.0)) : juce::String (t, t < 10.0 ? 1 : 0);
        const auto box = juce::Rectangle<float> (x - 26.0f, ruler.getY() + ruler.getHeight() * 0.3f, 52.0f, ruler.getHeight() * 0.7f);
        if (box.getRight() > area.getRight() + 4.0f) continue;
        draw::trackedText (g, label, box, juce::Justification::centred, Theme::valueFont (h), Theme::textDim);
    }
    draw::trackedText (g, step < 1.0 ? "MS" : "S", ruler.withTrimmedLeft (ruler.getWidth() - 22.0f),
                       juce::Justification::centredRight, Theme::captionFont (h * 0.92f), Theme::textDim.withAlpha (0.8f));
}

void SampleWaveView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    draw::insetSurface (g, bounds, 8.0f);

    if (sample == nullptr || sample->isEmpty())
    {
        paintEmptyState (g);
        return;
    }

    const auto area = plotArea();
    const float mid = area.getCentreY();

    // Selection: everything outside start..end is dimmed.
    const float xs = xForPosition (start), xe = xForPosition (end);
    g.setColour (accent.withAlpha (0.045f + 0.05f * energy));
    g.fillRect (area.withLeft (xs).withRight (juce::jmax (xs + 1.0f, xe)));

    // Amplitude guides at half scale give the envelope a sense of level.
    for (float f : { -0.5f, 0.5f })
    {
        g.setColour (juce::Colours::white.withAlpha (0.035f));
        g.drawLine (area.getX(), mid + f * area.getHeight() * 0.44f, area.getRight(), mid + f * area.getHeight() * 0.44f, 1.0f);
    }

    // Waveform: a filled peak envelope with a brighter RMS core.
    auto envelope = [&] (const std::vector<float>& lo, const std::vector<float>& hi, float gain)
    {
        juce::Path body;
        const int width = (int) lo.size();
        body.startNewSubPath (area.getX(), mid);
        for (int x = 0; x < width; ++x)
            body.lineTo (area.getX() + (float) x, mid - juce::jlimit (-1.0f, 1.0f, hi[(size_t) x] * gain) * area.getHeight() * 0.44f);
        for (int x = width - 1; x >= 0; --x)
            body.lineTo (area.getX() + (float) x, mid - juce::jlimit (-1.0f, 1.0f, lo[(size_t) x] * gain) * area.getHeight() * 0.44f);
        body.closeSubPath();
        return body;
    };

    const auto peaks = envelope (minPeaks, maxPeaks, normalise);
    juce::ColourGradient grad (accent.withAlpha (0.46f + 0.2f * energy), area.getCentreX(), mid,
                               accent.withAlpha (0.05f), area.getCentreX(), area.getY(), false);
    grad.addColour (0.5, accent.withAlpha (0.16f));
    g.setGradientFill (grad);
    g.fillPath (peaks);
    g.setColour (accent.withAlpha (0.85f));
    g.strokePath (peaks, juce::PathStrokeType (1.0f));

    {
        std::vector<float> lo ((size_t) rmsPeaks.size());
        for (size_t i = 0; i < rmsPeaks.size(); ++i) lo[i] = -rmsPeaks[i];
        g.setColour (accent.withAlpha (0.30f));
        g.fillPath (envelope (lo, rmsPeaks, normalise));
    }

    draw::hairline (g, area.getX(), mid, area.getRight(), mid, 0.14f);

    // Everything outside the playback range is pushed back.
    g.setColour (Theme::background.withAlpha (0.62f));
    g.fillRect (area.withRight (xs));
    g.fillRect (area.withLeft (xe));

    paintRuler (g);

    if (normalise > 1.05f)
        draw::trackedText (g, "NORMALISED +" + juce::String (20.0f * std::log10 (normalise), 1) + " DB",
                           area.reduced (6.0f, 4.0f).removeFromTop (12.0f), juce::Justification::topRight,
                           Theme::captionFont (8.5f), Theme::textDim);

    // Handles: a bright edge, a grip tab at the top and a label that stays inside the view.
    const float grip = juce::jlimit (7.0f, 12.0f, area.getHeight() * 0.055f);
    const float labelH = juce::jlimit (9.0f, 12.0f, area.getHeight() * 0.05f);
    const float labelWidth = juce::jmax (40.0f, labelH * 4.2f);

    for (int i = 0; i < 2; ++i)
    {
        const float x = i == 0 ? xs : xe;
        const bool lit = hovered == i || dragging == i;
        const float edge = juce::jlimit (area.getX(), area.getRight() - 2.0f, i == 0 ? x : x - 2.0f);
        g.setColour (accent.withAlpha (lit ? 1.0f : 0.8f));
        g.fillRect (edge, area.getY(), 2.0f, area.getHeight());

        juce::Path tab;
        const float tx = i == 0 ? edge : edge + 2.0f;
        tab.startNewSubPath (tx, area.getY());
        tab.lineTo (tx + (i == 0 ? grip : -grip), area.getY());
        tab.lineTo (tx, area.getY() + grip);
        tab.closeSubPath();
        if (lit) draw::glowPath (g, tab, accent, 1.0f, grip, 0.9f);
        g.setColour (accent.withAlpha (lit ? 1.0f : 0.85f));
        g.fillPath (tab);

        auto label = juce::Rectangle<float> (i == 0 ? edge + 5.0f : edge - labelWidth - 3.0f,
                                             area.getBottom() - labelH - 3.0f, labelWidth, labelH);
        label.setX (juce::jlimit (area.getX() + 3.0f, area.getRight() - labelWidth - 3.0f, label.getX()));
        draw::trackedText (g, i == 0 ? "START" : "END", label,
                           i == 0 ? juce::Justification::centredLeft : juce::Justification::centredRight,
                           Theme::labelFont (labelH * 0.78f), accent.withAlpha (lit ? 1.0f : 0.7f));
    }
}

//==============================================================================
SamplePanel::SamplePanel (AntiMatrProcessor& p)
    : AMPanel ("Sample", "Imported matter", Theme::ivory), processor (p)
{
    juce::StringArray names;
    for (int i = 0; i < BuiltInSamples::count(); ++i) names.add (juce::String (BuiltInSamples::name (i)).toUpperCase());
    names.add ("FILE...");
    builtIn = std::make_unique<AMChoice> ("Sample", names, Theme::ivory);
    builtIn->setShowLabel (false);
    builtIn->setTooltip ("Built-in energies, generated in code. FILE... loads any WAV / AIFF / FLAC / OGG.");
    builtIn->onChange = [this] (int index)
    {
        if (index >= BuiltInSamples::count()) chooseFile();
        else processor.selectBuiltInSample (index);
        refreshSample (true);
    };
    addAndMakeVisible (*builtIn);

    loadButton.setOutlined (true);
    loadButton.onClick = [this] { chooseFile(); };
    loadButton.setTooltip ("Load an audio file, or drop one anywhere on this panel.");
    addAndMakeVisible (loadButton);

    analyzeButton.setFilled (true);
    analyzeButton.onClick = [this] { runAnalysis(); };
    analyzeButton.setTooltip ("Estimate the partials of the sample and shape Matter to match them.");
    addAndMakeVisible (analyzeButton);

    addAndMakeVisible (waveView);

    auto& apvts = processor.parameters();
    auto* startParam = apvts.getParameter (ParameterRegistry::get (Param::sampleStart).id);
    auto* endParam = apvts.getParameter (ParameterRegistry::get (Param::sampleEnd).id);
    startAttachment = std::make_unique<juce::ParameterAttachment> (*startParam, [this] (float) { setRangeFromParams(); });
    endAttachment = std::make_unique<juce::ParameterAttachment> (*endParam, [this] (float) { setRangeFromParams(); });
    waveView.onDragStart = [this] { startAttachment->beginGesture(); endAttachment->beginGesture(); };
    waveView.onDragEnd = [this] { startAttachment->endGesture(); endAttachment->endGesture(); };
    waveView.onRangeChanged = [this] (float s, float e)
    {
        startAttachment->setValueAsPartOfGesture (s);
        endAttachment->setValueAsPartOfGesture (e);
    };
    startAttachment->sendInitialUpdate();
    endAttachment->sendInitialUpdate();

    for (auto param : { Param::sampleMode, Param::sampleRoot, Param::samplePitch, Param::sampleGrain,
                        Param::sampleSpread, Param::sampleKeytrack })
    {
        juce::String label (ParameterRegistry::get (param).name);
        if (label.startsWithIgnoreCase ("Sample ")) label = label.substring (7);
        controls.push_back (std::make_unique<BoundControl> (apvts, param, Theme::ivory, label));
        addAndMakeVisible (controls.back()->component());
    }

    refreshSample (true);
    startTimerHz (12);
}

SamplePanel::~SamplePanel()
{
    stopTimer();
}

void SamplePanel::setRangeFromParams()
{
    const auto values = processor.currentParamValues();
    waveView.setRange (paramValue (values, Param::sampleStart), paramValue (values, Param::sampleEnd));
}

void SamplePanel::chooseFile()
{
    chooser = std::make_unique<juce::FileChooser> ("Load a sample", juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                   "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();
                              if (file.existsAsFile()) processor.loadSampleFile (file);
                              refreshSample (true);
                          });
}

void SamplePanel::runAnalysis()
{
    if (processor.analyzeSampleToMatter())
    {
        showAnalysis();
    }
    else
    {
        statusIsWarning = true;
        statusLine = "NOTHING TO ANALYZE";
    }
    repaint();
}

void SamplePanel::showAnalysis()
{
    const auto& table = processor.lastAnalysis();
    if (! table.isValid()) return;
    statusIsWarning = false;
    statusLine = juce::String::fromUTF8 ("ANALYZED \xe2\x86\x92 MATTER:  ") + juce::String (table.count) + " PARTIALS   F0 "
               + juce::String (table.fundamentalHz, 1) + " HZ   FLOOR " + juce::String (juce::roundToInt (table.noiseFloorDb)) + " DB";
}

bool SamplePanel::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (f.endsWithIgnoreCase (".wav") || f.endsWithIgnoreCase (".aif") || f.endsWithIgnoreCase (".aiff")
            || f.endsWithIgnoreCase (".flac") || f.endsWithIgnoreCase (".ogg") || f.endsWithIgnoreCase (".mp3"))
            return true;
    return false;
}

void SamplePanel::filesDropped (const juce::StringArray& files, int, int)
{
    dropActive = false;
    for (const auto& f : files)
    {
        const juce::File file (f);
        if (file.existsAsFile() && processor.loadSampleFile (file)) break;
    }
    refreshSample (true);
}

void SamplePanel::refreshSample (bool force)
{
    const auto info = processor.currentSampleInfo();
    if (! force && info.version == shownVersion) return;
    shownVersion = info.version;

    waveView.setSample (processor.currentSample());
    waveView.setCaption (info.name);
    builtIn->setSelected (info.isBuiltIn() ? info.builtInIndex : BuiltInSamples::count(), juce::dontSendNotification);

    infoLine = info.name.toUpperCase() + "   " + secondsText (info.lengthSeconds())
             + "   " + juce::String (juce::roundToInt (info.sampleRate / 1000.0)) + " KHZ   "
             + (info.numChannels > 1 ? "STEREO" : "MONO");
    if (info.warning.isNotEmpty())
    {
        statusIsWarning = true;
        statusLine = info.warning.toUpperCase();
    }
    else if (processor.lastAnalysis().isValid() && processor.lastAnalysis().source == info.name)
    {
        showAnalysis();
    }
    setRangeFromParams();
    repaint();
}

void SamplePanel::timerCallback()
{
    if (! isShowing()) return;
    refreshSample (false);
    const auto& vs = processor.diagnostics().visualSnapshots.latest();
    waveView.setEnergy (juce::jlimit (0.0f, 1.0f, vs.sourceRms * 4.0f));
    setActivity (juce::jlimit (0.0f, 1.0f, vs.sourceRms * 2.0f));
}

void SamplePanel::paintOverChildren (juce::Graphics& g)
{
    // Info line sits in the header row, right aligned.
    auto header = headerBounds().toFloat();
    draw::trackedText (g, infoLine, header.reduced ((float) padding(), 0.0f), juce::Justification::centredRight,
                       Theme::captionFont (juce::jlimit (8.0f, 10.5f, header.getHeight() * 0.30f)), Theme::textSecondary);

    if (! statusStrip.isEmpty())
    {
        const auto strip = statusStrip.toFloat();
        const float h = juce::jlimit (8.0f, 10.5f, strip.getHeight() * 0.62f);
        const juce::String text = statusLine.isNotEmpty() ? statusLine
                                                          : juce::String ("DRAG THE HANDLES TO SET THE PLAYBACK RANGE");
        const auto colour = statusLine.isEmpty() ? Theme::textDim
                                                 : (statusIsWarning ? Theme::amber : Theme::cyan);
        if (statusLine.isNotEmpty())
        {
            g.setColour (colour.withAlpha (0.8f));
            g.fillRoundedRectangle (strip.withWidth (2.0f).reduced (0.0f, 1.0f).toFloat(), 1.0f);
        }
        draw::trackedText (g, text, strip.withTrimmedLeft (statusLine.isNotEmpty() ? 8.0f : 0.0f),
                           juce::Justification::centredLeft, Theme::captionFont (h), colour);
    }

    if (dropActive)
    {
        g.setColour (Theme::ivory.withAlpha (0.6f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (3.0f), 10.0f, 2.0f);
    }
}

void SamplePanel::resized()
{
    auto area = contentBounds();
    if (area.isEmpty()) return;
    const int gap = juce::jmax (6, area.getHeight() / 40);

    // ---- top row: built-in picker and file loading
    auto top = area.removeFromTop (juce::jlimit (26, 40, area.getHeight() / 8));
    const int buttonWidth = juce::jlimit (90, 150, top.getWidth() / 5);
    builtIn->setBounds (top.removeFromLeft (juce::jlimit (140, 260, top.getWidth() / 3)));
    top.removeFromLeft (gap);
    loadButton.setBounds (top.removeFromLeft (buttonWidth));
    area.removeFromTop (gap);

    // ---- controls row
    auto bottom = area.removeFromBottom (juce::jlimit (86, 150, (int) ((float) area.getHeight() * 0.34f)));
    area.removeFromBottom (gap);

    // The status line is a slim strip directly under the waveform; the waveform takes
    // everything else, so there is never a band of empty panel between the two.
    statusStrip = area.removeFromBottom (juce::jlimit (14, 22, area.getHeight() / 14));
    area.removeFromBottom (gap / 2);
    waveView.setBounds (area);

    auto analyze = bottom.removeFromRight (juce::jlimit (120, 200, bottom.getWidth() / 5));
    analyzeButton.setBounds (analyze.withSizeKeepingCentre (analyze.getWidth(), juce::jlimit (28, 38, analyze.getHeight() / 3)));
    bottom.removeFromRight (gap);

    std::vector<juce::Component*> comps;
    for (auto& c : controls) comps.push_back (&c->component());
    layoutGrid (bottom, comps, (int) comps.size(), gap / 2, 2);
}

//==============================================================================
GesturePanel::GesturePanel (AntiMatrProcessor& p)
    : AMPanel ("Gesture", "Living gesture", Theme::magenta), processor (p)
{
    auto& apvts = processor.parameters();

    mode = std::make_unique<AMOptionList> (paramChoices (Param::gestureMode), Theme::magenta);
    mode->setDescriptions (gestureDescriptions());
    mode->setTooltip ("The friction model that turns pressure and speed into excitation.");
    addAndMakeVisible (*mode);

    auto* modeParam = apvts.getParameter (ParameterRegistry::get (Param::gestureMode).id);
    modeAttachment = std::make_unique<juce::ParameterAttachment> (*modeParam, [this] (float v)
    {
        currentMode = juce::jlimit (0, paramChoices (Param::gestureMode).size() - 1, (int) std::lround (v));
        mode->setSelected (currentMode, juce::dontSendNotification);
        repaint();
    });
    mode->onChange = [this] (int index) { modeAttachment->setValueAsCompleteGesture ((float) index); };
    modeAttachment->sendInitialUpdate();

    pad.setTooltip ("Drag: pressure (x) and speed (y). Double-click resets.");
    pad.formatX = [] (float v) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; };
    pad.formatY = [] (float v) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; };
    addAndMakeVisible (pad);

    auto* px = apvts.getParameter (ParameterRegistry::get (Param::gesturePressure).id);
    auto* py = apvts.getParameter (ParameterRegistry::get (Param::gestureSpeed).id);
    padX = std::make_unique<juce::ParameterAttachment> (*px, [this] (float v) { pad.setPosition (v, pad.getY01(), juce::dontSendNotification); });
    padY = std::make_unique<juce::ParameterAttachment> (*py, [this] (float v) { pad.setPosition (pad.getX01(), v, juce::dontSendNotification); });
    pad.onDragStart = [this] { padX->beginGesture(); padY->beginGesture(); };
    pad.onDragEnd = [this] { padX->endGesture(); padY->endGesture(); };
    pad.onChange = [this] (float x, float y) { padX->setValueAsPartOfGesture (x); padY->setValueAsPartOfGesture (y); };
    pad.setDefault (ParameterRegistry::get (Param::gesturePressure).defaultValue,
                    ParameterRegistry::get (Param::gestureSpeed).defaultValue);
    padX->sendInitialUpdate();
    padY->sendInitialUpdate();

    for (auto param : { Param::gestureRoughness, Param::gesturePosition, Param::gestureMotion, Param::gestureBandwidth })
    {
        juce::String label (ParameterRegistry::get (param).name);
        controls.push_back (std::make_unique<BoundControl> (apvts, param, Theme::magenta, label));
        addAndMakeVisible (controls.back()->component());
    }

    startTimerHz (20);
}

GesturePanel::~GesturePanel()
{
    stopTimer();
}

void GesturePanel::timerCallback()
{
    if (! isShowing()) return;
    const auto& vs = processor.diagnostics().visualSnapshots.latest();
    const float energy = juce::jlimit (0.0f, 1.0f, vs.sourceRms * 4.0f);
    pad.setEnergy (energy);
    setActivity (juce::jlimit (0.0f, 1.0f, vs.sourceRms * 2.0f));
}

void GesturePanel::resized()
{
    auto area = contentBounds();
    if (area.isEmpty()) return;
    const int gap = juce::jmax (6, area.getWidth() / 90);

    auto left = area.removeFromLeft (juce::jlimit (150, 270, (int) ((float) area.getWidth() * 0.23f)));
    area.removeFromLeft (gap);
    mode->setBounds (left);

    auto padArea = area.removeFromLeft (juce::jlimit (160, 460, (int) ((float) area.getWidth() * 0.46f)));
    area.removeFromLeft (gap);
    pad.setBounds (padArea);

    std::vector<juce::Component*> comps;
    for (auto& c : controls) comps.push_back (&c->component());
    layoutGrid (area, comps, 2, gap, gap);
}

} // namespace am::ui

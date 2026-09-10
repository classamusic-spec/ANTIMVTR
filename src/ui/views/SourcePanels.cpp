#include "SourcePanels.h"

namespace am::ui
{

namespace
{
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

juce::Rectangle<float> SampleWaveView::plotArea() const
{
    return getLocalBounds().toFloat().reduced (2.0f, 2.0f);
}

void SampleWaveView::rebuildPeaks()
{
    const int width = juce::jmax (1, (int) plotArea().getWidth());
    if (width == peakWidth && ! minPeaks.empty()) return;

    peakWidth = width;
    minPeaks.assign ((size_t) width, 0.0f);
    maxPeaks.assign ((size_t) width, 0.0f);
    if (sample == nullptr || sample->isEmpty()) return;

    const int frames = sample->numFrames;
    for (int x = 0; x < width; ++x)
    {
        const int from = (int) ((int64_t) frames * x / width);
        const int to = juce::jmax (from + 1, (int) ((int64_t) frames * (x + 1) / width));
        float lo = 0.0f, hi = 0.0f;
        for (int c = 0; c < sample->numChannels; ++c)
        {
            const float* d = sample->channel (c);
            if (d == nullptr) continue;
            for (int i = from; i < to && i < frames; ++i)
            {
                lo = juce::jmin (lo, d[i]);
                hi = juce::jmax (hi, d[i]);
            }
        }
        minPeaks[(size_t) x] = lo;
        maxPeaks[(size_t) x] = hi;
    }
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

void SampleWaveView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    draw::insetSurface (g, bounds, 8.0f);

    const auto area = plotArea();
    const float mid = area.getCentreY();

    if (sample == nullptr || sample->isEmpty())
    {
        draw::trackedText (g, "NO SAMPLE", area, juce::Justification::centred,
                           Theme::captionFont (juce::jlimit (9.0f, 13.0f, area.getHeight() * 0.09f)), Theme::textDim);
        return;
    }

    // Selection: everything outside start..end is dimmed.
    const float xs = xForPosition (start), xe = xForPosition (end);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRect (area.withRight (xs));
    g.fillRect (area.withLeft (xe));
    g.setColour (accent.withAlpha (0.05f + 0.05f * energy));
    g.fillRect (area.withLeft (xs).withRight (juce::jmax (xs + 1.0f, xe)));

    // Waveform: a filled envelope lit from the centre line outwards.
    juce::Path body;
    const int width = (int) minPeaks.size();
    const float scale = area.getHeight() * 0.44f;
    body.startNewSubPath (area.getX(), mid);
    for (int x = 0; x < width; ++x)
        body.lineTo (area.getX() + (float) x, mid - maxPeaks[(size_t) x] * scale);
    for (int x = width - 1; x >= 0; --x)
        body.lineTo (area.getX() + (float) x, mid - minPeaks[(size_t) x] * scale);
    body.closeSubPath();

    juce::ColourGradient body_grad (accent.withAlpha (0.42f + 0.2f * energy), area.getCentreX(), mid,
                                    accent.withAlpha (0.04f), area.getCentreX(), area.getY(), false);
    body_grad.addColour (0.5, accent.withAlpha (0.14f));
    g.setGradientFill (body_grad);
    g.fillPath (body);
    g.setColour (accent.withAlpha (0.85f));
    g.strokePath (body, juce::PathStrokeType (1.0f));

    draw::hairline (g, area.getX(), mid, area.getRight(), mid, 0.12f);

    // Handles: a bright edge, a grip tab and a label that stays inside the view.
    const float grip = juce::jlimit (6.0f, 10.0f, area.getHeight() * 0.05f);
    const float labelWidth = 44.0f;
    for (int i = 0; i < 2; ++i)
    {
        const float x = i == 0 ? xs : xe;
        const bool lit = hovered == i || dragging == i;
        const float edge = juce::jlimit (area.getX(), area.getRight() - 2.0f, i == 0 ? x : x - 2.0f);
        g.setColour (accent.withAlpha (lit ? 1.0f : 0.75f));
        g.fillRect (edge, area.getY(), 2.0f, area.getHeight());
        juce::Rectangle<float> tab (i == 0 ? edge : edge + 2.0f - grip, area.getY(), grip, grip);
        g.fillRect (tab);
        if (lit) draw::glowRoundedRect (g, tab, 2.0f, accent, 10.0f, 0.9f);

        auto label = juce::Rectangle<float> (i == 0 ? edge + 5.0f : edge - labelWidth - 3.0f,
                                             area.getBottom() - 15.0f, labelWidth, 12.0f);
        label.setX (juce::jlimit (area.getX() + 3.0f, area.getRight() - labelWidth - 3.0f, label.getX()));
        draw::trackedText (g, i == 0 ? "START" : "END", label,
                           i == 0 ? juce::Justification::centredLeft : juce::Justification::centredRight,
                           Theme::captionFont (8.0f), accent.withAlpha (lit ? 0.95f : 0.5f));
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

    if (statusLine.isNotEmpty())
        draw::trackedText (g, statusLine, statusStrip.toFloat(), juce::Justification::centredLeft, Theme::captionFont (9.0f),
                           statusIsWarning ? Theme::amber : Theme::cyan);

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
    // The waveform keeps a sane height on tall windows; the slack below it carries the status line.
    const int waveHeight = juce::jmin (area.getHeight(), juce::jmax (200, getHeight() / 2));
    waveView.setBounds (area.removeFromTop (waveHeight));
    statusStrip = area.withTrimmedTop (gap / 2).withHeight (juce::jmax (14, getHeight() / 26));

    auto analyze = bottom.removeFromRight (juce::jlimit (120, 200, bottom.getWidth() / 5));
    analyzeButton.setBounds (analyze.withSizeKeepingCentre (analyze.getWidth(), juce::jlimit (28, 38, analyze.getHeight() / 3)));
    bottom.removeFromRight (gap);

    std::vector<juce::Component*> comps;
    for (auto& c : controls) comps.push_back (&c->component());
    layoutGrid (bottom, comps, (int) comps.size(), 2, 2);
}

//==============================================================================
GesturePanel::ModeList::ModeList (juce::StringArray items, juce::Colour c)
    : names (std::move (items)), accent (c)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void GesturePanel::ModeList::setSelected (int index)
{
    index = juce::jlimit (0, juce::jmax (0, names.size() - 1), index);
    if (index == selected) return;
    selected = index;
    repaint();
}

int GesturePanel::ModeList::rowAt (juce::Point<int> p) const
{
    if (names.isEmpty() || ! getLocalBounds().contains (p)) return -1;
    const float rowHeight = (float) getHeight() / (float) names.size();
    return juce::jlimit (0, names.size() - 1, (int) ((float) p.y / juce::jmax (1.0f, rowHeight)));
}

void GesturePanel::ModeList::mouseDown (const juce::MouseEvent& e)
{
    const int row = rowAt (e.getPosition());
    if (row >= 0 && onSelect) onSelect (row);
}

void GesturePanel::ModeList::mouseMove (const juce::MouseEvent& e)
{
    const int row = rowAt (e.getPosition());
    if (row != hovered) { hovered = row; repaint(); }
}

void GesturePanel::ModeList::paint (juce::Graphics& g)
{
    if (names.isEmpty()) return;
    const float rowHeight = (float) getHeight() / (float) names.size();
    const float fontHeight = juce::jlimit (9.0f, 12.5f, rowHeight * 0.36f);

    for (int i = 0; i < names.size(); ++i)
    {
        auto row = juce::Rectangle<float> (0.0f, (float) i * rowHeight, (float) getWidth(), rowHeight).reduced (0.0f, 1.5f);
        const bool isSelected = i == selected;
        const bool isHovered = i == hovered;

        if (isSelected)
        {
            g.setColour (accent.withAlpha (0.13f));
            g.fillRoundedRectangle (row, 5.0f);
            draw::glowRoundedRect (g, row, 5.0f, accent, 10.0f, 0.5f);
            g.setColour (accent);
            g.fillRoundedRectangle (row.withWidth (2.5f), 1.2f);
        }
        else if (isHovered)
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.fillRoundedRectangle (row, 5.0f);
        }

        draw::trackedText (g, names[i], row.withTrimmedLeft (12.0f), juce::Justification::centredLeft,
                           Theme::captionFont (fontHeight),
                           isSelected ? Theme::textPrimary : (isHovered ? Theme::textSecondary.brighter (0.2f) : Theme::textSecondary));
    }
}

//==============================================================================
GesturePanel::GesturePanel (AntiMatrProcessor& p)
    : AMPanel ("Gesture", "Living gesture", Theme::magenta), processor (p)
{
    auto& apvts = processor.parameters();

    mode = std::make_unique<ModeList> (paramChoices (Param::gestureMode), Theme::magenta);
    addAndMakeVisible (*mode);

    auto* modeParam = apvts.getParameter (ParameterRegistry::get (Param::gestureMode).id);
    modeAttachment = std::make_unique<juce::ParameterAttachment> (*modeParam, [this] (float v)
    {
        currentMode = juce::jlimit (0, paramChoices (Param::gestureMode).size() - 1, (int) std::lround (v));
        mode->setSelected (currentMode);
        repaint();
    });
    mode->onSelect = [this] (int index) { modeAttachment->setValueAsCompleteGesture ((float) index); };
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

void GesturePanel::paintOverChildren (juce::Graphics& g)
{
    static const char* descriptions[] = {
        "Stick and slip locked to the note. Pressure grips, speed drives, roughness roars in the slip.",
        "Dense friction grains. Speed sets how many, roughness how bright the surface is.",
        "Slow, dark, periodic friction with a breathing amplitude.",
        "Turbulent air through a formant body. Pressure opens it, speed stirs the turbulence.",
        "Broad, harsh rasp: hard contact, saturated and torn by roughness.",
        "Buzz and spark: jittered pulse trains, sputtering with pressure."
    };

    auto area = descriptionArea.toFloat();
    if (area.isEmpty()) return;

    juce::AttributedString text;
    text.append (descriptions[(size_t) juce::jlimit (0, 5, currentMode)],
                 Theme::bodyFont (juce::jlimit (10.0f, 12.5f, area.getHeight() * 0.20f)), Theme::textSecondary.brighter (0.1f));
    text.setLineSpacing (3.0f);
    juce::TextLayout layout;
    layout.createLayout (text, area.getWidth());
    layout.draw (g, area);
}

void GesturePanel::resized()
{
    auto area = contentBounds();
    if (area.isEmpty()) return;
    const int gap = juce::jmax (6, area.getWidth() / 90);

    auto left = area.removeFromLeft (juce::jlimit (140, 250, (int) ((float) area.getWidth() * 0.21f)));
    area.removeFromLeft (gap);
    const int rows = juce::jmax (1, paramChoices (Param::gestureMode).size());
    mode->setBounds (left.removeFromTop (juce::jmin (left.getHeight() - 40, rows * juce::jlimit (26, 40, left.getHeight() / (rows + 2)))));
    left.removeFromTop (gap);
    descriptionArea = left;

    auto padArea = area.removeFromLeft (juce::jlimit (160, 460, (int) ((float) area.getWidth() * 0.46f)));
    area.removeFromLeft (gap);
    pad.setBounds (padArea);

    std::vector<juce::Component*> comps;
    for (auto& c : controls) comps.push_back (&c->component());
    layoutGrid (area, comps, 2, 2, 2);
}

} // namespace am::ui

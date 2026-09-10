#include "Pages.h"

namespace am::ui
{

namespace
{
    int pagePad (const juce::Component& c) { return juce::jmax (10, c.getWidth() / 80); }

    juce::String envelopeText (float seconds)
    {
        return seconds < 1.0f ? juce::String (juce::roundToInt (seconds * 1000.0f)) + " ms" : juce::String (seconds, 2) + " s";
    }
}

//==============================================================================
ParamPanel::ParamPanel (AntiMatrProcessor& p, const juce::String& title, const juce::String& subtitle, juce::Colour accent,
                        std::vector<Param> params, int cols)
    : AMPanel (title, subtitle, accent), processor (p), columns (cols)
{
    for (auto param : params)
    {
        controls.push_back (std::make_unique<BoundControl> (processor.parameters(), param, accent));
        addAndMakeVisible (controls.back()->component());
    }
}

BoundControl* ParamPanel::control (Param p)
{
    for (auto& c : controls) if (c->param() == p) return c.get();
    return headerToggle != nullptr && headerToggle->param() == p ? headerToggle.get() : nullptr;
}

void ParamPanel::setHeaderToggle (Param p)
{
    headerToggle = std::make_unique<BoundControl> (processor.parameters(), p, getAccent(), " ");
    addAndMakeVisible (headerToggle->component());
    resized();
}

void ParamPanel::setHeroKnobs (bool hero)
{
    for (auto& c : controls) if (auto* k = c->knob()) k->setHero (hero);
}

void ParamPanel::setAccentFor (Param p, juce::Colour colour)
{
    if (auto* c = control (p))
    {
        if (auto* k = c->knob()) k->setAccent (colour);
        else if (auto* t = c->toggle()) t->setAccent (colour);
        else if (auto* ch = c->choice()) ch->setAccent (colour);
    }
}

void ParamPanel::resized()
{
    if (headerToggle != nullptr)
    {
        auto h = headerRightBounds();
        const int w = juce::jlimit (44, 60, h.getHeight());
        headerToggle->component().setBounds (h.removeFromRight (w).withSizeKeepingCentre (w, juce::jlimit (22, 30, h.getHeight())));
    }
    std::vector<juce::Component*> comps;
    for (auto& c : controls) comps.push_back (&c->component());
    if (comps.empty()) return;
    auto area = contentBounds();
    const int n = (int) comps.size();
    int cols = columns > 0 ? columns : juce::jlimit (1, n, juce::jmax (1, area.getWidth() / 96));
    // avoid a lonely last row when possible
    if (columns <= 0 && n > cols && n % cols == 1 && cols > 2) --cols;
    layoutGrid (area, comps, cols, 2, 2);
}

//==============================================================================
SourcePage::SourcePage (AntiMatrProcessor& p)
    : processor (p),
      selector ({ { "Wave", Icon::Wave, Theme::violet }, { "Dust", Icon::Dust, Theme::blue }, { "Impact", Icon::Impact, Theme::cyan },
                  { "Sample", Icon::Sample, Theme::ivory }, { "Gesture", Icon::Gesture, Theme::magenta } })
{
    addAndMakeVisible (sourcePanel);
    addAndMakeVisible (wavePanel);
    wavePanel.setCompact (true);
    sourcePanel.addAndMakeVisible (selector);
    wavePanel.addAndMakeVisible (wave);
    modeControl = std::make_unique<BoundControl> (processor.parameters(), Param::sourceMode, Theme::blue);
    sourcePanel.addAndMakeVisible (modeControl->component());

    auto* param = processor.parameters().getParameter (ParameterRegistry::get (Param::sourceSelected).id);
    selectorAttachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
    {
        selector.setSelected ((int) std::lround (v), juce::dontSendNotification);
        rebuild ((int) std::lround (v));
    });
    selector.onChange = [this] (int i) { selectorAttachment->setValueAsCompleteGesture ((float) i); };
    selectorAttachment->sendInitialUpdate();
    if (currentSource < 0) rebuild (0);

    wave.onArrow = [this] (int dir)
    {
        static const Param modeParams[] = { Param::waveTable, Param::dustMode, Param::impactMode, Param::sampleMode, Param::gestureMode };
        const Param mp = modeParams[juce::jlimit (0, 4, currentSource)];
        auto* modeParam = processor.parameters().getParameter (ParameterRegistry::get (mp).id);
        const int n = ParameterRegistry::get (mp).numChoices();
        const int cur = (int) std::lround (modeParam->convertFrom0to1 (modeParam->getValue()));
        modeParam->setValueNotifyingHost (modeParam->convertTo0to1 ((float) (((cur + dir) % n + n) % n)));
    };
    startTimerHz (30);
}

void SourcePage::rebuild (int source)
{
    if (source == currentSource) return;
    currentSource = source;
    sections.clear();
    auto add = [this] (const juce::String& title, const juce::String& subtitle, std::vector<Param> params, float weight, int cols = 0)
    {
        Section s;
        s.panel = std::make_unique<ParamPanel> (processor, title, subtitle, Theme::blue, std::move (params), cols);
        s.weight = weight;
        addAndMakeVisible (*s.panel);
        sections.push_back (std::move (s));
    };
    Param level = Param::waveLevel;
    switch (source)
    {
        case 1:
            add ("Dust", "Particle field", { Param::dustMode, Param::dustDensity, Param::dustColor, Param::dustGrain, Param::dustJitter, Param::dustPosition, Param::dustSpread, Param::dustStereo }, 0.72f, 4);
            add ("Pitch & seed", "", { Param::dustPitch, Param::dustSeed }, 0.28f, 2);
            level = Param::dustLevel;
            break;
        case 2:
            add ("Impact", "Strike field", { Param::impactMode, Param::impactHardness, Param::impactBrightness, Param::impactLength, Param::impactVelocity, Param::impactCurve, Param::impactRandom, Param::impactRate }, 1.0f, 4);
            level = Param::impactLevel;
            break;
        case 3:
            add ("Sample", "Imported matter", { Param::sampleMode, Param::sampleStart, Param::sampleEnd, Param::sampleGrain, Param::sampleSpread }, 0.62f, 5);
            add ("Pitch", "", { Param::samplePitch, Param::sampleRoot, Param::sampleKeytrack }, 0.38f, 3);
            level = Param::sampleLevel;
            break;
        case 4:
            add ("Gesture", "Living gesture", { Param::gestureMode, Param::gesturePressure, Param::gestureSpeed, Param::gestureRoughness, Param::gesturePosition, Param::gestureMotion, Param::gestureBandwidth }, 1.0f, 4);
            level = Param::gestureLevel;
            break;
        default:
            add ("Wavetable", "Fractured forms", { Param::waveTable, Param::wavePosition, Param::waveScan, Param::waveMorph, Param::waveUnison, Param::waveDetune, Param::waveSpread, Param::wavePhase, Param::wavePhaseRandom }, 0.5f, 5);
            add ("Pitch", "", { Param::waveOctave, Param::waveSemi, Param::waveFine }, 0.17f, 1);
            add ("Modulation", "Cross-modulation", { Param::waveFM, Param::wavePM, Param::waveAM, Param::waveRing, Param::waveSync, Param::waveModRatio }, 0.33f, 3);
            break;
    }
    levelControl = std::make_unique<BoundControl> (processor.parameters(), level, Theme::blue, "Level");
    sourcePanel.addAndMakeVisible (levelControl->component());
    static const juce::Colour accents[] = { Theme::violet, Theme::blue, Theme::cyan, Theme::ivory, Theme::magenta };
    wave.setAccent (accents[juce::jlimit (0, 4, source)]);
    resized();
}

void SourcePage::resized()
{
    const int pad = pagePad (*this), gap = pad;
    auto area = getLocalBounds().reduced (pad, pad / 2);
    auto left = area.removeFromLeft (juce::roundToInt ((float) area.getWidth() * 0.26f));
    area.removeFromLeft (gap);
    sourcePanel.setBounds (left);
    {
        auto c = sourcePanel.contentBounds();
        selector.setBounds (c.removeFromTop (juce::roundToInt ((float) c.getHeight() * 0.30f)));
        c.removeFromTop (gap);
        auto row = c.removeFromTop (juce::jlimit (60, 90, c.getHeight() / 4));
        if (modeControl != nullptr) modeControl->component().setBounds (row.removeFromLeft (row.getWidth() / 2));
        if (levelControl != nullptr) levelControl->component().setBounds (row);
    }
    auto top = area.removeFromTop (juce::roundToInt ((float) area.getHeight() * 0.38f));
    area.removeFromTop (gap);
    wavePanel.setBounds (top);
    wave.setBounds (wavePanel.contentBounds());

    float total = 0.0f;
    for (auto& s : sections) total += s.weight;
    for (size_t i = 0; i < sections.size(); ++i)
    {
        const bool last = i + 1 == sections.size();
        const int w = last ? area.getWidth() : juce::roundToInt ((float) (area.getWidth() - gap * (int) (sections.size() - 1)) * sections[i].weight / total);
        sections[i].panel->setBounds (area.removeFromLeft (w));
        area.removeFromLeft (gap);
    }
}

void SourcePage::timerCallback()
{
    if (! isShowing()) return;
    auto& diag = processor.diagnostics();
    const auto& vs = diag.visualSnapshots.latest();
    diag.taps[(int) Stage::Source].readLatest (tapL.data(), tapR.data(), (int) tapL.size());
    wave.setSamples (tapL.data(), (int) tapL.size());
    wave.setEnergy (juce::jlimit (0.0f, 1.0f, vs.sourceRms * 4.0f));
    selector.setEnergy (juce::jlimit (0.0f, 1.0f, vs.sourceRms * 4.0f));
    sourcePanel.setActivity (juce::jlimit (0.0f, 1.0f, vs.sourceRms * 2.0f));
    static const Param modeParams[] = { Param::waveTable, Param::dustMode, Param::impactMode, Param::sampleMode, Param::gestureMode };
    const Param mp = modeParams[juce::jlimit (0, 4, currentSource)];
    const auto choices = paramChoices (mp);
    wave.setCaption (choices[juce::jlimit (0, choices.size() - 1, paramChoice (processor.currentParamValues(), mp))]);
}

//==============================================================================
ShapePage::ShapePage (AntiMatrProcessor& p)
    : processor (p),
      matter (p, "Matter", "Turn matter into sound", Theme::cyan, { Param::shapeDensity, Param::shapeForm, Param::shapeMass, Param::shapeTension, Param::shapeDecay, Param::shapeSurface }, 3),
      materials (p, "Materials", "Blend two physical models", Theme::cyan, { Param::shapeMaterialA, Param::shapeMaterialB, Param::shapeBlend }, 3),
      topology (p, "Topology", "Node distribution & coupling", Theme::cyan, { Param::shapeTopology, Param::shapeCoupling, Param::shapeDistribution, Param::shapeSeed }, 4),
      response (p, "Response", "Excitation, pitch & output", Theme::cyan, { Param::shapeExcite, Param::shapeStrike, Param::shapeMix, Param::shapeStereo, Param::shapeKeytrack, Param::shapePitch }, 3)
{
    matter.setHeroKnobs (true);
    const Param mp[] = { Param::shapeDensity, Param::shapeForm, Param::shapeMass, Param::shapeTension, Param::shapeDecay, Param::shapeSurface };
    const juce::Colour accents[] = { Theme::cyan, Theme::blue, Theme::violet, Theme::cyan, Theme::magenta, Theme::ivory };
    for (int i = 0; i < 6; ++i) matter.setAccentFor (mp[i], accents[i]);
    for (auto* panel : { &matter, &materials, &topology, &response }) addAndMakeVisible (*panel);
    startTimerHz (20);
}

void ShapePage::resized()
{
    const int pad = pagePad (*this), gap = pad;
    auto area = getLocalBounds().reduced (pad, pad / 2);
    auto left = area.removeFromLeft (juce::roundToInt ((float) area.getWidth() * 0.46f));
    area.removeFromLeft (gap);
    matter.setBounds (left);
    auto top = area.removeFromTop (juce::roundToInt ((float) area.getHeight() * 0.33f));
    area.removeFromTop (gap);
    materials.setBounds (top);
    auto mid = area.removeFromTop (juce::roundToInt ((float) area.getHeight() * 0.5f));
    area.removeFromTop (gap);
    topology.setBounds (mid);
    response.setBounds (area);
}

void ShapePage::timerCallback()
{
    if (! isShowing()) return;
    const auto& vs = processor.diagnostics().visualSnapshots.latest();
    const Param mp[] = { Param::shapeDensity, Param::shapeForm, Param::shapeMass, Param::shapeTension, Param::shapeDecay, Param::shapeSurface };
    const float values[] = { vs.density, vs.form, vs.mass, vs.tension, vs.decay, vs.surface };
    for (int i = 0; i < 6; ++i)
        if (auto* c = matter.control (mp[i])) if (auto* k = c->knob()) k->modRing().setCurrent (values[i]);
    matter.setActivity (juce::jlimit (0.0f, 1.0f, vs.matterRms * 3.0f));
}

//==============================================================================
EvolvePage::EvolvePage (AntiMatrProcessor& p)
    : processor (p),
      operators (p, "Operators", "Movement & change", Theme::violet, { Param::evolveBend, Param::evolveMelt, Param::evolveTear, Param::evolveMagnet }, 4),
      bend (p, "Bend", "Deformation detail", Theme::violet, { Param::evolveBendPivot, Param::evolveBendRange, Param::evolveBendCurve }, 3),
      magnet (p, "Magnet", "Alignment target", Theme::violet, { Param::evolveMagnetTarget, Param::evolveCrush, Param::evolveFreeze }, 3),
      motion (p, "Motion", "Speed of change", Theme::violet, { Param::evolveSpeed, Param::evolveMotion, Param::evolveScatterSeed }, 3)
{
    for (auto* panel : { &operators, &bend, &magnet, &motion }) addAndMakeVisible (*panel);
    addAndMakeVisible (fieldPanel);
    fieldPanel.addAndMakeVisible (field);
    field.setTooltip ("Drag: gravity (x) and scatter (y). Double-click resets.");

    const juce::String names[] = { "Bend", "Melt", "Tear", "Magnet" };
    const Icon icons[] = { Icon::Bend, Icon::Melt, Icon::Tear, Icon::Magnet };
    for (int i = 0; i < 4; ++i)
    {
        cells[(size_t) i] = std::make_unique<EvolvePanel::OperatorCell> (names[i], icons[i]);
        cells[(size_t) i]->onClick = [this, i] { selectedAttachment->setValueAsCompleteGesture ((float) i); };
        operators.addAndMakeVisible (*cells[(size_t) i]);
    }
    auto& apvts = processor.parameters();
    auto* selParam = apvts.getParameter (ParameterRegistry::get (Param::evolveSelected).id);
    selectedAttachment = std::make_unique<juce::ParameterAttachment> (*selParam, [this] (float v)
    {
        const int idx = juce::jlimit (0, 3, (int) std::lround (v));
        for (int i = 0; i < 4; ++i) cells[(size_t) i]->setSelected (i == idx);
    });
    selectedAttachment->sendInitialUpdate();

    auto* gx = apvts.getParameter (ParameterRegistry::get (Param::evolveGravity).id);
    auto* gy = apvts.getParameter (ParameterRegistry::get (Param::evolveScatter).id);
    fieldX = std::make_unique<juce::ParameterAttachment> (*gx, [this] (float v) { field.setPosition (v, field.getY01(), juce::dontSendNotification); });
    fieldY = std::make_unique<juce::ParameterAttachment> (*gy, [this] (float v) { field.setPosition (field.getX01(), v, juce::dontSendNotification); });
    field.onDragStart = [this] { fieldX->beginGesture(); fieldY->beginGesture(); };
    field.onDragEnd   = [this] { fieldX->endGesture(); fieldY->endGesture(); };
    field.onChange = [this] (float x, float y) { fieldX->setValueAsPartOfGesture (x); fieldY->setValueAsPartOfGesture (y); };
    field.setDefault (ParameterRegistry::get (Param::evolveGravity).defaultValue, ParameterRegistry::get (Param::evolveScatter).defaultValue);
    fieldX->sendInitialUpdate();
    fieldY->sendInitialUpdate();
    startTimerHz (20);
}

void EvolvePage::resized()
{
    const int pad = pagePad (*this), gap = pad;
    auto area = getLocalBounds().reduced (pad, pad / 2);
    auto right = area.removeFromRight (juce::roundToInt ((float) area.getWidth() * 0.36f));
    area.removeFromRight (gap);
    fieldPanel.setBounds (right);
    field.setBounds (fieldPanel.contentBounds());

    auto top = area.removeFromTop (juce::roundToInt ((float) area.getHeight() * 0.56f));
    area.removeFromTop (gap);
    operators.setBounds (top);
    {
        auto c = operators.contentBounds();
        auto cellRow = c.removeFromTop (juce::roundToInt ((float) c.getHeight() * 0.5f));
        layoutKnobRow (cellRow, { cells[0].get(), cells[1].get(), cells[2].get(), cells[3].get() });
        c.removeFromTop (gap / 2);
        std::vector<juce::Component*> knobs;
        for (auto param : { Param::evolveBend, Param::evolveMelt, Param::evolveTear, Param::evolveMagnet })
            if (auto* ctl = operators.control (param)) knobs.push_back (&ctl->component());
        layoutGrid (c, knobs, 4);
    }
    const int w = (area.getWidth() - gap * 2) / 3;
    bend.setBounds (area.removeFromLeft (w));
    area.removeFromLeft (gap);
    magnet.setBounds (area.removeFromLeft (w));
    area.removeFromLeft (gap);
    motion.setBounds (area);
}

void EvolvePage::timerCallback()
{
    if (! isShowing()) return;
    const auto values = processor.currentParamValues();
    const Param ops[] = { Param::evolveBend, Param::evolveMelt, Param::evolveTear, Param::evolveMagnet };
    float total = 0.0f;
    for (int i = 0; i < 4; ++i) { const float v = paramValue (values, ops[i]); cells[(size_t) i]->setAmount (v); total += v; }
    operators.setActivity (juce::jlimit (0.0f, 1.0f, total * 0.5f));
    const auto& vs = processor.diagnostics().visualSnapshots.latest();
    field.setEnergy (juce::jlimit (0.0f, 1.0f, vs.rmsL * 4.0f));
}

//==============================================================================
FracturePage::FracturePage (AntiMatrProcessor& p)
    : processor (p),
      engine (p, "Fracture", "Break into new realities", Theme::magenta, { Param::fractureMode, Param::fractureFragments, Param::fractureMix }, 3),
      sequencer (p, "Sequencer", "Fragment steps", Theme::magenta, { Param::fractureSteps, Param::fractureRate, Param::fractureSync, Param::fractureDivision, Param::fractureSwing, Param::fractureDirection, Param::fractureProbability, Param::fractureSeed, Param::fractureRetrig }, 9),
      spectral (p, "Spectral", "Amount, motion & tone", Theme::magenta, { Param::fractureAmount, Param::fractureSpread, Param::fractureSequence, Param::fractureRandom, Param::fractureFeedback, Param::fracturePitch, Param::fractureDelay, Param::fractureDecay, Param::fractureTone, Param::fractureEvolve }, 5)
{
    engine.setHeaderToggle (Param::fractureOn);
    for (auto* panel : { &engine, &sequencer, &spectral }) addAndMakeVisible (*panel);
    engine.addAndMakeVisible (spectrum);
    sequencer.addAndMakeVisible (steps);
    steps.setTooltip ("Drag to draw the fragment pattern. Right-click for pattern tools.");
    const juce::Colour accents[] = { Theme::magenta, Theme::cyan, Theme::violet, Theme::ivory, Theme::magenta, Theme::cyan, Theme::violet, Theme::ivory, Theme::magenta, Theme::cyan };
    const Param sp[] = { Param::fractureAmount, Param::fractureSpread, Param::fractureSequence, Param::fractureRandom, Param::fractureFeedback, Param::fracturePitch, Param::fractureDelay, Param::fractureDecay, Param::fractureTone, Param::fractureEvolve };
    for (int i = 0; i < 10; ++i) spectral.setAccentFor (sp[i], accents[i]);

    auto* stepsParam = processor.parameters().getParameter (ParameterRegistry::get (Param::fractureSteps).id);
    stepsAttachment = std::make_unique<juce::ParameterAttachment> (*stepsParam, [this] (float v) { steps.setNumSteps ((int) std::lround (v)); });
    stepsAttachment->sendInitialUpdate();
    startTimerHz (30);
}

void FracturePage::resized()
{
    const int pad = pagePad (*this), gap = pad;
    auto area = getLocalBounds().reduced (pad, pad / 2);
    auto top = area.removeFromTop (juce::roundToInt ((float) area.getHeight() * 0.40f));
    area.removeFromTop (gap);
    engine.setBounds (top);
    {
        auto c = engine.contentBounds();
        auto controls = c.removeFromRight (juce::jlimit (240, 360, c.getWidth() / 4));
        c.removeFromRight (gap);
        spectrum.setBounds (c);
        std::vector<juce::Component*> comps;
        for (auto param : { Param::fractureMode, Param::fractureFragments, Param::fractureMix })
            if (auto* ctl = engine.control (param)) comps.push_back (&ctl->component());
        layoutGrid (controls, comps, 1, 0, 4);
    }
    auto mid = area.removeFromTop (juce::roundToInt ((float) area.getHeight() * 0.5f));
    area.removeFromTop (gap);
    sequencer.setBounds (mid);
    {
        auto c = sequencer.contentBounds();
        auto controls = c.removeFromRight (juce::jlimit (300, 520, c.getWidth() / 2));
        c.removeFromRight (gap);
        steps.setBounds (c);
        std::vector<juce::Component*> comps;
        for (auto param : { Param::fractureSteps, Param::fractureRate, Param::fractureSync, Param::fractureDivision, Param::fractureSwing, Param::fractureDirection, Param::fractureProbability, Param::fractureSeed, Param::fractureRetrig })
            if (auto* ctl = sequencer.control (param)) comps.push_back (&ctl->component());
        layoutGrid (controls, comps, 5, 2, 2);
    }
    spectral.setBounds (area);
}

void FracturePage::timerCallback()
{
    if (! isShowing()) return;
    auto& diag = processor.diagnostics();
    const auto& vs = diag.visualSnapshots.latest();
    const auto stage = vs.fractureOn ? Stage::PostFracture : Stage::PostMatter;
    diag.taps[(int) stage].readLatest (tapL.data(), tapR.data(), SpectrumAnalyzer::kSize);
    for (int i = 0; i < SpectrumAnalyzer::kSize; ++i) tapL[(size_t) i] = 0.5f * (tapL[(size_t) i] + tapR[(size_t) i]);
    analyzer.compute (tapL.data(), processor.engine().sampleRate(), bands.data(), AMSpectrumView::kBands);
    const auto values = processor.currentParamValues();
    spectrum.setFragmentCount (8 << juce::jlimit (0, 2, paramChoice (values, Param::fractureFragments)));
    spectrum.setActivity (vs.fractureOn ? vs.fractureActivity : 0.0f);
    spectrum.setMagnitudes (bands.data(), AMSpectrumView::kBands);
    engine.setActivity (vs.fractureOn ? vs.fractureActivity * 0.8f : 0.0f);

    // Playhead: advanced from the engine's sample clock at the sequencer rate
    // (the division at 120 BPM when synced) until the engine publishes a step index.
    const double sr = juce::jmax (1.0, processor.engine().sampleRate());
    const uint64_t now = vs.sampleTime;
    if (lastSampleTime != 0 && now > lastSampleTime)
    {
        double stepsPerSecond = paramValue (values, Param::fractureRate);
        if (paramBool (values, Param::fractureSync))
        {
            static const double beats[] = { 4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0, 1.5, 0.75, 0.375 };
            stepsPerSecond = 2.0 / beats[juce::jlimit (0, 11, paramChoice (values, Param::fractureDivision))];
        }
        playheadPhase += (double) (now - lastSampleTime) / sr * stepsPerSecond;
    }
    lastSampleTime = now;
    const int n = juce::jmax (1, steps.getNumSteps());
    steps.setPlayhead (vs.fractureOn && vs.activeVoices > 0 ? (float) std::fmod (playheadPhase, (double) n) : -1.0f);
}

//==============================================================================
SpacePage::SpacePage (AntiMatrProcessor& p) : processor (p)
{
    addAndMakeVisible (spacePanel);
    spacePanel.addAndMakeVisible (picker);
    picker.setArtOnly (true);
    auto& apvts = processor.parameters();
    auto* typeParam = apvts.getParameter (ParameterRegistry::get (Param::spaceType).id);
    typeAttachment = std::make_unique<juce::ParameterAttachment> (*typeParam, [this] (float v) { picker.type = (int) std::lround (v); picker.repaint(); });
    picker.onArrow = [this] (int dir) { const int n = SpaceArt::kNumTypes; typeAttachment->setValueAsCompleteGesture ((float) (((picker.type + dir) % n + n) % n)); };
    picker.onSelect = [this] (int i) { typeAttachment->setValueAsCompleteGesture ((float) i); };
    typeAttachment->sendInitialUpdate();

    const juce::Colour accents[] = { Theme::magenta, Theme::cyan, Theme::violet, Theme::ivory };
    const Param mp[] = { Param::spaceMix, Param::spaceSize, Param::spaceTone, Param::spaceFeedback };
    for (int i = 0; i < 4; ++i)
    {
        macros.push_back (std::make_unique<BoundControl> (apvts, mp[i], accents[i]));
        spacePanel.addAndMakeVisible (macros.back()->component());
    }

    auto module = [this] (const juce::String& title, const juce::String& subtitle, std::optional<Param> on, std::vector<Param> params)
    {
        auto m = std::make_unique<ParamPanel> (processor, title, subtitle, Theme::ivory, std::move (params));
        m->setCompact (true);
        if (on.has_value()) m->setHeaderToggle (*on);
        addAndMakeVisible (*m);
        modules.push_back (std::move (m));
    };
    module ("Distortion", "", Param::spaceDistOn,    { Param::spaceDistMode, Param::spaceDistDrive, Param::spaceDistMix });
    module ("Chorus", "",     Param::spaceChorusOn,  { Param::spaceChorusRate, Param::spaceChorusDepth, Param::spaceChorusMix });
    module ("Delay", "",      Param::spaceDelayOn,   { Param::spaceDelayTime, Param::spaceDelaySync, Param::spaceDelayFeedback, Param::spaceDelayTone, Param::spaceDelayMix });
    module ("Granular", "",   Param::spaceGrainOn,   { Param::spaceGrainSize, Param::spaceGrainDensity, Param::spaceGrainPitch, Param::spaceGrainMix });
    module ("Shift", "",      Param::spaceShiftOn,   { Param::spaceShiftAmount, Param::spaceShiftMix });
    module ("Diffusion", "",  Param::spaceDiffuseOn, { Param::spaceDiffuseAmount });
    module ("Reverb", "",     Param::spaceReverbOn,  { Param::spaceReverbSize, Param::spaceReverbDecay, Param::spaceReverbDamp, Param::spaceReverbPredelay, Param::spaceReverbMod, Param::spaceReverbMix });
    module ("EQ", "",         std::nullopt,          { Param::spaceEqLow, Param::spaceEqMid, Param::spaceEqHigh });
    module ("Dynamics", "",   Param::spaceCompOn,    { Param::spaceCompAmount, Param::spaceLimiterOn });
    startTimerHz (24);
}

void SpacePage::resized()
{
    const int pad = pagePad (*this), gap = pad;
    auto area = getLocalBounds().reduced (pad, pad / 2);
    auto left = area.removeFromLeft (juce::roundToInt ((float) area.getWidth() * 0.3f));
    area.removeFromLeft (gap);
    spacePanel.setBounds (left);
    {
        auto c = spacePanel.contentBounds();
        auto knobArea = c.removeFromBottom (juce::jlimit (70, 110, c.getHeight() / 4));
        c.removeFromBottom (gap);
        picker.setBounds (c);
        std::vector<juce::Component*> comps;
        for (auto& m : macros) comps.push_back (&m->component());
        layoutGrid (knobArea, comps, 4);
    }
    // FX rack: 3 columns, rows weighted by the number of controls
    const int cols = 3;
    const int rows = ((int) modules.size() + cols - 1) / cols;
    const int cellW = (area.getWidth() - gap * (cols - 1)) / cols;
    const int cellH = (area.getHeight() - gap * (rows - 1)) / rows;
    for (int i = 0; i < (int) modules.size(); ++i)
    {
        const int r = i / cols, c = i % cols;
        modules[(size_t) i]->setBounds (area.getX() + c * (cellW + gap), area.getY() + r * (cellH + gap), cellW, cellH);
    }
}

void SpacePage::timerCallback()
{
    if (! isShowing()) return;
    picker.phase += 1.0f / 24.0f;
    const auto& vs = processor.diagnostics().visualSnapshots.latest();
    picker.activity = juce::jlimit (0.0f, 1.0f, vs.rmsL * 3.0f);
    picker.repaint();
    spacePanel.setActivity (vs.spaceActivity * 0.5f);
}

//==============================================================================
ModPage::ModPage (AntiMatrProcessor& p) : processor (p)
{
    addAndMakeVisible (tabs);
    tabs.setStyle (AMTab::Style::Strip);
    tabs.onChange = [this] (int i) { showTab (i); };

    auto panel = [this] (std::vector<std::unique_ptr<ParamPanel>>& list, const juce::String& title, const juce::String& subtitle, std::vector<Param> params, int cols = 0)
    {
        auto pp = std::make_unique<ParamPanel> (processor, title, subtitle, Theme::amber, std::move (params), cols);
        addChildComponent (*pp);
        list.push_back (std::move (pp));
    };
    tabPanels.resize (5);
    panel (tabPanels[0], "LFO 1", "Low frequency oscillator", { Param::lfo1Rate, Param::lfo1Shape, Param::lfo1Sync, Param::lfo1Division, Param::lfo1Phase, Param::lfo1Symmetry, Param::lfo1Depth, Param::lfo1Retrig, Param::lfo1Fade }, 5);
    panel (tabPanels[0], "LFO 2", "", { Param::lfo2Rate, Param::lfo2Shape, Param::lfo2Sync, Param::lfo2Division, Param::lfo2Phase, Param::lfo2Symmetry, Param::lfo2Depth, Param::lfo2Retrig, Param::lfo2Fade }, 5);
    panel (tabPanels[0], "LFO 3", "", { Param::lfo3Rate, Param::lfo3Shape, Param::lfo3Sync, Param::lfo3Division, Param::lfo3Phase, Param::lfo3Symmetry, Param::lfo3Depth, Param::lfo3Retrig, Param::lfo3Fade }, 5);
    panel (tabPanels[0], "LFO 4", "", { Param::lfo4Rate, Param::lfo4Shape, Param::lfo4Sync, Param::lfo4Division, Param::lfo4Phase, Param::lfo4Symmetry, Param::lfo4Depth, Param::lfo4Retrig, Param::lfo4Fade }, 5);
    panel (tabPanels[1], "Envelope 1", "Modulation envelope", { Param::env1Attack, Param::env1Decay, Param::env1Sustain, Param::env1Release, Param::env1Curve, Param::env1Loop }, 6);
    panel (tabPanels[1], "Envelope 2", "", { Param::env2Attack, Param::env2Decay, Param::env2Sustain, Param::env2Release, Param::env2Curve, Param::env2Loop }, 6);
    panel (tabPanels[1], "Envelope 3", "", { Param::env3Attack, Param::env3Decay, Param::env3Sustain, Param::env3Release, Param::env3Curve, Param::env3Loop }, 6);
    panel (tabPanels[1], "Envelope 4", "", { Param::env4Attack, Param::env4Decay, Param::env4Sustain, Param::env4Release, Param::env4Curve, Param::env4Loop }, 6);
    panel (tabPanels[2], "Chaos 1", "Unstable generator", { Param::chaos1Type, Param::chaos1Rate, Param::chaos1Depth, Param::chaos1Stability, Param::chaos1Symmetry, Param::chaos1Seed }, 6);
    panel (tabPanels[2], "Chaos 2", "", { Param::chaos2Type, Param::chaos2Rate, Param::chaos2Depth, Param::chaos2Stability, Param::chaos2Symmetry, Param::chaos2Seed }, 6);
    panel (tabPanels[2], "Chaos 3", "", { Param::chaos3Type, Param::chaos3Rate, Param::chaos3Depth, Param::chaos3Stability, Param::chaos3Symmetry, Param::chaos3Seed }, 6);
    panel (tabPanels[2], "Chaos 4", "", { Param::chaos4Type, Param::chaos4Rate, Param::chaos4Depth, Param::chaos4Stability, Param::chaos4Symmetry, Param::chaos4Seed }, 6);
    panel (tabPanels[3], "Macros", "Eight performance controls", { Param::macro1, Param::macro2, Param::macro3, Param::macro4, Param::macro5, Param::macro6, Param::macro7, Param::macro8 }, 4);
    tabPanels[3].back()->setHeroKnobs (true);
    panel (tabPanels[4], "Amp", "Amplitude envelope", { Param::ampAttack, Param::ampDecay, Param::ampSustain, Param::ampRelease, Param::ampCurve, Param::ampVelocity }, 6);
    panel (tabPanels[4], "Master", "Voices, tuning & output", { Param::masterGain, Param::masterVoices, Param::masterQuality, Param::masterMode, Param::masterGlide, Param::masterBendRange, Param::masterTranspose, Param::masterFine }, 4);
    showTab (0);
}

void ModPage::showTab (int index)
{
    current = juce::jlimit (0, (int) tabPanels.size() - 1, index);
    for (int t = 0; t < (int) tabPanels.size(); ++t)
        for (auto& pp : tabPanels[(size_t) t]) pp->setVisible (t == current);
    resized();
}

void ModPage::resized()
{
    const int pad = pagePad (*this), gap = pad;
    auto area = getLocalBounds().reduced (pad, pad / 2);
    tabs.setBounds (area.removeFromTop (juce::jlimit (28, 38, area.getHeight() / 16)).withSizeKeepingCentre (juce::jmin (area.getWidth(), 640), juce::jlimit (28, 38, area.getHeight() / 16)));
    area.removeFromTop (gap);
    auto& list = tabPanels[(size_t) current];
    if (list.empty()) return;
    if (list.size() == 1) { list[0]->setBounds (area.withSizeKeepingCentre (juce::jmin (area.getWidth(), 900), juce::jmin (area.getHeight(), 360))); return; }
    if (list.size() == 2)
    {
        auto top = area.removeFromTop (juce::roundToInt ((float) area.getHeight() * 0.5f) - gap / 2);
        area.removeFromTop (gap);
        list[0]->setBounds (top);
        list[1]->setBounds (area);
        return;
    }
    const int cols = 2, rows = ((int) list.size() + 1) / 2;
    const int cellW = (area.getWidth() - gap) / cols, cellH = (area.getHeight() - gap * (rows - 1)) / rows;
    for (int i = 0; i < (int) list.size(); ++i)
        list[(size_t) i]->setBounds (area.getX() + (i % cols) * (cellW + gap), area.getY() + (i / cols) * (cellH + gap), cellW, cellH);
}

} // namespace am::ui

#include "Panels.h"

namespace am::ui
{

//==============================================================================
SourcePanel::SourcePanel (AntiMatrProcessor& p)
    : AMPanel ("Source", "Choose your energy", Theme::blue),
      processor (p),
      selector ({ { "Wave", Icon::Wave, Theme::violet }, { "Dust", Icon::Dust, Theme::blue },
                  { "Impact", Icon::Impact, Theme::cyan }, { "Sample", Icon::Sample, Theme::ivory } })
{
    addAndMakeVisible (selector);
    addAndMakeVisible (wave);
    selector.setTooltip ("Source: what creates the energy");

    auto* param = processor.parameters().getParameter (ParameterRegistry::get (Param::sourceSelected).id);
    selectorAttachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
    {
        selector.setSelected ((int) std::lround (v), juce::dontSendNotification);
        rebuildKnobs ((int) std::lround (v));
    });
    selector.onChange = [this] (int i) { selectorAttachment->setValueAsCompleteGesture ((float) i); };
    selectorAttachment->sendInitialUpdate();
    if (currentSource < 0) rebuildKnobs (0);

    wave.onArrow = [this] (int dir)
    {
        static const Param modeParams[] = { Param::waveTable, Param::dustMode, Param::impactMode, Param::sampleMode, Param::gestureMode };
        const Param mp = modeParams[juce::jlimit (0, 4, currentSource)];
        auto* modeParam = processor.parameters().getParameter (ParameterRegistry::get (mp).id);
        const int n = ParameterRegistry::get (mp).numChoices();
        const int cur = (int) std::lround (modeParam->convertFrom0to1 (modeParam->getValue()));
        const int next = ((cur + dir) % n + n) % n;
        modeParam->setValueNotifyingHost (modeParam->convertTo0to1 ((float) next));
    };

    startTimerHz (30);
}

void SourcePanel::rebuildKnobs (int sourceIndex)
{
    if (sourceIndex == currentSource) return;
    currentSource = sourceIndex;
    knobs.clear();
    auto& apvts = processor.parameters();
    const juce::Colour accents[] = { Theme::blue, Theme::blue, Theme::cyan, Theme::violet };
    int idx = 0;
    auto add = [&] (Param p, const juce::String& label = {}) { knobs.push_back (std::make_unique<BoundKnob> (apvts, p, accents[idx++ % 4], label)); addAndMakeVisible (knobs.back()->knob); };
    switch (sourceIndex)
    {
        case 1:  add (Param::dustDensity); add (Param::dustColor); add (Param::dustGrain); add (Param::dustSpread); break;
        case 2:  add (Param::impactHardness); add (Param::impactBrightness); add (Param::impactLength); add (Param::impactRandom); break;
        case 3:  add (Param::sampleStart); add (Param::sampleEnd); add (Param::samplePitch); add (Param::sampleSpread); break;
        case 4:  add (Param::gesturePressure); add (Param::gestureSpeed); add (Param::gestureRoughness); add (Param::gesturePosition); break;
        default: add (Param::wavePosition); add (Param::waveScan); add (Param::waveDetune); add (Param::waveSpread); break;
    }
    static const juce::Colour waveAccents[] = { Theme::violet, Theme::blue, Theme::cyan, Theme::ivory, Theme::magenta };
    wave.setAccent (waveAccents[juce::jlimit (0, 4, sourceIndex)]);
    resized();
}

void SourcePanel::resized()
{
    auto area = contentBounds();
    const int gap = juce::jmax (4, area.getHeight() / 40);
    auto selectorArea = area.removeFromTop (juce::roundToInt ((float) area.getHeight() * 0.30f));
    area.removeFromTop (gap);
    auto knobArea = area.removeFromBottom (juce::roundToInt ((float) area.getHeight() * 0.42f));
    area.removeFromBottom (gap);
    selector.setBounds (selectorArea);
    wave.setBounds (area);
    std::vector<juce::Component*> comps;
    for (auto& k : knobs) comps.push_back (&k->knob);
    layoutGrid (knobArea, comps, 4);
}

void SourcePanel::timerCallback()
{
    if (! isShowing()) return;
    auto& diag = processor.diagnostics();
    const auto& vs = diag.visualSnapshots.latest();
    diag.taps[(int) Stage::Source].readLatest (tapL.data(), tapR.data(), (int) tapL.size());
    wave.setSamples (tapL.data(), (int) tapL.size());
    wave.setEnergy (juce::jlimit (0.0f, 1.0f, vs.sourceRms * 4.0f));
    selector.setEnergy (juce::jlimit (0.0f, 1.0f, vs.sourceRms * 4.0f));
    setActivity (juce::jlimit (0.0f, 1.0f, vs.sourceRms * 2.0f));

    // caption: the selected source's mode / table name (real parameter state)
    static const Param modeParams[] = { Param::waveTable, Param::dustMode, Param::impactMode, Param::sampleMode, Param::gestureMode };
    const Param mp = modeParams[juce::jlimit (0, 4, currentSource)];
    const int mode = paramChoice (processor.currentParamValues(), mp);
    const auto choices = paramChoices (mp);
    wave.setCaption (choices[juce::jlimit (0, choices.size() - 1, mode)]);
}

//==============================================================================
ShapePanel::ShapePanel (AntiMatrProcessor& p)
    : AMPanel ("Shape", "Turn matter into sound", Theme::cyan), processor (p)
{
    auto& apvts = processor.parameters();
    const juce::Colour accents[] = { Theme::cyan, Theme::blue, Theme::violet, Theme::cyan, Theme::magenta, Theme::ivory };
    const Param simple[] = { Param::shapeDensity, Param::shapeForm, Param::shapeMass, Param::shapeTension, Param::shapeDecay, Param::shapeSurface };
    for (int i = 0; i < 6; ++i)
    {
        simpleKnobs.push_back (std::make_unique<BoundKnob> (apvts, simple[i], accents[i]));
        simpleKnobs.back()->knob.setHero (true);
        addAndMakeVisible (simpleKnobs.back()->knob);
    }
    const Param adv[] = { Param::shapeMaterialA, Param::shapeMaterialB, Param::shapeBlend, Param::shapeTopology,
                          Param::shapeCoupling, Param::shapeDistribution, Param::shapeExcite, Param::shapeStrike,
                          Param::shapeStereo, Param::shapeMix, Param::shapeKeytrack, Param::shapePitch };
    for (int i = 0; i < 12; ++i)
    {
        advancedControls.push_back (std::make_unique<BoundControl> (apvts, adv[i], accents[i % 6]));
        addChildComponent (advancedControls.back()->component());
    }
    addAndMakeVisible (mode);
    mode.setTooltip ("SIMPLE: the six Matter macros. ADVANCED: materials, topology, coupling.");
    mode.onChange = [this] (int i) { setAdvanced (i == 1); };
    startTimerHz (20);
}

void ShapePanel::setAdvanced (bool a)
{
    advanced = a;
    for (auto& k : simpleKnobs) k->knob.setVisible (! advanced);
    for (auto& c : advancedControls) c->component().setVisible (advanced);
    resized();
}

void ShapePanel::resized()
{
    auto area = contentBounds();
    auto modeArea = area.removeFromTop (juce::jlimit (26, 36, area.getHeight() / 10));
    mode.setBounds (modeArea.reduced (juce::jmax (4, area.getWidth() / 24), 0));
    area.removeFromTop (juce::jmax (4, area.getHeight() / 26));
    if (! advanced)
    {
        std::vector<juce::Component*> c;
        for (auto& k : simpleKnobs) c.push_back (&k->knob);
        layoutGrid (area, c, 3);
    }
    else
    {
        std::vector<juce::Component*> c;
        for (auto& k : advancedControls) c.push_back (&k->component());
        layoutGrid (area, c, 4, 4, 4);
    }
}

void ShapePanel::timerCallback()
{
    if (! isShowing() || advanced) return;
    // The ring shows where the engine's effective value sits relative to the knob (smoothing / modulation).
    const auto& vs = processor.diagnostics().visualSnapshots.latest();
    const float values[] = { vs.density, vs.form, vs.mass, vs.tension, vs.decay, vs.surface };
    for (int i = 0; i < 6; ++i)
        simpleKnobs[(size_t) i]->knob.modRing().setCurrent (values[i]);
}

//==============================================================================
EvolvePanel::OperatorCell::OperatorCell (const juce::String& label, Icon icon) : name (label.toUpperCase()), glyph (icon)
{
    setWantsKeyboardFocus (false);
}

void EvolvePanel::OperatorCell::setSelected (bool on)
{
    if (on == selected) return;
    selected = on;
    if (isShowing()) anim.animate (lit, on ? 1.0f : 0.0f); else lit.snap (on ? 1.0f : 0.0f);
    repaint();
}

void EvolvePanel::OperatorCell::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float on = lit.value;
    const float hv = hover.value * (1.0f - on);
    const float labelH = juce::jlimit (10.0f, 16.0f, b.getHeight() * 0.24f);
    auto tile = b.reduced (b.getWidth() * 0.06f, 2.0f);
    const float corner = juce::jmin (10.0f, tile.getWidth() * 0.12f);
    auto iconArea = tile.withTrimmedBottom (labelH + 6.0f).reduced (tile.getWidth() * 0.22f, tile.getHeight() * 0.12f);
    const float d = juce::jmin (iconArea.getWidth(), iconArea.getHeight());
    iconArea = iconArea.withSizeKeepingCentre (d, d);

    // tile
    if (on > 0.02f)
    {
        draw::glowRoundedRect (g, tile, corner, Theme::violet, 12.0f, 0.5f * on);
        juce::ColourGradient grad (Theme::violet.withAlpha (0.20f * on), tile.getX(), tile.getY(), Theme::violet.withAlpha (0.06f * on), tile.getX(), tile.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (tile, corner);
        g.setColour (Theme::violet.withAlpha (0.45f * on));
        g.drawRoundedRectangle (tile.reduced (0.5f), corner, 1.0f);
    }
    if (hv > 0.02f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.035f * hv));
        g.fillRoundedRectangle (tile, corner);
        g.setColour (Theme::border.withAlpha (0.1f * hv));
        g.drawRoundedRectangle (tile.reduced (0.5f), corner, 1.0f);
    }

    // icon with a glow proportional to the operator amount (real value) and selection
    const float glow = 0.35f * amount + 0.45f * on + 0.15f * hv;
    if (glow > 0.03f) draw::glowEllipse (g, iconArea, Theme::violet, d * 0.45f, glow);
    const auto col = Theme::textSecondary.interpolatedWith (Theme::textPrimary, juce::jmax (on, hv * 0.6f, amount * 0.6f));
    Icons::draw (g, glyph, iconArea, col, 0.85f);

    // amount bar beneath the icon
    {
        auto bar = juce::Rectangle<float> (tile.getX() + tile.getWidth() * 0.25f, iconArea.getBottom() + 3.0f, tile.getWidth() * 0.5f, 2.0f);
        g.setColour (Theme::knobTrack);
        g.fillRoundedRectangle (bar, 1.0f);
        if (amount > 0.01f)
        {
            auto litBar = bar.withWidth (bar.getWidth() * amount);
            draw::glowRoundedRect (g, litBar, 1.0f, Theme::violet, 4.0f, 0.5f);
            g.setColour (Theme::violet.withAlpha (0.9f));
            g.fillRoundedRectangle (litBar, 1.0f);
        }
    }

    const float h = juce::jlimit (8.5f, 11.5f, labelH * 0.66f);
    draw::trackedText (g, name, b.withTop (b.getBottom() - labelH), juce::Justification::centredTop,
                       on > 0.5f ? Theme::labelFontStrong (h) : Theme::labelFont (h), col);
}

Param EvolvePanel::operatorParam (int index)
{
    switch (index) { case 1: return Param::evolveMelt; case 2: return Param::evolveTear; case 3: return Param::evolveMagnet; default: return Param::evolveBend; }
}

EvolvePanel::EvolvePanel (AntiMatrProcessor& p)
    : AMPanel ("Evolve", "Movement & change", Theme::violet), processor (p)
{
    const juce::String names[] = { "Bend", "Melt", "Tear", "Magnet" };
    const Icon icons[] = { Icon::Bend, Icon::Melt, Icon::Tear, Icon::Magnet };
    const char* tips[] = { "BEND: deform the partial structure", "MELT: diffuse and blur the matter", "TEAR: separate the object into parts", "MAGNET: align partials to a target" };
    for (int i = 0; i < 4; ++i)
    {
        cells[(size_t) i] = std::make_unique<OperatorCell> (names[i], icons[i]);
        cells[(size_t) i]->onClick = [this, i] { selectOperator (i, false); };
        cells[(size_t) i]->setTooltip (tips[i]);
        addAndMakeVisible (*cells[(size_t) i]);
    }
    addAndMakeVisible (amount);
    addAndMakeVisible (speed);
    speed.setTooltip (paramTooltip (Param::evolveSpeed));
    auto& apvts = processor.parameters();
    speedAttachment = std::make_unique<SliderAttachment> (apvts, ParameterRegistry::get (Param::evolveSpeed).id, speed);
    speed.setDoubleClickReturnValue (true, ParameterRegistry::get (Param::evolveSpeed).defaultValue);

    auto* selParam = apvts.getParameter (ParameterRegistry::get (Param::evolveSelected).id);
    selectedAttachment = std::make_unique<juce::ParameterAttachment> (*selParam, [this] (float v) { selectOperator ((int) std::lround (v), true); });
    selectedAttachment->sendInitialUpdate();
    if (amountAttachment == nullptr) selectOperator (0, true);
    startTimerHz (20);
}

void EvolvePanel::selectOperator (int index, bool fromParameter)
{
    index = juce::jlimit (0, 3, index);
    if (! fromParameter)
    {
        selectedAttachment->setValueAsCompleteGesture ((float) index);
        return; // the attachment callback re-enters with fromParameter = true
    }
    selectedOperator = index;
    for (int i = 0; i < 4; ++i) cells[(size_t) i]->setSelected (i == index);
    amountAttachment.reset();
    amountAttachment = std::make_unique<SliderAttachment> (processor.parameters(), ParameterRegistry::get (operatorParam (index)).id, amount);
    amount.setDoubleClickReturnValue (true, ParameterRegistry::get (operatorParam (index)).defaultValue);
    amount.setTooltip (paramTooltip (operatorParam (index)));
    amount.setLabel ("Amount");
}

void EvolvePanel::timerCallback()
{
    if (! isShowing()) return;
    const auto values = processor.currentParamValues();
    float total = 0.0f;
    for (int i = 0; i < 4; ++i)
    {
        const float v = paramValue (values, operatorParam (i));
        cells[(size_t) i]->setAmount (v);
        total += v;
    }
    setActivity (juce::jlimit (0.0f, 1.0f, total * 0.5f));
}

void EvolvePanel::resized()
{
    auto area = contentBounds();
    const int gap = juce::jmax (4, area.getHeight() / 30);
    auto cellArea = area.removeFromTop (juce::roundToInt ((float) area.getHeight() * 0.52f));
    layoutKnobRow (cellArea, { cells[0].get(), cells[1].get(), cells[2].get(), cells[3].get() });
    area.removeFromTop (gap);
    const int sliderH = juce::jmax (18, area.getHeight() / 2 - gap / 2);
    amount.setBounds (area.removeFromTop (sliderH));
    area.removeFromTop (gap);
    speed.setBounds (area.removeFromTop (sliderH));
}

//==============================================================================
FracturePanel::FracturePanel (AntiMatrProcessor& p)
    : AMPanel ("Fracture", "Break into new realities", Theme::magenta), processor (p)
{
    addAndMakeVisible (onOff);
    addAndMakeVisible (spectrum);
    onOff.setTooltip ("Fracture on / off");
    auto& apvts = processor.parameters();
    auto* onParam = apvts.getParameter (ParameterRegistry::get (Param::fractureOn).id);
    onAttachment = std::make_unique<juce::ParameterAttachment> (*onParam, [this] (float v) { onOff.setSelected (v >= 0.5f ? 1 : 0, juce::dontSendNotification); });
    onOff.onChange = [this] (int i) { onAttachment->setValueAsCompleteGesture (i == 1 ? 1.0f : 0.0f); };
    onAttachment->sendInitialUpdate();

    const juce::Colour accents[] = { Theme::magenta, Theme::cyan, Theme::violet, Theme::ivory };
    const Param params[] = { Param::fractureAmount, Param::fractureSpread, Param::fractureSequence, Param::fractureRandom };
    for (int i = 0; i < 4; ++i)
    {
        knobs.push_back (std::make_unique<BoundKnob> (apvts, params[i], accents[i]));
        addAndMakeVisible (knobs.back()->knob);
    }
    startTimerHz (30);
}

void FracturePanel::resized()
{
    auto header = headerRightBounds();
    const int w = juce::jlimit (84, 130, header.getWidth() / 2);
    onOff.setBounds (header.removeFromRight (w).withSizeKeepingCentre (w, juce::jlimit (22, 30, header.getHeight() - 8)));

    auto area = contentBounds();
    const int gap = juce::jmax (4, area.getHeight() / 30);
    auto knobArea = area.removeFromBottom (juce::roundToInt ((float) area.getHeight() * 0.42f));
    area.removeFromBottom (gap);
    spectrum.setBounds (area);
    layoutKnobRow (knobArea, { &knobs[0]->knob, &knobs[1]->knob, &knobs[2]->knob, &knobs[3]->knob });
}

void FracturePanel::timerCallback()
{
    if (! isShowing()) return;
    auto& diag = processor.diagnostics();
    const auto& vs = diag.visualSnapshots.latest();
    const auto stage = vs.fractureOn ? Stage::PostFracture : Stage::PostMatter;
    diag.taps[(int) stage].readLatest (tapL.data(), tapR.data(), SpectrumAnalyzer::kSize);
    for (int i = 0; i < SpectrumAnalyzer::kSize; ++i) tapL[(size_t) i] = 0.5f * (tapL[(size_t) i] + tapR[(size_t) i]);
    analyzer.compute (tapL.data(), processor.engine().sampleRate(), bands.data(), AMSpectrumView::kBands);
    spectrum.setFragmentCount (8 << juce::jlimit (0, 2, paramChoice (processor.currentParamValues(), Param::fractureFragments)));
    spectrum.setActivity (vs.fractureOn ? vs.fractureActivity : 0.0f);
    spectrum.setMagnitudes (bands.data(), AMSpectrumView::kBands);
    setActivity (vs.fractureOn ? vs.fractureActivity * 0.8f : 0.0f);
}

//==============================================================================
SpacePanel::SpacePicker::SpacePicker() { setWantsKeyboardFocus (false); }

juce::Rectangle<float> SpacePanel::SpacePicker::nameBounds() const
{
    const auto b = getLocalBounds().toFloat();
    if (artOnly) return juce::Rectangle<float> (juce::jmin (b.getWidth() - 20.0f, 220.0f), 34.0f).withCentre ({ b.getCentreX(), b.getBottom() - 30.0f });
    auto left = b.withWidth (b.getWidth() * 0.6f).reduced (10.0f, 0.0f);
    return left.withSizeKeepingCentre (left.getWidth(), juce::jlimit (26.0f, 40.0f, b.getHeight() * 0.42f));
}

int SpacePanel::SpacePicker::zoneAt (juce::Point<int> p) const
{
    const auto n = nameBounds();
    if (! n.contains (p.toFloat())) return 0;
    const float zone = juce::jmin (n.getWidth() * 0.25f, 34.0f);
    if (p.x < n.getX() + zone) return -1;
    if (p.x > n.getRight() - zone) return 1;
    return 2;
}

void SpacePanel::SpacePicker::mouseMove (const juce::MouseEvent& e) { const int z = zoneAt (e.getPosition()); if (z != hoverZone) { hoverZone = z; repaint(); } }

void SpacePanel::SpacePicker::mouseDown (const juce::MouseEvent& e)
{
    const int z = zoneAt (e.getPosition());
    if (z == -1 || z == 1) { if (onArrow) onArrow (z); return; }
    if (z == 2 && onSelect)
    {
        juce::PopupMenu m;
        m.addSectionHeader ("SPACE");
        for (int i = 0; i < SpaceArt::kNumTypes; ++i) m.addItem (i + 1, SpaceArt::name (i), true, i == type);
        juce::Component::SafePointer<SpacePicker> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int r) { if (safe != nullptr && r > 0 && safe->onSelect) safe->onSelect (r - 1); });
    }
}

void SpacePanel::SpacePicker::paint (juce::Graphics& g)
{
    const int t = juce::jlimit (0, SpaceArt::kNumTypes - 1, type);
    const auto b = getLocalBounds().toFloat();
    const float corner = juce::jmin (10.0f, b.getHeight() * 0.15f);
    const auto tint = SpaceArt::tint (t);

    if (artOnly)
    {
        SpaceArt::draw (g, b, t, phase, activity, corner);
    }
    else
    {
        draw::insetSurface (g, b, corner);
        auto artArea = b.withLeft (b.getWidth() * 0.6f + 4.0f).reduced (4.0f);
        SpaceArt::draw (g, artArea, t, phase, activity, corner - 2.0f);
    }

    // Name pill with chevrons
    auto nameArea = nameBounds();
    const float pillCorner = nameArea.getHeight() * 0.5f;
    if (hoverZone != 0) draw::glowRoundedRect (g, nameArea, pillCorner, tint, 8.0f, 0.3f);
    g.setColour (artOnly ? Theme::panel.withAlpha (0.85f) : Theme::panel);
    g.fillRoundedRectangle (nameArea, pillCorner);
    g.setColour (Theme::border.withMultipliedAlpha (hoverZone != 0 ? 1.8f : 1.0f));
    g.drawRoundedRectangle (nameArea.reduced (0.5f), pillCorner, 1.0f);
    const float zone = juce::jmin (nameArea.getWidth() * 0.25f, 34.0f);
    draw::chevron (g, nameArea.withWidth (zone).reduced (zone * 0.3f, nameArea.getHeight() * 0.3f), -1, hoverZone == -1 ? tint : Theme::textSecondary);
    draw::chevron (g, nameArea.withLeft (nameArea.getRight() - zone).reduced (zone * 0.3f, nameArea.getHeight() * 0.3f), 1, hoverZone == 1 ? tint : Theme::textSecondary);
    const float h = juce::jlimit (9.0f, 13.0f, nameArea.getHeight() * 0.36f);
    draw::trackedText (g, SpaceArt::name (t), nameArea.reduced (zone, 0.0f), juce::Justification::centred, Theme::labelFontStrong (h),
                       hoverZone == 2 ? Theme::textPrimary.interpolatedWith (tint, 0.4f) : Theme::textPrimary);
}

SpacePanel::SpacePanel (AntiMatrProcessor& p)
    : AMPanel ("Space", "Place it anywhere", Theme::ivory), processor (p)
{
    addAndMakeVisible (picker);
    picker.setTooltip ("Space preset: the environment the sound lives in");
    auto& apvts = processor.parameters();
    auto* typeParam = apvts.getParameter (ParameterRegistry::get (Param::spaceType).id);
    typeAttachment = std::make_unique<juce::ParameterAttachment> (*typeParam, [this] (float v) { picker.type = (int) std::lround (v); picker.repaint(); });
    picker.onArrow = [this] (int dir) { const int n = SpaceArt::kNumTypes; typeAttachment->setValueAsCompleteGesture ((float) (((picker.type + dir) % n + n) % n)); };
    picker.onSelect = [this] (int i) { typeAttachment->setValueAsCompleteGesture ((float) i); };
    typeAttachment->sendInitialUpdate();

    const juce::Colour accents[] = { Theme::magenta, Theme::cyan, Theme::violet, Theme::ivory };
    const Param params[] = { Param::spaceMix, Param::spaceSize, Param::spaceTone, Param::spaceFeedback };
    for (int i = 0; i < 4; ++i)
    {
        knobs.push_back (std::make_unique<BoundKnob> (apvts, params[i], accents[i]));
        addAndMakeVisible (knobs.back()->knob);
    }
    startTimerHz (24);
}

void SpacePanel::resized()
{
    auto area = contentBounds();
    const int gap = juce::jmax (4, area.getHeight() / 30);
    auto knobArea = area.removeFromBottom (juce::roundToInt ((float) area.getHeight() * 0.42f));
    area.removeFromBottom (gap);
    picker.setBounds (area);
    layoutKnobRow (knobArea, { &knobs[0]->knob, &knobs[1]->knob, &knobs[2]->knob, &knobs[3]->knob });
}

void SpacePanel::timerCallback()
{
    if (! isShowing()) return;
    picker.phase += 1.0f / 24.0f;
    const auto& vs = processor.diagnostics().visualSnapshots.latest();
    picker.activity = juce::jlimit (0.0f, 1.0f, vs.rmsL * 3.0f);
    picker.repaint();
    setActivity (vs.spaceActivity * 0.5f);
}

} // namespace am::ui

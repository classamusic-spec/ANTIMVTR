#include "Panels.h"

namespace am::ui
{

BoundKnob::BoundKnob (juce::AudioProcessorValueTreeState& apvts, Param p, juce::Colour accent, const juce::String& labelOverride)
    : knob (labelOverride.isNotEmpty() ? labelOverride : juce::String (ParameterRegistry::get (p).name), accent)
{
    const auto& d = ParameterRegistry::get (p);
    attachment = std::make_unique<SliderAttachment> (apvts, d.id, knob);
    knob.setDoubleClickReturnValue (true, d.defaultValue);
    if (d.min < 0.0f && d.max > 0.0f) knob.setBipolar (true);
}

void layoutKnobRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> knobs, int rows)
{
    const int n = (int) knobs.size();
    if (n == 0) return;
    const int perRow = (n + rows - 1) / rows;
    const int cellW = area.getWidth() / perRow;
    const int cellH = area.getHeight() / rows;
    int i = 0;
    for (auto* k : knobs)
    {
        const int r = i / perRow, c = i % perRow;
        k->setBounds (area.getX() + c * cellW, area.getY() + r * cellH, cellW, cellH);
        ++i;
    }
}

//==============================================================================
SourcePanel::SourcePanel (AntiMatrProcessor& p)
    : AMPanel ("Source", "Choose your energy", Theme::blue),
      processor (p),
      selector ({ { "Wave", Icon::Wave, Theme::violet }, { "Dust", Icon::Dust, Theme::blue },
                  { "Impact", Icon::Impact, Theme::cyan }, { "Sample", Icon::Sample, Theme::ivory } })
{
    addAndMakeVisible (selector);
    addAndMakeVisible (wave);
    wave.setCaption ("Fractured Forms");

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
        auto* tableParam = processor.parameters().getParameter (ParameterRegistry::get (Param::waveTable).id);
        const int n = ParameterRegistry::get (Param::waveTable).numChoices();
        const int cur = (int) std::lround (tableParam->convertFrom0to1 (tableParam->getValue()));
        const int next = ((cur + dir) % n + n) % n;
        tableParam->setValueNotifyingHost (tableParam->convertTo0to1 ((float) next));
    };

    startTimerHz (30);
}

void SourcePanel::rebuildKnobs (int sourceIndex)
{
    if (sourceIndex == currentSource) return;
    currentSource = sourceIndex;
    knobs.clear();
    auto& apvts = processor.parameters();
    auto add = [&] (Param p, const juce::String& label = {}) { knobs.push_back (std::make_unique<BoundKnob> (apvts, p, Theme::blue, label)); addAndMakeVisible (knobs.back()->knob); };
    switch (sourceIndex)
    {
        case 1:  add (Param::dustDensity); add (Param::dustColor); add (Param::dustGrain); add (Param::dustSpread); break;
        case 2:  add (Param::impactHardness); add (Param::impactBrightness); add (Param::impactLength); add (Param::impactRandom); break;
        case 3:  add (Param::sampleStart); add (Param::sampleEnd); add (Param::samplePitch); add (Param::sampleSpread); break;
        case 4:  add (Param::gesturePressure); add (Param::gestureSpeed); add (Param::gestureRoughness); add (Param::gesturePosition); break;
        default: add (Param::wavePosition); add (Param::waveScan); add (Param::waveDetune); add (Param::waveSpread); break;
    }
    const auto names = juce::StringArray { "Fractured Forms", "Particle Field", "Strike Field", "Imported Matter", "Living Gesture" };
    wave.setCaption (names[juce::jlimit (0, names.size() - 1, sourceIndex)]);
    resized();
}

void SourcePanel::resized()
{
    auto area = contentBounds();
    const int gap = juce::jmax (4, area.getHeight() / 40);
    auto selectorArea = area.removeFromTop (juce::roundToInt ((float) area.getHeight() * 0.30f));
    area.removeFromTop (gap);
    auto knobArea = area.removeFromBottom (juce::roundToInt ((float) area.getHeight() * 0.40f));
    area.removeFromBottom (gap);
    selector.setBounds (selectorArea);
    wave.setBounds (area);
    std::vector<juce::Component*> comps;
    for (auto& k : knobs) comps.push_back (&k->knob);
    if (comps.size() == 4) layoutKnobRow (knobArea, { comps[0], comps[1], comps[2], comps[3] });
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
        addAndMakeVisible (simpleKnobs.back()->knob);
    }
    const Param adv[] = { Param::shapeMaterialA, Param::shapeMaterialB, Param::shapeBlend, Param::shapeTopology, Param::shapeCoupling, Param::shapeDistribution,
                          Param::shapeExcite, Param::shapeStereo, Param::shapeMix };
    for (int i = 0; i < 9; ++i)
    {
        advancedKnobs.push_back (std::make_unique<BoundKnob> (apvts, adv[i], accents[i % 6]));
        addChildComponent (advancedKnobs.back()->knob);
    }
    addAndMakeVisible (mode);
    mode.onChange = [this] (int i) { setAdvanced (i == 1); };
}

void ShapePanel::setAdvanced (bool a)
{
    advanced = a;
    for (auto& k : simpleKnobs) k->knob.setVisible (! advanced);
    for (auto& k : advancedKnobs) k->knob.setVisible (advanced);
    resized();
}

void ShapePanel::resized()
{
    auto area = contentBounds();
    auto modeArea = area.removeFromTop (juce::jlimit (26, 40, area.getHeight() / 9));
    mode.setBounds (modeArea.reduced (juce::jmax (4, area.getWidth() / 30), 0));
    area.removeFromTop (juce::jmax (4, area.getHeight() / 30));
    if (! advanced)
    {
        layoutKnobRow (area, { &simpleKnobs[0]->knob, &simpleKnobs[1]->knob, &simpleKnobs[2]->knob,
                               &simpleKnobs[3]->knob, &simpleKnobs[4]->knob, &simpleKnobs[5]->knob }, 2);
    }
    else
    {
        std::vector<juce::Component*> c;
        for (auto& k : advancedKnobs) c.push_back (&k->knob);
        layoutKnobRow (area, { c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8] }, 3);
    }
}

//==============================================================================
void EvolvePanel::OperatorCell::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float labelH = juce::jlimit (10.0f, 16.0f, b.getHeight() * 0.28f);
    auto iconArea = b.withTrimmedBottom (labelH).reduced (b.getWidth() * 0.22f, 4.0f);
    const float d = juce::jmin (iconArea.getWidth(), iconArea.getHeight());
    iconArea = iconArea.withSizeKeepingCentre (d, d);

    const bool lit = selected || hover;
    const float glow = 0.25f * amount + (selected ? 0.55f : 0.0f) + (hover ? 0.2f : 0.0f);
    if (glow > 0.05f)
        draw::glowEllipse (g, iconArea, Theme::violet, d * 0.35f, glow);
    if (selected)
    {
        g.setColour (Theme::violet.withAlpha (0.10f));
        g.fillRoundedRectangle (b.reduced (2.0f), 8.0f);
    }
    Icons::draw (g, glyph, iconArea, lit ? Theme::textPrimary : Theme::textSecondary.brighter (0.2f), 0.9f);
    const float h = juce::jlimit (8.5f, 12.0f, labelH * 0.7f);
    draw::trackedText (g, name.toUpperCase(), b.withTop (b.getBottom() - labelH), juce::Justification::centredTop, Theme::labelFont (h),
                       lit ? Theme::textPrimary : Theme::textSecondary);
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
    for (int i = 0; i < 4; ++i)
    {
        cells[(size_t) i] = std::make_unique<OperatorCell> (names[i], icons[i]);
        cells[(size_t) i]->onClick = [this, i] { selectOperator (i, false); };
        addAndMakeVisible (*cells[(size_t) i]);
    }
    addAndMakeVisible (amount);
    addAndMakeVisible (speed);
    auto& apvts = processor.parameters();
    speedAttachment = std::make_unique<SliderAttachment> (apvts, ParameterRegistry::get (Param::evolveSpeed).id, speed);

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
    for (int i = 0; i < 4; ++i) { cells[(size_t) i]->selected = (i == index); cells[(size_t) i]->repaint(); }
    amountAttachment.reset();
    amountAttachment = std::make_unique<SliderAttachment> (processor.parameters(), ParameterRegistry::get (operatorParam (index)).id, amount);
    amount.setDoubleClickReturnValue (true, ParameterRegistry::get (operatorParam (index)).defaultValue);
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
        if (std::abs (cells[(size_t) i]->amount - v) > 0.01f) { cells[(size_t) i]->amount = v; cells[(size_t) i]->repaint(); }
        total += v;
    }
    setActivity (juce::jlimit (0.0f, 1.0f, total * 0.5f));
}

void EvolvePanel::resized()
{
    auto area = contentBounds();
    const int gap = juce::jmax (4, area.getHeight() / 30);
    auto cellArea = area.removeFromTop (juce::roundToInt ((float) area.getHeight() * 0.50f));
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
    onOff.setBounds (header.removeFromRight (juce::jlimit (90, 140, header.getWidth() / 2)).withSizeKeepingCentre (juce::jlimit (90, 140, header.getWidth() / 2), juce::jlimit (24, 34, header.getHeight() - 6)));

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

void SpacePanel::SpacePicker::mouseDown (const juce::MouseEvent& e)
{
    if (onArrow == nullptr) return;
    const int w = getWidth();
    if (e.x < w * 0.12f) onArrow (-1);
    else if (e.x < w * 0.62f) onArrow (1);
}

void SpacePanel::SpacePicker::paint (juce::Graphics& g)
{
    static const char* names[] = { "NEBULA", "VOID", "CHAMBER", "ORBIT", "DREAM", "MACHINE", "SHIMMER", "DUST" };
    static const juce::Colour tints[] = { Theme::violet, juce::Colour (0xff3a3a55), Theme::amber, Theme::blue, Theme::magenta, Theme::cyan, Theme::ivory, Theme::textSecondary };
    const int t = juce::jlimit (0, 7, type);

    const auto b = getLocalBounds().toFloat();
    const float corner = juce::jmin (10.0f, b.getHeight() * 0.15f);
    draw::insetSurface (g, b, corner);

    auto nameArea = b.withWidth (b.getWidth() * 0.62f).reduced (6.0f);
    auto artArea  = b.withLeft (nameArea.getRight() + 4.0f).reduced (4.0f);

    // Name pill with chevrons
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (nameArea, corner);
    g.setColour (Theme::border);
    g.drawRoundedRectangle (nameArea, corner, 1.0f);
    const float ch = juce::jmin (10.0f, nameArea.getHeight() * 0.3f);
    g.setColour (Theme::textSecondary);
    juce::Path l, r;
    l.startNewSubPath (nameArea.getX() + 16.0f, nameArea.getCentreY() - ch * 0.5f); l.lineTo (nameArea.getX() + 10.0f, nameArea.getCentreY()); l.lineTo (nameArea.getX() + 16.0f, nameArea.getCentreY() + ch * 0.5f);
    r.startNewSubPath (nameArea.getRight() - 16.0f, nameArea.getCentreY() - ch * 0.5f); r.lineTo (nameArea.getRight() - 10.0f, nameArea.getCentreY()); r.lineTo (nameArea.getRight() - 16.0f, nameArea.getCentreY() + ch * 0.5f);
    g.strokePath (l, juce::PathStrokeType (1.2f)); g.strokePath (r, juce::PathStrokeType (1.2f));
    const float h = juce::jlimit (9.0f, 13.0f, nameArea.getHeight() * 0.36f);
    draw::trackedText (g, names[t], nameArea, juce::Justification::centred, Theme::labelFont (h), Theme::textPrimary);

    // Procedural environment art: a spiral swirl tinted by the space type.
    {
        juce::Graphics::ScopedSaveState save (g);
        juce::Path clip; clip.addRoundedRectangle (artArea, corner);
        g.reduceClipRegion (clip);
        g.setColour (juce::Colour (0xff050509));
        g.fillRoundedRectangle (artArea, corner);
        const auto c = artArea.getCentre();
        const float R = juce::jmin (artArea.getWidth(), artArea.getHeight()) * 0.5f;
        juce::ColourGradient glow (tints[t].withAlpha (0.35f + 0.3f * activity), c.x, c.y, tints[t].withAlpha (0.0f), c.x + R * 1.2f, c.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (artArea.expanded (R * 0.3f));
        for (int arm = 0; arm < 3; ++arm)
        {
            juce::Path spiral;
            for (int i = 0; i <= 60; ++i)
            {
                const float u = (float) i / 60.0f;
                const float a = u * 5.0f + (float) arm * 2.094f + phase * (0.5f + 0.5f * (float) t / 7.0f);
                const float rad = R * 0.05f + R * 0.95f * u;
                juce::Point<float> pt (c.x + std::cos (a) * rad, c.y + std::sin (a) * rad * 0.6f);
                if (i == 0) spiral.startNewSubPath (pt); else spiral.lineTo (pt);
            }
            draw::glowPath (g, spiral, tints[t].withAlpha (0.6f), 1.0f, 5.0f, 0.4f + 0.4f * activity);
        }
        juce::Random rng (t * 31 + 7);
        for (int i = 0; i < 40; ++i)
        {
            const float x = artArea.getX() + rng.nextFloat() * artArea.getWidth();
            const float y = artArea.getY() + rng.nextFloat() * artArea.getHeight();
            g.setColour (Theme::textPrimary.withAlpha (0.1f + 0.4f * rng.nextFloat()));
            g.fillEllipse (x, y, 1.2f, 1.2f);
        }
    }
}

SpacePanel::SpacePanel (AntiMatrProcessor& p)
    : AMPanel ("Space", "Place it anywhere", Theme::ivory), processor (p)
{
    addAndMakeVisible (picker);
    auto& apvts = processor.parameters();
    auto* typeParam = apvts.getParameter (ParameterRegistry::get (Param::spaceType).id);
    typeAttachment = std::make_unique<juce::ParameterAttachment> (*typeParam, [this] (float v) { picker.type = (int) std::lround (v); picker.repaint(); });
    picker.onArrow = [this] (int dir) { const int n = 8; typeAttachment->setValueAsCompleteGesture ((float) (((picker.type + dir) % n + n) % n)); };
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
    picker.phase += 0.02f;
    const auto& vs = processor.diagnostics().visualSnapshots.latest();
    picker.activity = juce::jlimit (0.0f, 1.0f, vs.rmsL * 3.0f);
    picker.repaint();
    setActivity (vs.spaceActivity * 0.5f);
}

} // namespace am::ui

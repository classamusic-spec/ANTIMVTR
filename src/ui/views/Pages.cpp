#include "Pages.h"
#include "ui/UILayout.h"

namespace am::ui
{

namespace
{
    int pagePad (const juce::Component& c) { return juce::jmax (10, c.getWidth() / 80); }

    juce::String envelopeText (float seconds)
    {
        return seconds < 1.0f ? juce::String (juce::roundToInt (seconds * 1000.0f)) + " ms" : juce::String (seconds, 2) + " s";
    }

    /** The three columns of a deep page, as rectangles inside `area`. */
    struct Columns { juce::Rectangle<int> sidebar, centre, controls; };

    Columns pageColumns (juce::Rectangle<int> area, int gap, float sidebarFraction = 0.165f, float controlsFraction = 0.295f)
    {
        const auto spans = layout::pageColumns (area.getWidth(), gap, sidebarFraction, controlsFraction);
        auto at = [&area] (const layout::Span& s)
        {
            return juce::Rectangle<int> (area.getX() + s.start, area.getY(), s.size, area.getHeight());
        };
        return { at (spans.sidebar), at (spans.centre), at (spans.controls) };
    }

    /**
        Lays a sidebar panel out: the pill list takes the height its rows want from
        the top, and any footer controls (a MAIN / ADVANCED segment, an OFF / ON
        switch, a mode stepper) are anchored to the bottom.

        The list spreads its rows over whatever height it is given, so handing it
        the whole panel would make each pill a slab; it is sized to its content.
    */
    void layoutSidebar (AMPanel& panel, AMOptionList& list, int rows, bool withDescription,
                        const std::vector<juce::Component*>& footer, int gap)
    {
        auto c = panel.contentBounds();
        for (auto it = footer.rbegin(); it != footer.rend(); ++it)
        {
            if (*it == nullptr || ! (*it)->isVisible()) continue;
            const int h = juce::jlimit (28, 56, c.getHeight() / 9);
            (*it)->setBounds (c.removeFromBottom (h));
            c.removeFromBottom (gap / 2);
        }
        // The list spreads its rows over whatever height it has and takes a fifth of
        // it for the description, so the height is solved for the row size we want
        // rather than taken from the panel.
        const int rowH = juce::jlimit (32, 68, juce::roundToInt ((float) c.getWidth() * 0.30f));
        const int rowsH = juce::jmax (1, rows) * rowH;
        int want = rowsH;
        if (withDescription)
            want = rowsH + juce::roundToInt (juce::jlimit (26.0f, 64.0f, (float) rowsH / 0.8f * 0.2f));
        list.setBounds (c.getHeight() > want ? c.withHeight (want) : c);
    }
}

//==============================================================================
ParamPanel::ParamPanel (AntiMatrProcessor& p, const juce::String& title, const juce::String& subtitle, juce::Colour accent,
                        std::vector<Param> params, int cols, const juce::String& labelPrefix)
    : AMPanel (title, subtitle, accent), processor (p), columns (cols)
{
    defaultPrefix = labelPrefix.isNotEmpty() ? labelPrefix : title;
    setParams (std::move (params), cols, defaultPrefix);
}

std::unique_ptr<BoundControl> ParamPanel::makeControl (Param param, const juce::String& prefix)
{
    juce::String label (ParameterRegistry::get (param).name);
    const juce::String p = prefix.trim().toUpperCase() + " ";
    if (label.toUpperCase().startsWith (p) && label.length() > p.length()) label = label.substring (p.length());
    return std::make_unique<BoundControl> (processor.parameters(), param, getAccent(), label);
}

void ParamPanel::setParams (std::vector<Param> params, int cols, const juce::String& labelPrefix)
{
    for (auto& c : controls) removeChildComponent (&c->component());
    controls.clear();
    columns = cols;
    const juce::String prefix = labelPrefix.isNotEmpty() ? labelPrefix : defaultPrefix;
    for (auto param : params)
    {
        controls.push_back (makeControl (param, prefix));
        addAndMakeVisible (controls.back()->component());
    }
    resized();
}

void ParamPanel::setFooterRows (std::vector<std::vector<Param>> rows, const juce::String& labelPrefix)
{
    for (auto& c : footer) removeChildComponent (&c->component());
    footer.clear();
    footerRowSizes.clear();
    const juce::String prefix = labelPrefix.isNotEmpty() ? labelPrefix : defaultPrefix;
    for (const auto& row : rows)
    {
        if (row.empty()) continue;
        footerRowSizes.push_back ((int) row.size());
        for (auto param : row)
        {
            footer.push_back (makeControl (param, prefix));
            addAndMakeVisible (footer.back()->component());
        }
    }
    resized();
}

void ParamPanel::setTopRows (std::vector<std::vector<Param>> rows, const juce::String& labelPrefix)
{
    for (auto& c : topControls) removeChildComponent (&c->component());
    topControls.clear();
    topRowSizes.clear();
    const juce::String prefix = labelPrefix.isNotEmpty() ? labelPrefix : defaultPrefix;
    for (const auto& row : rows)
    {
        if (row.empty()) continue;
        topRowSizes.push_back ((int) row.size());
        for (auto param : row)
        {
            topControls.push_back (makeControl (param, prefix));
            addAndMakeVisible (topControls.back()->component());
        }
    }
    resized();
}

BoundControl* ParamPanel::control (Param p)
{
    for (auto& c : controls) if (c->param() == p) return c.get();
    for (auto& c : footer) if (c->param() == p) return c.get();
    for (auto& c : topControls) if (c->param() == p) return c.get();
    return headerToggle != nullptr && headerToggle->param() == p ? headerToggle.get() : nullptr;
}

void ParamPanel::setHeaderToggle (Param p, bool asSwitch)
{
    if (asSwitch)
    {
        headerSwitch = std::make_unique<AMSegment> (juce::StringArray { "Off", "On" }, getAccent());
        headerSwitch->setTooltip (paramTooltip (p));
        auto* param = processor.parameters().getParameter (ParameterRegistry::get (p).id);
        auto* raw = headerSwitch.get();
        headerSwitchAttachment = std::make_unique<juce::ParameterAttachment> (*param, [raw] (float v)
        {
            raw->setSelected (v >= 0.5f ? 1 : 0, juce::dontSendNotification);
        });
        headerSwitch->onChange = [this] (int i) { headerSwitchAttachment->setValueAsCompleteGesture (i == 1 ? 1.0f : 0.0f); };
        headerSwitchAttachment->sendInitialUpdate();
        addAndMakeVisible (*headerSwitch);
    }
    else
    {
        headerToggle = std::make_unique<BoundControl> (processor.parameters(), p, getAccent(), " ");
        addAndMakeVisible (headerToggle->component());
    }
    resized();
}

void ParamPanel::clearHeaderToggle()
{
    if (headerToggle != nullptr) removeChildComponent (&headerToggle->component());
    headerToggle.reset();
    if (headerSwitch != nullptr) removeChildComponent (headerSwitch.get());
    headerSwitchAttachment.reset();
    headerSwitch.reset();
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

void ParamPanel::refreshModRings (const ModulationSnapshot& snapshot)
{
    for (auto& c : controls) c->refreshModRing (snapshot);
    for (auto& c : footer) c->refreshModRing (snapshot);
    for (auto& c : topControls) c->refreshModRing (snapshot);
}

void ParamPanel::resized()
{
    if (headerToggle != nullptr)
    {
        auto h = headerRightBounds();
        const int w = juce::jlimit (44, 60, h.getHeight());
        headerToggle->component().setBounds (h.removeFromRight (w).withSizeKeepingCentre (w, juce::jlimit (22, 30, h.getHeight())));
    }
    if (headerSwitch != nullptr)
    {
        auto h = headerRightBounds();
        const int w = juce::jlimit (88, 130, h.getWidth() / 2);
        headerSwitch->setBounds (h.removeFromRight (w).withSizeKeepingCentre (w, juce::jlimit (22, 30, h.getHeight())));
    }
    auto area = contentBounds();
    if (area.isEmpty()) return;
    const int gap = juce::jmax (3, area.getHeight() / 40);

    if (! topRowSizes.empty())
    {
        int index = 0;
        for (int r = 0; r < (int) topRowSizes.size(); ++r)
        {
            const int rowH = juce::jlimit (30, 66, juce::roundToInt ((float) area.getHeight() * 0.16f));
            auto row = area.removeFromTop (rowH);
            area.removeFromTop (gap);
            std::vector<juce::Component*> comps;
            for (int i = 0; i < topRowSizes[(size_t) r]; ++i, ++index)
                if (index < (int) topControls.size()) comps.push_back (&topControls[(size_t) index]->component());
            layoutGrid (row, comps, (int) juce::jmax ((size_t) 1, comps.size()), gap, 0);
        }
    }

    if (display != nullptr && displayFraction > 0.0f && area.getHeight() > 90)
    {
        display->setBounds (area.removeFromTop (juce::roundToInt ((float) area.getHeight() * displayFraction)));
        area.removeFromTop (juce::jmax (4, area.getHeight() / 24));
    }

    // The footer rows take the height they need from the bottom; the grid keeps the rest,
    // so a stepper row never steals a knob's diameter and never leaves a band of empty panel.
    if (! footerRowSizes.empty())
    {
        const int rows = (int) footerRowSizes.size();
        const int rowH = juce::jlimit (30, 76, juce::roundToInt ((float) area.getHeight() * (1.0f - gridFraction) / (float) rows));
        auto band = area.removeFromBottom (juce::jmin (area.getHeight() - 10, rowH * rows + gap * (rows - 1)));
        int index = 0;
        for (int r = 0; r < rows; ++r)
        {
            auto row = band.removeFromTop (rowH);
            if (r + 1 < rows) band.removeFromTop (gap);
            std::vector<juce::Component*> comps;
            for (int i = 0; i < footerRowSizes[(size_t) r]; ++i, ++index)
                if (index < (int) footer.size()) comps.push_back (&footer[(size_t) index]->component());
            layoutGrid (row, comps, (int) juce::jmax ((size_t) 1, comps.size()), gap, 0);
        }
        area.removeFromBottom (gap);
    }

    std::vector<juce::Component*> comps;
    for (auto& c : controls) comps.push_back (&c->component());
    if (comps.empty()) return;
    const int n = (int) comps.size();
    const int cols = columns > 0 ? columns : layout::gridColumns (n, area.getWidth(), area.getHeight());
    // The cluster panel on a deep page is much taller than a row of knobs, so the
    // rows share the slack out between them instead of leaving a dead band at each end.
    layoutGrid (area, comps, cols, 2, 2, true);
}

//==============================================================================
namespace
{
    /** What each source puts on the right of the SOURCE page. */
    struct SourceCluster
    {
        std::vector<Param> knobs;                 ///< six, two rows of three
        std::vector<std::vector<Param>> rows;     ///< stepper rows under them
        std::vector<Param> advanced;              ///< the ADVANCED tab, empty when there is none
    };

    SourceCluster sourceCluster (int source)
    {
        switch (source)
        {
            case 1:  return { { Param::dustDensity, Param::dustColor, Param::dustGrain,
                                Param::dustJitter, Param::dustSpread, Param::dustLevel },
                              { { Param::dustMode, Param::dustSeed },
                                { Param::dustPosition, Param::dustStereo, Param::dustPitch } },
                              {} };
            case 2:  return { { Param::impactHardness, Param::impactBrightness, Param::impactLength,
                                Param::impactVelocity, Param::impactCurve, Param::impactLevel },
                              { { Param::impactMode },
                                { Param::impactRandom, Param::impactRate } },
                              {} };
            case 4:  return { { Param::gesturePressure, Param::gestureSpeed, Param::gestureRoughness,
                                Param::gesturePosition, Param::gestureMotion, Param::gestureBandwidth },
                              { { Param::gestureMode, Param::gestureLevel } },
                              {} };
            default: return { { Param::wavePosition, Param::waveScan, Param::waveMorph,
                                Param::waveDetune, Param::waveSpread, Param::waveLevel },
                              { { Param::waveTable, Param::waveUnison },
                                { Param::waveOctave, Param::waveSemi, Param::waveFine } },
                              { Param::waveFM, Param::wavePM, Param::waveAM,
                                Param::waveRing, Param::waveSync, Param::waveModRatio,
                                Param::wavePhase, Param::wavePhaseRandom } };
        }
    }

    const juce::Colour& sourceAccent (int source)
    {
        static const juce::Colour accents[] = { Theme::violet, Theme::blue, Theme::cyan, Theme::ivory, Theme::magenta };
        return accents[juce::jlimit (0, 4, source)];
    }
}

Param SourcePage::modeParam (int source)
{
    static const Param modes[] = { Param::waveTable, Param::dustMode, Param::impactMode, Param::sampleMode, Param::gestureMode };
    return modes[juce::jlimit (0, 4, source)];
}

SourcePage::SourcePage (AntiMatrProcessor& p)
    : processor (p),
      sidebar ({ "Wave", "Dust", "Impact", "Sample", "Gesture" }, Theme::blue)
{
    addAndMakeVisible (sidebarPanel);
    addAndMakeVisible (mesh);
    sidebarPanel.addAndMakeVisible (sidebar);
    sidebarPanel.addAndMakeVisible (tabs);
    sidebar.setIcons ({ Icon::Wave, Icon::Dust, Icon::Impact, Icon::Sample, Icon::Gesture });
    sidebar.setDescriptions ({ "Wavetables scanned, morphed and stacked in unison.",
                               "Coloured noise, crackle and impulse clouds.",
                               "Clicks, plucks, strikes and membrane hits.",
                               "Any audio as energy, pitched and spread.",
                               "Bowed, scraped and rubbed excitation." });
    sidebar.setTooltip ("Source: what creates the energy Matter is struck with.");
    tabs.setTooltip ("MAIN: the six controls that shape this source. ADVANCED: everything else it can do.");
    tabs.onChange = [this] (int i) { advanced = i == 1; applyTab(); resized(); };

    sourceModeControl = std::make_unique<BoundControl> (processor.parameters(), Param::sourceMode, Theme::blue, "Mode");
    sidebarPanel.addAndMakeVisible (sourceModeControl->component());

    cluster = std::make_unique<ParamPanel> (processor, "Wavetable", "Fractured forms", Theme::blue, std::vector<Param> {}, 3, " ");
    addAndMakeVisible (*cluster);

    auto* param = processor.parameters().getParameter (ParameterRegistry::get (Param::sourceSelected).id);
    selectorAttachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
    {
        sidebar.setSelected ((int) std::lround (v), juce::dontSendNotification);
        rebuild ((int) std::lround (v));
    });
    sidebar.onChange = [this] (int i) { selectorAttachment->setValueAsCompleteGesture ((float) i); };
    selectorAttachment->sendInitialUpdate();
    if (currentSource < 0) rebuild (0);
    startTimerHz (30);
}

SourcePage::~SourcePage()
{
    stopTimer();
}

void SourcePage::rebuild (int source)
{
    if (source == currentSource) return;
    currentSource = juce::jlimit (0, 4, source);
    samplePanel.reset();
    gesturePanel.reset();

    // SAMPLE brings its own display — the waveform with the playback handles — so it
    // takes the middle and the right together. Every other source uses the mesh.
    const bool dedicated = currentSource == 3;
    if (currentSource == 3) { samplePanel = std::make_unique<SamplePanel> (processor); addAndMakeVisible (*samplePanel); }
    if (currentSource == 4) { gesturePanel = std::make_unique<GesturePanel> (processor); addChildComponent (*gesturePanel); }

    // The table selector under the screen names whatever the selected source calls its shapes.
    tableSelector.reset();
    tableAttachment.reset();
    if (! dedicated)
    {
        const auto mp = modeParam (currentSource);
        tableSelector = std::make_unique<AMChoice> (juce::String (ParameterRegistry::get (mp).name),
                                                    paramChoices (mp), sourceAccent (currentSource));
        tableSelector->setShowLabel (false);
        tableSelector->setTooltip (paramTooltip (mp));
        addAndMakeVisible (*tableSelector);
        auto* modeParameter = processor.parameters().getParameter (ParameterRegistry::get (mp).id);
        auto* raw = tableSelector.get();
        tableAttachment = std::make_unique<juce::ParameterAttachment> (*modeParameter, [raw] (float v)
        {
            raw->setSelected ((int) std::lround (v), juce::dontSendNotification);
        });
        tableSelector->onChange = [this] (int i) { tableAttachment->setValueAsCompleteGesture ((float) i); };
        tableAttachment->sendInitialUpdate();
    }

    const auto set = sourceCluster (currentSource);
    mainParams = set.knobs;
    mainRows = set.rows;
    advancedParams = set.advanced;

    static const char* names[]    = { "Wavetable", "Dust", "Impact", "Sample", "Gesture" };
    static const char* taglines[] = { "Fractured forms", "Particle field", "Strike field", "Imported matter", "Living gesture" };
    cluster->setTitle (names[currentSource]);
    cluster->setSubtitle (taglines[currentSource]);
    cluster->setAccent (sourceAccent (currentSource));
    mesh.setAccent (Theme::blue);
    mesh.setTitle (juce::String (names[currentSource]) + "  /  " + juce::String (taglines[currentSource]));
    sidebar.setAccent (Theme::blue);

    // ADVANCED only exists where a source has more than its six shaping controls, or
    // (GESTURE) a panel of its own behind them.
    const bool hasAdvanced = ! advancedParams.empty() || gesturePanel != nullptr;
    tabs.setVisible (hasAdvanced && ! dedicated);
    if (! hasAdvanced) { advanced = false; tabs.setSelected (0, juce::dontSendNotification); }
    applyTab();
    resized();
}

void SourcePage::applyTab()
{
    if (cluster == nullptr) return;
    const bool wide = samplePanel != nullptr || (gesturePanel != nullptr && advanced);
    if (gesturePanel != nullptr) gesturePanel->setVisible (advanced);
    mesh.setVisible (! wide);
    cluster->setVisible (! wide);
    if (tableSelector != nullptr) tableSelector->setVisible (! wide);
    static const char* prefixes[] = { "Wave", "Dust", "Impact", "Sample", "Gesture" };
    const juce::String prefix (prefixes[juce::jlimit (0, 4, currentSource)]);
    if (advanced && ! advancedParams.empty())
    {
        cluster->setFooterRows ({}, prefix);
        cluster->setParams (advancedParams, 3, prefix);
        cluster->setSubtitle ("Cross-modulation & phase");
    }
    else
    {
        cluster->setParams (mainParams, 3, prefix);
        cluster->setFooterRows (mainRows, prefix);
        cluster->setGridFraction (mainRows.empty() ? 1.0f : 0.56f);
    }
    const juce::Colour ring[] = { Theme::cyan, Theme::blue, Theme::violet, Theme::cyan, Theme::blue, Theme::ivory };
    const auto& shown = advanced && ! advancedParams.empty() ? advancedParams : mainParams;
    for (size_t i = 0; i < shown.size(); ++i) cluster->setAccentFor (shown[i], ring[i % 6]);
    cluster->setHeroKnobs (! advanced);
}

void SourcePage::resized()
{
    const int pad = pagePad (*this), gap = pad;
    auto area = getLocalBounds().reduced (pad, pad / 2);
    const auto cols = pageColumns (area, gap);

    sidebarPanel.setBounds (cols.sidebar);
    layoutSidebar (sidebarPanel, sidebar, 5, true,
                   { &tabs, sourceModeControl != nullptr ? &sourceModeControl->component() : nullptr }, gap);

    // A source with a panel of its own takes the middle and the right together.
    const auto wide = cols.centre.getUnion (cols.controls);
    if (samplePanel != nullptr) { samplePanel->setBounds (wide); return; }
    if (gesturePanel != nullptr && advanced) { gesturePanel->setBounds (wide); return; }

    auto centre = cols.centre;
    const int selectorH = juce::jlimit (30, 46, centre.getHeight() / 15);
    auto selectorArea = centre.removeFromBottom (selectorH);
    centre.removeFromBottom (gap);
    mesh.setBounds (centre);
    if (tableSelector != nullptr)
        tableSelector->setBounds (selectorArea.withSizeKeepingCentre (juce::jmin (selectorArea.getWidth(), juce::jmax (180, selectorArea.getWidth() / 2)), selectorH));

    cluster->setBounds (cols.controls);
}

void SourcePage::timerCallback()
{
    if (! isShowing()) return;
    auto& diag = processor.diagnostics();
    const auto& vs = diag.visualSnapshots.latest();
    const float energy = juce::jlimit (0.0f, 1.0f, vs.sourceRms * 4.0f);
    const auto values = processor.currentParamValues();

    if (mesh.isVisible())
    {
        const auto mp = modeParam (currentSource);
        const auto choices = paramChoices (mp);
        const int index = juce::jlimit (0, juce::jmax (0, choices.size() - 1), paramChoice (values, mp));
        const Param posParam[] = { Param::wavePosition, Param::dustDensity, Param::impactHardness, Param::sampleStart, Param::gesturePressure };
        const Param scanParam[] = { Param::waveScan, Param::dustJitter, Param::impactRandom, Param::sampleGrain, Param::gestureMotion };
        const Param morphParam[] = { Param::waveMorph, Param::dustColor, Param::impactBrightness, Param::sampleSpread, Param::gestureRoughness };
        mesh.setShape (index, paramValue (values, posParam[currentSource]),
                       paramValue (values, scanParam[currentSource]),
                       paramValue (values, morphParam[currentSource]));
        mesh.setCaption (choices[index]);
        mesh.setEnergy (energy);
        mesh.advance (1.0f / 30.0f);
    }
    sidebarPanel.setActivity (juce::jlimit (0.0f, 1.0f, vs.sourceRms * 2.0f));

    const auto& mod = latestModulation (processor);
    if (cluster != nullptr) cluster->refreshModRings (mod);
}

//==============================================================================
ShapePage::ShapePage (AntiMatrProcessor& p)
    : processor (p),
      sidebar ({ "Simple", "Advanced", "Material" }, Theme::cyan)
{
    addAndMakeVisible (sidebarPanel);
    addAndMakeVisible (lattice);
    addAndMakeVisible (blendPanel);
    sidebarPanel.addAndMakeVisible (sidebar);
    sidebar.setIcons ({ Icon::Shape, Icon::Settings, Icon::Dna });
    sidebar.setDescriptions ({ "The six macros that shape the object.",
                               "Excitation, pitch and output.",
                               "The two physical models and how their nodes are wired." });
    sidebar.setTooltip ("SIMPLE: the six Matter macros. ADVANCED: excitation, pitch and output. MATERIAL: the two physical models and how their nodes are wired.");
    sidebar.onChange = [this] (int i) { showTab (i); };

    // The blend selector under the screen: the two models and the crossfade between them.
    materialA = std::make_unique<BoundControl> (processor.parameters(), Param::shapeMaterialA, Theme::cyan, "Material A");
    materialB = std::make_unique<BoundControl> (processor.parameters(), Param::shapeMaterialB, Theme::violet, "Material B");
    blendPanel.setCompact (true);
    blendPanel.addAndMakeVisible (materialA->component());
    blendPanel.addAndMakeVisible (materialB->component());
    blendPanel.addAndMakeVisible (blend);
    blend.setTooltip (paramTooltip (Param::shapeBlend));
    blendAttachment = std::make_unique<SliderAttachment> (processor.parameters(), ParameterRegistry::get (Param::shapeBlend).id, blend);
    blend.setDoubleClickReturnValue (true, ParameterRegistry::get (Param::shapeBlend).defaultValue);

    cluster = std::make_unique<ParamPanel> (processor, "Matter", "Turn matter into sound", Theme::cyan,
                                            std::vector<Param> {}, 3, "Shape");
    addAndMakeVisible (*cluster);
    lattice.setTitle ("Matter / node lattice");
    showTab (0);
    startTimerHz (24);
}

void ShapePage::showTab (int index)
{
    current = juce::jlimit (0, 2, index);
    sidebar.setSelected (current, juce::dontSendNotification);
    switch (current)
    {
        case 1:
            cluster->setTitle ("Response");
            cluster->setSubtitle ("Excitation, pitch & output");
            cluster->setParams ({ Param::shapeExcite, Param::shapeStrike, Param::shapeMix,
                                  Param::shapeStereo, Param::shapeKeytrack, Param::shapePitch }, 3, "Shape");
            cluster->setFooterRows ({}, "Shape");
            break;
        case 2:
            cluster->setTitle ("Topology");
            cluster->setSubtitle ("Node distribution & coupling");
            cluster->setParams ({ Param::shapeCoupling, Param::shapeDistribution, Param::shapeSeed }, 3, "Shape");
            cluster->setFooterRows ({ { Param::shapeTopology } }, "Shape");
            cluster->setGridFraction (0.62f);
            break;
        default:
            cluster->setTitle ("Matter");
            cluster->setSubtitle ("Turn matter into sound");
            cluster->setParams ({ Param::shapeDensity, Param::shapeForm, Param::shapeMass,
                                  Param::shapeTension, Param::shapeDecay, Param::shapeSurface }, 3, "Shape");
            cluster->setFooterRows ({}, "Shape");
            cluster->setHeroKnobs (true);
            break;
    }
    const Param mp[] = { Param::shapeDensity, Param::shapeForm, Param::shapeMass, Param::shapeTension, Param::shapeDecay, Param::shapeSurface };
    const juce::Colour accents[] = { Theme::cyan, Theme::blue, Theme::violet, Theme::cyan, Theme::magenta, Theme::ivory };
    for (int i = 0; i < 6; ++i) cluster->setAccentFor (mp[i], accents[i]);
    resized();
}

void ShapePage::resized()
{
    const int pad = pagePad (*this), gap = pad;
    auto area = getLocalBounds().reduced (pad, pad / 2);
    const auto cols = pageColumns (area, gap);

    sidebarPanel.setBounds (cols.sidebar);
    layoutSidebar (sidebarPanel, sidebar, 3, true, {}, gap);

    auto centre = cols.centre;
    const int blendH = juce::jlimit (74, 116, centre.getHeight() / 7);
    blendPanel.setBounds (centre.removeFromBottom (blendH));
    centre.removeFromBottom (gap);
    lattice.setBounds (centre);
    {
        auto c = blendPanel.contentBounds();
        const int stepper = juce::jlimit (120, 230, c.getWidth() / 4);
        materialA->component().setBounds (c.removeFromLeft (stepper));
        materialB->component().setBounds (c.removeFromRight (stepper));
        blend.setBounds (c.reduced (gap, juce::jmax (0, (c.getHeight() - juce::jlimit (22, 34, c.getHeight())) / 2)));
    }

    cluster->setBounds (cols.controls);
}

void ShapePage::timerCallback()
{
    if (! isShowing()) return;
    const auto& vs = processor.diagnostics().visualSnapshots.latest();
    const auto values = processor.currentParamValues();
    lattice.setMatter (vs.density, vs.form, vs.mass, vs.tension, vs.surface,
                       paramChoice (values, Param::shapeTopology), (int) paramValue (values, Param::shapeSeed));
    lattice.setEnergy (juce::jlimit (0.0f, 1.0f, vs.matterRms * 3.0f));
    lattice.setCaption (paramChoices (Param::shapeMaterialA)[juce::jlimit (0, 8, paramChoice (values, Param::shapeMaterialA))]
                        + juce::String ("   /   ")
                        + paramChoices (Param::shapeMaterialB)[juce::jlimit (0, 8, paramChoice (values, Param::shapeMaterialB))]);
    lattice.advance (1.0f / 24.0f);
    sidebarPanel.setActivity (juce::jlimit (0.0f, 1.0f, vs.matterRms * 3.0f));

    const auto& mod = latestModulation (processor);
    cluster->refreshModRings (mod);
    if (materialA != nullptr) materialA->refreshModRing (mod);
    if (materialB != nullptr) materialB->refreshModRing (mod);

    // Without a routing the ring still shows where the engine's effective value sits.
    const Param mp[] = { Param::shapeDensity, Param::shapeForm, Param::shapeMass, Param::shapeTension, Param::shapeDecay, Param::shapeSurface };
    const float live[] = { vs.density, vs.form, vs.mass, vs.tension, vs.decay, vs.surface };
    for (int i = 0; i < 6; ++i)
        if (! mod.isModulated (mp[i]))
            if (auto* c = cluster->control (mp[i])) if (auto* k = c->knob()) k->modRing().setCurrent (live[i]);
}

//==============================================================================
EvolvePage::EvolvePage (AntiMatrProcessor& p)
    : processor (p),
      sidebar ({ "Main", "Advanced", "Motion" }, Theme::violet)
{
    addAndMakeVisible (sidebarPanel);
    addAndMakeVisible (ribbon);
    addAndMakeVisible (operatorPanel);
    addChildComponent (magnetPanel);
    addChildComponent (fieldPanel);
    sidebarPanel.addAndMakeVisible (sidebar);
    sidebar.setIcons ({ Icon::Evolve, Icon::Settings, Icon::Swirl });
    sidebar.setDescriptions ({ "Bend, melt, tear and magnet.",
                               "The deformation detail behind the operators.",
                               "Gravity, scatter and how fast it all moves." });
    sidebar.setTooltip ("MAIN: bend, melt, tear and magnet. ADVANCED: the deformation detail. MOTION: how fast it all moves.");
    sidebar.onChange = [this] (int i) { showTab (i); };
    ribbon.setTitle ("Evolve / deformation");

    // The four operators: a knob each, the selected one also driven by the AMOUNT slider.
    const Param ops[] = { Param::evolveBend, Param::evolveMelt, Param::evolveTear, Param::evolveMagnet };
    const juce::Colour accents[] = { Theme::violet, Theme::indigo, Theme::magenta, Theme::blue };
    for (int i = 0; i < 4; ++i)
    {
        operatorKnobs[(size_t) i] = std::make_unique<BoundControl> (processor.parameters(), ops[i], accents[i]);
        // Dragging an operator also selects it, so the AMOUNT slider under the row
        // always drives the operator the hand is already on.
        if (auto* k = operatorKnobs[(size_t) i]->knob())
            k->onDragStart = [this, i] { if (selectedAttachment != nullptr) selectedAttachment->setValueAsCompleteGesture ((float) i); };
        operatorPanel.addAndMakeVisible (operatorKnobs[(size_t) i]->component());
    }
    operatorPanel.addAndMakeVisible (amount);
    operatorPanel.addAndMakeVisible (speed);
    speed.setTooltip (paramTooltip (Param::evolveSpeed));
    speedAttachment = std::make_unique<SliderAttachment> (processor.parameters(), ParameterRegistry::get (Param::evolveSpeed).id, speed);
    speed.setDoubleClickReturnValue (true, ParameterRegistry::get (Param::evolveSpeed).defaultValue);

    auto& apvts = processor.parameters();
    auto* selParam = apvts.getParameter (ParameterRegistry::get (Param::evolveSelected).id);
    selectedAttachment = std::make_unique<juce::ParameterAttachment> (*selParam, [this] (float v) { bindAmount ((int) std::lround (v)); });
    selectedAttachment->sendInitialUpdate();
    if (amountAttachment == nullptr) bindAmount (0);

    // Seven alignment targets: a list shows them all instead of hiding six behind a stepper.
    magnetList = std::make_unique<AMOptionList> (paramChoices (Param::evolveMagnetTarget), Theme::violet);
    magnetList->setTooltip (paramTooltip (Param::evolveMagnetTarget));
    magnetPanel.addAndMakeVisible (*magnetList);
    {
        auto* target = apvts.getParameter (ParameterRegistry::get (Param::evolveMagnetTarget).id);
        magnetAttachment = std::make_unique<juce::ParameterAttachment> (*target, [this] (float v)
        {
            magnetList->setSelected ((int) std::lround (v), juce::dontSendNotification);
        });
        magnetList->onChange = [this] (int i) { magnetAttachment->setValueAsCompleteGesture ((float) i); };
        magnetAttachment->sendInitialUpdate();
    }

    fieldPanel.addAndMakeVisible (field);
    field.setTooltip ("Drag: gravity (x) and scatter (y). Double-click resets.");
    {
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
    }

    cluster = std::make_unique<ParamPanel> (processor, "Bend", "Deformation detail", Theme::violet, std::vector<Param> {}, 3, "Evolve");
    addChildComponent (*cluster);
    showTab (0);
    startTimerHz (24);
}

void EvolvePage::bindAmount (int op)
{
    selectedOperator = juce::jlimit (0, 3, op);
    static const Param ops[] = { Param::evolveBend, Param::evolveMelt, Param::evolveTear, Param::evolveMagnet };
    const auto param = ops[selectedOperator];
    amountAttachment.reset();
    amountAttachment = std::make_unique<SliderAttachment> (processor.parameters(), ParameterRegistry::get (param).id, amount);
    amount.setDoubleClickReturnValue (true, ParameterRegistry::get (param).defaultValue);
    amount.setLabel ("Amount");
    amount.setTooltip (paramTooltip (param));
}

void EvolvePage::showTab (int index)
{
    current = juce::jlimit (0, 2, index);
    sidebar.setSelected (current, juce::dontSendNotification);
    operatorPanel.setVisible (current == 0);
    magnetPanel.setVisible (current == 1);
    fieldPanel.setVisible (current != 1);
    cluster->setVisible (current != 0);
    if (current == 1)
    {
        cluster->setTitle ("Bend");
        cluster->setSubtitle ("Deformation detail");
        cluster->setParams ({ Param::evolveBendPivot, Param::evolveBendRange, Param::evolveBendCurve,
                              Param::evolveCrush, Param::evolveFreeze }, 3, "Bend");
        cluster->setFooterRows ({}, "Evolve");
    }
    else if (current == 2)
    {
        cluster->setTitle ("Motion");
        cluster->setSubtitle ("Speed of change");
        cluster->setParams ({ Param::evolveSpeed, Param::evolveMotion, Param::evolveScatterSeed }, 3, "Evolve");
        cluster->setFooterRows ({}, "Evolve");
    }
    resized();
}

void EvolvePage::resized()
{
    const int pad = pagePad (*this), gap = pad;
    auto area = getLocalBounds().reduced (pad, pad / 2);
    const auto cols = pageColumns (area, gap, 0.155f, 0.325f);

    sidebarPanel.setBounds (cols.sidebar);
    layoutSidebar (sidebarPanel, sidebar, 3, true, {}, gap);
    ribbon.setBounds (cols.centre);

    auto right = cols.controls;
    auto layoutField = [this] (juce::Rectangle<int> bounds)
    {
        fieldPanel.setBounds (bounds);
        // The pad centres a square plot inside what it is given, after reserving a band
        // on its left for the SCATTER caption — so it is shifted half that band left and
        // the square lands in the middle of the panel rather than right of it.
        auto c = fieldPanel.contentBounds();
        const int caption = juce::jlimit (15, 26, juce::roundToInt ((float) juce::jmin (c.getWidth(), c.getHeight()) * 0.075f));
        field.setBounds (c.translated (-caption / 2, 0));
    };
    if (current == 0)
    {
        // The operators need a row of knobs and two sliders and no more; the rest of
        // the column goes to the field, whose pad is square and wants the height.
        operatorPanel.setBounds (right.removeFromTop (juce::roundToInt ((float) right.getHeight() * 0.46f)));
        right.removeFromTop (gap);
        layoutField (right);
        auto c = operatorPanel.contentBounds();
        // A row of four knobs and the two sliders under them, kept together as one
        // block in the middle of the panel: the slack becomes an even border
        // instead of a hole under the operators.
        const int cellW = c.getWidth() / 4;
        const int knobH = juce::jmin (c.getHeight() / 2, juce::roundToInt ((float) cellW * 1.30f));
        const int sliderH = juce::jlimit (26, 44, c.getHeight() / 9);
        const int blockH = knobH + gap * 2 + sliderH * 2 + gap;
        c = c.withSizeKeepingCentre (c.getWidth(), juce::jmin (c.getHeight(), blockH));
        std::vector<juce::Component*> knobs;
        for (auto& k : operatorKnobs) knobs.push_back (&k->component());
        layoutGrid (c.removeFromTop (knobH), knobs, 4, gap / 3, 0);
        c.removeFromTop (gap * 2);
        amount.setBounds (c.removeFromTop (sliderH));
        c.removeFromTop (gap);
        speed.setBounds (c.removeFromTop (sliderH));
        return;
    }

    const int clusterH = current == 1 ? juce::roundToInt ((float) right.getHeight() * 0.46f)
                                      : juce::roundToInt ((float) right.getHeight() * 0.34f);
    cluster->setBounds (right.removeFromTop (clusterH));
    right.removeFromTop (gap);
    if (current == 1)
    {
        magnetPanel.setBounds (right);
        magnetList->setBounds (magnetPanel.contentBounds());
    }
    else
    {
        layoutField (right);
    }
}

void EvolvePage::timerCallback()
{
    if (! isShowing()) return;
    const auto values = processor.currentParamValues();
    const auto& vs = processor.diagnostics().visualSnapshots.latest();
    ribbon.setOperators (paramValue (values, Param::evolveBend), paramValue (values, Param::evolveMelt),
                         paramValue (values, Param::evolveTear), paramValue (values, Param::evolveMagnet),
                         paramValue (values, Param::evolveSpeed));
    ribbon.setEnergy (juce::jlimit (0.0f, 1.0f, vs.rmsL * 4.0f));
    ribbon.advance (1.0f / 24.0f);
    field.setEnergy (juce::jlimit (0.0f, 1.0f, vs.rmsL * 4.0f));

    float total = 0.0f;
    for (auto p : { Param::evolveBend, Param::evolveMelt, Param::evolveTear, Param::evolveMagnet }) total += paramValue (values, p);
    sidebarPanel.setActivity (juce::jlimit (0.0f, 1.0f, total * 0.5f));
    operatorPanel.setActivity (juce::jlimit (0.0f, 1.0f, total * 0.5f));

    const auto& mod = latestModulation (processor);
    for (auto& k : operatorKnobs) if (k != nullptr) k->refreshModRing (mod);
    if (cluster != nullptr) cluster->refreshModRings (mod);
}

//==============================================================================
FracturePage::FracturePage (AntiMatrProcessor& p)
    : processor (p),
      sidebar ({ "Main", "Sequencer", "Fragments" }, Theme::magenta)
{
    addAndMakeVisible (sidebarPanel);
    addAndMakeVisible (shards);
    addChildComponent (steps);
    sidebarPanel.addAndMakeVisible (sidebar);
    sidebarPanel.addAndMakeVisible (onOff);
    sidebar.setIcons ({ Icon::Fracture, Icon::Grid, Icon::Shuffle });
    sidebar.setDescriptions ({ "The spectral engine: mode, amount and spread.",
                               "The fragment pattern and how it runs.",
                               "Fragment count, tone, feedback and decay." });
    sidebar.setTooltip ("MAIN: the spectral engine. SEQUENCER: the fragment pattern. FRAGMENTS: tone, feedback and decay.");
    sidebar.onChange = [this] (int i) { showTab (i); };
    shards.setTitle ("Fracture / shards");

    onOff.setTooltip ("Fracture on / off");
    auto* onParam = processor.parameters().getParameter (ParameterRegistry::get (Param::fractureOn).id);
    onAttachment = std::make_unique<juce::ParameterAttachment> (*onParam, [this] (float v) { onOff.setSelected (v >= 0.5f ? 1 : 0, juce::dontSendNotification); });
    onOff.onChange = [this] (int i) { onAttachment->setValueAsCompleteGesture (i == 1 ? 1.0f : 0.0f); };
    onAttachment->sendInitialUpdate();

    cluster = std::make_unique<ParamPanel> (processor, "Spectral", "Amount, motion & tone", Theme::magenta,
                                            std::vector<Param> {}, 2, "Fracture");
    addAndMakeVisible (*cluster);

    steps.setTooltip ("Drag to draw the fragment pattern. Right-click for pattern tools.");
    auto* stepsParam = processor.parameters().getParameter (ParameterRegistry::get (Param::fractureSteps).id);
    stepsAttachment = std::make_unique<juce::ParameterAttachment> (*stepsParam, [this] (float v) { steps.setNumSteps ((int) std::lround (v)); });
    stepsAttachment->sendInitialUpdate();

    // The step editor edits the gate of each sequencer step in the processor's FractureTable
    // (published to the engine on every change); presets and A/B flow back through the timer.
    pullStepsFromProcessor();
    steps.onStepChanged = [this] (int, float) { pushStepsToProcessor(); };
    steps.onPatternChanged = [this] { pushStepsToProcessor(); };

    showTab (0);
    startTimerHz (30);
}

void FracturePage::showTab (int index)
{
    current = juce::jlimit (0, 2, index);
    sidebar.setSelected (current, juce::dontSendNotification);
    steps.setVisible (current == 1);
    switch (current)
    {
        case 1:
            cluster->setDisplay (nullptr, 0.0f);
            spectrum.setVisible (false);
            cluster->setTitle ("Sequencer");
            cluster->setSubtitle ("Fragment steps");
            cluster->setTopRows ({}, "Fracture");
            cluster->setParams ({ Param::fractureSteps, Param::fractureRate, Param::fractureSync,
                                  Param::fractureDivision, Param::fractureSwing, Param::fractureDirection,
                                  Param::fractureProbability, Param::fractureSeed, Param::fractureRetrig }, 3, "Fracture");
            cluster->setFooterRows ({}, "Fracture");
            break;
        case 2:
            cluster->setDisplay (nullptr, 0.0f);
            spectrum.setVisible (false);
            cluster->setTitle ("Fragments");
            cluster->setSubtitle ("Tone, feedback & decay");
            cluster->setTopRows ({ { Param::fractureFragments } }, "Fracture");
            cluster->setParams ({ Param::fractureMix, Param::fractureFeedback, Param::fracturePitch,
                                  Param::fractureDelay, Param::fractureDecay, Param::fractureTone,
                                  Param::fractureEvolve }, 2, "Fracture");
            cluster->setFooterRows ({}, "Fracture");
            break;
        default:
            cluster->setTitle ("Spectral");
            cluster->setSubtitle ("Amount, motion & tone");
            cluster->setTopRows ({ { Param::fractureMode } }, "Fracture");
            cluster->setParams ({ Param::fractureAmount, Param::fractureSpread,
                                  Param::fractureSequence, Param::fractureRandom }, 2, "Fracture");
            cluster->setFooterRows ({}, "Fracture");
            spectrum.setVisible (true);
            cluster->setDisplay (&spectrum, 0.32f);
            break;
    }
    const juce::Colour accents[] = { Theme::magenta, Theme::cyan, Theme::violet, Theme::ivory };
    const Param sp[] = { Param::fractureAmount, Param::fractureSpread, Param::fractureSequence, Param::fractureRandom };
    for (int i = 0; i < 4; ++i) cluster->setAccentFor (sp[i], accents[i]);
    resized();
}

void FracturePage::pushStepsToProcessor()
{
    if (pushing) return;
    pushing = true;
    auto table = processor.getFractureTable();
    for (int i = 0; i < kMaxSequencerSteps; ++i)
    {
        const float g = juce::jlimit (0.0f, 1.0f, steps.getStep (i));
        table.steps[(size_t) i].gate = g;
        shownGates[(size_t) i] = g;
    }
    processor.setFractureTable (table);
    pushing = false;
}

void FracturePage::pullStepsFromProcessor()
{
    const auto& table = processor.getFractureTable();
    bool changed = false;
    std::vector<float> values ((size_t) kMaxSequencerSteps);
    for (int i = 0; i < kMaxSequencerSteps; ++i)
    {
        const float g = juce::jlimit (0.0f, 1.0f, table.steps[(size_t) i].gate);
        values[(size_t) i] = g;
        if (std::abs (g - shownGates[(size_t) i]) > 1.0e-4f) changed = true;
        shownGates[(size_t) i] = g;
    }
    if (changed)
    {
        pushing = true;                                   // the editor's callbacks must not echo this back
        steps.setSteps (values, juce::dontSendNotification);
        pushing = false;
    }
}

void FracturePage::resized()
{
    const int pad = pagePad (*this), gap = pad;
    auto area = getLocalBounds().reduced (pad, pad / 2);
    const auto cols = pageColumns (area, gap);

    sidebarPanel.setBounds (cols.sidebar);
    layoutSidebar (sidebarPanel, sidebar, 3, true, { &onOff }, gap);

    auto centre = cols.centre;
    if (current == 1)
    {
        // The pattern belongs under the shards: it needs the width, and the screen
        // above it is what the pattern is doing to the object.
        steps.setBounds (centre.removeFromBottom (juce::jlimit (90, 190, juce::roundToInt ((float) centre.getHeight() * 0.30f))));
        centre.removeFromBottom (gap);
    }
    shards.setBounds (centre);
    cluster->setBounds (cols.controls);
}

void FracturePage::timerCallback()
{
    if (! isShowing()) return;
    auto& diag = processor.diagnostics();
    const auto& vs = diag.visualSnapshots.latest();
    const auto values = processor.currentParamValues();

    shards.setFracture (8 << juce::jlimit (0, 2, paramChoice (values, Param::fractureFragments)),
                        paramValue (values, Param::fractureAmount), paramValue (values, Param::fractureSpread),
                        paramValue (values, Param::fractureRandom), vs.fractureOn,
                        (int) paramValue (values, Param::fractureSeed));
    shards.setEnergy (vs.fractureOn ? juce::jlimit (0.0f, 1.0f, vs.fractureActivity) : juce::jlimit (0.0f, 1.0f, vs.rmsL * 3.0f));
    shards.setCaption (paramChoices (Param::fractureMode)[juce::jlimit (0, 3, paramChoice (values, Param::fractureMode))]);
    shards.advance (1.0f / 30.0f);
    sidebarPanel.setActivity (vs.fractureOn ? vs.fractureActivity * 0.8f : 0.0f);

    if (spectrum.isVisible())
    {
        const auto stage = vs.fractureOn ? Stage::PostFracture : Stage::PostMatter;
        diag.taps[(int) stage].readLatest (tapL.data(), tapR.data(), SpectrumAnalyzer::kSize);
        for (int i = 0; i < SpectrumAnalyzer::kSize; ++i) tapL[(size_t) i] = 0.5f * (tapL[(size_t) i] + tapR[(size_t) i]);
        analyzer.compute (tapL.data(), processor.engine().sampleRate(), bands.data(), AMSpectrumView::kBands);
        spectrum.setFragmentCount (8 << juce::jlimit (0, 2, paramChoice (values, Param::fractureFragments)));
        spectrum.setActivity (vs.fractureOn ? vs.fractureActivity : 0.0f);
        spectrum.setMagnitudes (bands.data(), AMSpectrumView::kBands);
    }

    const auto& mod = latestModulation (processor);
    cluster->refreshModRings (mod);

    if (current != 1) return;

    // Presets / A/B / undo may have replaced the table underneath the editor.
    if (! steps.isMouseButtonDown()) pullStepsFromProcessor();

    // Playhead: the engine's current sequencer step (wait-free), smoothed within the step from the
    // sample clock so the glow travels instead of jumping.
    FractureEngine::FragmentActivity act;
    processor.engine().fractureEngine().fillFragmentActivity (act);
    const int n = juce::jmax (1, steps.getNumSteps());
    const double sr = juce::jmax (1.0, processor.engine().sampleRate());
    const uint64_t now = vs.sampleTime;
    if (act.currentStep != lastEngineStep)
    {
        lastEngineStep = act.currentStep;
        playheadPhase = 0.0;
        lastSampleTime = now;
    }
    else if (lastSampleTime != 0 && now > lastSampleTime)
    {
        double stepsPerSecond = paramValue (values, Param::fractureRate);
        if (paramBool (values, Param::fractureSync))
        {
            static const double beats[] = { 4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0, 1.5, 0.75, 0.375 };
            stepsPerSecond = 2.0 / beats[juce::jlimit (0, 11, paramChoice (values, Param::fractureDivision))];
        }
        playheadPhase = juce::jmin (0.95, playheadPhase + (double) (now - lastSampleTime) / sr * stepsPerSecond);
        lastSampleTime = now;
    }
    const float position = (float) ((act.currentStep % n) + playheadPhase);
    steps.setPlayhead (vs.fractureOn && vs.activeVoices > 0 ? position : -1.0f);
}

//==============================================================================
SpacePage::SpacePage (AntiMatrProcessor& p)
    : processor (p),
      sidebar ({ "Nebula", "Void", "Chamber", "Orbit", "Dream", "Machine", "Shimmer", "Dust" }, Theme::ivory)
{
    addAndMakeVisible (sidebarPanel);
    addAndMakeVisible (space);
    addAndMakeVisible (macroPanel);
    addAndMakeVisible (enginePanel);
    sidebarPanel.addAndMakeVisible (sidebar);
    sidebar.setIcons ({ Icon::Swirl, Icon::Space, Icon::Grid, Icon::Magnet,
                        Icon::Sparkle, Icon::Settings, Icon::Dna, Icon::Dust });
    sidebar.setTooltip ("The environment the sound lives in.");
    space.setTitle ("Space");

    auto& apvts = processor.parameters();
    auto* typeParam = apvts.getParameter (ParameterRegistry::get (Param::spaceType).id);
    typeAttachment = std::make_unique<juce::ParameterAttachment> (*typeParam, [this] (float v)
    {
        const int t = (int) std::lround (v);
        sidebar.setSelected (t, juce::dontSendNotification);
        space.setType (t);
        space.setAccent (SpaceArt::tint (t));
        space.setCaption (SpaceArt::name (t));
    });
    sidebar.onChange = [this] (int i) { typeAttachment->setValueAsCompleteGesture ((float) i); };
    typeAttachment->sendInitialUpdate();

    const juce::Colour accents[] = { Theme::magenta, Theme::cyan, Theme::violet, Theme::ivory };
    const Param mp[] = { Param::spaceMix, Param::spaceSize, Param::spaceTone, Param::spaceFeedback };
    for (int i = 0; i < 4; ++i)
    {
        macros.push_back (std::make_unique<BoundControl> (apvts, mp[i], accents[i]));
        if (auto* k = macros.back()->knob()) k->setHero (true);
        macroPanel.addAndMakeVisible (macros.back()->component());
    }

    // The rack: every module in the space chain, in the order the signal meets them.
    // The pip on a tile powers its module; the rest of the tile brings its controls up.
    auto addModule = [this] (const juce::String& name, Icon icon, std::optional<Param> power,
                             std::vector<Param> params, const juce::String& prefix)
    {
        auto m = std::make_unique<Module>();
        m->name = name;
        m->prefix = prefix.isNotEmpty() ? prefix : name;
        m->power = power;
        m->params = std::move (params);
        m->tile = std::make_unique<AMModuleTile> (name, icon, Theme::ivory);
        m->tile->setHasPower (power.has_value());
        m->tile->setTooltip (name + ": click to bring its controls up, click the lamp to switch it "
                             + (power.has_value() ? "on and off." : "— always on."));
        const int index = (int) modules.size();
        m->tile->onSelect = [this, index] { selectModule (index); };
        if (power.has_value())
        {
            auto* raw = m->tile.get();
            auto* param = processor.parameters().getParameter (ParameterRegistry::get (*power).id);
            m->attachment = std::make_unique<juce::ParameterAttachment> (*param, [raw] (float v) { raw->setPowered (v >= 0.5f); });
            auto* att = m->attachment.get();
            m->tile->onPower = [att] (bool on) { att->setValueAsCompleteGesture (on ? 1.0f : 0.0f); };
            m->attachment->sendInitialUpdate();
        }
        enginePanel.addAndMakeVisible (*m->tile);
        modules.push_back (std::move (m));
    };

    addModule ("Diffusion", Icon::Swirl,    Param::spaceDiffuseOn, { Param::spaceDiffuseAmount }, "Diffusion");
    addModule ("Delay",     Icon::Lfo,      Param::spaceDelayOn,   { Param::spaceDelayTime, Param::spaceDelaySync, Param::spaceDelayFeedback, Param::spaceDelayTone, Param::spaceDelayMix }, "Delay");
    addModule ("Reverb",    Icon::Space,    Param::spaceReverbOn,  { Param::spaceReverbSize, Param::spaceReverbDecay, Param::spaceReverbDamp, Param::spaceReverbPredelay, Param::spaceReverbMod, Param::spaceReverbMix }, "Reverb");
    addModule ("Spectral",  Icon::Dna,      Param::spaceShiftOn,   { Param::spaceShiftAmount, Param::spaceShiftMix, Param::spaceGrainOn, Param::spaceGrainSize, Param::spaceGrainDensity, Param::spaceGrainPitch, Param::spaceGrainMix }, " ");
    addModule ("Width",     Icon::Scatter,  Param::spaceChorusOn,  { Param::spaceChorusRate, Param::spaceChorusDepth, Param::spaceChorusMix }, "Chorus");
    addModule ("Comp",      Icon::Macro,    Param::spaceCompOn,    { Param::spaceCompAmount, Param::spaceLimiterOn }, " ");
    addModule ("Drive",     Icon::Impact,   Param::spaceDistOn,    { Param::spaceDistMode, Param::spaceDistDrive, Param::spaceDistMix }, "Distortion");
    addModule ("EQ",        Icon::Env,      std::nullopt,          { Param::spaceEqLow, Param::spaceEqMid, Param::spaceEqHigh }, "EQ");

    moduleControls = std::make_unique<ParamPanel> (processor, "Diffusion", "", Theme::ivory, std::vector<Param> {}, 0, "Diffusion");
    addAndMakeVisible (*moduleControls);
    selectModule (2);                                   // REVERB is on by default, so it opens on it
    startTimerHz (24);
}

void SpacePage::selectModule (int index)
{
    selectedModule = juce::jlimit (0, (int) modules.size() - 1, index);
    for (int i = 0; i < (int) modules.size(); ++i) modules[(size_t) i]->tile->setSelected (i == selectedModule);
    auto& m = *modules[(size_t) selectedModule];
    moduleControls->setTitle (m.name);
    moduleControls->setSubtitle (m.power.has_value() ? "Module controls" : "Always on");
    moduleControls->setParams (m.params, 0, m.prefix);
    if (m.power.has_value()) moduleControls->setHeaderToggle (*m.power, true);
    else moduleControls->clearHeaderToggle();
    resized();
}

void SpacePage::resized()
{
    const int pad = pagePad (*this), gap = pad;
    auto area = getLocalBounds().reduced (pad, pad / 2);
    const auto cols = pageColumns (area, gap, 0.155f, 0.315f);

    sidebarPanel.setBounds (cols.sidebar);
    layoutSidebar (sidebarPanel, sidebar, 8, false, {}, gap);
    space.setBounds (cols.centre);

    auto right = cols.controls;
    macroPanel.setBounds (right.removeFromTop (juce::roundToInt ((float) right.getHeight() * 0.30f)));
    {
        std::vector<juce::Component*> comps;
        for (auto& m : macros) comps.push_back (&m->component());
        layoutGrid (macroPanel.contentBounds(), comps, 4);
    }
    right.removeFromTop (gap);

    const int n = (int) modules.size();
    const int perRow = 4;
    const int tileRows = (n + perRow - 1) / perRow;
    const int tileH = juce::jlimit (46, 72, right.getWidth() / 8);
    enginePanel.setBounds (right.removeFromTop (tileH * tileRows + juce::jlimit (34, 54, right.getHeight() / 9)));
    {
        auto c = enginePanel.contentBounds();
        const int cellW = c.getWidth() / perRow;
        const int h = juce::jmin (c.getHeight() / tileRows, juce::roundToInt ((float) cellW * 0.78f));
        const int top = c.getY() + juce::jmax (0, (c.getHeight() - h * tileRows) / 2);
        for (int i = 0; i < n; ++i)
            modules[(size_t) i]->tile->setBounds (c.getX() + (i % perRow) * cellW, top + (i / perRow) * h, cellW, h);
    }
    right.removeFromTop (gap);
    moduleControls->setBounds (right);
}

void SpacePage::timerCallback()
{
    if (! isShowing()) return;
    const auto& vs = processor.diagnostics().visualSnapshots.latest();
    // The space never looks asleep: it keeps a floor of light even in silence, and
    // what the instrument is playing brightens it from there.
    space.setEnergy (0.45f + 0.55f * juce::jlimit (0.0f, 1.0f, vs.rmsL * 3.0f));
    space.advance (1.0f / 24.0f);
    sidebarPanel.setActivity (vs.spaceActivity * 0.5f);
    macroPanel.setActivity (vs.spaceActivity * 0.5f);

    const auto& mod = latestModulation (processor);
    for (auto& m : macros) m->refreshModRing (mod);
    if (moduleControls != nullptr) moduleControls->refreshModRings (mod);
}
//==============================================================================
ModPage::ModPage (AntiMatrProcessor& p) : processor (p)
{
    addAndMakeVisible (tabs);
    tabs.setStyle (AMTab::Style::Strip);
    tabs.onChange = [this] (int i) { showTab (i); };

    auto panel = [this] (std::vector<std::unique_ptr<ParamPanel>>& list, const juce::String& title, const juce::String& subtitle, std::vector<Param> params, int cols = 0, const juce::String& prefix = {})
    {
        auto pp = std::make_unique<ParamPanel> (processor, title, subtitle, Theme::amber, std::move (params), cols, prefix);
        addChildComponent (*pp);
        list.push_back (std::move (pp));
    };
    routings = std::make_unique<ModRoutingPanel> (processor);
    addChildComponent (*routings);

    tabPanels.resize (6);
    panel (tabPanels[1], "LFO 1", "Low frequency oscillator", { Param::lfo1Rate, Param::lfo1Shape, Param::lfo1Sync, Param::lfo1Division, Param::lfo1Phase, Param::lfo1Symmetry, Param::lfo1Depth, Param::lfo1Retrig, Param::lfo1Fade }, 5);
    panel (tabPanels[1], "LFO 2", "", { Param::lfo2Rate, Param::lfo2Shape, Param::lfo2Sync, Param::lfo2Division, Param::lfo2Phase, Param::lfo2Symmetry, Param::lfo2Depth, Param::lfo2Retrig, Param::lfo2Fade }, 5);
    panel (tabPanels[1], "LFO 3", "", { Param::lfo3Rate, Param::lfo3Shape, Param::lfo3Sync, Param::lfo3Division, Param::lfo3Phase, Param::lfo3Symmetry, Param::lfo3Depth, Param::lfo3Retrig, Param::lfo3Fade }, 5);
    panel (tabPanels[1], "LFO 4", "", { Param::lfo4Rate, Param::lfo4Shape, Param::lfo4Sync, Param::lfo4Division, Param::lfo4Phase, Param::lfo4Symmetry, Param::lfo4Depth, Param::lfo4Retrig, Param::lfo4Fade }, 5);
    panel (tabPanels[2], "Envelope 1", "Modulation envelope", { Param::env1Attack, Param::env1Decay, Param::env1Sustain, Param::env1Release, Param::env1Curve, Param::env1Loop }, 6, "Env 1");
    panel (tabPanels[2], "Envelope 2", "", { Param::env2Attack, Param::env2Decay, Param::env2Sustain, Param::env2Release, Param::env2Curve, Param::env2Loop }, 6, "Env 2");
    panel (tabPanels[2], "Envelope 3", "", { Param::env3Attack, Param::env3Decay, Param::env3Sustain, Param::env3Release, Param::env3Curve, Param::env3Loop }, 6, "Env 3");
    panel (tabPanels[2], "Envelope 4", "", { Param::env4Attack, Param::env4Decay, Param::env4Sustain, Param::env4Release, Param::env4Curve, Param::env4Loop }, 6, "Env 4");
    panel (tabPanels[3], "Chaos 1", "Unstable generator", { Param::chaos1Type, Param::chaos1Rate, Param::chaos1Depth, Param::chaos1Stability, Param::chaos1Symmetry, Param::chaos1Seed }, 6);
    panel (tabPanels[3], "Chaos 2", "", { Param::chaos2Type, Param::chaos2Rate, Param::chaos2Depth, Param::chaos2Stability, Param::chaos2Symmetry, Param::chaos2Seed }, 6);
    panel (tabPanels[3], "Chaos 3", "", { Param::chaos3Type, Param::chaos3Rate, Param::chaos3Depth, Param::chaos3Stability, Param::chaos3Symmetry, Param::chaos3Seed }, 6);
    panel (tabPanels[3], "Chaos 4", "", { Param::chaos4Type, Param::chaos4Rate, Param::chaos4Depth, Param::chaos4Stability, Param::chaos4Symmetry, Param::chaos4Seed }, 6);
    panel (tabPanels[4], "Macros", "Eight performance controls", { Param::macro1, Param::macro2, Param::macro3, Param::macro4, Param::macro5, Param::macro6, Param::macro7, Param::macro8 }, 4);
    tabPanels[4].back()->setHeroKnobs (true);
    panel (tabPanels[5], "Amp", "Amplitude envelope", { Param::ampAttack, Param::ampDecay, Param::ampSustain, Param::ampRelease, Param::ampCurve, Param::ampVelocity }, 6);
    panel (tabPanels[5], "Master", "Voices, tuning & output", { Param::masterGain, Param::masterVoices, Param::masterQuality, Param::masterMode, Param::masterGlide, Param::masterBendRange, Param::masterTranspose, Param::masterFine }, 4);
    // Every envelope panel shows the shape its knobs are making.
    {
        const Param sets[][5] =
        {
            { Param::env1Attack, Param::env1Decay, Param::env1Sustain, Param::env1Release, Param::env1Curve },
            { Param::env2Attack, Param::env2Decay, Param::env2Sustain, Param::env2Release, Param::env2Curve },
            { Param::env3Attack, Param::env3Decay, Param::env3Sustain, Param::env3Release, Param::env3Curve },
            { Param::env4Attack, Param::env4Decay, Param::env4Sustain, Param::env4Release, Param::env4Curve },
        };
        for (int i = 0; i < 4; ++i)
        {
            juce::ignoreUnused (sets);
            envelopeViews.push_back (std::make_unique<AMEnvelopeView> (Theme::amber));
            tabPanels[2][(size_t) i]->setDisplay (envelopeViews.back().get(), 0.46f);
        }
        envelopeViews.push_back (std::make_unique<AMEnvelopeView> (Theme::amber));
        tabPanels[5][0]->setDisplay (envelopeViews.back().get(), 0.44f);
    }

    showTab (0);
    startTimerHz (20);
}

void ModPage::timerCallback()
{
    if (! isShowing()) return;
    const auto& mod = latestModulation (processor);
    for (auto& pp : tabPanels[(size_t) current]) pp->refreshModRings (mod);

    if (current != 2 && current != 5) return;
    static const Param sets[][5] =
    {
        { Param::env1Attack, Param::env1Decay, Param::env1Sustain, Param::env1Release, Param::env1Curve },
        { Param::env2Attack, Param::env2Decay, Param::env2Sustain, Param::env2Release, Param::env2Curve },
        { Param::env3Attack, Param::env3Decay, Param::env3Sustain, Param::env3Release, Param::env3Curve },
        { Param::env4Attack, Param::env4Decay, Param::env4Sustain, Param::env4Release, Param::env4Curve },
        { Param::ampAttack,  Param::ampDecay,  Param::ampSustain,  Param::ampRelease,  Param::ampCurve },
    };
    const auto values = processor.currentParamValues();
    const auto& vs = processor.diagnostics().visualSnapshots.latest();
    const float activity = juce::jlimit (0.0f, 1.0f, (float) vs.activeVoices * 0.5f);
    const int from = current == 2 ? 0 : 4, to = current == 2 ? 4 : 5;
    for (int i = from; i < to && i < (int) envelopeViews.size(); ++i)
    {
        envelopeViews[(size_t) i]->setEnvelope (paramValue (values, sets[i][0]), paramValue (values, sets[i][1]),
                                                paramValue (values, sets[i][2]), paramValue (values, sets[i][3]),
                                                paramValue (values, sets[i][4]));
        envelopeViews[(size_t) i]->setActivity (activity);
    }
}

void ModPage::showTab (int index)
{
    current = juce::jlimit (0, (int) tabPanels.size() - 1, index);
    tabs.setSelected (current, juce::dontSendNotification);   // the bar and the content can never disagree
    for (int t = 0; t < (int) tabPanels.size(); ++t)
        for (auto& pp : tabPanels[(size_t) t]) pp->setVisible (t == current);
    if (routings != nullptr) routings->setVisible (current == 0);
    resized();
}

void ModPage::resized()
{
    const int pad = pagePad (*this), gap = pad;
    auto area = getLocalBounds().reduced (pad, pad / 2);
    tabs.setBounds (area.removeFromTop (juce::jlimit (28, 38, area.getHeight() / 16)).withSizeKeepingCentre (juce::jmin (area.getWidth(), 640), juce::jlimit (28, 38, area.getHeight() / 16)));
    area.removeFromTop (gap);
    if (current == 0)
    {
        if (routings != nullptr)
            routings->setBounds (area.withSizeKeepingCentre (juce::jmin (area.getWidth(), 1180), area.getHeight()));
        return;
    }
    auto& list = tabPanels[(size_t) current];
    if (list.empty()) return;
    if (list.size() == 1) { list[0]->setBounds (area.withSizeKeepingCentre (juce::jmin (area.getWidth(), 1280), juce::jmin (area.getHeight(), 560))); return; }
    if (list.size() == 2)
    {
        // Weighted by content: a panel with one row of knobs does not need the same
        // height as one with two, plus a curve display.
        const float w0 = (float) list[0]->rowsNeeded() + (list[0]->hasDisplay() ? 1.5f : 0.0f) + 0.7f;
        const float w1 = (float) list[1]->rowsNeeded() + (list[1]->hasDisplay() ? 1.5f : 0.0f) + 0.7f;
        auto top = area.removeFromTop (juce::roundToInt ((float) (area.getHeight() - gap) * w0 / juce::jmax (0.1f, w0 + w1)));
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

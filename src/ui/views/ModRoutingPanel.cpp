#include "ModRoutingPanel.h"
#include "ui/UILayout.h"

namespace am::ui
{

namespace
{
    /** "SHAPE  DECAY" — the group plus the parameter, so a target reads unambiguously. */
    juce::String targetName (Param p)
    {
        const auto& d = ParameterRegistry::get (p);
        return juce::String (ParameterRegistry::groupName (d.group)).toUpperCase() + "  " + juce::String (d.name).toUpperCase();
    }

    /** The section colour of a destination, so a routing row is coloured by where it lands. */
    juce::Colour destinationColour (Param p)
    {
        switch (ParameterRegistry::get (p).group)
        {
            case ParamGroup::Source:
            case ParamGroup::Wave:
            case ParamGroup::Dust:
            case ParamGroup::Impact:
            case ParamGroup::Sample:
            case ParamGroup::Gesture:  return Theme::accentFor (Theme::Section::Source);
            case ParamGroup::Shape:    return Theme::accentFor (Theme::Section::Shape);
            case ParamGroup::Evolve:   return Theme::accentFor (Theme::Section::Evolve);
            case ParamGroup::Fracture: return Theme::accentFor (Theme::Section::Fracture);
            case ParamGroup::Space:    return Theme::accentFor (Theme::Section::Space);
            default:                   return Theme::accentFor (Theme::Section::Mod);
        }
    }

    /** Depth as a share of the target's range, in the target's own units. */
    juce::String depthText (const ModRouting& r)
    {
        const auto& d = ParameterRegistry::get (r.target);
        return layout::modDepthText (r.depth, d.min, d.max, d.unit);
    }
}

//==============================================================================
ModRowLayout ModRowLayout::forRow (juce::Rectangle<int> row) noexcept
{
    const auto c = layout::modRowColumns (row.getWidth(), row.getHeight());
    const int y = row.getY(), h = row.getHeight(), x = row.getX();
    auto full = [x, y, h] (const layout::Span& s) { return juce::Rectangle<int> (x + s.start, y, s.size, h); };

    ModRowLayout l;
    l.chip        = full (c.chip).reduced (0, c.chipInsetY);
    l.source      = full (c.source);
    l.arrow       = full (c.arrow);
    l.destination = full (c.destination);
    l.depth       = full (c.depth);
    l.value       = full (c.value);
    l.power       = full (c.power);
    l.polarity    = full (c.polarity).withSizeKeepingCentre (c.polarity.size, c.polarityHeight);
    l.remove      = full (c.remove);
    return l;
}

int modScopeColumns (int count, int width) noexcept
{
    return layout::modScopeColumns (count, width);
}

//==============================================================================
/** Bipolar depth bar: filled from the centre in the destination's colour, with a live overlay. */
class DepthBar : public juce::Slider
{
public:
    DepthBar()
    {
        setSliderStyle (juce::Slider::LinearHorizontal);
        setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        setRange (-1.0, 1.0, 0.001);
        setDoubleClickReturnValue (true, 0.0);
        setWantsKeyboardFocus (false);
        setRepaintsOnMouseActivity (true);
    }

    void setTint (juce::Colour c) { tint = c; repaint(); }
    void setDimmed (bool d) { if (d != dimmed) { dimmed = d; repaint(); } }
    /** Live contribution of the source, -1 .. 1. */
    void setActivity (float a)
    {
        const float v = std::isfinite (a) ? juce::jlimit (-1.0f, 1.0f, a) : 0.0f;
        if (std::abs (v - activity) < 0.01f) return;
        activity = v;
        repaint();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        setMouseDragSensitivity (e.mods.isShiftDown() ? 1400 : 260);
        juce::Slider::mouseDown (e);
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        setMouseDragSensitivity (e.mods.isShiftDown() ? 1400 : 260);
        juce::Slider::mouseDrag (e);
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float trackH = juce::jlimit (6.0f, 16.0f, b.getHeight() * 0.46f);
        auto track = b.withSizeKeepingCentre (b.getWidth(), trackH);
        const float corner = trackH * 0.5f;
        const float alpha = dimmed ? 0.4f : 1.0f;
        const float lit = (isMouseOverOrDragging() ? 1.0f : 0.0f);

        // A depth slider is a control, not a display: a light capsule cut into the
        // row, with the destination's colour filling it (SPEC section 3).
        draw::capsuleTrack (g, track, corner, 1.0f);

        const float centre = track.getCentreX();
        const float half = track.getWidth() * 0.5f - 1.0f;
        const float value = juce::jlimit (-1.0f, 1.0f, (float) getValue());
        const float x = centre + value * half;

        // Set depth: a solid bar from zero, so the sign is visible without reading the number.
        if (std::abs (value) > 0.004f)
        {
            const auto bar = juce::Rectangle<float> (juce::jmin (centre, x), track.getY() + 1.0f,
                                                     std::abs (x - centre), track.getHeight() - 2.0f);
            g.setColour (tint.withAlpha (0.55f * alpha + 0.16f * lit));
            g.fillRoundedRectangle (bar, corner - 1.0f);
            g.setColour (tint.withAlpha (0.85f * alpha));
            g.fillRoundedRectangle (bar.withWidth (juce::jmax (1.5f, bar.getWidth())).removeFromBottom (juce::jmax (1.5f, track.getHeight() * 0.22f)), 1.0f);

            // Live contribution: a slim amber lane inside the bar, so what the source is doing
            // right now is visible without hiding the destination's colour.
            const float live = value * activity;
            if (std::abs (live) > 0.004f)
            {
                const float lx = centre + live * half;
                const float laneH = juce::jmax (2.0f, track.getHeight() * 0.4f);
                const auto now = juce::Rectangle<float> (juce::jmin (centre, lx), track.getCentreY() - laneH * 0.5f,
                                                         std::abs (lx - centre), laneH);
                g.setColour (Theme::amber.withAlpha (0.85f * alpha));
                g.fillRoundedRectangle (now, laneH * 0.5f);
            }
        }

        // Zero tick and value handle.
        g.setColour (Theme::textDim.withAlpha (0.75f * alpha));
        g.fillRect (centre - 0.5f, track.getY() - 1.0f, 1.0f, track.getHeight() + 2.0f);
        g.setColour (Theme::textPrimary.withAlpha ((0.75f + 0.25f * lit) * alpha));
        g.fillRoundedRectangle (x - 1.25f, track.getY() - 1.5f, 2.5f, track.getHeight() + 3.0f, 1.25f);

        g.setColour (tint.withAlpha ((0.18f + 0.3f * lit) * alpha));
        g.drawRoundedRectangle (track.reduced (0.5f), corner, 1.0f);
    }

private:
    juce::Colour tint { Theme::amber };
    float activity = 0.0f;
    bool dimmed = false;
};

//==============================================================================
/** One routing: source to target, depth, polarity, enable and remove. */
class ModRoutingPanel::Row : public juce::Component
{
public:
    Row (ModRoutingPanel& o, int rowIndex) : owner (o), index (rowIndex)
    {
        depth.onValueChange = [this] { owner.setDepth (index, (float) depth.getValue()); };
        depth.setTooltip ("Modulation depth as a share of the destination's range. Drag to set, double-click to zero, shift-drag for fine control.");
        addAndMakeVisible (depth);

        polarity.onChange = [this] (int i) { owner.setBipolar (index, i == 0); };
        polarity.setTooltip ("BI: the source swings both ways around the knob.  UNI: it only adds.");
        addAndMakeVisible (polarity);

        power.onChange = [this] (bool on) { owner.setEnabled (index, on); };
        power.setTooltip ("Mute this routing without deleting it.");
        addAndMakeVisible (power);

        remove.onClick = [this] { owner.removeRouting (index); };
        remove.setTooltip ("Remove this routing.");
        addAndMakeVisible (remove);
    }

    void setRouting (const ModRouting& r, float sourceValue)
    {
        routing = r;
        tint = destinationColour (r.target);
        depth.setValue (r.depth, juce::dontSendNotification);
        depth.setTint (tint);
        depth.setDimmed (! r.enabled);
        polarity.setSelected (r.bipolar ? 0 : 1, juce::dontSendNotification);
        polarity.setAccent (r.enabled ? tint : Theme::textDim);
        power.setToggleState (r.enabled, juce::dontSendNotification);
        power.setAccent (tint);
        setActivity (sourceValue);
        repaint();
    }

    void setActivity (float sourceValue)
    {
        const float v = std::isfinite (sourceValue) ? juce::jlimit (-1.0f, 1.0f, sourceValue) : 0.0f;
        depth.setActivity (routing.enabled ? v : 0.0f);
        if (std::abs (v - activity) < 0.02f) return;
        activity = v;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float corner = juce::jmin (6.0f, b.getHeight() * 0.26f);
        const float alpha = routing.enabled ? 1.0f : 0.42f;
        const auto l = ModRowLayout::forRow (getLocalBounds());

        // Surface: a graphite row washed with a trace of the destination's colour.
        juce::ColourGradient surface (Theme::panelInset.brighter (0.35f).interpolatedWith (tint, 0.07f * alpha), b.getX(), b.getY(),
                                      Theme::panelInset.brighter (0.35f), b.getRight(), b.getY(), false);
        g.setGradientFill (surface);
        g.fillRoundedRectangle (b, corner);
        g.setColour (tint.withAlpha (0.10f * alpha + 0.10f * hover));
        g.drawRoundedRectangle (b.reduced (0.5f), corner, 1.0f);

        // Destination chip: brightness follows what the source is doing right now.
        {
            const auto chip = l.chip.toFloat();
            const float live = routing.enabled ? juce::jlimit (0.0f, 1.0f, std::abs (activity) * std::abs (routing.depth) * 1.6f) : 0.0f;
            if (live > 0.02f) draw::glowRoundedRect (g, chip, chip.getWidth() * 0.5f, tint, chip.getWidth() * 3.0f, live * 0.8f);
            g.setColour (tint.withAlpha ((0.45f + 0.55f * live) * alpha));
            g.fillRoundedRectangle (chip, chip.getWidth() * 0.5f);
        }

        const float h = juce::jlimit (8.5f, 12.0f, b.getHeight() * 0.4f);

        const juce::String sourceText = juce::String (modSourceName (routing.source)).toUpperCase();
        draw::trackedText (g, sourceText, l.source.toFloat(), juce::Justification::centredLeft,
                           draw::fitFont (Theme::labelFontStrong (h), sourceText, (float) l.source.getWidth()),
                           // Amber at full strength is a pale orange on a light row: the
                           // modulation colour has to be deepened to carry as lettering.
                           Theme::amber.darker (0.55f).withAlpha (alpha));

        {
            juce::Path p;
            const auto arrow = l.arrow.toFloat();
            const float s = juce::jmin (arrow.getWidth(), arrow.getHeight()) * 0.2f;
            const auto c = arrow.getCentre();
            p.startNewSubPath (c.x - s, c.y - s);
            p.lineTo (c.x + s * 0.8f, c.y);
            p.lineTo (c.x - s, c.y + s);
            g.setColour (tint.withAlpha (0.6f * alpha));
            g.strokePath (p, juce::PathStrokeType (juce::jmax (1.0f, s * 0.4f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const auto name = targetName (routing.target);
        draw::trackedText (g, name, l.destination.toFloat(), juce::Justification::centredLeft,
                           draw::fitFont (Theme::labelFont (h), name, (float) l.destination.getWidth()),
                           tint.darker (0.3f).interpolatedWith (Theme::textPrimary, 0.45f).withAlpha (alpha));

        draw::trackedText (g, depthText (routing), l.value.toFloat(), juce::Justification::centredRight,
                           Theme::valueFont (h * 0.95f), Theme::textValue.withAlpha (alpha));
    }

    void resized() override
    {
        const auto l = ModRowLayout::forRow (getLocalBounds());
        depth.setBounds (l.depth);
        polarity.setBounds (l.polarity);
        power.setBounds (l.power);
        remove.setBounds (l.remove);
    }

    void mouseEnter (const juce::MouseEvent&) override { hover = 1.0f; repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { hover = 0.0f; repaint(); }

    int index = 0;

private:
    ModRoutingPanel& owner;
    ModRouting routing;
    juce::Colour tint { Theme::amber };
    float activity = 0.0f, hover = 0.0f;
    DepthBar depth;
    AMSegment polarity { { "BI", "UNI" }, Theme::amber };
    AMToggle power { "", Theme::amber };
    AMIconButton remove { Icon::Close, Theme::textSecondary };
};

//==============================================================================
/** Live scope for one modulation source: recent output plus how many destinations it drives. */
class ModRoutingPanel::SourceScope : public juce::Component,
                                     public juce::SettableTooltipClient
{
public:
    SourceScope (ModSource s, int destinations)
        : source (s), targets (destinations), bipolar (modSourceIsBipolar (s))
    {
        setTooltip (juce::String (modSourceName (s)) + " drives " + juce::String (destinations)
                    + (destinations == 1 ? " destination." : " destinations."));
    }

    void push (float v)
    {
        if (! std::isfinite (v)) v = 0.0f;
        history[(size_t) head] = juce::jlimit (-1.0f, 1.0f, v);
        head = (head + 1) % kPoints;
        if (filled < kPoints) ++filled;
        repaint();
    }

    ModSource getSource() const noexcept { return source; }
    int getTargets() const noexcept { return targets; }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float corner = juce::jlimit (4.0f, 8.0f, b.getHeight() * 0.14f);
        draw::insetSurface (g, b, corner);

        const float labelH = juce::jlimit (9.0f, 12.0f, b.getHeight() * 0.2f);
        auto inner = b.reduced (juce::jmax (6.0f, b.getWidth() * 0.05f), juce::jmax (4.0f, b.getHeight() * 0.09f));
        auto caption = inner.removeFromTop (labelH * 1.25f);

        const float v = filled > 0 ? history[(size_t) ((head - 1 + kPoints) % kPoints)] : 0.0f;
        const juce::String name = juce::String (modSourceName (source)).toUpperCase();
        draw::trackedText (g, name, caption, juce::Justification::centredLeft,
                           draw::fitFont (Theme::labelFontStrong (labelH), name, caption.getWidth() * 0.6f), Theme::amber);
        draw::trackedText (g, juce::String (v, 2), caption, juce::Justification::centredRight,
                           Theme::valueFont (labelH * 0.92f), Theme::textValue);

        inner.removeFromTop (juce::jmax (2.0f, b.getHeight() * 0.04f));
        auto plot = inner;
        if (plot.getHeight() < 6.0f) return;

        draw::trackedText (g, juce::String::fromUTF8 ("\xe2\x86\x92 ") + juce::String (targets),
                           plot.withHeight (labelH * 0.9f), juce::Justification::centredLeft,
                           Theme::captionFont (labelH * 0.72f), Theme::textDim);
        if (filled < 2) return;

        const float mid = bipolar ? plot.getCentreY() : plot.getBottom() - plot.getHeight() * 0.08f;
        const float span = bipolar ? plot.getHeight() * 0.42f : plot.getHeight() * 0.84f;

        // Rails at the extremes, so a source resting at zero still reads as a graph.
        g.setColour (juce::Colours::white.withAlpha (0.035f));
        g.drawLine (plot.getX(), mid - span, plot.getRight(), mid - span, 1.0f);
        if (bipolar) g.drawLine (plot.getX(), mid + span, plot.getRight(), mid + span, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawLine (plot.getX(), mid, plot.getRight(), mid, 1.0f);

        juce::Path line;
        for (int i = 0; i < filled; ++i)
        {
            const float v = history[(size_t) ((head - filled + i + kPoints * 2) % kPoints)];
            const float x = plot.getX() + plot.getWidth() * (float) i / (float) juce::jmax (1, filled - 1);
            const float y = mid - v * span;
            if (i == 0) line.startNewSubPath (x, y); else line.lineTo (x, y);
        }

        {
            juce::Path fill (line);
            fill.lineTo (plot.getRight(), mid);
            fill.lineTo (plot.getX(), mid);
            fill.closeSubPath();
            g.setColour (Theme::amber.withAlpha (0.12f));
            g.fillPath (fill);
        }
        draw::glowPath (g, line, Theme::amber, juce::jmax (1.0f, plot.getHeight() * 0.035f), plot.getHeight() * 0.12f, 0.5f);

        // Current value, right on the leading edge.
        draw::glowDot (g, { plot.getRight(), mid - v * span }, juce::jmax (1.6f, plot.getHeight() * 0.06f), Theme::amber, 0.9f);
    }

private:
    static constexpr int kPoints = 128;
    ModSource source;
    int targets = 0;
    bool bipolar = true;
    std::array<float, kPoints> history {};
    int head = 0, filled = 0;
};

//==============================================================================
ModRoutingPanel::ModRoutingPanel (AntiMatrProcessor& p)
    : AMPanel ("Routings", "Source to destination", Theme::amber), processor (p)
{
    table = processor.getModRoutings();

    sourceButton.setOutlined (true);
    sourceButton.setTooltip ("Choose the modulation source to assign.");
    sourceButton.onClick = [this] { showSourceMenu(); };
    addAndMakeVisible (sourceButton);

    assignButton.setFilled (true);
    assignButton.setTooltip ("Arm assignment, then click any knob in the plug-in.");
    assignButton.onClick = [this]
    {
        ModAssign::get().arm (pendingSource, this, [this] (ModSource s, Param t) { return addRouting (s, t); });
    };
    addAndMakeVisible (assignButton);

    targetButton.setOutlined (true);
    targetButton.setTooltip ("Pick the destination from a list instead of clicking a knob.");
    targetButton.onClick = [this] { showTargetMenu(); };
    addAndMakeVisible (targetButton);

    clearButton.setOutlined (true);
    clearButton.setTooltip ("Remove every routing in the patch.");
    clearButton.onClick = [this]
    {
        ModRoutingTable empty;
        processor.setModRoutings (empty);
    };
    addAndMakeVisible (clearButton);

    viewport.setViewedComponent (&rowHolder, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);

    ModAssign::get().addChangeListener (this);
    processor.addChangeListener (this);

    rebuildRows();
    updateAssignButton();
    startTimerHz (30);
}

ModRoutingPanel::~ModRoutingPanel()
{
    stopTimer();
    processor.removeChangeListener (this);
    ModAssign::get().cancelFor (this);
    ModAssign::get().removeChangeListener (this);
}

//==============================================================================
int ModRoutingPanel::rowHeight() const
{
    return juce::jlimit (26, 40, juce::roundToInt ((float) contentBounds().getHeight() * 0.062f));
}

void ModRoutingPanel::resized()
{
    auto area = contentBounds();
    if (area.isEmpty()) return;
    const int gap = juce::jmax (4, area.getHeight() / 44);
    const int barH = juce::jlimit (24, 36, area.getHeight() / 12);

    auto bar = area.removeFromTop (barH);
    sourceButton.setBounds (bar.removeFromLeft (juce::jmax (90, bar.getWidth() / 6)));
    bar.removeFromLeft (gap);
    assignButton.setBounds (bar.removeFromLeft (juce::jmax (96, bar.getWidth() / 5)));
    bar.removeFromLeft (gap);
    targetButton.setBounds (bar.removeFromLeft (juce::jmax (120, bar.getWidth() / 4)));
    clearButton.setBounds (bar.removeFromRight (juce::jmax (80, bar.getWidth() / 5)));

    area.removeFromTop (gap * 2);

    const int h = rowHeight();
    const bool empty = rows.empty();

    if (empty)
    {
        headerArea = {};
        scopeArea = {};
        listArea = area;
        viewport.setVisible (false);
        for (auto& s : scopes) s->setVisible (false);
        return;
    }

    viewport.setVisible (true);
    headerArea = area.removeFromTop (juce::jlimit (12, 18, barH / 2));
    area.removeFromTop (gap);

    // The list keeps only the height its rows need; everything left over becomes source scopes,
    // so the panel is never part table and part hole.
    const int needed = (int) rows.size() * (h + 2);
    const int leftover = area.getHeight() - needed - gap * 2;
    const int scopeH = (! scopes.empty() && leftover >= 76) ? juce::jmin (leftover, juce::roundToInt ((float) area.getHeight() * 0.62f)) : 0;

    scopeArea = scopeH > 0 ? area.removeFromBottom (scopeH) : juce::Rectangle<int>();
    listArea = area;
    viewport.setBounds (listArea);

    const int holderW = viewport.getWidth() - (needed > listArea.getHeight() ? 10 : 0);
    rowHolder.setSize (holderW, juce::jmax (listArea.getHeight(), needed));
    for (size_t i = 0; i < rows.size(); ++i)
        rows[i]->setBounds (0, (int) i * (h + 2), holderW, h);

    headerArea = headerArea.withX (listArea.getX()).withWidth (holderW);

    if (! scopeArea.isEmpty() && ! scopes.empty())
    {
        auto grid = scopeArea.withTrimmedTop (gap);
        const int cols = modScopeColumns ((int) scopes.size(), grid.getWidth());
        std::vector<juce::Component*> comps;
        for (auto& s : scopes) { s->setVisible (true); comps.push_back (s.get()); }
        layoutGrid (grid, comps, cols, gap, gap);
    }
    else
    {
        for (auto& s : scopes) s->setVisible (false);
    }
}

//==============================================================================
void ModRoutingPanel::paintTableHeader (juce::Graphics& g)
{
    if (headerArea.isEmpty() || rows.empty()) return;
    const auto l = ModRowLayout::forRow (headerArea.withHeight (rowHeight()));
    const float h = juce::jlimit (8.0f, 10.5f, (float) headerArea.getHeight() * 0.72f);
    auto at = [this] (juce::Rectangle<int> r) { return r.withY (headerArea.getY()).withHeight (headerArea.getHeight()).toFloat(); };

    draw::trackedText (g, "SOURCE", at (l.source), juce::Justification::centredLeft, Theme::captionFont (h), Theme::textDim);
    draw::trackedText (g, "DESTINATION", at (l.destination), juce::Justification::centredLeft, Theme::captionFont (h), Theme::textDim);
    draw::trackedText (g, "DEPTH", at (l.depth), juce::Justification::centredLeft, Theme::captionFont (h), Theme::textDim);
    draw::trackedText (g, "AMOUNT", at (l.value), juce::Justification::centredRight, Theme::captionFont (h), Theme::textDim);
    draw::trackedText (g, "ON", at (l.power), juce::Justification::centred, Theme::captionFont (h), Theme::textDim);
    draw::trackedText (g, "POLARITY", at (l.polarity), juce::Justification::centred, Theme::captionFont (h), Theme::textDim);

    g.setColour (Theme::borderSoft);
    g.fillRect (headerArea.getX(), headerArea.getBottom(), headerArea.getWidth(), 1);
}

void ModRoutingPanel::paintEmptyState (juce::Graphics& g)
{
    auto area = listArea.toFloat();
    if (area.getHeight() < 40.0f) return;

    const bool armed = ModAssign::get().isArmedFor (this);

    static const char* steps[] =
    {
        "PICK A SOURCE",
        "PRESS ASSIGN AND CLICK A KNOB, OR CHOOSE TARGET",
        "SET HOW FAR IT MOVES WITH THE DEPTH BAR"
    };
    const juce::String headline = armed ? "CLICK ANY KNOB TO FINISH THE ASSIGNMENT" : "NOTHING IS ROUTED YET";

    // The card is sized to its content so the empty state reads as a designed
    // object rather than a large box with text floating near the top.
    auto measure = [&steps, &headline] (float u)
    {
        float w = juce::GlyphArrangement::getStringWidth (Theme::labelFontStrong (u * 0.95f), headline);
        for (const auto* step : steps)
            w = juce::jmax (w, juce::GlyphArrangement::getStringWidth (Theme::captionFont (u * 0.8f), step) + u * 2.3f);
        return w;
    };

    float unit = juce::jlimit (11.0f, 20.0f, juce::jmin (area.getWidth() * 0.019f, area.getHeight() * 0.07f));
    float widest = measure (unit);

    auto padXFor = [] (float u) { return u * 2.2f; };
    auto padYFor = [] (float u) { return u * 1.5f; };
    auto contentHFor = [] (float u) { return u * (2.2f + 0.5f + 1.6f + 0.5f) + u * 1.7f * 3.0f; };
    float padX = padXFor (unit), padY = padYFor (unit), contentH = contentHFor (unit);
    while (unit > 9.0f && (widest + padX * 2.0f > area.getWidth() - 8.0f || contentH + padY * 2.0f > area.getHeight() - 8.0f))
    {
        unit -= 0.5f;
        widest = measure (unit);
        padX = padXFor (unit); padY = padYFor (unit); contentH = contentHFor (unit);
    }

    const float cardW = juce::jmin (area.getWidth() - 8.0f, widest + padX * 2.0f);
    const float cardH = juce::jmin (area.getHeight() - 8.0f, contentH + padY * 2.0f);
    auto card = juce::Rectangle<float> (cardW, cardH).withCentre (area.getCentre());
    const float corner = juce::jmin (14.0f, cardH * 0.1f);

    g.setColour (Theme::panelInset.withAlpha (0.55f));
    g.fillRoundedRectangle (card, corner);
    {
        juce::Path outline;
        outline.addRoundedRectangle (card.reduced (0.5f), corner);
        const float dashes[] = { 5.0f, 5.0f };
        juce::Path dashed;
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, outline, dashes, 2);
        g.setColour ((armed ? Theme::amber : Theme::border).withAlpha (armed ? 0.45f + 0.35f * assignPhase : 0.6f));
        g.fillPath (dashed);
    }
    if (armed)
        draw::glowRoundedRect (g, card, corner, Theme::amber, unit * 1.2f, 0.25f + 0.3f * assignPhase);

    auto inner = card.reduced (padX, padY);

    auto iconArea = inner.removeFromTop (unit * 2.2f).withSizeKeepingCentre (unit * 1.9f, unit * 1.9f);
    Icons::draw (g, Icon::Grid, iconArea, Theme::amber.withAlpha (armed ? 0.9f : 0.45f), 1.0f);
    inner.removeFromTop (unit * 0.5f);

    draw::trackedText (g, headline, inner.removeFromTop (unit * 1.6f), juce::Justification::centred,
                       Theme::labelFontStrong (unit * 0.95f), armed ? Theme::amber : Theme::textSecondary);
    inner.removeFromTop (unit * 0.5f);

    // The three steps share one left edge so they read as a list, not as three centred lines.
    const float diskD = unit * 1.2f;
    const float listW = juce::jmin (inner.getWidth(), widest);
    auto listArea_ = inner.withSizeKeepingCentre (listW, inner.getHeight()).withTop (inner.getY());
    for (int i = 0; i < 3; ++i)
    {
        auto line = listArea_.removeFromTop (unit * 1.7f);
        auto disk = line.removeFromLeft (diskD).withSizeKeepingCentre (diskD, diskD);
        g.setColour (Theme::amber.withAlpha (0.16f));
        g.fillEllipse (disk);
        draw::trackedText (g, juce::String (i + 1), disk, juce::Justification::centred, Theme::labelFontStrong (diskD * 0.6f), Theme::amber);
        draw::trackedText (g, steps[i], line.withTrimmedLeft (unit * 0.7f), juce::Justification::centredLeft,
                           Theme::captionFont (unit * 0.8f), Theme::textDim);
    }
}

void ModRoutingPanel::paint (juce::Graphics& g)
{
    AMPanel::paint (g);
    if (rows.empty()) paintEmptyState (g);
    else              paintTableHeader (g);
}

//==============================================================================
void ModRoutingPanel::rebuildRows()
{
    rows.clear();
    for (int i = 0; i < table.size(); ++i)
    {
        auto row = std::make_unique<Row> (*this, i);
        rowHolder.addAndMakeVisible (*row);
        rows.push_back (std::move (row));
    }
    rebuildScopes();
    refreshRows();
    resized();
    repaint();
}

void ModRoutingPanel::rebuildScopes()
{
    scopes.clear();
    // One scope per distinct source in the table, in the order the rows use them.
    std::array<int, (size_t) kNumModSources> destinations {};
    std::vector<ModSource> order;
    for (const auto& r : table)
    {
        const int i = (int) r.source;
        if (i <= 0 || i >= kNumModSources) continue;
        if (destinations[(size_t) i]++ == 0) order.push_back (r.source);
    }
    for (auto s : order)
    {
        if ((int) scopes.size() >= 8) break;
        auto scope = std::make_unique<SourceScope> (s, destinations[(size_t) s]);
        addChildComponent (*scope);
        scopes.push_back (std::move (scope));
    }
}

void ModRoutingPanel::refreshRows()
{
    const auto& snapshot = latestModulation (processor);
    for (size_t i = 0; i < rows.size(); ++i)
        rows[i]->setRouting (table[(int) i], snapshot.valueOf (table[(int) i].source));
}

void ModRoutingPanel::timerCallback()
{
    if (! isShowing()) return;
    const auto& snapshot = latestModulation (processor);
    for (size_t i = 0; i < rows.size() && (int) i < table.size(); ++i)
        rows[i]->setActivity (snapshot.valueOf (table[(int) i].source));
    for (auto& scope : scopes)
        if (scope->isVisible()) scope->push (snapshot.valueOf (scope->getSource()));

    if (ModAssign::get().isArmedFor (this))
    {
        assignPhase = 0.5f + 0.5f * std::sin ((float) juce::Time::getMillisecondCounter() * 0.004f);
        if (rows.empty()) repaint (listArea);
    }
}

void ModRoutingPanel::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source == &ModAssign::get())
    {
        updateAssignButton();
        repaint();
        return;
    }

    const auto& live = processor.getModRoutings();
    if (live == table) return;

    bool structural = live.size() != table.size();
    for (int i = 0; ! structural && i < live.size(); ++i)
        structural = live[i].source != table[i].source || live[i].target != table[i].target;

    table = live;
    if (structural) rebuildRows();
    else refreshRows();
}

void ModRoutingPanel::updateAssignButton()
{
    const bool armed = ModAssign::get().isArmedFor (this);
    assignButton.setButtonText (armed ? "CLICK A KNOB" : "ASSIGN");
    assignButton.setAccent (armed ? Theme::amber.brighter (0.3f) : Theme::amber);
    sourceButton.setButtonText (juce::String (modSourceName (pendingSource)).toUpperCase());
    repaint();
}

//==============================================================================
void ModRoutingPanel::showSourceMenu()
{
    juce::PopupMenu menu;
    for (int group = 0; group < (int) ModSourceGroup::Count; ++group)
    {
        juce::PopupMenu sub;
        for (int i = 1; i < kNumModSources; ++i)
        {
            const auto s = (ModSource) i;
            if ((int) modSourceGroup (s) != group) continue;
            sub.addItem (i, modSourceName (s), true, s == pendingSource);
        }
        menu.addSubMenu (modSourceGroupName ((ModSourceGroup) group), sub);
    }

    juce::Component::SafePointer<ModRoutingPanel> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&sourceButton), [safe] (int result)
    {
        if (safe == nullptr || result <= 0 || result >= kNumModSources) return;
        safe->pendingSource = (ModSource) result;
        if (ModAssign::get().isArmedFor (safe))
        {
            ModAssign::get().cancel();
            ModAssign::get().arm (safe->pendingSource, safe, [safe] (ModSource s, Param t) { return safe != nullptr && safe->addRouting (s, t); });
        }
        safe->updateAssignButton();
    });
}

void ModRoutingPanel::showTargetMenu()
{
    juce::PopupMenu menu;
    for (int group = 0; group < (int) ParamGroup::Count; ++group)
    {
        juce::PopupMenu sub;
        int items = 0;
        for (const auto& d : ParameterRegistry::all())
        {
            if ((int) d.group != group || ! d.modulatable) continue;
            const bool taken = table.indexOf (pendingSource, d.param) >= 0;
            sub.addItem (paramIndex (d.param) + 1, d.name, ! taken, taken);
            ++items;
        }
        if (items > 0)
            menu.addSubMenu (juce::String (ParameterRegistry::groupName ((ParamGroup) group)).toUpperCase(), sub);
    }

    juce::Component::SafePointer<ModRoutingPanel> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&targetButton), [safe] (int result)
    {
        if (safe == nullptr || result <= 0 || result > kNumParams) return;
        safe->addRouting (safe->pendingSource, paramFromIndex (result - 1));
    });
}

//==============================================================================
bool ModRoutingPanel::addRouting (ModSource source, Param target)
{
    ModRouting r;
    r.source  = source;
    r.target  = target;
    r.depth   = 0.35f;
    r.curve   = 0.0f;
    r.bipolar = modSourceIsBipolar (source);
    r.enabled = true;

    auto next = processor.getModRoutings();
    if (next.add (r) < 0) return false;
    processor.setModRoutings (next);
    return true;
}

void ModRoutingPanel::removeRouting (int index)
{
    auto next = processor.getModRoutings();
    if (! next.remove (index)) return;
    processor.setModRoutings (next);
}

void ModRoutingPanel::setDepth (int index, float depth)
{
    auto next = processor.getModRoutings();
    if (! next.setDepth (index, depth)) return;
    processor.setModRoutings (next);
}

void ModRoutingPanel::setBipolar (int index, bool bipolar)
{
    auto next = processor.getModRoutings();
    if (! next.setBipolar (index, bipolar)) return;
    processor.setModRoutings (next);
}

void ModRoutingPanel::setEnabled (int index, bool on)
{
    auto next = processor.getModRoutings();
    if (! next.setEnabled (index, on)) return;
    processor.setModRoutings (next);
}

} // namespace am::ui

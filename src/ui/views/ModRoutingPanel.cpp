#include "ModRoutingPanel.h"

namespace am::ui
{

namespace
{
    /** "Shape ▸ Decay" — the group plus the parameter, so a target reads unambiguously. */
    juce::String targetName (Param p)
    {
        const auto& d = ParameterRegistry::get (p);
        return juce::String (ParameterRegistry::groupName (d.group)).toUpperCase() + "  " + juce::String (d.name).toUpperCase();
    }

    /** Depth as a share of the target's range, in the target's own units. */
    juce::String depthText (const ModRouting& r)
    {
        const auto& d = ParameterRegistry::get (r.target);
        const float amount = r.depth * (d.max - d.min);
        juce::String s = (amount >= 0.0f ? "+" : "") + juce::String (amount, std::abs (amount) < 10.0f ? 2 : 1);
        if (d.unit[0] != 0) s << " " << d.unit;
        return s;
    }
}

//==============================================================================
/** One routing: source ▸ target, depth, polarity, enable and remove. */
class ModRoutingPanel::Row : public juce::Component
{
public:
    Row (ModRoutingPanel& o, int rowIndex) : owner (o), index (rowIndex)
    {
        depth.setSliderStyle (juce::Slider::LinearHorizontal);
        depth.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        depth.setRange (-1.0, 1.0, 0.001);
        depth.setShowLabel (false);
        depth.setShowValue (false);
        depth.setDoubleClickReturnValue (true, 0.0);
        depth.setBipolar (true);
        depth.onValueChange = [this] { owner.setDepth (index, (float) depth.getValue()); };
        depth.setTooltip ("Modulation depth as a share of the destination's range. Double-click to zero.");
        addAndMakeVisible (depth);

        polarity.onChange = [this] (bool on) { owner.setBipolar (index, on); };
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
        depth.setValue (r.depth, juce::dontSendNotification);
        polarity.setToggleState (r.bipolar, juce::dontSendNotification);
        polarity.setLabel (r.bipolar ? "BI" : "UNI");
        power.setToggleState (r.enabled, juce::dontSendNotification);
        setActivity (sourceValue);
        repaint();
    }

    void setActivity (float sourceValue)
    {
        const float v = std::isfinite (sourceValue) ? juce::jlimit (-1.0f, 1.0f, sourceValue) : 0.0f;
        if (std::abs (v - activity) < 0.01f) return;
        activity = v;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float corner = juce::jmin (5.0f, b.getHeight() * 0.22f);
        const float alpha = routing.enabled ? 1.0f : 0.42f;

        draw::insetSurface (g, b, corner);

        // Live activity: a thin underline showing how hard the source is pushing right now.
        {
            const float amount = juce::jlimit (0.0f, 1.0f, std::abs (activity) * std::abs (routing.depth));
            if (routing.enabled && amount > 0.004f)
            {
                auto lane = b.reduced (2.0f).removeFromBottom (juce::jmax (1.5f, b.getHeight() * 0.075f));
                g.setColour (Theme::amber.withAlpha (0.55f));
                g.fillRoundedRectangle (lane.withWidth (lane.getWidth() * amount), lane.getHeight() * 0.5f);
            }
        }

        const float h = juce::jlimit (8.0f, 12.0f, b.getHeight() * 0.42f);
        auto text = b.reduced (juce::jmax (5.0f, b.getHeight() * 0.28f), 0.0f).withWidth (nameWidth());

        auto sourceArea = text.removeFromLeft (text.getWidth() * 0.36f);
        draw::trackedText (g, juce::String (modSourceName (routing.source)).toUpperCase(), sourceArea,
                           juce::Justification::centredLeft,
                           draw::fitFont (Theme::labelFontStrong (h), modSourceName (routing.source), sourceArea.getWidth()),
                           Theme::amber.withAlpha (alpha));

        auto arrow = text.removeFromLeft (juce::jmax (12.0f, h * 1.4f));
        g.setColour (Theme::textDim.withAlpha (alpha));
        {
            juce::Path p;
            const float s = juce::jmin (arrow.getWidth(), arrow.getHeight()) * 0.22f;
            const auto c = arrow.getCentre();
            p.startNewSubPath (c.x - s, c.y - s);
            p.lineTo (c.x + s * 0.8f, c.y);
            p.lineTo (c.x - s, c.y + s);
            g.strokePath (p, juce::PathStrokeType (juce::jmax (1.0f, s * 0.34f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const auto name = targetName (routing.target);
        draw::trackedText (g, name, text, juce::Justification::centredLeft,
                           draw::fitFont (Theme::labelFont (h), name, text.getWidth()),
                           Theme::textPrimary.withAlpha (alpha));

        // Depth readout to the right of the slider.
        const auto value = juce::Rectangle<float> ((float) depth.getRight() + 4.0f, b.getY(), (float) valueWidth(), b.getHeight());
        draw::trackedText (g, depthText (routing), value, juce::Justification::centredLeft,
                           Theme::valueFont (h * 0.95f), Theme::textValue.withAlpha (alpha));
    }

    void resized() override
    {
        auto b = getLocalBounds();
        const int pad = juce::jmax (4, b.getHeight() / 5);
        b.reduce (pad, juce::jmax (2, b.getHeight() / 10));

        remove.setBounds (b.removeFromRight (b.getHeight()));
        b.removeFromRight (pad / 2);
        polarity.setBounds (b.removeFromRight (juce::jmax (30, b.getHeight() * 2)));
        b.removeFromRight (pad / 2);
        power.setBounds (b.removeFromRight (juce::jmax (26, (int) (b.getHeight() * 1.6f))));
        b.removeFromRight (pad);
        b.removeFromRight (valueWidth());
        b.removeFromLeft (nameWidth());
        depth.setBounds (b);
    }

    int nameWidth() const  { return juce::jmax (110, getWidth() / 3); }
    int valueWidth() const { return juce::jmax (46, getWidth() / 11); }

    int index = 0;

private:
    ModRoutingPanel& owner;
    ModRouting routing;
    float activity = 0.0f;
    AMSlider depth { "", Theme::amber };
    AMToggle polarity { "BI", Theme::amber };
    AMToggle power { "ON", Theme::amber };
    AMIconButton remove { Icon::Close, Theme::textSecondary };
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
    startTimerHz (20);
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
    return juce::jlimit (24, 40, juce::jmax (24, contentBounds().getHeight() / 11));
}

void ModRoutingPanel::resized()
{
    auto area = contentBounds();
    if (area.isEmpty()) return;
    const int gap = juce::jmax (4, area.getHeight() / 40);
    const int barH = juce::jlimit (22, 34, area.getHeight() / 11);

    auto bar = area.removeFromTop (barH);
    sourceButton.setBounds (bar.removeFromLeft (juce::jmax (90, bar.getWidth() / 6)));
    bar.removeFromLeft (gap);
    assignButton.setBounds (bar.removeFromLeft (juce::jmax (96, bar.getWidth() / 5)));
    bar.removeFromLeft (gap);
    targetButton.setBounds (bar.removeFromLeft (juce::jmax (120, bar.getWidth() / 4)));
    clearButton.setBounds (bar.removeFromRight (juce::jmax (80, bar.getWidth() / 5)));

    area.removeFromTop (gap * 2);
    headerArea = area.removeFromTop (juce::jlimit (12, 18, barH / 2)).reduced (juce::jmax (5, barH / 5), 0);
    area.removeFromTop (gap);
    viewport.setBounds (area);

    const int h = rowHeight();
    rowHolder.setSize (viewport.getWidth() - (viewport.isVerticalScrollBarShown() ? 10 : 0),
                       juce::jmax (viewport.getHeight(), (int) rows.size() * (h + 2)));
    for (size_t i = 0; i < rows.size(); ++i)
        rows[i]->setBounds (0, (int) i * (h + 2), rowHolder.getWidth(), h);
}

void ModRoutingPanel::paint (juce::Graphics& g)
{
    AMPanel::paint (g);

    // Column captions, so an empty list still reads as a table.
    if (! headerArea.isEmpty())
    {
        auto header = headerArea.toFloat();
        const float h = juce::jlimit (8.0f, 10.5f, header.getHeight() * 0.72f);
        const int nameW = rows.empty() ? juce::jmax (110, headerArea.getWidth() / 3) : rows.front()->nameWidth();
        auto names = header.removeFromLeft ((float) nameW);
        draw::trackedText (g, "SOURCE", names.removeFromLeft (names.getWidth() * 0.36f), juce::Justification::centredLeft,
                           Theme::captionFont (h), Theme::textDim);
        draw::trackedText (g, "DESTINATION", names, juce::Justification::centredLeft, Theme::captionFont (h), Theme::textDim);
        draw::trackedText (g, "DEPTH", header, juce::Justification::centredLeft, Theme::captionFont (h), Theme::textDim);
        draw::trackedText (g, "ON   POLARITY", header, juce::Justification::centredRight, Theme::captionFont (h), Theme::textDim);
        g.setColour (Theme::borderSoft);
        g.fillRect (header.withY (header.getBottom() + 1.0f).withHeight (1.0f).withLeft (headerArea.toFloat().getX()));
    }

    if (! rows.empty()) return;

    auto area = viewport.getBounds().toFloat();
    const float h = juce::jlimit (11.0f, 15.0f, area.getHeight() * 0.055f);
    const bool armed = ModAssign::get().isArmedFor (this);

    auto line = area.removeFromTop (area.getHeight() * 0.42f).removeFromBottom (h * 2.4f);
    draw::trackedText (g, armed ? "CLICK ANY KNOB TO FINISH THE ASSIGNMENT" : "NOTHING IS ROUTED YET",
                       line, juce::Justification::centred,
                       Theme::labelFontStrong (h), armed ? Theme::amber : Theme::textSecondary);

    const juce::String hints[] =
    {
        "1.  PICK A SOURCE",
        "2.  PRESS ASSIGN, THEN CLICK ANY KNOB     OR     CHOOSE TARGET FROM THE LIST",
        "3.  SET HOW FAR IT MOVES WITH THE DEPTH SLIDER"
    };
    for (const auto& hint : hints)
        draw::trackedText (g, hint, area.removeFromTop (h * 1.9f), juce::Justification::centred,
                           Theme::captionFont (h * 0.86f), Theme::textDim);
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
    refreshRows();
    resized();
    repaint();
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

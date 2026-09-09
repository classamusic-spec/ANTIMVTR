#include "TopBar.h"

namespace am::ui
{

TopBar::TopBar (AntiMatrProcessor& p) : processor (p)
{
    addAndMakeVisible (logo);
    addAndMakeVisible (prev);
    addAndMakeVisible (next);
    addAndMakeVisible (browse);
    addAndMakeVisible (random);
    addAndMakeVisible (mutate);
    addAndMakeVisible (settings);

    prev.onClick    = [this] { processor.loadNextPreset (-1); };
    next.onClick    = [this] { processor.loadNextPreset (1); };
    random.onClick  = [this] { processor.randomizePatch(); };
    mutate.onClick  = [this] { showMutateMenu(); };
    browse.onClick  = [this] { if (onBrowse) onBrowse(); else showBrowseMenu(); };
    settings.onClick = [this] { if (onSettings) onSettings(); else showSettingsMenu(); };

    mutate.setAccent (Theme::violet);
    random.setAccent (Theme::cyan);
    browse.setAccent (Theme::blue);

    processor.addChangeListener (this);
    startTimerHz (20);
}

TopBar::~TopBar()
{
    processor.removeChangeListener (this);
}

void TopBar::timerCallback()
{
    const auto& vs = processor.diagnostics().visualSnapshots.latest();
    logo.setEnergy (juce::jlimit (0.0f, 1.0f, vs.rmsL * 4.0f));
    logo.repaint();
}

void TopBar::showMutateMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader ("MUTATE");
    m.addItem (1, "Subtle — close relative");
    m.addItem (2, "Evolve — meaningful variation");
    m.addItem (3, "Extreme — structural mutation");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&mutate), [this] (int r)
    {
        if (r == 1) processor.mutate (MutationStrength::Subtle);
        else if (r == 2) processor.mutate (MutationStrength::Evolve);
        else if (r == 3) processor.mutate (MutationStrength::Extreme);
    });
}

void TopBar::showBrowseMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader ("FACTORY");
    auto& presets = processor.presets();
    for (int i = 0; i < presets.numFactoryPresets(); ++i)
        m.addItem (100 + i, presets.factoryPreset (i).name + "   ·   " + presets.factoryPreset (i).category, true, i == processor.currentPresetIndex());
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&browse), [this] (int r)
    {
        if (r >= 100) processor.loadFactoryPreset (r - 100);
    });
}

void TopBar::showSettingsMenu()
{
    juce::PopupMenu m;
    auto& apvts = processor.parameters();
    auto addChoice = [&] (Param p, int base)
    {
        const auto& d = ParameterRegistry::get (p);
        juce::PopupMenu sub;
        juce::StringArray items; items.addTokens (juce::String (d.choices), "|", "");
        auto* param = apvts.getParameter (d.id);
        const int cur = (int) std::lround (param->convertFrom0to1 (param->getValue()));
        for (int i = 0; i < items.size(); ++i) sub.addItem (base + i, items[i], true, i == cur);
        m.addSubMenu (d.name, sub);
    };
    addChoice (Param::masterVoices, 200);
    addChoice (Param::masterQuality, 300);
    addChoice (Param::masterMode, 400);
    m.addSeparator();
    m.addItem (1, "ANTI-MATR " + juce::String (ANTIMATR_VERSION_STRING), false);

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&settings), [this] (int r)
    {
        auto set = [this] (Param p, int index)
        {
            auto* param = processor.parameters().getParameter (ParameterRegistry::get (p).id);
            param->setValueNotifyingHost (param->convertTo0to1 ((float) index));
        };
        if (r >= 400) set (Param::masterMode, r - 400);
        else if (r >= 300) set (Param::masterQuality, r - 300);
        else if (r >= 200) set (Param::masterVoices, r - 200);
    });
}

void TopBar::resized()
{
    auto area = getLocalBounds();
    const int h = getHeight();
    const int pad = juce::jmax (8, getWidth() / 70);
    area.reduce (pad, 0);

    logo.setBounds (area.removeFromLeft (juce::jlimit (240, 460, getWidth() * 28 / 100)).reduced (0, h / 6));

    auto right = area.removeFromRight (juce::jlimit (260, 420, getWidth() * 27 / 100));
    const int btnH = juce::jlimit (24, 40, h / 2);
    auto rightRow = right.withSizeKeepingCentre (right.getWidth(), btnH);
    settings.setBounds (rightRow.removeFromRight (btnH));
    rightRow.removeFromRight (pad);
    const int btnW = rightRow.getWidth() / 3;
    browse.setBounds (rightRow.removeFromLeft (btnW));
    random.setBounds (rightRow.removeFromLeft (btnW));
    mutate.setBounds (rightRow.removeFromLeft (btnW));

    auto centre = area.withSizeKeepingCentre (juce::jmin (area.getWidth() - pad * 2, juce::jlimit (300, 460, getWidth() * 28 / 100)), btnH + 4);
    centre.setY (h / 2 - (btnH + 4) / 2 - h / 10);
    presetArea = centre;
    prev.setBounds (centre.removeFromLeft (btnH));
    next.setBounds (centre.removeFromRight (btnH));
}

void TopBar::paint (juce::Graphics& g)
{
    // Preset pill
    auto pill = presetArea.toFloat();
    const float corner = pill.getHeight() * 0.5f;
    g.setColour (Theme::panelInset);
    g.fillRoundedRectangle (pill, corner);
    g.setColour (Theme::border);
    g.drawRoundedRectangle (pill.reduced (0.5f), corner, 1.0f);

    const float h = juce::jlimit (10.0f, 14.5f, pill.getHeight() * 0.38f);
    draw::trackedText (g, processor.currentPresetName().toUpperCase(), pill.reduced (pill.getHeight(), 0.0f), juce::Justification::centred, Theme::labelFont (h), Theme::textPrimary);

    // Tags line
    auto tags = processor.currentPresetTags();
    if (! tags.isEmpty())
    {
        juce::String line;
        for (int i = 0; i < tags.size(); ++i) { if (i > 0) line << juce::String::fromUTF8 ("   \xC2\xB7   "); line << tags[i].toUpperCase(); }
        auto tagArea = pill.withY (pill.getBottom() + 2.0f).withHeight (h * 1.6f);
        draw::trackedText (g, line, tagArea, juce::Justification::centred, Theme::captionFont (juce::jmax (7.5f, h * 0.72f)), Theme::textSecondary);
    }
}

//==============================================================================
juce::StringArray NavBar::pageNames() { return { "Main", "Source", "Shape", "Evolve", "Fracture", "Space", "Mod" }; }

NavBar::NavBar (AntiMatrProcessor& p, bool showLab) : processor (p), lab (showLab)
{
    const Icon icons[] = { Icon::Main, Icon::Source, Icon::Shape, Icon::Evolve, Icon::Fracture, Icon::Space, Icon::Mod };
    auto names = pageNames();
    for (int i = 0; i < names.size(); ++i)
    {
        auto t = std::make_unique<Tab> (names[i], icons[i]);
        t->onClick = [this, i] { setPage (i); if (onPageChange) onPageChange (i); };
        addAndMakeVisible (*t);
        tabs.push_back (std::move (t));
    }
    if (lab)
    {
        auto t = std::make_unique<Tab> ("Lab", Icon::Lab);
        const int idx = (int) tabs.size();
        t->onClick = [this, idx] { setPage (idx); if (onPageChange) onPageChange (idx); };
        addAndMakeVisible (*t);
        tabs.push_back (std::move (t));
    }
    addAndMakeVisible (ab);
    ab.setSelected (processor.currentABSlot(), juce::dontSendNotification);
    ab.onChange = [this] (int i) { processor.selectABSlot (i); if (onABChange) onABChange (i); };
    setPage (0);
}

void NavBar::setPage (int index)
{
    page = juce::jlimit (0, (int) tabs.size() - 1, index);
    for (int i = 0; i < (int) tabs.size(); ++i) { tabs[(size_t) i]->selected = (i == page); tabs[(size_t) i]->repaint(); }
}

void NavBar::Tab::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    if (selected)
    {
        juce::ColourGradient grad (Theme::blue.withAlpha (0.16f), b.getX(), b.getBottom(), Theme::blue.withAlpha (0.0f), b.getX(), b.getY(), false);
        g.setGradientFill (grad);
        g.fillRect (b);
        juce::Path line; line.startNewSubPath (b.getX() + 6.0f, b.getBottom() - 1.5f); line.lineTo (b.getRight() - 6.0f, b.getBottom() - 1.5f);
        draw::glowPath (g, line, Theme::blue, 1.5f, 8.0f, 0.8f);
    }
    g.setColour (Theme::borderSoft);
    g.drawLine (b.getRight(), b.getY() + b.getHeight() * 0.2f, b.getRight(), b.getBottom() - b.getHeight() * 0.2f, 1.0f);

    const float labelH = juce::jlimit (9.0f, 12.0f, b.getHeight() * 0.16f);
    auto iconArea = b.withTrimmedBottom (labelH * 2.2f).reduced (0.0f, b.getHeight() * 0.16f);
    const float d = juce::jmin (iconArea.getWidth(), iconArea.getHeight());
    iconArea = iconArea.withSizeKeepingCentre (d, d);
    const auto col = selected ? Theme::textPrimary : (hover ? Theme::textSecondary.brighter (0.5f) : Theme::textSecondary);
    if (selected) draw::glowEllipse (g, iconArea, Theme::blue, d * 0.5f, 0.5f);
    Icons::draw (g, icon, iconArea, col, 0.8f);
    draw::trackedText (g, name.toUpperCase(), b.withTop (iconArea.getBottom() + 2.0f), juce::Justification::centredTop, Theme::labelFont (labelH), col);
}

void NavBar::resized()
{
    auto area = getLocalBounds();
    const int pad = juce::jmax (8, getWidth() / 70);
    area.reduce (pad, juce::jmax (4, getHeight() / 10));

    auto right = area.removeFromRight (juce::jlimit (220, 380, getWidth() * 24 / 100));
    const int abH = juce::jlimit (22, 30, getHeight() / 3);
    ab.setBounds (right.removeFromLeft (juce::jlimit (70, 96, right.getWidth() / 4)).withSizeKeepingCentre (juce::jlimit (70, 96, right.getWidth() / 4), abH));

    auto tabArea = area.withWidth (juce::jmin (area.getWidth(), juce::jlimit (600, 980, getWidth() * 60 / 100)));
    tabArea.setX (area.getX() + (area.getWidth() - tabArea.getWidth()) / 2 - pad);
    const int w = tabArea.getWidth() / (int) tabs.size();
    for (auto& t : tabs) t->setBounds (tabArea.removeFromLeft (w));
}

void NavBar::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (Theme::border);
    g.drawLine (b.getX() + 8.0f, b.getY(), b.getRight() - 8.0f, b.getY(), 1.0f);

    const float h = juce::jlimit (7.5f, 9.5f, b.getHeight() * 0.11f);
    const int pad = juce::jmax (8, getWidth() / 70);
    draw::trackedText (g, "V" + juce::String (ANTIMATR_VERSION_STRING), b.withWidth (120.0f).withTrimmedLeft ((float) pad), juce::Justification::centredLeft, Theme::captionFont (h), Theme::textDim);

    auto brand = b.withLeft (b.getRight() - 190.0f).withTrimmedRight ((float) pad);
    draw::trackedText (g, "INSTRUMENTS", brand.withHeight (b.getHeight() * 0.5f), juce::Justification::bottomRight, Theme::captionFont (h), Theme::textDim);
    draw::trackedText (g, "FOR A MORE STRANGE TOMORROW", brand.withTop (b.getCentreY()), juce::Justification::topRight, Theme::captionFont (h), Theme::textDim);
}

} // namespace am::ui

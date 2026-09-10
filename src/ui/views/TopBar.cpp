#include "TopBar.h"
#include "PresetBrowser.h"

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

    prev.setOutlined (true);
    next.setOutlined (true);
    prev.setAccent (Theme::textSecondary);
    next.setAccent (Theme::textSecondary);
    prev.setTooltip ("Previous preset");
    next.setTooltip ("Next preset");
    browse.setTooltip ("Open the preset browser");
    random.setTooltip ("Generate a new random patch");
    mutate.setTooltip ("Mutate the current patch (Subtle / Evolve / Extreme)");
    settings.setTooltip ("Voices, quality, voice mode");

    prev.onClick    = [this] { processor.loadNextPreset (-1); };
    next.onClick    = [this] { processor.loadNextPreset (1); };
    random.onClick  = [this] { processor.randomizePatch(); };
    mutate.onClick  = [this] { showMutateMenu(); };
    browse.onClick  = [this] { if (onBrowse) onBrowse(); else openBrowser(); };
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
    closeBrowser();
}

bool TopBar::isBrowserOpen() const noexcept { return browser != nullptr && browser->isVisible(); }

void TopBar::openBrowser()
{
    auto* host = getParentComponent();
    if (host == nullptr) { showBrowseMenu(); return; }
    if (browser == nullptr)
    {
        browser = std::make_unique<PresetBrowser> (processor);
        browser->onClose = [this] { closeBrowser(); };
    }
    host->addAndMakeVisible (*browser);
    browser->setBounds (host->getLocalBounds());
    browser->toFront (true);
    browser->grabKeyboardFocus();
    browse.setToggleState (true, juce::dontSendNotification);
}

void TopBar::closeBrowser()
{
    if (browser == nullptr) return;
    if (auto* host = browser->getParentComponent()) host->removeChildComponent (browser.get());
    browser->setVisible (false);
    browse.setToggleState (false, juce::dontSendNotification);
}

void TopBar::timerCallback()
{
    const auto& vs = processor.diagnostics().visualSnapshots.latest();
    logo.setEnergy (juce::jlimit (0.0f, 1.0f, vs.rmsL * 4.0f));
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
    const int pad = juce::jmax (10, getWidth() / 64);
    area.reduce (pad, 0);

    logo.setBounds (area.removeFromLeft (juce::jlimit (200, 320, getWidth() * 19 / 100)).reduced (0, h / 5));

    auto right = area.removeFromRight (juce::jlimit (250, 400, getWidth() * 26 / 100));
    const int btnH = juce::jlimit (24, 40, h / 2);
    auto rightRow = right.withSizeKeepingCentre (right.getWidth(), btnH);
    settings.setBounds (rightRow.removeFromRight (btnH));
    rightRow.removeFromRight (pad / 2);
    const int btnW = rightRow.getWidth() / 3;
    browse.setBounds (rightRow.removeFromLeft (btnW));
    random.setBounds (rightRow.removeFromLeft (btnW));
    mutate.setBounds (rightRow.removeFromLeft (btnW));

    const int pillH = juce::jlimit (28, 42, h * 42 / 100);
    auto centre = area.withSizeKeepingCentre (juce::jmin (area.getWidth() - pad * 2, juce::jlimit (300, 460, getWidth() * 28 / 100)), pillH);
    centre.setY (h / 2 - pillH / 2 - h / 9);
    presetArea = centre;
    const int chev = pillH - 8;
    prev.setBounds (centre.removeFromLeft (pillH).withSizeKeepingCentre (chev, chev));
    next.setBounds (centre.removeFromRight (pillH).withSizeKeepingCentre (chev, chev));
    if (browser != nullptr && browser->getParentComponent() != nullptr)
        browser->setBounds (browser->getParentComponent()->getLocalBounds());
}

void TopBar::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    // faint separator beneath the bar
    juce::ColourGradient sep (juce::Colours::transparentWhite, b.getX(), 0.0f, juce::Colours::white.withAlpha (0.05f), b.getCentreX(), 0.0f, false);
    sep.addColour (1.0, juce::Colours::transparentWhite);
    g.setGradientFill (sep);
    g.fillRect (b.withTop (b.getBottom() - 1.0f));

    // Preset pill
    auto pill = presetArea.toFloat();
    const float corner = pill.getHeight() * 0.5f;
    g.setColour (Theme::panelEdge.withAlpha (0.8f));
    g.drawRoundedRectangle (pill.expanded (0.5f), corner + 0.5f, 1.0f);
    juce::ColourGradient fill (Theme::panelTop, pill.getX(), pill.getY(), Theme::panelInset, pill.getX(), pill.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (pill, corner);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawLine (pill.getX() + corner, pill.getY() + 1.0f, pill.getRight() - corner, pill.getY() + 1.0f, 1.0f);
    g.setColour (Theme::border);
    g.drawRoundedRectangle (pill.reduced (0.5f), corner, 1.0f);

    const float h = juce::jlimit (10.0f, 14.0f, pill.getHeight() * 0.36f);
    draw::trackedText (g, processor.currentPresetName().toUpperCase(), pill.reduced (pill.getHeight() + 4.0f, 0.0f), juce::Justification::centred,
                       Theme::displayFont (h, 0.2f), Theme::textPrimary);

    // Tags line
    auto tags = processor.currentPresetTags();
    if (! tags.isEmpty())
    {
        juce::String line;
        for (int i = 0; i < tags.size(); ++i) { if (i > 0) line << juce::String::fromUTF8 ("   \xC2\xB7   "); line << tags[i].toUpperCase(); }
        auto tagArea = pill.withY (pill.getBottom() + 3.0f).withHeight (h * 1.6f);
        draw::trackedText (g, line, tagArea, juce::Justification::centred, Theme::captionFont (juce::jmax (7.5f, h * 0.68f)), Theme::textSecondary);
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
        auto t = std::make_unique<AMTab> (names[i], icons[i], Theme::blue);
        t->onClick = [this, i] { setPage (i); if (onPageChange) onPageChange (i); };
        addAndMakeVisible (*t);
        tabs.push_back (std::move (t));
    }
    if (lab)
    {
        auto t = std::make_unique<AMTab> ("Lab", Icon::Lab, Theme::amber);
        const int idx = (int) tabs.size();
        t->onClick = [this, idx] { setPage (idx); if (onPageChange) onPageChange (idx); };
        addAndMakeVisible (*t);
        tabs.push_back (std::move (t));
    }
    addAndMakeVisible (ab);
    ab.setSelected (processor.currentABSlot(), juce::dontSendNotification);
    ab.onChange = [this] (int i) { processor.selectABSlot (i); morph.setValue (i, juce::dontSendNotification); if (onABChange) onABChange (i); };
    morph.setRange (0.0, 1.0, 0.0);
    morph.setValue (processor.currentABMorph(), juce::dontSendNotification);
    morph.setShowLabel (false);
    morph.setShowValue (false);
    morph.setTooltip ("Morph between the A and B slots");
    morph.setDoubleClickReturnValue (true, (double) processor.currentABSlot());
    morph.onValueChange = [this] { processor.morphAB ((float) morph.getValue()); };
    addAndMakeVisible (morph);
    processor.addChangeListener (this);
    abCopy.setTooltip ("Copy this slot to the other A/B slot");
    abCopy.onClick = [this] { processor.copyABToOther(); };
    addAndMakeVisible (abCopy);

    output.setShowLabel (false);
    output.setShowValue (false);
    output.setTooltip ("Output level");
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.parameters(), ParameterRegistry::get (Param::masterGain).id, output);
    output.setDoubleClickReturnValue (true, ParameterRegistry::get (Param::masterGain).defaultValue);
    addAndMakeVisible (output);
    setPage (0);
}

NavBar::~NavBar()
{
    processor.removeChangeListener (this);
}

void NavBar::changeListenerCallback (juce::ChangeBroadcaster*)
{
    ab.setSelected (processor.currentABSlot(), juce::dontSendNotification);
    if (! morph.isMouseButtonDown())
        morph.setValue (processor.currentABMorph(), juce::dontSendNotification);
}

void NavBar::setPage (int index)
{
    page = juce::jlimit (0, (int) tabs.size() - 1, index);
    for (int i = 0; i < (int) tabs.size(); ++i) tabs[(size_t) i]->setSelected (i == page);
}

void NavBar::resized()
{
    auto area = getLocalBounds();
    const int pad = juce::jmax (10, getWidth() / 64);
    area.reduce (pad, juce::jmax (4, getHeight() / 12));

    const float capH = juce::jlimit (7.5f, 9.5f, (float) getHeight() * 0.11f);
    captionWidth = juce::roundToInt (draw::trackedTextWidth (Theme::captionFont (capH), "FOR A MORE STRANGE TOMORROW")) + 6;
    auto right = area.removeFromRight (juce::jlimit (240, 420, getWidth() * 24 / 100) + captionWidth);
    const int rowH = juce::jlimit (22, 30, getHeight() / 3);
    auto row = right.withSizeKeepingCentre (right.getWidth(), rowH);
    row.removeFromRight (captionWidth + pad / 2);   // brand caption
    const int abW = juce::jlimit (60, 84, row.getWidth() / 4);
    ab.setBounds (row.removeFromLeft (abW));
    row.removeFromLeft (pad / 2);
    morph.setBounds (row.removeFromLeft (abW).reduced (2, 0));
    row.removeFromLeft (pad / 2);
    abCopy.setBounds (row.removeFromLeft (rowH));
    output.setBounds (row.reduced (2, 0));

    area.removeFromLeft (juce::jlimit (50, 90, getWidth() * 5 / 100));   // version caption
    area.removeFromRight (pad);
    const int tabW = juce::jlimit (56, 112, area.getWidth() / (int) tabs.size());
    auto tabArea = area.withSizeKeepingCentre (tabW * (int) tabs.size(), area.getHeight());
    for (auto& t : tabs) t->setBounds (tabArea.removeFromLeft (tabW));
}

void NavBar::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    juce::ColourGradient sep (juce::Colours::transparentWhite, b.getX(), 0.0f, juce::Colours::white.withAlpha (0.07f), b.getCentreX(), 0.0f, false);
    sep.addColour (1.0, juce::Colours::transparentWhite);
    g.setGradientFill (sep);
    g.fillRect (b.withHeight (1.0f));

    const float h = juce::jlimit (7.5f, 9.5f, b.getHeight() * 0.11f);
    const int pad = juce::jmax (10, getWidth() / 64);
    draw::trackedText (g, "V" + juce::String (ANTIMATR_VERSION_STRING), b.withWidth (120.0f).withTrimmedLeft ((float) pad), juce::Justification::centredLeft, Theme::captionFont (h), Theme::textDim);

    auto brand = b.withLeft (b.getRight() - (float) captionWidth - (float) pad).withTrimmedRight ((float) pad);
    draw::trackedText (g, "INSTRUMENTS", brand.withHeight (b.getHeight() * 0.5f), juce::Justification::bottomRight, Theme::captionFont (h), Theme::textDim);
    draw::trackedText (g, "FOR A MORE STRANGE TOMORROW", brand.withTop (b.getCentreY()), juce::Justification::topRight, Theme::captionFont (h), Theme::textDim);
}

} // namespace am::ui

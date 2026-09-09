#include "AntiMatrEditor.h"
#include "dev/dsplab/DSPLabView.h"

namespace am
{

AntiMatrEditor::AntiMatrEditor (AntiMatrProcessor& p)
    : AudioProcessorEditor (p), processor (p), topBar (p), navBar (p, ANTIMATR_DEV != 0), mainView (p)
{
    setLookAndFeel (&lookAndFeel);
    setWantsKeyboardFocus (true);

    addAndMakeVisible (topBar);
    addAndMakeVisible (navBar);
    addAndMakeVisible (mainView);

    // Deep pages (generic in Phase 0; dedicated designs arrive in Phase 13).
    using ui::GroupPage;
    pages.push_back (std::make_unique<GroupPage> (p, "Source", "Choose your energy", std::vector<ParamGroup> { ParamGroup::Source, ParamGroup::Wave, ParamGroup::Dust, ParamGroup::Impact, ParamGroup::Sample, ParamGroup::Gesture }, ui::Theme::blue));
    pages.push_back (std::make_unique<GroupPage> (p, "Shape", "Turn matter into sound", std::vector<ParamGroup> { ParamGroup::Shape }, ui::Theme::cyan));
    pages.push_back (std::make_unique<GroupPage> (p, "Evolve", "Movement & change", std::vector<ParamGroup> { ParamGroup::Evolve }, ui::Theme::violet));
    pages.push_back (std::make_unique<GroupPage> (p, "Fracture", "Break into new realities", std::vector<ParamGroup> { ParamGroup::Fracture }, ui::Theme::magenta));
    pages.push_back (std::make_unique<GroupPage> (p, "Space", "Place it anywhere", std::vector<ParamGroup> { ParamGroup::Space }, ui::Theme::ivory));
    pages.push_back (std::make_unique<GroupPage> (p, "Mod", "Movement sources", std::vector<ParamGroup> { ParamGroup::Mod, ParamGroup::Macro, ParamGroup::Amp, ParamGroup::Master }, ui::Theme::amber));
    for (auto& pg : pages) addChildComponent (*pg);

   #if ANTIMATR_DEV
    labView = std::make_unique<dev::DSPLabView> (p);
    addChildComponent (*labView);
   #endif

    navBar.onPageChange = [this] (int i) { showPage (i); };

    setResizable (true, true);
    setResizeLimits (kMinWidth, kMinHeight, 4000, 2500);
    getConstrainer()->setFixedAspectRatio ((double) AntiMatrProcessor::kDefaultWidth / (double) AntiMatrProcessor::kDefaultHeight);

    int w = processor.lastEditorWidth, h = processor.lastEditorHeight;
    if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        const auto area = display->userArea;
        if (w > area.getWidth() - 40 || h > area.getHeight() - 80)
        {
            const double scale = juce::jmin ((area.getWidth() - 40.0) / w, (area.getHeight() - 80.0) / h);
            w = juce::jmax (kMinWidth, (int) (w * scale));
            h = juce::jmax (kMinHeight, (int) (h * scale));
        }
    }
    setSize (w, h);
    startTimerHz (30);
}

AntiMatrEditor::~AntiMatrEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void AntiMatrEditor::showPage (int index)
{
    const int numPages = 1 + (int) pages.size() + (labView != nullptr ? 1 : 0);
    page = juce::jlimit (0, numPages - 1, index);
    mainView.setVisible (page == 0);
    for (int i = 0; i < (int) pages.size(); ++i) pages[(size_t) i]->setVisible (page == i + 1);
    if (labView != nullptr) labView->setVisible (page == (int) pages.size() + 1);
    navBar.setPage (page);
    resized();
}

bool AntiMatrEditor::keyPressed (const juce::KeyPress& key)
{
    if (labView != nullptr && key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown() && key.getKeyCode() == 'D')
    {
        showPage (page == (int) pages.size() + 1 ? 0 : (int) pages.size() + 1);
        return true;
    }
    if (key.getKeyCode() == juce::KeyPress::escapeKey) { showPage (0); return true; }
    return false;
}

void AntiMatrEditor::timerCallback()
{
    // Adaptive frame rate: drop the visualizer to 30 fps when the audio thread is under pressure.
    if (++frameCounter % 30 == 0)
    {
        const auto perf = processor.diagnostics().profiler.snapshot();
        mainView.visualizer().setTargetFrameRate (perf.totalMovingPercent > 70.0f ? 30 : 60);
    }
}

void AntiMatrEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    juce::ColourGradient grad (ui::Theme::backgroundTop, b.getCentreX(), b.getY(), ui::Theme::background, b.getCentreX(), b.getBottom(), false);
    g.setGradientFill (grad);
    g.fillAll();

    // Faint vignette so panels float on the void.
    juce::ColourGradient vignette (juce::Colours::transparentBlack, b.getCentreX(), b.getCentreY(), juce::Colours::black.withAlpha (0.35f), b.getX(), b.getY(), true);
    g.setGradientFill (vignette);
    g.fillRect (b);
}

void AntiMatrEditor::resized()
{
    processor.lastEditorWidth = getWidth();
    processor.lastEditorHeight = getHeight();

    auto area = getLocalBounds();
    const int topH = juce::roundToInt ((float) getHeight() * 0.098f);
    const int navH = juce::roundToInt ((float) getHeight() * 0.092f);
    topBar.setBounds (area.removeFromTop (topH));
    navBar.setBounds (area.removeFromBottom (navH));
    mainView.setBounds (area);
    for (auto& pg : pages) pg->setBounds (area);
    if (labView != nullptr) labView->setBounds (area);
}

} // namespace am

#pragma once

#include "Controls.h"
#include "ui/components/AMXYPad.h"
#include "ui/components/AMOptionList.h"

namespace am::ui
{

/**
    Waveform of the loaded sample with draggable START and END handles.

    The peaks are re-scanned only when the sample or the width changes, so
    dragging a handle costs one repaint, not a rescan.
*/
class SampleWaveView : public juce::Component,
                       public juce::SettableTooltipClient
{
public:
    explicit SampleWaveView (juce::Colour accent);

    /** Shows a new sample (null clears the display). */
    void setSample (SampleRef sample);
    void setRange (float start, float end);
    void setEnergy (float e) { if (std::abs (e - energy) > 0.02f) { energy = e; repaint(); } }
    void setCaption (const juce::String& text) { caption = text.toUpperCase(); repaint(); }

    std::function<void (float, float)> onRangeChanged;   ///< start, end (0..1)
    std::function<void()> onDragStart, onDragEnd;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent&) override { hovered = -1; repaint(); }

private:
    juce::Rectangle<float> plotArea() const;
    juce::Rectangle<float> rulerArea() const;
    void rebuildPeaks();
    void paintEmptyState (juce::Graphics& g);
    void paintRuler (juce::Graphics& g);
    int  handleAt (juce::Point<int> p) const;
    float xForPosition (float position01) const;
    float positionForX (float x) const;

    SampleRef sample;
    juce::Colour accent;
    std::vector<float> minPeaks, maxPeaks, rmsPeaks;
    int peakWidth = 0;
    float normalise = 1.0f;          ///< display gain so quiet material still fills the view
    float start = 0.0f, end = 1.0f, energy = 0.0f;
    int dragging = -1, hovered = -1;
    juce::String caption;
};

//==============================================================================
/**
    SOURCE page — SAMPLE.

    Built-in picker, file loading (button or drag and drop), the waveform with
    start/end handles, the playback controls and ANALYZE → MATTER.
*/
class SamplePanel : public AMPanel,
                    public juce::FileDragAndDropTarget,
                    private juce::Timer
{
public:
    explicit SamplePanel (AntiMatrProcessor& p);
    ~SamplePanel() override;

    void resized() override;
    void paintOverChildren (juce::Graphics& g) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dropActive = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dropActive = false; repaint(); }
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    void timerCallback() override;
    void refreshSample (bool force);
    void setRangeFromParams();
    void chooseFile();
    void runAnalysis();
    void showAnalysis();

    AntiMatrProcessor& processor;
    std::unique_ptr<AMChoice> builtIn;
    AMButton  loadButton { "Load File", Theme::ivory };
    AMButton  analyzeButton { juce::String::fromUTF8 ("Analyze \xe2\x86\x92 Matter"), Theme::cyan };
    SampleWaveView waveView { Theme::ivory };
    std::vector<std::unique_ptr<BoundControl>> controls;
    std::unique_ptr<juce::ParameterAttachment> startAttachment, endAttachment;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Rectangle<int> statusStrip;
    juce::String infoLine, statusLine;
    bool statusIsWarning = false, dropActive = false;
    uint32_t shownVersion = 0xffffffffu;
};

//==============================================================================
/**
    SOURCE page — GESTURE.

    Mode picker with a one-line description, the pressure × speed pad and the
    four shaping controls.
*/
class GesturePanel : public AMPanel,
                     private juce::Timer
{
public:
    explicit GesturePanel (AntiMatrProcessor& p);
    ~GesturePanel() override;

    void resized() override;

private:
    void timerCallback() override;

    AntiMatrProcessor& processor;
    std::unique_ptr<AMOptionList> mode;
    AMXYPad  pad { "Pressure", "Speed", Theme::magenta };
    std::vector<std::unique_ptr<BoundControl>> controls;
    std::unique_ptr<juce::ParameterAttachment> modeAttachment, padX, padY;
    int currentMode = 0;
};

} // namespace am::ui

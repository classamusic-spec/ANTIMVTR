#pragma once

#include "dev/diagnostics/Diagnostics.h"
#include "ui/AntiMatrTheme.h"
#include "ui/components/AMDrawing.h"

#include <functional>
#include <vector>

namespace am
{
    class AntiMatrProcessor;
}

namespace am::dev
{

/** Everything a DSP LAB view needs for one refresh tick. */
struct LabFrame
{
    AntiMatrProcessor&        processor;
    Diagnostics&              diagnostics;
    const DiagnosticSnapshot& snapshot;
    double                    sampleRate = 48000.0;
    int                       tick = 0;      ///< monotonic frame counter (25 Hz)
};

/**
    Base class of every DSP LAB view.

    The shell owns the only timer: it reads one snapshot per tick and calls
    `updateFrame` on the views that are actually on screen, so hidden pages
    cost nothing (no FFTs, no table rebuilds).
*/
class LabView : public juce::Component
{
public:
    ~LabView() override = default;
    virtual void updateFrame (const LabFrame&) {}
};

//==============================================================================
/** Human readable stage name. */
const char* stageName (Stage s) noexcept;
/** Short stage name for tight labels. */
const char* stageShortName (Stage s) noexcept;
/** Accent colour used consistently for a stage across the lab. */
juce::Colour stageColour (Stage s) noexcept;

/** Formats a linear gain as a dBFS string ("-inf" below the floor). */
juce::String dbString (float gain, int decimals = 1);

//==============================================================================
/** A titled inset surface: the standard container of the lab. */
class LabPanel : public juce::Component
{
public:
    explicit LabPanel (juce::String titleText = {});

    void setTitle (const juce::String& t);
    void setSubtitle (const juce::String& t);
    void setAccent (juce::Colour c) { accent = c; repaint(); }

    void paint (juce::Graphics& g) override;
    void resized() override;

    /** Area inside the frame and under the title, where content is drawn. */
    juce::Rectangle<int> contentBounds() const;
    juce::Rectangle<float> contentBoundsF() const { return contentBounds().toFloat(); }

    static constexpr float kTitleHeight = 17.0f;

protected:
    juce::String title, subtitle;
    juce::Colour accent { ui::Theme::cyan };
};

//==============================================================================
/** Two-column key/value readout. Rows are set wholesale on each refresh. */
class KeyValueTable : public LabPanel
{
public:
    using Row = std::pair<juce::String, juce::String>;

    explicit KeyValueTable (juce::String titleText = {}) : LabPanel (std::move (titleText)) {}

    void setRows (std::vector<Row> rows);
    /** Highlights a row's value colour (used for non-zero safety counters). */
    void setRowColour (int index, juce::Colour c);
    void clearRowColours() { rowColours.clear(); repaint(); }
    void setRowHeight (float h) { rowHeight = h; repaint(); }
    void setLabelWidthFraction (float f) { labelFraction = juce::jlimit (0.2f, 0.9f, f); repaint(); }

    void paint (juce::Graphics& g) override;

private:
    std::vector<Row> rows;
    std::vector<std::pair<int, juce::Colour>> rowColours;
    float rowHeight = 15.0f;
    float labelFraction = 0.55f;
};

//==============================================================================
/**
    Compact sortable table.

    Wraps juce::TableListBox so the lab gets scrolling, click-to-sort headers
    and cell painting in the ANTI-MATR palette without every view repeating
    the boilerplate. Data is supplied as strings plus a sort key per cell.
*/
class LabTable : public juce::Component,
                 private juce::TableListBoxModel
{
public:
    struct Column
    {
        juce::String name;
        int   width = 60;
        bool  numeric = true;      ///< right aligned and sorted numerically
    };

    struct Cell
    {
        juce::String text;
        double sortValue = 0.0;
        juce::Colour colour { ui::Theme::textPrimary };
    };

    using Row = std::vector<Cell>;

    LabTable();
    ~LabTable() override;

    void setColumns (std::vector<Column> columns);
    /** Sets the column (1-based) and direction the table sorts by initially. */
    void setDefaultSort (int columnId, bool forwards);
    void setRows (std::vector<Row> rows);
    void setRowColourFn (std::function<juce::Colour (int row)> fn) { rowTint = std::move (fn); }

    int  numRows() const noexcept { return (int) data.size(); }
    int  selectedRow() const;
    void selectRow (int row);
    /** Called with the model row index (-1 when nothing is selected). */
    std::function<void (int)> onSelectionChanged;

    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    int  getNumRows() override { return (int) order.size(); }
    void paintRowBackground (juce::Graphics&, int rowNumber, int width, int height, bool selected) override;
    void paintCell (juce::Graphics&, int rowNumber, int columnId, int width, int height, bool selected) override;
    void sortOrderChanged (int newSortColumnId, bool isForwards) override;
    void selectedRowsChanged (int lastRowSelected) override;

    void rebuildOrder();

    juce::TableListBox table;
    std::vector<Column> columns;
    std::vector<Row> data;
    std::vector<int> order;          ///< view row -> model row
    std::function<juce::Colour (int)> rowTint;
    int sortColumn = 0;
    bool sortForwards = true;
};

//==============================================================================
/** Horizontal bar meter with a dB scale, one row per stage. */
class StageMeters : public LabPanel
{
public:
    StageMeters();

    void setLevels (const DiagnosticSnapshot& s);
    void paint (juce::Graphics& g) override;

private:
    StageLevels levels[(int) Stage::Count] {};
    float holdPeak[(int) Stage::Count] {};
};

//==============================================================================
/** Drawing helpers shared by the plots. */
namespace plot
{
    /** Log frequency grid with labels; returns the plot area minus any labels. */
    void frequencyGrid (juce::Graphics& g, juce::Rectangle<float> area, float minHz, float maxHz, bool labels = true);
    /** Horizontal dB grid (0 .. floorDb). */
    void decibelGrid (juce::Graphics& g, juce::Rectangle<float> area, float floorDb, bool labels = true);
    /** Fills + strokes a spectrum curve from log-spaced 0..1 magnitudes. */
    void spectrumCurve (juce::Graphics& g, juce::Rectangle<float> area, const float* bands, int numBands,
                        juce::Colour colour, float fillAlpha = 0.18f);
    /** Draws a stereo waveform pair. */
    void waveform (juce::Graphics& g, juce::Rectangle<float> area, const float* l, const float* r, int n,
                   juce::Colour left, juce::Colour right);
    /** A caption in the lab's tracked style. */
    void caption (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                  juce::Colour colour = ui::Theme::textSecondary, float height = 9.0f,
                  juce::Justification just = juce::Justification::centredLeft);
    /** "no data" placeholder, so an empty view never looks broken. */
    void emptyState (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& text);
}

//==============================================================================
/** Styles a text button in the lab palette. */
void styleButton (juce::Button& b, juce::Colour accent = ui::Theme::cyan);
/** Styles a combo box in the lab palette. */
void styleCombo (juce::ComboBox& c);
/** Styles a toggle button in the lab palette. */
void styleToggle (juce::ToggleButton& t, juce::Colour accent = ui::Theme::cyan);

} // namespace am::dev

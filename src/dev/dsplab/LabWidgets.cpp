#include "LabWidgets.h"

#include <algorithm>

namespace am::dev
{

using namespace am::ui;

//==============================================================================
const char* stageName (Stage s) noexcept
{
    switch (s)
    {
        case Stage::Source:       return "Source";
        case Stage::PostMatter:   return "Post-Matter";
        case Stage::PostEvolve:   return "Post-Evolve";
        case Stage::PostFracture: return "Post-Fracture";
        case Stage::PostSpace:    return "Post-Space";
        case Stage::Master:       return "Master";
        default:                  return "?";
    }
}

const char* stageShortName (Stage s) noexcept
{
    switch (s)
    {
        case Stage::Source:       return "SRC";
        case Stage::PostMatter:   return "MTR";
        case Stage::PostEvolve:   return "EVO";
        case Stage::PostFracture: return "FRC";
        case Stage::PostSpace:    return "SPC";
        case Stage::Master:       return "MST";
        default:                  return "?";
    }
}

juce::Colour stageColour (Stage s) noexcept
{
    switch (s)
    {
        case Stage::Source:       return Theme::blue;
        case Stage::PostMatter:   return Theme::cyan;
        case Stage::PostEvolve:   return Theme::violet;
        case Stage::PostFracture: return Theme::magenta;
        case Stage::PostSpace:    return Theme::ivory;
        case Stage::Master:       return Theme::amber;
        default:                  return Theme::textSecondary;
    }
}

juce::String dbString (float gain, int decimals)
{
    if (! std::isfinite (gain) || gain <= 1.0e-7f)
        return "-inf";
    return juce::String (gainToDb (gain), decimals);
}

//==============================================================================
LabPanel::LabPanel (juce::String titleText) : title (std::move (titleText))
{
    setInterceptsMouseClicks (false, true);
}

void LabPanel::setTitle (const juce::String& t)
{
    if (title != t) { title = t; repaint(); }
}

void LabPanel::setSubtitle (const juce::String& t)
{
    if (subtitle != t) { subtitle = t; repaint(); }
}

juce::Rectangle<int> LabPanel::contentBounds() const
{
    auto b = getLocalBounds().reduced (6, 5);
    if (title.isNotEmpty())
        b.removeFromTop ((int) kTitleHeight);
    return b;
}

void LabPanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    draw::insetSurface (g, b, 5.0f);

    if (title.isEmpty())
        return;

    auto header = b.reduced (7.0f, 4.0f).removeFromTop (kTitleHeight - 2.0f);
    if (subtitle.isNotEmpty())
    {
        const float subWidth = juce::jmin (header.getWidth() * 0.62f,
                                           Theme::font (9.5f).getStringWidthFloat (subtitle) + 8.0f);
        auto right = header.removeFromRight (subWidth);
        draw::trackedText (g, subtitle, right, juce::Justification::centredRight, Theme::font (9.5f), Theme::textDim);
    }
    draw::trackedText (g, title.toUpperCase(), header, juce::Justification::centredLeft, Theme::captionFont (9.0f), accent);
}

void LabPanel::resized() {}

//==============================================================================
void KeyValueTable::setRows (std::vector<Row> newRows)
{
    if (newRows == rows)
        return;
    rows = std::move (newRows);
    repaint();
}

void KeyValueTable::setRowColour (int index, juce::Colour c)
{
    rowColours.emplace_back (index, c);
}

void KeyValueTable::paint (juce::Graphics& g)
{
    LabPanel::paint (g);

    auto area = contentBoundsF();
    g.setFont (Theme::font (11.0f));

    for (size_t i = 0; i < rows.size(); ++i)
    {
        if (area.getHeight() < rowHeight)
            break;

        auto row = area.removeFromTop (rowHeight);
        juce::Colour valueColour = Theme::textPrimary;
        for (const auto& c : rowColours)
            if (c.first == (int) i) valueColour = c.second;

        if (rows[i].first.isEmpty() && rows[i].second.isEmpty())
            continue;

        // A row with an empty value is a section divider.
        if (rows[i].second.isEmpty())
        {
            draw::trackedText (g, rows[i].first.toUpperCase(), row, juce::Justification::centredLeft,
                               Theme::captionFont (8.5f), Theme::textDim);
            g.setFont (Theme::font (11.0f));
            continue;
        }

        g.setColour (Theme::textSecondary);
        g.drawText (rows[i].first, row.removeFromLeft (row.getWidth() * labelFraction),
                    juce::Justification::centredLeft, false);
        g.setColour (valueColour);
        g.drawText (rows[i].second, row, juce::Justification::centredRight, false);
    }
}

//==============================================================================
LabTable::LabTable()
{
    table.setModel (this);
    table.setHeaderHeight (18);
    table.setRowHeight (14);
    table.setColour (juce::TableListBox::backgroundColourId, juce::Colours::transparentBlack);
    table.setColour (juce::TableListBox::outlineColourId, juce::Colours::transparentBlack);
    table.getViewport()->setScrollBarThickness (8);

    auto& header = table.getHeader();
    header.setColour (juce::TableHeaderComponent::backgroundColourId, Theme::panelTop);
    header.setColour (juce::TableHeaderComponent::outlineColourId, Theme::borderSoft);
    header.setColour (juce::TableHeaderComponent::textColourId, Theme::cyan);
    header.setColour (juce::TableHeaderComponent::highlightColourId, Theme::cyan.withAlpha (0.12f));

    addAndMakeVisible (table);
}

LabTable::~LabTable()
{
    table.setModel (nullptr);
}

void LabTable::setColumns (std::vector<Column> newColumns)
{
    columns = std::move (newColumns);
    auto& header = table.getHeader();
    header.removeAllColumns();
    for (size_t i = 0; i < columns.size(); ++i)
        header.addColumn (columns[i].name, (int) i + 1, columns[i].width, 24, -1,
                          juce::TableHeaderComponent::visible | juce::TableHeaderComponent::sortable);
    sortColumn = 1;
    sortForwards = true;
    header.setSortColumnId (1, true);
}

void LabTable::setDefaultSort (int columnId, bool forwards)
{
    sortColumn = juce::jlimit (1, juce::jmax (1, (int) columns.size()), columnId);
    sortForwards = forwards;
    table.getHeader().setSortColumnId (sortColumn, forwards);
    rebuildOrder();
    table.repaint();
}

void LabTable::setRows (std::vector<Row> newRows)
{
    const bool sizeChanged = newRows.size() != data.size();
    data = std::move (newRows);
    rebuildOrder();
    if (sizeChanged)
        table.updateContent();
    table.repaint();
}

void LabTable::rebuildOrder()
{
    order.resize (data.size());
    for (size_t i = 0; i < order.size(); ++i)
        order[i] = (int) i;

    const int col = sortColumn - 1;
    if (col < 0 || columns.empty())
        return;

    const bool numeric = col < (int) columns.size() ? columns[(size_t) col].numeric : true;
    std::stable_sort (order.begin(), order.end(), [&] (int a, int b)
    {
        const auto& ra = data[(size_t) a];
        const auto& rb = data[(size_t) b];
        if (col >= (int) ra.size() || col >= (int) rb.size())
            return a < b;
        const bool less = numeric ? ra[(size_t) col].sortValue < rb[(size_t) col].sortValue
                                  : ra[(size_t) col].text.compareNatural (rb[(size_t) col].text) < 0;
        const bool greater = numeric ? rb[(size_t) col].sortValue < ra[(size_t) col].sortValue
                                     : rb[(size_t) col].text.compareNatural (ra[(size_t) col].text) < 0;
        if (! less && ! greater)
            return a < b;
        return sortForwards ? less : greater;
    });
}

void LabTable::sortOrderChanged (int newSortColumnId, bool isForwards)
{
    sortColumn = newSortColumnId;
    sortForwards = isForwards;
    rebuildOrder();
    table.repaint();
}

void LabTable::selectedRowsChanged (int lastRowSelected)
{
    if (onSelectionChanged == nullptr)
        return;
    const int model = (lastRowSelected >= 0 && lastRowSelected < (int) order.size()) ? order[(size_t) lastRowSelected] : -1;
    onSelectionChanged (model);
}

int LabTable::selectedRow() const
{
    const int view = table.getSelectedRow();
    return (view >= 0 && view < (int) order.size()) ? order[(size_t) view] : -1;
}

void LabTable::selectRow (int modelRow)
{
    for (size_t i = 0; i < order.size(); ++i)
        if (order[i] == modelRow)
        {
            table.selectRow ((int) i, true, true);
            return;
        }
}

void LabTable::paintRowBackground (juce::Graphics& g, int rowNumber, int width, int height, bool selected)
{
    juce::Colour c = (rowNumber % 2) == 0 ? juce::Colours::transparentBlack : juce::Colours::white.withAlpha (0.015f);
    if (rowTint != nullptr && rowNumber >= 0 && rowNumber < (int) order.size())
        c = c.overlaidWith (rowTint (order[(size_t) rowNumber]));
    if (selected)
        c = Theme::cyan.withAlpha (0.16f);
    g.setColour (c);
    g.fillRect (0, 0, width, height);
}

void LabTable::paintCell (juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool selected)
{
    if (rowNumber < 0 || rowNumber >= (int) order.size())
        return;

    const auto& row = data[(size_t) order[(size_t) rowNumber]];
    const int col = columnId - 1;
    if (col < 0 || col >= (int) row.size())
        return;

    g.setFont (Theme::font (10.5f));
    g.setColour (selected ? Theme::textPrimary : row[(size_t) col].colour);
    const bool numeric = col < (int) columns.size() ? columns[(size_t) col].numeric : true;
    g.drawText (row[(size_t) col].text, 3, 0, width - 6, height,
                numeric ? juce::Justification::centredRight : juce::Justification::centredLeft, false);
}

void LabTable::resized()
{
    table.setBounds (getLocalBounds());
}

void LabTable::paint (juce::Graphics& g)
{
    if (data.empty())
        plot::emptyState (g, getLocalBounds().toFloat().withTrimmedTop (20.0f), "NO ROWS");
}

//==============================================================================
StageMeters::StageMeters() : LabPanel ("Stage levels  rms / peak dBFS") {}

void StageMeters::setLevels (const DiagnosticSnapshot& s)
{
    for (int i = 0; i < (int) Stage::Count; ++i)
    {
        levels[i] = s.stages[i];
        holdPeak[i] = juce::jmax (holdPeak[i] * 0.94f, s.stages[i].peak);
    }
    repaint();
}

void StageMeters::paint (juce::Graphics& g)
{
    LabPanel::paint (g);

    auto area = contentBoundsF();
    const float rowH = juce::jmin (18.0f, area.getHeight() / (float) Stage::Count);
    constexpr float floorDb = -72.0f;

    for (int i = 0; i < (int) Stage::Count; ++i)
    {
        if (area.getHeight() < rowH) break;
        auto row = area.removeFromTop (rowH);
        auto label = row.removeFromLeft (34.0f);
        auto readout = row.removeFromRight (86.0f);
        auto bar = row.reduced (2.0f, rowH * 0.22f);

        const auto colour = stageColour ((Stage) i);
        draw::trackedText (g, stageShortName ((Stage) i), label, juce::Justification::centredLeft,
                           Theme::captionFont (8.5f), colour.withAlpha (0.85f));

        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.fillRoundedRectangle (bar, 1.5f);

        auto normalise = [] (float gain) { return juce::jlimit (0.0f, 1.0f, (gainToDb (gain) - floorDb) / -floorDb); };
        const float rmsW = normalise (levels[i].rms) * bar.getWidth();
        g.setColour (colour.withAlpha (0.55f));
        g.fillRoundedRectangle (bar.withWidth (juce::jmax (1.0f, rmsW)), 1.5f);

        const float peakX = bar.getX() + normalise (levels[i].peak) * bar.getWidth();
        g.setColour (levels[i].peak >= 0.999f ? Theme::magenta : colour);
        g.drawLine (peakX, bar.getY(), peakX, bar.getBottom(), 1.5f);

        const float holdX = bar.getX() + normalise (holdPeak[i]) * bar.getWidth();
        g.setColour (colour.withAlpha (0.35f));
        g.drawLine (holdX, bar.getY(), holdX, bar.getBottom(), 1.0f);

        g.setFont (Theme::font (9.5f));
        g.setColour (levels[i].peak >= 0.999f ? Theme::magenta : Theme::textSecondary);
        g.drawText (dbString (levels[i].rms) + " / " + dbString (levels[i].peak), readout,
                    juce::Justification::centredRight, false);
    }
}

//==============================================================================
namespace plot
{

void caption (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
              juce::Colour colour, float height, juce::Justification just)
{
    draw::trackedText (g, text, area, just, Theme::captionFont (height), colour);
}

void emptyState (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& text)
{
    draw::trackedText (g, text.toUpperCase(), area, juce::Justification::centred, Theme::captionFont (9.5f), Theme::textDim);
}

void frequencyGrid (juce::Graphics& g, juce::Rectangle<float> area, float minHz, float maxHz, bool labels)
{
    if (! (maxHz > minHz) || minHz <= 0.0f)
        return;

    const double span = std::log (maxHz / minHz);
    static const float decades[] = { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    g.setFont (Theme::font (8.5f));

    for (float f : decades)
    {
        if (f < minHz || f > maxHz) continue;
        const float u = (float) (std::log (f / minHz) / span);
        const float x = area.getX() + u * area.getWidth();
        const bool major = (f == 100.0f || f == 1000.0f || f == 10000.0f);
        g.setColour (juce::Colours::white.withAlpha (major ? 0.09f : 0.045f));
        g.drawLine (x, area.getY(), x, area.getBottom(), 1.0f);

        if (labels && major)
        {
            g.setColour (Theme::textDim);
            g.drawText (f >= 1000.0f ? juce::String (f / 1000.0f, 0) + "k" : juce::String (f, 0),
                        juce::Rectangle<float> (x + 2.0f, area.getBottom() - 11.0f, 30.0f, 10.0f),
                        juce::Justification::centredLeft, false);
        }
    }
}

void decibelGrid (juce::Graphics& g, juce::Rectangle<float> area, float floorDb, bool labels)
{
    g.setFont (Theme::font (8.5f));
    for (int db = 0; db >= (int) floorDb; db -= 24)
    {
        const float u = (float) db / floorDb;
        const float y = area.getY() + u * area.getHeight();
        g.setColour (juce::Colours::white.withAlpha (db == 0 ? 0.10f : 0.045f));
        g.drawLine (area.getX(), y, area.getRight(), y, 1.0f);
        if (labels)
        {
            g.setColour (Theme::textDim);
            g.drawText (juce::String (db), juce::Rectangle<float> (area.getX() + 2.0f, y + 1.0f, 28.0f, 9.0f),
                        juce::Justification::centredLeft, false);
        }
    }
}

void spectrumCurve (juce::Graphics& g, juce::Rectangle<float> area, const float* bands, int numBands,
                    juce::Colour colour, float fillAlpha)
{
    if (bands == nullptr || numBands < 2)
        return;

    juce::Path p;
    p.startNewSubPath (area.getX(), area.getBottom());
    for (int i = 0; i < numBands; ++i)
    {
        const float x = area.getX() + area.getWidth() * (float) i / (float) (numBands - 1);
        const float v = std::isfinite (bands[i]) ? juce::jlimit (0.0f, 1.0f, bands[i]) : 0.0f;
        p.lineTo (x, area.getBottom() - v * area.getHeight());
    }
    p.lineTo (area.getRight(), area.getBottom());

    if (fillAlpha > 0.0f)
    {
        juce::Path filled (p);
        filled.closeSubPath();
        g.setColour (colour.withAlpha (fillAlpha));
        g.fillPath (filled);
    }
    g.setColour (colour);
    g.strokePath (p, juce::PathStrokeType (1.2f));
}

void waveform (juce::Graphics& g, juce::Rectangle<float> area, const float* l, const float* r, int n,
               juce::Colour left, juce::Colour right)
{
    if (n < 2)
        return;

    g.setColour (Theme::borderSoft);
    g.drawLine (area.getX(), area.getCentreY(), area.getRight(), area.getCentreY(), 1.0f);
    for (float ref : { 0.5f, 1.0f })
    {
        g.setColour (juce::Colours::white.withAlpha (ref == 1.0f ? 0.07f : 0.035f));
        g.drawLine (area.getX(), area.getCentreY() - ref * area.getHeight() * 0.5f,
                    area.getRight(), area.getCentreY() - ref * area.getHeight() * 0.5f, 1.0f);
        g.drawLine (area.getX(), area.getCentreY() + ref * area.getHeight() * 0.5f,
                    area.getRight(), area.getCentreY() + ref * area.getHeight() * 0.5f, 1.0f);
    }

    // One vertical min/max span per pixel column keeps dense buffers honest.
    const int columns = juce::jmax (2, (int) area.getWidth());
    auto drawChannel = [&] (const float* data, juce::Colour colour)
    {
        if (data == nullptr) return;
        g.setColour (colour);
        for (int c = 0; c < columns; ++c)
        {
            const int i0 = (int) ((int64_t) c * n / columns);
            const int i1 = juce::jmax (i0 + 1, (int) ((int64_t) (c + 1) * n / columns));
            float lo = 1.0f, hi = -1.0f;
            for (int i = i0; i < i1 && i < n; ++i)
            {
                const float v = std::isfinite (data[i]) ? juce::jlimit (-1.0f, 1.0f, data[i]) : 0.0f;
                lo = juce::jmin (lo, v);
                hi = juce::jmax (hi, v);
            }
            if (lo > hi) continue;
            const float x = area.getX() + (float) c + 0.5f;
            const float y0 = area.getCentreY() - hi * area.getHeight() * 0.5f;
            const float y1 = area.getCentreY() - lo * area.getHeight() * 0.5f;
            g.drawLine (x, y0, x, juce::jmax (y1, y0 + 0.6f), 1.0f);
        }
    };

    drawChannel (r, right);
    drawChannel (l, left);
}

} // namespace plot

//==============================================================================
void styleButton (juce::Button& b, juce::Colour accent)
{
    b.setColour (juce::TextButton::buttonColourId, Theme::panelTop);
    b.setColour (juce::TextButton::buttonOnColourId, accent.withAlpha (0.28f));
    b.setColour (juce::TextButton::textColourOffId, Theme::textSecondary);
    b.setColour (juce::TextButton::textColourOnId, Theme::textPrimary);
}

void styleCombo (juce::ComboBox& c)
{
    c.setColour (juce::ComboBox::backgroundColourId, Theme::panelInset);
    c.setColour (juce::ComboBox::outlineColourId, Theme::border);
    c.setColour (juce::ComboBox::textColourId, Theme::textPrimary);
    c.setColour (juce::ComboBox::arrowColourId, Theme::cyan);
}

void styleToggle (juce::ToggleButton& t, juce::Colour accent)
{
    t.setColour (juce::ToggleButton::textColourId, Theme::textSecondary);
    t.setColour (juce::ToggleButton::tickColourId, accent);
    t.setColour (juce::ToggleButton::tickDisabledColourId, Theme::textDim);
}

} // namespace am::dev

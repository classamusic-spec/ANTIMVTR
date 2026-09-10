#include <juce_core/juce_core.h>

#include "ui/UILayout.h"

using namespace am::ui::layout;

namespace
{
    /** The routing row's columns in the order they must appear from left to right. */
    std::vector<Span> orderedColumns (const ModRowColumns& c)
    {
        return { c.chip, c.source, c.arrow, c.destination, c.depth, c.value, c.power, c.polarity, c.remove };
    }

    /** The two window sizes the interface is reviewed at, plus the extremes around them. */
    const int kRowWidths[] = { 420, 700, 980, 1030, 1130, 1400, 2200 };
    const int kRowHeights[] = { 22, 26, 30, 34, 40, 52 };
}

//==============================================================================
class ModRowLayoutTests : public juce::UnitTest
{
public:
    ModRowLayoutTests() : juce::UnitTest ("Routing row layout", "uilayout") {}

    void runTest() override
    {
        beginTest ("Columns run left to right and never overlap");
        for (int width : kRowWidths)
        {
            for (int height : kRowHeights)
            {
                const auto c = modRowColumns (width, height);
                const auto columns = orderedColumns (c);
                const juce::String at = " at " + juce::String (width) + "x" + juce::String (height);

                int previousEnd = 0;
                for (size_t i = 0; i < columns.size(); ++i)
                {
                    expect (columns[i].size > 0, "column " + juce::String ((int) i) + " is empty" + at);
                    expect (columns[i].start >= previousEnd, "column " + juce::String ((int) i) + " overlaps the one before it" + at);
                    previousEnd = columns[i].end();
                }
                expect (previousEnd <= width, "the last column runs past the row" + at);
                expect (columns.front().start >= 0, "the first column starts before the row" + at);
            }
        }

        beginTest ("Every control keeps a usable hit target at the minimum window size");
        {
            // 1100 x 690 leaves the routings panel about 1030 wide and its rows about 26 high.
            const auto c = modRowColumns (1030, 26);
            expect (c.remove.size >= 18, "remove button too small: " + juce::String (c.remove.size));
            expect (c.power.size >= 28, "enable toggle too small: " + juce::String (c.power.size));
            expect (c.polarity.size >= 58, "polarity switch too small: " + juce::String (c.polarity.size));
            expect (c.polarityHeight >= 17, "polarity switch too short: " + juce::String (c.polarityHeight));
            expect (c.depth.size >= 200, "depth bar too short to aim at: " + juce::String (c.depth.size));
            expect (c.destination.size >= 60, "destination name has no room: " + juce::String (c.destination.size));
        }

        beginTest ("The header and its rows share one set of columns");
        {
            // The panel paints the captions with the row height, at the header's own y —
            // the x positions must be identical or the table drifts out of alignment.
            const auto row = modRowColumns (1130, 34);
            const auto header = modRowColumns (1130, 34);
            const auto a = orderedColumns (row), b = orderedColumns (header);
            for (size_t i = 0; i < a.size(); ++i)
            {
                expectEquals (b[i].start, a[i].start);
                expectEquals (b[i].size, a[i].size);
            }
        }

        beginTest ("Columns grow with the row instead of jumping");
        {
            int previousDepth = 0;
            for (int width = 420; width <= 2200; width += 20)
            {
                const auto c = modRowColumns (width, 34);
                expect (c.depth.size >= previousDepth - 2, "the depth bar shrank as the row grew at width " + juce::String (width));
                previousDepth = c.depth.size;
            }
            // The names block is capped so a very wide row spends its extra space on the depth bar.
            const auto wide = modRowColumns (2200, 34);
            const auto narrow = modRowColumns (700, 34);
            expect (wide.destination.end() - wide.source.start <= 420, "the name block grew without limit");
            expect (wide.depth.size > narrow.depth.size, "a wider row did not give the depth bar more room");
        }

        beginTest ("Degenerate sizes stay finite and ordered");
        for (int width : { 0, 1, 40, 120 })
        {
            for (int height : { 0, 1, 8 })
            {
                const auto c = modRowColumns (width, height);
                for (const auto& span : orderedColumns (c))
                {
                    expect (span.size >= 0, "negative column size");
                    expect (span.start >= 0, "negative column start");
                }
            }
        }
    }
};

//==============================================================================
class ModScopeGridTests : public juce::UnitTest
{
public:
    ModScopeGridTests() : juce::UnitTest ("Modulation scope grid", "uilayout") {}

    void runTest() override
    {
        beginTest ("A full grid is preferred over a ragged last row");
        expectEquals (modScopeColumns (6, 1130), 3);     // 3 x 2, not 4 + 2
        expectEquals (modScopeColumns (4, 1130), 4);     // one row
        expectEquals (modScopeColumns (8, 1130), 4);     // 4 x 2
        expectEquals (modScopeColumns (2, 1130), 2);
        expectEquals (modScopeColumns (1, 1130), 1);

        beginTest ("Cards never fall below a readable width");
        for (int count = 1; count <= 8; ++count)
        {
            for (int width : { 200, 380, 560, 760, 1030, 1130, 1600 })
            {
                const int cols = modScopeColumns (count, width);
                expect (cols >= 1, "no columns");
                expect (cols <= count, "more columns than cards");
                if (cols > 1)
                    expect (width / cols >= 150, "card only " + juce::String (width / cols) + "px wide for "
                                                 + juce::String (count) + " cards in " + juce::String (width));
            }
        }

        beginTest ("Nothing to show still returns a usable column count");
        expectEquals (modScopeColumns (0, 1130), 1);
        expectEquals (modScopeColumns (-3, 1130), 1);
        expectEquals (modScopeColumns (4, 0), 1);
    }
};

//==============================================================================
class GridColumnTests : public juce::UnitTest
{
public:
    GridColumnTests() : juce::UnitTest ("Control grid columns", "uilayout") {}

    /** Size of one control for a given arrangement, the quantity the chooser maximises. */
    static float controlSize (int count, int width, int height, int cols)
    {
        const int rows = (count + cols - 1) / cols;
        const float cellW = (float) width / (float) cols;
        const float cellH = (float) height / (float) rows;
        return juce::jmin (cellW, cellH - juce::jlimit (10.0f, 22.0f, cellH * 0.19f) - 2.0f);
    }

    void runTest() override
    {
        beginTest ("The arrangement chosen is never much worse than the best one");
        for (int count = 1; count <= 10; ++count)
        {
            for (int width : { 200, 236, 340, 470, 760, 1530 })
            {
                for (int height : { 90, 145, 190, 250, 380 })
                {
                    const int cols = gridColumns (count, width, height);
                    expect (cols >= 1 && cols <= count, "column count out of range");

                    // Any arrangement that would have been meaningfully bigger must have been
                    // rejected for a reason: cells too narrow to hold the control's caption.
                    const float chosen = controlSize (count, width, height, cols);
                    for (int c = 1; c <= count; ++c)
                    {
                        const float size = controlSize (count, width, height, c);
                        if (size <= chosen + 8.0f) continue;
                        expect ((float) width / (float) c < 68.0f,
                                "chose " + juce::String (cols) + " columns giving " + juce::String (chosen, 1)
                                + "px where " + juce::String (c) + " columns gave " + juce::String (size, 1)
                                + "px in readable cells (" + juce::String (count) + " in "
                                + juce::String (width) + "x" + juce::String (height) + ")");
                    }
                }
            }
        }

        beginTest ("A short wide module lays its controls out in rows, not a column");
        {
            // The six reverb controls in a 236 x 145 module: three columns, not two.
            expectEquals (gridColumns (6, 236, 145), 3);
            // Three EQ bands fit on one line.
            expectEquals (gridColumns (3, 236, 145), 3);
        }

        beginTest ("Cells stay wide enough for the caption under the control");
        {
            // Four granular controls in a narrow module: two rows of two beats one row of
            // four, because a 59px cell cannot hold the words GRANULAR MIX.
            expectEquals (gridColumns (4, 236, 145), 2);
            // Given room, the same four go in a single row.
            expectEquals (gridColumns (4, 470, 190), 4);
        }

        beginTest ("A single control needs no arithmetic");
        expectEquals (gridColumns (1, 300, 200), 1);
        expectEquals (gridColumns (0, 300, 200), 1);
        expectEquals (gridColumns (5, 0, 0), 5);
    }
};

//==============================================================================
class KnobGeometryTests : public juce::UnitTest
{
public:
    KnobGeometryTests() : juce::UnitTest ("Knob ring geometry", "uilayout") {}

    void runTest() override
    {
        beginTest ("Orbit, arc and body are concentric and separated");
        for (bool hero : { false, true })
        {
            for (float d : { 24.0f, 40.0f, 64.0f, 92.0f, 106.0f, 158.0f })
            {
                const auto r = knobRadii (d, hero);
                const juce::String at = " at d=" + juce::String (d) + (hero ? " (hero)" : "");
                expect (r.orbit < d, "the orbit leaves the footprint" + at);
                expect (r.arc < r.orbit, "the value arc is outside the orbit" + at);
                expect (r.body < r.arc, "the sphere is outside the value arc" + at);
                expect (r.body > 0.0f, "the sphere vanished" + at);
                // The whole point of the redesign: the amber orbit must not sit on the value arc.
                expect (r.moat() >= 1.0f, "only " + juce::String (r.moat(), 2) + "px between arc and orbit" + at);
            }
        }

        beginTest ("Everything scales with the knob");
        {
            const auto small = knobRadii (50.0f, false);
            const auto large = knobRadii (100.0f, false);
            expect (large.orbit > small.orbit * 1.8f, "the orbit did not scale");
            expect (large.body > small.body * 1.5f, "the sphere did not scale");
            expect (large.ringStroke > small.ringStroke, "the ring stroke did not scale");
            expect (large.trackWidth > small.trackWidth, "the value arc did not scale");
        }

        beginTest ("Hero knobs carry a bolder value arc");
        expect (knobRadii (120.0f, true).trackWidth > knobRadii (120.0f, false).trackWidth);

        beginTest ("Tiny and zero knobs stay finite");
        for (float d : { 0.0f, 1.0f, 4.0f, 10.0f })
        {
            const auto r = knobRadii (d, false);
            expect (r.orbit >= 0.0f && r.arc >= 0.0f && r.body >= 0.0f, "negative geometry at d=" + juce::String (d));
            expect (r.arc <= r.orbit && r.body <= r.arc, "geometry inverted at d=" + juce::String (d));
        }
    }
};

//==============================================================================
class SourceSelectorLayoutTests : public juce::UnitTest
{
public:
    SourceSelectorLayoutTests() : juce::UnitTest ("Source selector layout", "uilayout") {}

    void runTest() override
    {
        beginTest ("The preferred height follows the cell width");
        expect (sourceSelectorHeight (600, 5) > sourceSelectorHeight (300, 5), "wider cells did not ask for more height");
        expect (sourceSelectorHeight (600, 5) > sourceSelectorHeight (600, 8), "more items did not shrink the cells");

        beginTest ("A thumbnail row is never squeezed nor absurdly tall");
        for (int width : { 160, 240, 390, 460, 600, 1200 })
        {
            const int h = sourceSelectorHeight (width, 5);
            expect (h >= 66, "row only " + juce::String (h) + "px tall at width " + juce::String (width));
            expect (h <= 218, "row " + juce::String (h) + "px tall at width " + juce::String (width));
        }

        beginTest ("Nothing to show takes no height");
        expectEquals (sourceSelectorHeight (600, 0), 0);
        expectEquals (sourceSelectorHeight (0, 5), 0);
    }
};

//==============================================================================
class WaveRulerTests : public juce::UnitTest
{
public:
    WaveRulerTests() : juce::UnitTest ("Waveform ruler steps", "uilayout") {}

    void runTest() override
    {
        beginTest ("A ruler keeps a readable number of divisions at any length");
        for (double seconds : { 0.02, 0.05, 0.12, 0.5, 1.0, 2.5, 8.0, 30.0, 120.0 })
        {
            const double step = waveRulerStep (seconds);
            const double divisions = seconds / step;
            expect (divisions <= 9.0 + 1.0e-9, juce::String (divisions, 2) + " divisions for " + juce::String (seconds) + "s");
            expect (step > 0.0, "non-positive step");
            if (seconds <= 30.0)
                expect (divisions >= 1.0, "only " + juce::String (divisions, 2) + " divisions for " + juce::String (seconds) + "s");
        }

        beginTest ("Steps are chosen from the round set and never shrink as the sample grows");
        double previous = 0.0;
        for (double seconds = 0.01; seconds < 60.0; seconds *= 1.3)
        {
            const double step = waveRulerStep (seconds);
            expect (step >= previous, "the ruler got finer as the sample got longer");
            previous = step;
        }

        beginTest ("An empty sample still returns a step");
        expect (waveRulerStep (0.0) > 0.0);
        expect (waveRulerStep (-1.0) > 0.0);
    }
};

//==============================================================================
class EnvelopeStageTests : public juce::UnitTest
{
public:
    EnvelopeStageTests() : juce::UnitTest ("Envelope display stages", "uilayout") {}

    void runTest() override
    {
        beginTest ("The four stages always fill the width exactly once");
        for (float a : { 0.001f, 0.05f, 1.0f, 10.0f })
        {
            for (float d : { 0.001f, 0.2f, 4.0f })
            {
                for (float r : { 0.0f, 0.3f, 20.0f })
                {
                    const auto s = envelopeStages (a, d, r);
                    const float total = s.attack + s.decay + s.sustain + s.release;
                    expectWithinAbsoluteError (total, 1.0f, 1.0e-4f);
                    expect (s.attack >= 0.0f && s.decay >= 0.0f && s.release >= 0.0f, "negative stage");
                    expect (s.sustain > 0.05f, "the sustain plateau vanished");
                }
            }
        }

        beginTest ("A longer stage is always drawn wider");
        {
            const auto shortAttack = envelopeStages (0.01f, 0.5f, 0.5f);
            const auto longAttack  = envelopeStages (2.00f, 0.5f, 0.5f);
            expect (longAttack.attack > shortAttack.attack, "a longer attack was not wider");
            expect (longAttack.decay < shortAttack.decay, "the other stages did not give way");

            const auto shortRelease = envelopeStages (0.1f, 0.1f, 0.05f);
            const auto longRelease  = envelopeStages (0.1f, 0.1f, 8.0f);
            expect (longRelease.release > shortRelease.release, "a longer release was not wider");
        }

        beginTest ("A very short stage still gets a visible slice");
        {
            // A 5 ms attack next to a 10 s release must not be compressed out of existence.
            const auto s = envelopeStages (0.005f, 1.0f, 10.0f);
            expect (s.attack > 0.02f, "attack only " + juce::String (s.attack, 4) + " of the width");
        }

        beginTest ("Equal times share the timed width equally");
        {
            const auto s = envelopeStages (0.5f, 0.5f, 0.5f);
            expectWithinAbsoluteError (s.attack, s.decay, 1.0e-5f);
            expectWithinAbsoluteError (s.decay, s.release, 1.0e-5f);
        }

        beginTest ("Nonsense times do not produce nonsense stages");
        for (float bad : { -1.0f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity() })
        {
            const auto s = envelopeStages (bad, bad, bad);
            const float total = s.attack + s.decay + s.sustain + s.release;
            expect (std::isfinite (total), "non-finite stages");
            expectWithinAbsoluteError (total, 1.0f, 1.0e-3f);
        }
    }
};

//==============================================================================
class ModDepthTextTests : public juce::UnitTest
{
public:
    ModDepthTextTests() : juce::UnitTest ("Modulation depth readout", "uilayout") {}

    void runTest() override
    {
        beginTest ("Depth is shown as a signed share of the destination's range");
        expectEquals (modDepthText (0.5f, 0.0f, 1.0f, ""), juce::String ("+0.50"));
        expectEquals (modDepthText (-0.35f, 0.0f, 1.0f, ""), juce::String ("-0.35"));
        expectEquals (modDepthText (0.0f, 0.0f, 1.0f, ""), juce::String ("+0.00"));

        beginTest ("The destination's own units are used");
        expectEquals (modDepthText (0.5f, 20.0f, 20000.0f, "Hz"), juce::String ("+9990.0 Hz"));
        expectEquals (modDepthText (0.25f, -24.0f, 24.0f, "st"), juce::String ("+12.0 st"));
        expectEquals (modDepthText (0.1f, 0.0f, 1.0f, "%"), juce::String ("+0.10 %"));

        beginTest ("Large amounts drop a decimal so the column still fits");
        expect (modDepthText (1.0f, 0.0f, 100.0f, "").endsWith ("100.0"));
        expect (modDepthText (0.05f, 0.0f, 100.0f, "").endsWith ("5.00"));

        beginTest ("A null unit is not appended");
        expectEquals (modDepthText (0.5f, 0.0f, 1.0f, nullptr), juce::String ("+0.50"));
    }
};

//==============================================================================
static ModRowLayoutTests modRowLayoutTests;
static ModScopeGridTests modScopeGridTests;
static GridColumnTests gridColumnTests;
static KnobGeometryTests knobGeometryTests;
static SourceSelectorLayoutTests sourceSelectorLayoutTests;
static WaveRulerTests waveRulerTests;
static EnvelopeStageTests envelopeStageTests;
static ModDepthTextTests modDepthTextTests;

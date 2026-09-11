#include <juce_core/juce_core.h>

#include "ui/views/PresetBrowserModel.h"

#include <set>

using namespace am::ui::browser;

/*
    THE BROWSER AT 300 PRESETS

    PresetBrowser.cpp was written for 36: one card component per preset and
    one chip per distinct tag, laid out in a single row. At 300 patches with
    the closed vocabulary that is 300 components and about fifty chips, and
    the chip row silently dropped everything past the eighth.

    The arithmetic behind the fix lives in PresetBrowserModel.h so it can be
    checked here, against a synthetic bank of 300, without a screen. The
    components themselves are reviewed with a render.
*/
namespace
{
    /** The window sizes the instrument is reviewed at, and the extremes around them. */
    const int kWidths[]  = { 1100, 1400, 1600, 2200 };
    const int kHeights[] = {  690,  900, 1000, 1400 };

    /** A synthetic bank: `count` presets spread over the twelve categories with
        tags drawn from the closed vocabulary. Nothing is registered anywhere —
        this is the browser's own data structure, filled by hand. */
    Catalogue syntheticBank (int count)
    {
        static const char* categories[] = { "PAD", "BASS", "KEYS", "PLUCK", "LEAD", "TEXTURE",
                                            "PERCUSSION", "DRONE", "FX", "SEQUENCE", "EVOLVING", "CINEMATIC" };
        std::vector<juce::String> vocabulary;
        for (const auto& group : tagVocabulary())
        {
            juce::StringArray names;
            names.addTokens (juce::String (group.tags), "|", "");
            for (const auto& n : names) vocabulary.push_back (n.toLowerCase());
        }

        Catalogue c;
        for (int i = 0; i < count; ++i)
        {
            juce::StringArray tags;
            for (int t = 0; t < 4; ++t)
                tags.addIfNotAlreadyThere (vocabulary[(size_t) ((i * 7 + t * 13 + t) % (int) vocabulary.size())]);
            c.add ("Patch " + juce::String (i), categories[i % 12], tags);
        }
        c.finish();
        return c;
    }
}

//==============================================================================
class PresetBrowserCatalogueTests : public juce::UnitTest
{
public:
    PresetBrowserCatalogueTests() : juce::UnitTest ("Preset browser catalogue", "uilayout") {}

    void runTest() override
    {
        beginTest ("the catalogue counts every category and collects every tag once");
        {
            const auto c = syntheticBank (300);
            expectEquals (c.size(), 300);
            expectEquals (c.categories[0], juce::String ("ALL"));
            expectEquals (c.categoryCounts[0], 300);
            expectEquals (c.categories.size(), 13, "ALL plus the twelve categories");

            int summed = 0;
            for (size_t i = 1; i < c.categoryCounts.size(); ++i) summed += c.categoryCounts[i];
            expectEquals (summed, 300, "the category counts must add up to the bank");

            std::set<juce::String> distinct;
            for (const auto& e : c.entries)
                for (const auto& t : e.tags) distinct.insert (t);
            expectEquals (c.tags.size(), (int) distinct.size(), "every distinct tag gets exactly one chip");
        }

        beginTest ("chips are ordered by the vocabulary, not alphabetically");
        {
            const auto c = syntheticBank (300);
            int previous = -1;
            for (const auto& t : c.tags)
            {
                const int order = tagOrder (t);
                expect (order >= previous, "tag " + t + " is out of vocabulary order");
                previous = order;
            }
            // CHARACTER comes before USE wherever both are present.
            const int dark = c.tags.indexOf ("DARK", true), layer = c.tags.indexOf ("LAYER", true);
            if (dark >= 0 && layer >= 0) expect (dark < layer, "CHARACTER chips must come before USE chips");
        }

        beginTest ("the closed vocabulary from the brief is the one the browser knows");
        {
            int total = 0;
            for (const auto& group : tagVocabulary())
            {
                juce::StringArray names;
                names.addTokens (juce::String (group.tags), "|", "");
                total += names.size();
                for (const auto& n : names)
                    expectEquals (tagGroupOf (n), juce::String (group.name), n + " is in the wrong group");
            }
            expect (total >= 45, "the vocabulary lost tags: " + juce::String (total));
            expectEquals (tagGroupOf ("NOT-A-REAL-TAG"), juce::String(), "unknown tags belong to no group");
            expect (tagOrder ("NOT-A-REAL-TAG") > tagOrder ("DRIFT"), "unknown tags sort after the vocabulary");
        }
    }
};
static PresetBrowserCatalogueTests presetBrowserCatalogueTests;

//==============================================================================
class PresetBrowserFilterTests : public juce::UnitTest
{
public:
    PresetBrowserFilterTests() : juce::UnitTest ("Preset browser filter", "uilayout") {}

    void runTest() override
    {
        Catalogue c;
        c.add ("Void Bloom",    "PAD",  { "dark", "breathing", "wide" });
        c.add ("Carbon Bass",   "BASS", { "dark", "dirty", "sub" });
        c.add ("Glass Creature","PAD",  { "glassy", "evolving", "high" });
        c.finish();

        std::vector<int> out;

        beginTest ("ALL shows everything, a category narrows it");
        {
            Filter f;
            applyFilter (c, f, out);
            expectEquals ((int) out.size(), 3);

            f.category = "PAD";
            applyFilter (c, f, out);
            expectEquals ((int) out.size(), 2);
            f.category = "bass";                    // the list is case insensitive
            applyFilter (c, f, out);
            expectEquals ((int) out.size(), 1);
            expectEquals (out[0], 1);
        }

        beginTest ("tags stack: a preset must carry all of the active ones");
        {
            Filter f;
            f.tags = { "DARK" };
            applyFilter (c, f, out);
            expectEquals ((int) out.size(), 2);

            f.tags.add ("SUB");
            applyFilter (c, f, out);
            expectEquals ((int) out.size(), 1, "DARK + SUB is only the bass");

            f.tags.add ("HIGH");
            applyFilter (c, f, out);
            expect (out.empty(), "a contradictory tag set shows nothing, it does not fall back");
        }

        beginTest ("typing finds a name, a category or a tag, and every term has to match");
        {
            Filter f;
            f.query = "bloom";
            applyFilter (c, f, out);
            expectEquals ((int) out.size(), 1);
            expectEquals (out[0], 0);

            f.query = "PAD";                        // the category is searchable
            applyFilter (c, f, out);
            expectEquals ((int) out.size(), 2);

            f.query = "glassy";                     // so are the tags
            applyFilter (c, f, out);
            expectEquals ((int) out.size(), 1);

            f.query = "dark pad";                   // both terms, not either
            applyFilter (c, f, out);
            expectEquals ((int) out.size(), 1);
            expectEquals (out[0], 0);

            f.query = "   ";                        // whitespace is not a query
            applyFilter (c, f, out);
            expectEquals ((int) out.size(), 3);
        }

        beginTest ("a category, tags and a query narrow together");
        {
            Filter f;
            f.category = "PAD";
            f.tags = { "DARK" };
            f.query = "bloom";
            applyFilter (c, f, out);
            expectEquals ((int) out.size(), 1);

            f.category = "BASS";
            applyFilter (c, f, out);
            expect (out.empty(), "the filters intersect, they do not compete");
        }

        beginTest ("filtering 300 presets is a scan, not a rebuild");
        {
            const auto big = syntheticBank (300);
            Filter f;
            f.query = "patch 2";
            const auto started = juce::Time::getMillisecondCounter();
            for (int i = 0; i < 200; ++i) applyFilter (big, f, out);   // 200 keystrokes
            const auto ms = juce::Time::getMillisecondCounter() - started;
            expect (! out.empty(), "the query matched nothing");
            expect (ms < 400, "200 filter passes over 300 presets took " + juce::String ((int) ms) + " ms");
        }
    }
};
static PresetBrowserFilterTests presetBrowserFilterTests;

//==============================================================================
class PresetBrowserGridTests : public juce::UnitTest
{
public:
    PresetBrowserGridTests() : juce::UnitTest ("Preset browser grid", "uilayout") {}

    void runTest() override
    {
        beginTest ("the grid fits its viewport at every width and holds every card");
        {
            for (const int width : kWidths)
            {
                // The grid sits right of the category column, so it is narrower than the window.
                const int viewportWidth = width - juce::jlimit (150, 220, width / 7) - 2 * juce::jlimit (20, 60, width / 26);
                for (const int count : { 1, 36, 137, 300 })
                {
                    const auto g = gridGeometry (viewportWidth, count);
                    const juce::String at = " at " + juce::String (viewportWidth) + " wide with " + juce::String (count);

                    expect (g.columns >= 2 && g.columns <= 6, "column count out of range" + at);
                    expect (g.cardWidth * g.columns + g.gap * (g.columns - 1) <= viewportWidth, "the row is wider than the viewport" + at);
                    expectEquals (g.rows, (count + g.columns - 1) / g.columns, "row count" + at);
                    expect (g.contentHeight >= g.rows * g.rowPitch() - g.gap, "the scrolled area cannot hold its rows" + at);

                    const auto last = cardBounds (g, count - 1);
                    expect (last.right() <= viewportWidth, "the last card runs past the viewport" + at);
                    expect (last.bottom() <= g.contentHeight, "the last card runs past the scrolled area" + at);
                }
            }
        }

        beginTest ("cards never overlap");
        {
            const auto g = gridGeometry (1100, 300);
            for (int slot = 0; slot + 1 < 300; ++slot)
            {
                const auto a = cardBounds (g, slot);
                for (int other = slot + 1; other < juce::jmin (300, slot + 2 * g.columns); ++other)
                    expect (! a.intersects (cardBounds (g, other)),
                            "slots " + juce::String (slot) + " and " + juce::String (other) + " overlap");
            }
        }

        beginTest ("the card pool is bounded by the window, not by the bank");
        {
            for (const int height : kHeights)
            {
                const auto small = gridGeometry (1100, 36);
                const auto big   = gridGeometry (1100, 300);
                const int poolSmall = gridPoolSize (small, height, 1);
                const int poolBig   = gridPoolSize (big, height, 1);
                expectEquals (poolBig, poolSmall, "the pool grew with the bank at height " + juce::String (height));
                expect (poolBig <= 64, "the pool is too big to be a pool: " + juce::String (poolBig));
                expect (poolBig >= big.columns * 2, "the pool cannot even cover one screen");
            }
        }

        beginTest ("the visible window always covers what the viewport is showing");
        {
            const int viewHeight = 700;
            const auto g = gridGeometry (1100, 300);
            const int maxScroll = juce::jmax (0, g.contentHeight - viewHeight);

            for (int scroll = 0; scroll <= maxScroll; scroll += 37)
            {
                const auto w = gridWindow (g, scroll, viewHeight, 1, 300);
                expect (w.first >= 0 && w.last() <= 300, "the window left the bank at scroll " + juce::String (scroll));
                expect (w.count > 0, "empty window at scroll " + juce::String (scroll));
                expect (w.count <= gridPoolSize (g, viewHeight, 1), "the window is bigger than the pool");

                // Every card that would be on screen has to be inside the window.
                for (int slot = 0; slot < 300; ++slot)
                {
                    const auto b = cardBounds (g, slot);
                    const bool onScreen = b.bottom() > scroll && b.y < scroll + viewHeight;
                    if (onScreen)
                        expect (w.contains (slot), "slot " + juce::String (slot) + " is on screen at scroll "
                                                    + juce::String (scroll) + " but outside the window");
                }
            }
        }

        beginTest ("the window degrades gracefully at the edges");
        {
            const auto g = gridGeometry (1100, 5);
            const auto w = gridWindow (g, 0, 700, 1, 5);
            expectEquals (w.first, 0);
            expectEquals (w.count, 5, "a bank smaller than the pool shows all of it");

            const auto none = gridWindow (g, 0, 700, 1, 0);
            expectEquals (none.count, 0, "an empty filter result has no window");
        }
    }
};
static PresetBrowserGridTests presetBrowserGridTests;

//==============================================================================
class PresetBrowserChipTests : public juce::UnitTest
{
public:
    PresetBrowserChipTests() : juce::UnitTest ("Preset browser chips", "uilayout") {}

    void runTest() override
    {
        beginTest ("every chip gets a rectangle — the vocabulary is not silently truncated");
        {
            // Fifty tags at roughly the widths the real chips measure.
            std::vector<int> widths;
            for (int i = 0; i < 50; ++i) widths.push_back (44 + (i * 7) % 46);

            for (const int width : { 300, 520, 760, 1100, 1600 })
            {
                const auto flow = flowChips (widths, width, 20, 6, 5);
                expectEquals ((int) flow.bounds.size(), (int) widths.size(),
                              "chips went missing at " + juce::String (width) + " wide");
                for (size_t i = 0; i < flow.bounds.size(); ++i)
                {
                    const auto& b = flow.bounds[i];
                    expect (b.width > 0 && b.height > 0,
                            "chip " + juce::String ((int) i) + " has no size at " + juce::String (width) + " wide");
                    expect (b.right() <= width,
                            "chip " + juce::String ((int) i) + " runs past the strip at " + juce::String (width) + " wide");
                }
                expect (flow.height >= 20, "the strip has no height at " + juce::String (width) + " wide");
                expect (flow.rows >= 2, "fifty chips cannot fit on fewer than two rows at " + juce::String (width) + " wide");
            }
        }

        beginTest ("chips on the same row do not overlap");
        {
            std::vector<int> widths;
            for (int i = 0; i < 50; ++i) widths.push_back (44 + (i * 11) % 60);
            const auto flow = flowChips (widths, 760, 20, 6, 5);
            for (size_t i = 1; i < flow.bounds.size(); ++i)
            {
                const auto& a = flow.bounds[i - 1];
                const auto& b = flow.bounds[i];
                if (a.y == b.y)
                    expect (b.x >= a.right(), "chips " + juce::String ((int) i - 1) + " and " + juce::String ((int) i) + " overlap");
            }
        }

        beginTest ("a chip wider than the strip is clamped rather than dropped");
        {
            const auto flow = flowChips ({ 900 }, 300, 20, 6, 5);
            expectEquals ((int) flow.bounds.size(), 1);
            expectEquals (flow.bounds[0].width, 300);
        }

        beginTest ("the strip is a whole number of rows at every window size");
        {
            for (const int height : kHeights)
            {
                const auto strip = chipStripGeometry (height);
                expectEquals (strip.height, strip.visibleRows * strip.chipHeight + (strip.visibleRows - 1) * strip.gapY,
                              "the strip shows part of a row at height " + juce::String (height));
                expect (strip.chipHeight >= 18, "chips too short to hit at height " + juce::String (height));

                // A chip flowed into that strip must land on a row that is fully
                // inside it, or the top row of the vocabulary is clipped.
                std::vector<int> widths (12, 70);
                const auto flow = flowChips (widths, 600, strip.chipHeight, strip.gapX, strip.gapY);
                expect (flow.bounds.front().bottom() <= strip.height,
                        "the first chip row does not fit the strip at height " + juce::String (height));
            }
        }

        beginTest ("an empty vocabulary lays out to nothing");
        {
            const auto flow = flowChips ({}, 600, 20, 6, 5);
            expect (flow.bounds.empty());
            expectEquals (flow.height, 0);
        }
    }
};
static PresetBrowserChipTests presetBrowserChipTests;

//==============================================================================
class PresetBrowserCategoryListTests : public juce::UnitTest
{
public:
    PresetBrowserCategoryListTests() : juce::UnitTest ("Preset browser category list", "uilayout") {}

    void runTest() override
    {
        beginTest ("ALL plus twelve categories fit the left column at the smallest window");
        {
            for (const int height : kHeights)
            {
                // The list gets the window height less the header and the padding.
                const int pad = 26, listHeight = height - pad * 2 - juce::jlimit (44, 64, height / 14) - pad;
                const int rows = 14;                       // "CATEGORIES" + ALL + twelve
                const int rowHeight = categoryRowHeight (listHeight, rows);
                expect (rowHeight >= 20, "rows shrank below a usable hit target at height " + juce::String (height));
                expect (rowHeight * rows <= listHeight,
                        "the category list does not fit at height " + juce::String (height)
                        + " (" + juce::String (rowHeight * rows) + " > " + juce::String (listHeight) + ")");
            }
        }

        beginTest ("row height never runs away on a tall window");
        {
            expect (categoryRowHeight (4000, 14) <= 36, "rows must stay a list, not become buttons");
            expect (categoryRowHeight (0, 14) >= 20, "a zero height must still give a usable row");
        }
    }
};
static PresetBrowserCategoryListTests presetBrowserCategoryListTests;

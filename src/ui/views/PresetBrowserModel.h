#pragma once

#include <juce_core/juce_core.h>

#include <algorithm>
#include <vector>

/**
    Pure browser arithmetic — the part of the preset BROWSER that has to keep
    working when the factory bank is 300 patches instead of 36.

    Nothing here touches a component, a graphics context or the processor, so
    the catalogue, the filter, the card grid and the tag chip row can all be
    unit tested headlessly (the test runner links the core modules only).
    PresetBrowser.cpp turns these numbers into components; it never invents
    sizes or filter rules of its own.
*/
namespace am::ui::browser
{

//==============================================================================
/** One catalogue row: what the browser knows about a preset without building it. */
struct Entry
{
    juce::String      name;        ///< as registered
    juce::String      category;    ///< upper case
    juce::StringArray tags;        ///< upper case
    juce::String      haystack;    ///< upper case "NAME CATEGORY TAG TAG …", built by finish()
};

//==============================================================================
/** A group of the closed tag vocabulary (docs/PRESET_BRIEF.md §6). */
struct TagGroup
{
    const char* name;
    const char* tags;   ///< '|' separated, in the order the brief lists them
};

/** The closed vocabulary, in chip order. Tags outside it still show, at the end. */
inline const std::vector<TagGroup>& tagVocabulary()
{
    static const std::vector<TagGroup> groups
    {
        { "CHARACTER", "DARK|BRIGHT|WARM|COLD|CLEAN|DIRTY|SOFT|HARSH|METALLIC|WOODEN|GLASSY|ORGANIC|SYNTHETIC|HOLLOW|VOCAL" },
        { "MOTION",    "STATIC|BREATHING|PULSING|EVOLVING|RHYTHMIC|CHAOTIC|MORPHING|UNSTABLE" },
        { "SPACE",     "DRY|CLOSE|ROOMY|WIDE|HUGE|DISTANT" },
        { "REGISTER",  "SUB|LOW|MID|HIGH|AIR" },
        { "GESTURE",   "STRUCK|PLUCKED|BOWED|BLOWN|GRANULAR|NOISY|RESONANT|FORMANT|SCRAPED" },
        { "USE",       "CHORDS|MELODIC|LAYER|INTRO|TRANSITION|IMPACT|DRIFT" }
    };
    return groups;
}

/** Position of `upperTag` in the vocabulary, or a value past the end for tags outside it. */
inline int tagOrder (const juce::String& upperTag) noexcept
{
    int running = 0;
    for (const auto& group : tagVocabulary())
    {
        juce::StringArray names;
        names.addTokens (juce::String (group.tags), "|", "");
        const int i = names.indexOf (upperTag, true);
        if (i >= 0) return running + i;
        running += names.size();
    }
    return running + 1000;   // unknown tags sort after the vocabulary, alphabetically
}

/** Vocabulary group a tag belongs to, or an empty string when it is outside the list. */
inline juce::String tagGroupOf (const juce::String& upperTag)
{
    for (const auto& group : tagVocabulary())
    {
        juce::StringArray names;
        names.addTokens (juce::String (group.tags), "|", "");
        if (names.contains (upperTag, true)) return juce::String (group.name);
    }
    return {};
}

//==============================================================================
/**
    The whole library as the browser sees it: rows, the category list with
    counts, and every distinct tag in vocabulary order.
*/
struct Catalogue
{
    std::vector<Entry> entries;
    juce::StringArray  categories;      ///< "ALL" first, then in first-seen (registration) order
    std::vector<int>   categoryCounts;  ///< parallel to `categories`
    juce::StringArray  tags;            ///< distinct tags, vocabulary order then alphabetical

    void clear()
    {
        entries.clear();
        categories.clear();
        categoryCounts.clear();
        tags.clear();
    }

    void add (const juce::String& name, const juce::String& category, const juce::StringArray& presetTags)
    {
        Entry e;
        e.name = name;
        e.category = category.toUpperCase();
        for (const auto& t : presetTags)
            e.tags.addIfNotAlreadyThere (t.toUpperCase());
        entries.push_back (std::move (e));
    }

    /** Derives the search keys, the category list and the tag list. Call once after adding. */
    void finish()
    {
        categories.clear();
        categoryCounts.clear();
        tags.clear();

        categories.add ("ALL");
        categoryCounts.push_back ((int) entries.size());

        for (auto& e : entries)
        {
            // One uppercase string per preset, built once, so typing a query is a
            // substring scan over the bank rather than a re-tokenise of every row.
            e.haystack = e.name.toUpperCase() + " " + e.category + " " + e.tags.joinIntoString (" ");

            const int at = categories.indexOf (e.category, true);
            if (at < 0) { categories.add (e.category); categoryCounts.push_back (1); }
            else        { ++categoryCounts[(size_t) at]; }

            for (const auto& t : e.tags) tags.addIfNotAlreadyThere (t);
        }

        // Chips read as the brief's groups: CHARACTER, MOTION, SPACE, REGISTER,
        // GESTURE, USE — then anything an author invented, alphabetically.
        std::vector<juce::String> ordered;
        ordered.reserve ((size_t) tags.size());
        for (const auto& t : tags) ordered.push_back (t);
        std::stable_sort (ordered.begin(), ordered.end(),
                          [] (const juce::String& a, const juce::String& b)
                          {
                              const int oa = tagOrder (a), ob = tagOrder (b);
                              return oa != ob ? oa < ob : a.compareNatural (b) < 0;
                          });
        tags.clear();
        for (const auto& t : ordered) tags.add (t);
    }

    int size() const noexcept { return (int) entries.size(); }
};

//==============================================================================
/** What the browser is currently showing. */
struct Filter
{
    juce::String      category { "ALL" };
    juce::String      query;          ///< free text; whitespace separated terms, all must match
    juce::StringArray tags;           ///< upper case; a preset must carry all of them
};

/** True when `entry` survives the filter. */
inline bool matches (const Entry& entry, const Filter& filter)
{
    if (filter.category.isNotEmpty() && ! filter.category.equalsIgnoreCase ("ALL")
        && ! entry.category.equalsIgnoreCase (filter.category))
        return false;

    for (const auto& t : filter.tags)
        if (! entry.tags.contains (t, true))
            return false;

    const auto query = filter.query.trim();
    if (query.isEmpty()) return true;

    // Every term must appear somewhere: "dark pad" finds the dark pads, not
    // everything dark plus everything in PAD.
    juce::StringArray terms;
    terms.addTokens (query.toUpperCase(), " ", "");
    terms.removeEmptyStrings();
    for (const auto& term : terms)
        if (! entry.haystack.contains (term))
            return false;
    return true;
}

/** Indices of the entries that survive the filter, in catalogue order. */
inline void applyFilter (const Catalogue& catalogue, const Filter& filter, std::vector<int>& out)
{
    out.clear();
    out.reserve (catalogue.entries.size());
    for (int i = 0; i < (int) catalogue.entries.size(); ++i)
        if (matches (catalogue.entries[(size_t) i], filter))
            out.push_back (i);
}

//==============================================================================
/** Geometry of the card grid for a given viewport width and number of visible cards. */
struct GridGeometry
{
    int columns      = 1;
    int cardWidth    = 0;
    int cardHeight   = 0;
    int gap          = 0;
    int rows         = 0;
    int contentHeight = 0;   ///< height the scrolled component needs

    int rowPitch() const noexcept { return cardHeight + gap; }
};

inline GridGeometry gridGeometry (int viewportWidth, int numVisible) noexcept
{
    GridGeometry g;
    const int width = juce::jmax (1, viewportWidth);
    g.gap = juce::jmax (8, width / 80);
    g.columns = juce::jlimit (2, 6, width / 230);
    g.cardWidth = juce::jmax (1, (width - g.gap * (g.columns - 1)) / g.columns);
    g.cardHeight = juce::jmax (1, juce::roundToInt ((float) g.cardWidth * 0.82f));
    g.rows = (juce::jmax (0, numVisible) + g.columns - 1) / g.columns;
    g.contentHeight = juce::jmax (1, g.rows * g.rowPitch());
    return g;
}

/** A rectangle. juce::Rectangle lives in juce_graphics and this header has to
    stay linkable into the console test runner, which has no GUI modules. */
struct Rect
{
    int x = 0, y = 0, width = 0, height = 0;

    int right() const noexcept  { return x + width; }
    int bottom() const noexcept { return y + height; }
    bool isEmpty() const noexcept { return width <= 0 || height <= 0; }
    bool intersects (const Rect& o) const noexcept
    {
        return x < o.right() && o.x < right() && y < o.bottom() && o.y < bottom();
    }
};

/** Bounds of the card in grid slot `slot` (0-based, reading order). */
inline Rect cardBounds (const GridGeometry& g, int slot) noexcept
{
    const int row = slot / juce::jmax (1, g.columns);
    const int column = slot % juce::jmax (1, g.columns);
    return { column * (g.cardWidth + g.gap), row * g.rowPitch(), g.cardWidth, g.cardHeight };
}

//==============================================================================
/**
    The slice of the grid that is worth having components for.

    The browser keeps a small pool of cards and moves them as the viewport
    scrolls, so the component count depends on the window size and never on
    the size of the bank.
*/
struct GridWindow
{
    int first = 0;   ///< first grid slot with a card
    int count = 0;   ///< how many slots

    int last() const noexcept { return first + count; }
    bool contains (int slot) const noexcept { return slot >= first && slot < last(); }
};

/** Cards needed to cover `viewportHeight` plus `overscanRows` rows above and below. */
inline int gridPoolSize (const GridGeometry& g, int viewportHeight, int overscanRows) noexcept
{
    const int visibleRows = juce::jmax (1, (juce::jmax (0, viewportHeight) + g.rowPitch() - 1) / juce::jmax (1, g.rowPitch()) + 1);
    return (visibleRows + 2 * juce::jmax (0, overscanRows)) * juce::jmax (1, g.columns);
}

inline GridWindow gridWindow (const GridGeometry& g, int scrollY, int viewportHeight,
                              int overscanRows, int numVisible) noexcept
{
    GridWindow w;
    const int total = juce::jmax (0, numVisible);
    if (total == 0) return w;

    const int pitch = juce::jmax (1, g.rowPitch());
    const int firstRow = juce::jmax (0, juce::jmax (0, scrollY) / pitch - juce::jmax (0, overscanRows));
    const int poolRows = juce::jmax (1, gridPoolSize (g, viewportHeight, overscanRows) / juce::jmax (1, g.columns));

    w.first = juce::jlimit (0, juce::jmax (0, total - 1), firstRow * g.columns);
    w.count = juce::jmin (total - w.first, poolRows * g.columns);
    return w;
}

//==============================================================================
/**
    Wrapping flow layout for the tag chips.

    The closed vocabulary is about fifty tags; one row holds eight. The old
    single-row layout silently dropped everything that did not fit, which made
    most of the vocabulary unreachable — so the chips wrap and every one of
    them gets a real rectangle.
*/
struct ChipLayout
{
    std::vector<Rect> bounds;
    int rows   = 0;
    int height = 0;   ///< total height of the flowed block
};

inline ChipLayout flowChips (const std::vector<int>& widths, int areaWidth,
                             int chipHeight, int gapX, int gapY)
{
    ChipLayout out;
    out.bounds.reserve (widths.size());
    const int width = juce::jmax (1, areaWidth);
    int x = 0, y = 0, row = widths.empty() ? 0 : 1;

    for (int w : widths)
    {
        const int chipW = juce::jlimit (1, width, w);
        if (x > 0 && x + chipW > width)      // start a new row
        {
            x = 0;
            y += chipHeight + gapY;
            ++row;
        }
        out.bounds.push_back ({ x, y, chipW, chipHeight });
        x += chipW + gapX;
    }

    out.rows = row;
    out.height = widths.empty() ? 0 : y + chipHeight;
    return out;
}

//==============================================================================
/** Geometry of the wrapping chip strip: whole rows only, never a sliver of the next. */
struct ChipStrip
{
    int chipHeight  = 18;
    int gapX        = 6;
    int gapY        = 5;
    int visibleRows = 2;
    int height      = 0;   ///< height of the strip's viewport
};

inline ChipStrip chipStripGeometry (int browserHeight, int visibleRows = 2) noexcept
{
    ChipStrip s;
    s.visibleRows = juce::jmax (1, visibleRows);
    s.chipHeight = juce::jlimit (18, 24, juce::jmax (0, browserHeight) / 42);
    // Exactly the rows asked for. A strip an odd number of pixels tall shows
    // half of the row below it, which reads as a rendering fault rather than as
    // "there is more, scroll".
    s.height = s.visibleRows * s.chipHeight + (s.visibleRows - 1) * s.gapY;
    return s;
}

/** Row height that keeps `numRows` (header included) inside `areaHeight`. */
inline int categoryRowHeight (int areaHeight, int numRows) noexcept
{
    const int rows = juce::jmax (1, numRows);
    return juce::jlimit (20, 36, juce::jmax (0, areaHeight) / rows);
}

} // namespace am::ui::browser

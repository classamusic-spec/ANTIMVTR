#pragma once

#include "PresetManager.h"

namespace am
{

/**
    FACTORY CONTENT — the reference library (SPEC §48–50).

    Every patch is pure code: parameters, a Fracture table, modulation
    routings and (for SAMPLE patches) a built-in sample reference. Nothing is
    loaded from disk and nothing is random — a preset builds byte-identically
    on every machine, which is what makes the validator and the mutation tests
    meaningful.

    Registration order is the browser order: the library is grouped by
    category (PAD, BASS, KEYS, PLUCK, LEAD, TEXTURE, PERCUSSION, DRONE, FX,
    SEQUENCE, EVOLVING, CINEMATIC) and each patch declares tags that describe
    the sound rather than the technique used to make it.
*/
namespace FactoryContent
{

/** Registers the whole reference library on a preset manager (message thread). */
void registerAll (PresetManager& manager);

//==============================================================================
/**
    What a rendered patch of a category is expected to measure.

    A pad and a percussion hit legitimately land in different places: the pad
    holds a steady level for the whole note while the percussion patch spends
    most of the render decaying. One global RMS window would either pass a
    silent pluck or fail an honest pad, so the gate is per category.

    The window applies to the validator's standard render (one note, held
    2 s, 1.5 s of tail).
*/
struct CategorySpec
{
    const char* category;
    float minRms;      ///< below this the patch does not carry — it is not audible enough to load
    float maxRms;      ///< above this it is louder than the rest of the bank
    float minPeak;     ///< the note has to actually arrive
    float maxPeak;     ///< headroom for chords on top of a single note
    const char* text;  ///< what the category is for
};

/** The categories the library uses, in browser order. */
const std::vector<CategorySpec>& categories();

/** Level expectations for a category; falls back to a permissive window for unknown ones. */
const CategorySpec& categorySpec (const juce::String& category) noexcept;

/** True when `category` is one of the declared categories (INIT counts). */
bool isKnownCategory (const juce::String& category) noexcept;

//==============================================================================
/** Modulation routings the builders asked for but the table rejected (must stay 0). */
int rejectedRoutings() noexcept;
void resetRejectedRoutings() noexcept;

} // namespace FactoryContent
} // namespace am

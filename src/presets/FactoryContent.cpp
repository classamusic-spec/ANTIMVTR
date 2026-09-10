#include "FactoryContent.h"
#include "FactoryBuilders.h"

namespace am::FactoryContent
{

//==============================================================================
// The library.
//
// One translation unit per category, each registering its own patches in
// browser order. Splitting them keeps every file reviewable as the bank grows
// and lets separate authors work without ever touching the same file; the only
// shared thing is `FactoryBuilders.h`, the patch-sheet vocabulary.
//==============================================================================
void registerPad (PresetManager& manager);
void registerBass (PresetManager& manager);
void registerKeys (PresetManager& manager);
void registerPluck (PresetManager& manager);
void registerLead (PresetManager& manager);
void registerTexture (PresetManager& manager);
void registerPercussion (PresetManager& manager);
void registerDrone (PresetManager& manager);
void registerFx (PresetManager& manager);
void registerSequence (PresetManager& manager);
void registerEvolving (PresetManager& manager);
void registerCinematic (PresetManager& manager);

void registerAll (PresetManager& manager)
{
    registerPad (manager);
    registerBass (manager);
    registerKeys (manager);
    registerPluck (manager);
    registerLead (manager);
    registerTexture (manager);
    registerPercussion (manager);
    registerDrone (manager);
    registerFx (manager);
    registerSequence (manager);
    registerEvolving (manager);
    registerCinematic (manager);
}

const std::vector<CategorySpec>& categories()
{
    static const std::vector<CategorySpec> specs
    {
        { "INIT",       0.004f, 0.60f, 0.02f, 1.0f, "The blank starting point" },
        { "PAD",        0.020f, 0.50f, 0.08f, 1.0f, "Held chords, slow movement" },
        { "BASS",       0.020f, 0.55f, 0.08f, 1.0f, "Weight and definition below the mix" },
        { "KEYS",       0.010f, 0.50f, 0.08f, 1.0f, "Struck and played" },
        { "PLUCK",      0.006f, 0.45f, 0.08f, 1.0f, "Short, precise, physical" },
        { "LEAD",       0.020f, 0.55f, 0.10f, 1.0f, "One line, in front" },
        { "TEXTURE",    0.008f, 0.50f, 0.06f, 1.0f, "Surfaces rather than notes" },
        { "PERCUSSION", 0.004f, 0.45f, 0.06f, 1.0f, "Struck objects, tuned" },
        { "DRONE",      0.015f, 0.55f, 0.08f, 1.0f, "One note, held forever" },
        { "FX",         0.006f, 0.50f, 0.06f, 1.0f, "Sound design material" },
        { "SEQUENCE",   0.008f, 0.50f, 0.08f, 1.0f, "The patch plays the pattern" },
        { "EVOLVING",   0.010f, 0.50f, 0.08f, 1.0f, "Different at the end of the note" },
        { "CINEMATIC",  0.010f, 0.55f, 0.08f, 1.0f, "Scoring material" }
    };
    return specs;
}

const CategorySpec& categorySpec (const juce::String& category) noexcept
{
    for (const auto& c : categories())
        if (category.equalsIgnoreCase (c.category))
            return c;

    static const CategorySpec unknown { "UNKNOWN", 0.002f, 0.70f, 0.02f, 1.0f, "Unclassified" };
    return unknown;
}

bool isKnownCategory (const juce::String& category) noexcept
{
    for (const auto& c : categories())
        if (category.equalsIgnoreCase (c.category))
            return true;
    return false;
}

int  rejectedRoutings() noexcept      { return gRejectedRoutings.load (std::memory_order_relaxed); }
void resetRejectedRoutings() noexcept { gRejectedRoutings.store (0, std::memory_order_relaxed); }

} // namespace am::FactoryContent

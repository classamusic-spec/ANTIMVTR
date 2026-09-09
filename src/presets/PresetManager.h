#pragma once

#include "state/StateManager.h"
#include <functional>
#include <vector>

namespace am
{

/** A factory preset is generated from code: a name, category, tags and a builder. */
struct FactoryPreset
{
    juce::String name;
    juce::String category;
    juce::StringArray tags;
    std::function<void (PatchState&)> build;
};

/**
    Manages factory (code-generated) and user (on-disk JSON) presets.

    Disk access only ever happens on the message thread; the audio thread
    receives a fully built PatchState through the processor's state path.
*/
class PresetManager
{
public:
    PresetManager();

    int numFactoryPresets() const noexcept { return (int) factory.size(); }
    const FactoryPreset& factoryPreset (int index) const { return factory[(size_t) index]; }

    /** Builds a PatchState for a factory preset. */
    PatchState buildFactory (int index) const;

    /** Finds a factory preset by name (case-insensitive). Returns -1 if missing. */
    int findFactory (const juce::String& name) const;

    /** Returns the "Init" patch. */
    static PatchState initPatch();

    /** Directory where user presets are stored. */
    static juce::File userPresetDirectory();

    /** Enumerates user preset files (*.antimatr.json). */
    juce::Array<juce::File> userPresetFiles() const;

    bool saveUserPreset (const PatchState& state, const juce::File& file, juce::String* error = nullptr) const;
    bool loadUserPreset (const juce::File& file, PatchState& out, juce::String* error = nullptr) const;

    /** Registers an additional factory preset (used by the factory content module). */
    void addFactory (FactoryPreset preset);

private:
    std::vector<FactoryPreset> factory;
    void registerBuiltIns();
};

} // namespace am

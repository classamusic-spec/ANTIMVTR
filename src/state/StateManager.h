#pragma once

#include "ParameterRegistry.h"
#include <juce_data_structures/juce_data_structures.h>

namespace am
{

/**
    Serialisable description of a complete ANTI-MATR patch.

    The state is a JSON document with these top-level sections:
      meta      — schema, plugin version, name, author, category, tags
      params    — { "<param id>": value }  (natural, denormalised values)
      matter    — topology / material data not exposed as host parameters
      fracture  — per-fragment tables and sequencer steps
      mod       — modulation routings
      space     — space rack extras
      sample    — sample references (path, root, loop points)
      ab        — A/B snapshots (two nested param maps)
      dna       — mutation DNA overrides
      seeds     — random seeds

    Anything not understood by an older version is preserved on round trip
    by keeping the raw `extra` object. Unknown parameter IDs are dropped
    with a warning; missing IDs fall back to their registry defaults.
*/
struct PatchState
{
    static constexpr int kSchemaVersion = 1;

    struct Meta
    {
        int          schemaVersion = kSchemaVersion;
        juce::String pluginVersion;
        juce::String name     = "Init";
        juce::String author   = "ANTI-MATR";
        juce::String category = "INIT";
        juce::StringArray tags;
        juce::String comment;
    } meta;

    ParamValues params = ParameterRegistry::defaults();

    /** Sections owned by other engines; kept as JSON so they can evolve independently. */
    juce::var matter, fracture, mod, space, sample, ab, dna, seeds, extra;

    /** UI-only state (e.g. current page) — never part of a preset. */
    juce::var ui;
};

//==============================================================================
class StateManager
{
public:
    /** Serialises a patch to a JSON document (pretty-printed for presets). */
    static juce::String toJson (const PatchState& state, bool pretty = true);

    /** Parses a JSON document. Returns false if the document is not an ANTI-MATR state.
        Missing parameters are set to defaults; out-of-range values are clamped. */
    static bool fromJson (const juce::String& json, PatchState& out, juce::String* warnings = nullptr);

    /** Same as fromJson but from a parsed var (used by the preset system). */
    static bool fromVar (const juce::var& v, PatchState& out, juce::String* warnings = nullptr);

    /** Builds a var tree (for embedding into other documents). */
    static juce::var toVar (const PatchState& state);

    /** Upgrades an older document in place. Returns the schema version after migration. */
    static int migrate (juce::DynamicObject& root, int fromVersion);

    /** Binary blob helpers used by the plugin's get/setStateInformation. */
    static juce::MemoryBlock toBinary (const PatchState& state);
    static bool fromBinary (const void* data, size_t size, PatchState& out, juce::String* warnings = nullptr);

    /** Builds the ValueTree layout expected by juce::AudioProcessorValueTreeState::replaceState. */
    static juce::ValueTree toParameterTree (const ParamValues& values, const juce::Identifier& rootType);

    /** Reads parameter values out of a ValueTree produced by AudioProcessorValueTreeState. */
    static void fromParameterTree (const juce::ValueTree& tree, ParamValues& values);
};

} // namespace am

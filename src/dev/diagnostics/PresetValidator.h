#pragma once

#include "SafetyMonitor.h"
#include "SignalMetrics.h"
#include "presets/PresetManager.h"

#include <juce_core/juce_core.h>
#include <memory>
#include <vector>

namespace am
{
    class SynthEngine;
}

namespace am::dev
{

/** What validating one preset produced (§86). */
struct PresetValidationResult
{
    int          index = -1;
    juce::String name;
    juce::String category;

    bool parametersInRange = false;   ///< every value inside its registry range
    bool jsonRoundTrip     = false;   ///< StateManager JSON round trip is value-identical

    float    peak        = 0.0f;
    float    rms         = 0.0f;
    float    dc          = 0.0f;
    float    crestFactor = 0.0f;
    float    tailRms     = 0.0f;      ///< RMS of the last 100 ms after the release
    float    centroidHz  = 0.0f;
    int      nonFinite   = 0;
    uint32_t safetyTotal = 0;
    uint32_t safetyCounts[SafetyMonitor::kNumEvents] {};
    float    cpuAvgPercent  = 0.0f;
    float    cpuPeakPercent = 0.0f;
    double   renderSeconds  = 0.0;    ///< wall-clock time the offline render took
    int      maxActiveVoices = 0;

    juce::StringArray issues;         ///< empty means the preset passed

    bool passed() const noexcept { return issues.isEmpty(); }
};

/** How the validator renders and judges each preset. */
struct PresetValidatorOptions
{
    double sampleRate     = 48000.0;
    int    blockSize      = 128;
    double holdSeconds    = 2.0;     ///< note held
    double releaseSeconds = 1.5;     ///< rendered after the note-off
    int    midiNote       = 60;
    float  velocity       = 0.85f;
    float  peakLimit      = 1.0f;    ///< above this the preset is flagged
    float  silenceRms     = 1.0e-5f; ///< below this the preset is flagged as silent
    float  cpuLimit       = 80.0f;   ///< average % of block budget
    float  dcLimit        = 0.02f;
};

/**
    PRESET VALIDATOR (§86)

    Builds every factory preset, checks the parameter bounds and the JSON
    round trip, then renders a test note offline through its OWN SynthEngine
    instance — never the live one — and records level, non-finite samples,
    safety counters and CPU.

    `validateOne` is synchronous and free of threading so the unit tests can
    call it directly; `startValidation` runs the whole set on a background
    juce::Thread and publishes results for a view to poll.
*/
class PresetValidator : private juce::Thread
{
public:
    using Options = PresetValidatorOptions;

    PresetValidator();
    ~PresetValidator() override;

    /** Starts validating every factory preset on a background thread. */
    void startValidation (const Options& options = Options());
    void cancelValidation();

    bool  busy() const;
    float progress() const;                  ///< 0..1
    int   totalPresets() const;
    std::vector<PresetValidationResult> results() const;
    juce::String summary() const;

    /** Validates one preset synchronously using the supplied engine.
        The engine is prepared and reset by this call; nothing else is touched. */
    static PresetValidationResult validateOne (PresetManager& presets, int index,
                                               const Options& options, SynthEngine& engine);

    /** Validates one preset with a freshly allocated engine (convenience for tests). */
    static PresetValidationResult validateOne (PresetManager& presets, int index,
                                               const Options& options = Options());

private:
    void run() override;

    mutable juce::CriticalSection lock;
    std::vector<PresetValidationResult> completed;
    PresetManager presetManager;
    Options opts;
    std::atomic<int> total { 0 };
    std::atomic<int> done  { 0 };
};

} // namespace am::dev

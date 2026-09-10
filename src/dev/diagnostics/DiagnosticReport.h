#pragma once

#include "DiagnosticSnapshot.h"
#include "state/ParameterRegistry.h"

#include <juce_data_structures/juce_data_structures.h>

namespace am::dev
{

/**
    DIAGNOSTIC REPORT (§88)

    A JSON description of what the engine is doing right now: build, host
    environment, engine configuration, CPU profile, safety counters, the
    active parameter values, materials, topology seed and the current preset
    name. It contains no user audio and no file system paths.

    The struct is filled by DSP LAB on the message thread; the conversion is
    pure so the unit tests can verify the document without a GUI.
*/
struct DiagnosticReportInput
{
    juce::String buildVersion;
    juce::String os;
    juce::String presetName;
    juce::StringArray presetTags;
    int  abSlot = 0;
    juce::String timestamp;             ///< ISO-8601; filled by `now()` when empty

    DiagnosticSnapshot snapshot;
    ParamValues        parameters = ParameterRegistry::defaults();

    /** Optional extra measurements the caller wants recorded (name → value). */
    std::vector<std::pair<juce::String, double>> extras;

    /** Fills build / OS / timestamp from the running process. */
    static DiagnosticReportInput environment();
};

class DiagnosticReport
{
public:
    /** Builds the report document. */
    static juce::var toVar (const DiagnosticReportInput& in);

    /** Serialises the report (pretty printed). */
    static juce::String toJson (const DiagnosticReportInput& in);

    /** Writes the report to a file. Message thread only. */
    static juce::Result writeTo (const juce::File& file, const DiagnosticReportInput& in);

    /** Default export location: <Desktop>/antimatr-diagnostics-<time>.json */
    static juce::File defaultFile();

    /** Parameters whose value differs from the registry default ("active"). */
    static int numActiveParameters (const ParamValues& values) noexcept;
};

} // namespace am::dev

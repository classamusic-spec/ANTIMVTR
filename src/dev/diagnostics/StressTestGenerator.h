#pragma once

#include "core/Random.h"
#include "state/ParameterRegistry.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

namespace am::dev
{

/** The stress scenarios of §84. */
enum class StressTest : uint8_t
{
    SustainedNote = 0,
    Chord16,
    MaxPolyphony,
    NoteFlood,
    ExtremeVelocity,
    PitchBendSweep,
    AutomationSweep,
    MacroMotion,
    PresetSwitchLoop,
    LongRun,
    Count
};

/** One thing the controller has to do at a point in time. */
struct StressAction
{
    enum class Kind : uint8_t { NoteOn, NoteOff, AllNotesOff, PitchBend, Parameter, Preset };

    Kind  kind       = Kind::NoteOn;
    int   channel    = 1;
    int   note       = 60;
    float velocity   = 0.8f;      ///< 0..1
    float bend       = 0.0f;      ///< -1..1
    int   paramIndex = -1;        ///< index into ParamValues for Kind::Parameter
    float normalised = 0.0f;      ///< 0..1 target for Kind::Parameter
    int   preset     = 0;         ///< factory preset index for Kind::Preset
    double time      = 0.0;       ///< seconds since the test started
};

/**
    Deterministic action schedule for the DSP LAB stress tests (§84).

    The generator owns no threads and touches no engine: it turns elapsed time
    into the MIDI / parameter / preset actions a scenario requires, so the
    behaviour can be unit tested without a GUI, a timer or an audio device.
    `StressTestController` executes what this produces through
    `AntiMatrProcessor::injectMidi` and the host parameter tree.
*/
class StressTestGenerator
{
public:
    struct Config
    {
        int      numFactoryPresets = 1;
        int      maxVoices         = 64;
        uint32_t seed              = 0x5EED0001u;
        int      sweepParameter    = -1;    ///< parameter index for AutomationSweep (-1 = pick a default)
    };

    /** Arms a scenario. Time restarts at zero. */
    void start (StressTest test, const Config& config);

    /** Appends every action due between the previous call and `timeSeconds`.
        Call it repeatedly with a monotonically increasing time. */
    void advance (double timeSeconds, std::vector<StressAction>& out);

    /** Actions needed to leave the engine quiet (note-offs, bend to centre). */
    void stop (std::vector<StressAction>& out);

    bool   isRunning() const noexcept   { return running; }
    double elapsed() const noexcept     { return currentTime; }
    /** Nominal length in seconds; scenarios that run until stopped return 0. */
    double duration() const noexcept;
    int    notesHeld() const noexcept   { return heldCount; }
    StressTest test() const noexcept    { return kind; }

    static const char* name (StressTest t) noexcept;
    static const char* description (StressTest t) noexcept;

    /** Parameter used by the AutomationSweep scenario when none is configured. */
    static int defaultSweepParameter() noexcept;

private:
    void emitNoteOn (std::vector<StressAction>& out, int note, float velocity, double t);
    void emitNoteOff (std::vector<StressAction>& out, int note, double t);
    void releaseAll (std::vector<StressAction>& out, double t);
    /** Fires `fn(stepIndex, stepTime)` for every step of a fixed-rate stream. */
    template <typename Fn>
    void forEachStep (double from, double to, double interval, Fn&& fn);

    StressTest kind = StressTest::SustainedNote;
    Config cfg;
    Rng rng;
    bool   running = false;
    double currentTime = 0.0;
    double lastTime = 0.0;
    int    heldCount = 0;
    std::array<bool, 128> held {};
    int    presetCursor = 0;
    std::array<float, kNumMacros> macroValue {};
};

} // namespace am::dev

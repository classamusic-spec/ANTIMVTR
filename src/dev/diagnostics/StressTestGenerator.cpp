#include "StressTestGenerator.h"

#include <algorithm>
#include <cmath>

namespace am::dev
{

namespace
{
    constexpr double kChordSpread   = 0.012;  // seconds between the notes of a chord
    constexpr double kPolyInterval  = 0.06;   // one more voice every 60 ms
    constexpr double kFloodInterval = 0.025;  // 40 notes per second
    constexpr double kFloodHold     = 0.12;
    constexpr double kVelocityStep  = 0.25;
    constexpr double kBendRate      = 0.02;   // 50 Hz bend updates
    constexpr double kBendPeriod    = 4.0;
    constexpr double kSweepPeriod   = 6.0;
    constexpr double kSweepRate     = 0.05;   // 20 automation writes per second
    constexpr double kMacroRate     = 0.05;
    constexpr double kPresetPeriod  = 0.75;
    constexpr double kLongRunPeriod = 2.5;

    constexpr int kChordRoot = 48;
    const int kChordOffsets[16] = { 0, 3, 7, 10, 12, 15, 19, 22, 24, 27, 31, 34, 36, 39, 43, 46 };
}

const char* StressTestGenerator::name (StressTest t) noexcept
{
    switch (t)
    {
        case StressTest::SustainedNote:    return "SUSTAINED NOTE";
        case StressTest::Chord16:          return "16-NOTE CHORD";
        case StressTest::MaxPolyphony:     return "MAX POLYPHONY";
        case StressTest::NoteFlood:        return "RAPID NOTE FLOOD";
        case StressTest::ExtremeVelocity:  return "EXTREME VELOCITY";
        case StressTest::PitchBendSweep:   return "PITCH BEND SWEEP";
        case StressTest::AutomationSweep:  return "AUTOMATION SWEEP";
        case StressTest::MacroMotion:      return "ALL MACROS RANDOM";
        case StressTest::PresetSwitchLoop: return "PRESET SWITCH LOOP";
        case StressTest::LongRun:          return "LONG DURATION RUN";
        default:                           return "UNKNOWN";
    }
}

const char* StressTestGenerator::description (StressTest t) noexcept
{
    switch (t)
    {
        case StressTest::SustainedNote:    return "One note held indefinitely: decay, DC and denormal behaviour.";
        case StressTest::Chord16:          return "16 simultaneous notes: voice mixing headroom and CPU.";
        case StressTest::MaxPolyphony:     return "Adds a voice every 60 ms up to the polyphony limit: stealing and CPU ceiling.";
        case StressTest::NoteFlood:        return "40 short notes per second at random pitches: allocation and voice churn.";
        case StressTest::ExtremeVelocity:  return "Alternates velocity 1 and 127: level safety at both ends.";
        case StressTest::PitchBendSweep:   return "Continuous +/- bend sweep on a held note: coefficient validity.";
        case StressTest::AutomationSweep:  return "Sweeps one parameter 0 -> 1 -> 0 while a note plays: smoothing and clicks.";
        case StressTest::MacroMotion:      return "All 8 macros perform a random walk: modulation load.";
        case StressTest::PresetSwitchLoop: return "Loads factory presets in a loop under a sustained note: state churn.";
        case StressTest::LongRun:          return "Re-triggers a chord every 2.5 s indefinitely: drift and leaks.";
        default:                           return "";
    }
}

int StressTestGenerator::defaultSweepParameter() noexcept
{
    return paramIndex (Param::shapeDensity);
}

double StressTestGenerator::duration() const noexcept
{
    switch (kind)
    {
        case StressTest::Chord16:         return 8.0;
        case StressTest::MaxPolyphony:    return kPolyInterval * (double) juce::jmax (1, cfg.maxVoices) + 4.0;
        case StressTest::ExtremeVelocity: return 12.0;
        case StressTest::PitchBendSweep:  return kBendPeriod * 3.0;
        case StressTest::AutomationSweep: return kSweepPeriod * 2.0;
        default:                          return 0.0;   // runs until stopped
    }
}

void StressTestGenerator::start (StressTest t, const Config& config)
{
    kind = t;
    cfg = config;
    if (cfg.sweepParameter < 0 || cfg.sweepParameter >= kNumParams)
        cfg.sweepParameter = defaultSweepParameter();
    cfg.maxVoices = juce::jlimit (1, kMaxVoices, cfg.maxVoices);
    cfg.numFactoryPresets = juce::jmax (1, cfg.numFactoryPresets);

    rng.reseed (cfg.seed);
    running = true;
    currentTime = 0.0;
    lastTime = -1.0e-9;     // so an action scheduled exactly at t = 0 fires on the first advance
    heldCount = 0;
    held.fill (false);
    presetCursor = 0;
    macroValue.fill (0.5f);
}

void StressTestGenerator::emitNoteOn (std::vector<StressAction>& out, int note, float velocity, double t)
{
    note = juce::jlimit (0, 127, note);
    if (held[(size_t) note])
        emitNoteOff (out, note, t);

    StressAction a;
    a.kind = StressAction::Kind::NoteOn;
    a.note = note;
    a.velocity = juce::jlimit (0.0f, 1.0f, velocity);
    a.time = t;
    out.push_back (a);
    held[(size_t) note] = true;
    ++heldCount;
}

void StressTestGenerator::emitNoteOff (std::vector<StressAction>& out, int note, double t)
{
    note = juce::jlimit (0, 127, note);
    if (! held[(size_t) note])
        return;

    StressAction a;
    a.kind = StressAction::Kind::NoteOff;
    a.note = note;
    a.time = t;
    out.push_back (a);
    held[(size_t) note] = false;
    --heldCount;
}

void StressTestGenerator::releaseAll (std::vector<StressAction>& out, double t)
{
    for (int n = 0; n < 128; ++n)
        if (held[(size_t) n])
            emitNoteOff (out, n, t);
}

template <typename Fn>
void StressTestGenerator::forEachStep (double from, double to, double interval, Fn&& fn)
{
    if (interval <= 0.0) return;
    // Steps live at k * interval; emit every k in (from, to].
    int first = (int) std::floor (from / interval) + 1;
    const int last = (int) std::floor (to / interval);
    if (first < 0) first = 0;
    for (int k = first; k <= last; ++k)
        fn (k, (double) k * interval);
}

void StressTestGenerator::advance (double timeSeconds, std::vector<StressAction>& out)
{
    if (! running || timeSeconds <= lastTime)
        return;

    const double from = lastTime, to = timeSeconds;
    currentTime = to;

    switch (kind)
    {
        case StressTest::SustainedNote:
            if (from < 0.0 && to >= 0.0)
                emitNoteOn (out, 60, 0.85f, 0.0);
            break;

        case StressTest::Chord16:
            forEachStep (from, to, kChordSpread, [&] (int k, double t)
            {
                if (k < 16)
                    emitNoteOn (out, kChordRoot + kChordOffsets[k], 0.7f, t);
            });
            break;

        case StressTest::MaxPolyphony:
            forEachStep (from, to, kPolyInterval, [&] (int k, double t)
            {
                if (k < cfg.maxVoices)
                    emitNoteOn (out, 24 + (k * 5) % 72, 0.6f, t);
            });
            break;

        case StressTest::NoteFlood:
            forEachStep (from, to, kFloodInterval, [&] (int k, double t)
            {
                // Release the note started kFloodHold ago, then start a new one.
                const int releaseIndex = k - (int) std::round (kFloodHold / kFloodInterval);
                if (releaseIndex >= 0)
                {
                    Rng r (hashSeed (cfg.seed, (uint32_t) releaseIndex));
                    emitNoteOff (out, 30 + r.nextInt (60), t);
                }
                Rng r (hashSeed (cfg.seed, (uint32_t) k));
                emitNoteOn (out, 30 + r.nextInt (60), 0.4f + 0.6f * r.nextFloat(), t);
            });
            break;

        case StressTest::ExtremeVelocity:
            forEachStep (from, to, kVelocityStep, [&] (int k, double t)
            {
                releaseAll (out, t);
                emitNoteOn (out, 55 + (k % 7) * 2, (k % 2) == 0 ? 1.0f / 127.0f : 1.0f, t);
            });
            break;

        case StressTest::PitchBendSweep:
            if (from < 0.0 && to >= 0.0)
                emitNoteOn (out, 57, 0.8f, 0.0);
            forEachStep (from, to, kBendRate, [&] (int, double t)
            {
                StressAction a;
                a.kind = StressAction::Kind::PitchBend;
                a.bend = (float) std::sin (kTwoPi * t / kBendPeriod);
                a.time = t;
                out.push_back (a);
            });
            break;

        case StressTest::AutomationSweep:
            if (from < 0.0 && to >= 0.0)
                emitNoteOn (out, 52, 0.8f, 0.0);
            forEachStep (from, to, kSweepRate, [&] (int, double t)
            {
                const double phase = std::fmod (t, kSweepPeriod) / kSweepPeriod;
                StressAction a;
                a.kind = StressAction::Kind::Parameter;
                a.paramIndex = cfg.sweepParameter;
                a.normalised = (float) (phase < 0.5 ? phase * 2.0 : (1.0 - phase) * 2.0);
                a.time = t;
                out.push_back (a);
            });
            break;

        case StressTest::MacroMotion:
            if (from < 0.0 && to >= 0.0)
                emitNoteOn (out, 45, 0.75f, 0.0);
            forEachStep (from, to, kMacroRate, [&] (int, double t)
            {
                for (int m = 0; m < kNumMacros; ++m)
                {
                    macroValue[(size_t) m] = juce::jlimit (0.0f, 1.0f, macroValue[(size_t) m] + 0.08f * rng.nextBipolar());
                    StressAction a;
                    a.kind = StressAction::Kind::Parameter;
                    a.paramIndex = paramIndex (Param::macro1) + m;
                    a.normalised = macroValue[(size_t) m];
                    a.time = t;
                    out.push_back (a);
                }
            });
            break;

        case StressTest::PresetSwitchLoop:
            if (from < 0.0 && to >= 0.0)
                emitNoteOn (out, 50, 0.8f, 0.0);
            forEachStep (from, to, kPresetPeriod, [&] (int, double t)
            {
                StressAction a;
                a.kind = StressAction::Kind::Preset;
                a.preset = presetCursor % cfg.numFactoryPresets;
                a.time = t;
                out.push_back (a);
                ++presetCursor;
            });
            break;

        case StressTest::LongRun:
            forEachStep (from, to, kLongRunPeriod, [&] (int k, double t)
            {
                releaseAll (out, t);
                for (int i = 0; i < 6; ++i)
                    emitNoteOn (out, 36 + ((k * 5 + i * 7) % 48), 0.65f, t);
            });
            break;

        default:
            break;
    }

    lastTime = to;
}

void StressTestGenerator::stop (std::vector<StressAction>& out)
{
    if (! running)
        return;

    releaseAll (out, currentTime);

    StressAction all;
    all.kind = StressAction::Kind::AllNotesOff;
    all.time = currentTime;
    out.push_back (all);

    if (kind == StressTest::PitchBendSweep)
    {
        StressAction bend;
        bend.kind = StressAction::Kind::PitchBend;
        bend.bend = 0.0f;
        bend.time = currentTime;
        out.push_back (bend);
    }

    running = false;
}

} // namespace am::dev

#include "FractureSequencer.h"

namespace am
{

double FractureSequencer::divisionBeats (int division) noexcept
{
    // "1/1|1/2|1/4|1/8|1/16|1/32|1/4T|1/8T|1/16T|1/4D|1/8D|1/16D" — in quarter notes.
    static constexpr double kBeats[] = { 4.0, 2.0, 1.0, 0.5, 0.25, 0.125,
                                         1.0 * 2.0 / 3.0, 0.5 * 2.0 / 3.0, 0.25 * 2.0 / 3.0,
                                         1.5, 0.75, 0.375 };
    constexpr int n = (int) (sizeof (kBeats) / sizeof (kBeats[0]));
    return kBeats[juce::jlimit (0, n - 1, division)];
}

double FractureSequencer::stepSamples (const Settings& s, double sampleRate) noexcept
{
    double seconds;
    if (s.sync)
    {
        const double bpm = juce::jlimit (10.0, 999.0, s.bpm);
        seconds = divisionBeats (s.division) * 60.0 / bpm;
    }
    else
    {
        seconds = 1.0 / (double) juce::jlimit (0.01f, 200.0f, s.rateHz);
    }
    return juce::jlimit (16.0, sampleRate * 30.0, seconds * sampleRate);
}

void FractureSequencer::prepare (double sampleRate)
{
    sr = sampleRate > 0.0 ? sampleRate : 48000.0;
    reset (currentSeed);
}

void FractureSequencer::reset (uint32_t seed)
{
    currentSeed = seed;
    restart();
}

void FractureSequencer::restart() noexcept
{
    rng.reseed (currentSeed);
    position = 0.0;
    steppedCount = 0;
    cursor = 0;
    pingPongDir = 1;
    needsFirstStep = true;
    current = StepState();
}

void FractureSequencer::beginStep (const Settings& s, const FractureTable& table)
{
    const int n = juce::jlimit (1, kMaxSequencerSteps, s.numSteps);

    if (needsFirstStep)
    {
        cursor = (s.direction == FractureDirection::Backward) ? n - 1 : 0;
        pingPongDir = 1;
        needsFirstStep = false;
    }
    else
    {
        switch (s.direction)
        {
            case FractureDirection::Backward: cursor = (cursor + n - 1) % n; break;
            case FractureDirection::Random:   cursor = rng.nextInt (n);      break;
            case FractureDirection::PingPong:
                if (n <= 1) { cursor = 0; break; }
                cursor += pingPongDir;
                if (cursor >= n)     { cursor = n - 2; pingPongDir = -1; }
                else if (cursor < 0) { cursor = 1;     pingPongDir =  1; }
                break;
            case FractureDirection::Forward:
            case FractureDirection::Count:
            default: cursor = (cursor + 1) % n; break;
        }
        ++steppedCount;
    }

    cursor = juce::jlimit (0, n - 1, cursor);
    const auto& st = table.steps[(size_t) cursor];

    current.index  = cursor;
    current.mask   = st.mask;
    current.gate   = st.gate;
    current.pitch  = st.pitch;
    current.pan    = st.pan;
    current.gain   = st.gain;
    current.evolve = st.evolve;
    current.shape  = st.shape;

    const float p = juce::jlimit (0.0f, 1.0f, st.probability) * juce::jlimit (0.0f, 1.0f, s.probability);
    current.active = rng.chance (p);

    if (s.randomAmount > 1.0e-4f)
    {
        const float r = juce::jlimit (0.0f, 1.0f, s.randomAmount);
        current.gate  *= 1.0f - r * rng.nextFloat() * 0.85f;
        current.gain  *= 1.0f - r * rng.nextFloat() * 0.5f;
        current.pitch += r * rng.nextBipolar() * 12.0f;
        current.pan    = juce::jlimit (-1.0f, 1.0f, current.pan + r * rng.nextBipolar());
        // Flip fragment-selection bits with probability r/2 each.
        uint32_t flips = 0;
        for (int b = 0; b < kMaxFractureFragments; ++b)
            if (rng.chance (r * 0.5f)) flips |= (1u << b);
        current.mask ^= flips;
    }

    // Swing: lengthen even steps, shorten odd ones (2:1 at swing = 1).
    const double base = stepSamples (s, sr);
    const double sw = (double) juce::jlimit (0.0f, 1.0f, s.swing) / 3.0;
    stepLength = juce::jmax (16.0, base * (((cursor & 1) == 0) ? 1.0 + sw : 1.0 - sw));
}

bool FractureSequencer::advance (int samples, const Settings& s, const FractureTable& table)
{
    bool stepped = false;

    if (needsFirstStep)
    {
        beginStep (s, table);
        stepped = true;
    }

    position += (double) samples;

    int guard = 0;
    while (position >= stepLength && guard++ < 64)
    {
        position -= stepLength;
        beginStep (s, table);
        stepped = true;
    }

    if (guard >= 64) position = 0.0;   // absurd rate: resynchronise rather than spin
    return stepped;
}

} // namespace am

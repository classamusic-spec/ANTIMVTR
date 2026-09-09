#include "WaveSource.h"
#include "core/Random.h"

namespace am
{

void WaveSource::prepare (double sampleRate, int)
{
    sr = sampleRate;
    reset();
}

void WaveSource::reset()
{
    phase = 0.0;
    lastLevel = 0.0f;
}

void WaveSource::noteOn (const NoteState& note, const ParamValues& params)
{
    const float startPhase = paramValue (params, Param::wavePhase);
    const float randomAmount = paramValue (params, Param::wavePhaseRandom);
    Rng rng (hashSeed (note.noteId, 0x57A7E));
    phase = (double) startPhase + (double) (randomAmount * rng.nextFloat());
    phase -= std::floor (phase);
}

void WaveSource::render (float* l, float* r, int n, const RenderContext& ctx, const NoteState& note)
{
    const float level = ctx.param (Param::waveLevel);
    const double pitchOffset = (double) ctx.param (Param::waveOctave) * 12.0
                             + (double) ctx.param (Param::waveSemi)
                             + (double) ctx.param (Param::waveFine) * 0.01;
    const double freq = note.frequency * std::pow (2.0, pitchOffset / 12.0);
    const double inc = juce::jlimit (0.0, (double) kMaxFrequencyRatio, freq / ctx.sampleRate);

    double ph = phase;
    for (int i = 0; i < n; ++i)
    {
        const float s = (float) std::sin (ph * kTwoPi) * level;
        l[i] = s;
        r[i] = s;
        ph += inc;
        if (ph >= 1.0) ph -= 1.0;
    }
    phase = ph;
    lastLevel = level;
}

} // namespace am

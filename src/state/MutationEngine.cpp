#include "MutationEngine.h"

namespace am
{

void MutationEngine::safeRange (Param p, float& lo, float& hi) noexcept
{
    const auto& d = ParameterRegistry::get (p);
    lo = d.min; hi = d.max;

    switch (p)
    {
        case Param::fractureFeedback:
        case Param::spaceFeedback:
        case Param::spaceDelayFeedback:  hi = 0.85f; break;
        case Param::masterTranspose:     lo = -12.0f; hi = 12.0f; break;
        case Param::waveOctave:          lo = -2.0f; hi = 1.0f; break;
        case Param::shapePitch:
        case Param::dustPitch:
        case Param::samplePitch:
        case Param::fracturePitch:
        case Param::spaceGrainPitch:     lo = -12.0f; hi = 12.0f; break;
        case Param::waveUnison:          hi = 6.0f; break;
        case Param::ampAttack:           hi = 3.0f; break;
        case Param::ampRelease:          hi = 6.0f; break;
        case Param::spaceMix:            hi = 0.7f; break;
        case Param::spaceDistDrive:      hi = 0.7f; break;
        default: break;
    }
}

float MutationEngine::weight (Param p) noexcept
{
    switch (p)
    {
        case Param::shapeDensity: case Param::shapeForm: case Param::shapeMass:
        case Param::shapeTension: case Param::shapeDecay: case Param::shapeSurface:
        case Param::evolveBend: case Param::evolveMelt: case Param::evolveTear: case Param::evolveMagnet:
            return 1.0f;
        case Param::masterTranspose: case Param::waveOctave:
            return 0.25f;
        case Param::ampSustain: case Param::ampVelocity:
            return 0.3f;
        default:
            return 0.6f;
    }
}

void MutationEngine::mutate (ParamValues& values, MutationStrength strength, uint32_t seed, CategoryMask categories)
{
    Rng rng (seed);

    float sigma = 0.06f, choiceProb = 0.05f, boolProb = 0.03f;
    switch (strength)
    {
        case MutationStrength::Subtle:  sigma = 0.06f; choiceProb = 0.05f; boolProb = 0.03f; break;
        case MutationStrength::Evolve:  sigma = 0.18f; choiceProb = 0.20f; boolProb = 0.10f; break;
        case MutationStrength::Extreme: sigma = 0.40f; choiceProb = 0.45f; boolProb = 0.25f; break;
    }

    for (const auto& d : ParameterRegistry::all())
    {
        if (d.mutation == MutationCategory::None) continue;
        if ((categories & maskFor (d.mutation)) == 0) continue;

        // Pitch changes only at higher strengths, and never for the master tuning at Subtle.
        if (d.mutation == MutationCategory::Pitch && strength == MutationStrength::Subtle) continue;

        const size_t i = (size_t) paramIndex (d.param);
        float lo, hi;
        safeRange (d.param, lo, hi);
        const float w = weight (d.param);
        float v = values[i];

        switch (d.kind)
        {
            case ParamKind::Float:
            {
                const float range = hi - lo;
                v += rng.nextGaussian() * sigma * w * range;
                v = juce::jlimit (lo, hi, v);
                break;
            }
            case ParamKind::Int:
            {
                if (rng.chance (choiceProb * w))
                {
                    const float range = hi - lo;
                    v += std::round (rng.nextGaussian() * sigma * w * range);
                    v = juce::jlimit (lo, hi, v);
                }
                break;
            }
            case ParamKind::Choice:
            {
                if (rng.chance (choiceProb * w))
                    v = (float) rng.nextInt (d.numChoices());
                break;
            }
            case ParamKind::Bool:
            {
                if (rng.chance (boolProb * w))
                    v = v >= 0.5f ? 0.0f : 1.0f;
                break;
            }
        }
        values[i] = d.clampValue (v);
    }

    // Keep the current source audible: SINGLE mode never mutates into a silent layer.
    values[(size_t) paramIndex (Param::sourceMode)] = 0.0f;
}

void MutationEngine::randomize (ParamValues& values, uint32_t seed)
{
    ParameterRegistry::fillDefaults (values);
    Rng rng (seed ^ 0xA5A5A5A5u);

    for (const auto& d : ParameterRegistry::all())
    {
        if (d.mutation == MutationCategory::None) continue;
        const size_t i = (size_t) paramIndex (d.param);
        float lo, hi;
        safeRange (d.param, lo, hi);

        switch (d.kind)
        {
            case ParamKind::Float:
            {
                // Mix of a preferred central region and the full safe range.
                const float t = rng.chance (0.6f) ? 0.5f + 0.5f * rng.nextGaussian() * 0.4f : rng.nextFloat();
                values[i] = d.clampValue (lo + (hi - lo) * clamp01 (t));
                break;
            }
            case ParamKind::Int:
                values[i] = d.clampValue (lo + std::round ((hi - lo) * rng.nextFloat()));
                break;
            case ParamKind::Choice:
                if (rng.chance (0.5f)) values[i] = (float) rng.nextInt (d.numChoices());
                break;
            case ParamKind::Bool:
                if (rng.chance (0.3f)) values[i] = values[i] >= 0.5f ? 0.0f : 1.0f;
                break;
        }
    }

    // A random patch is always immediately playable.
    values[(size_t) paramIndex (Param::sourceMode)] = 0.0f;
    values[(size_t) paramIndex (Param::sourceSelected)] = (float) rng.nextInt (3); // WAVE / DUST / IMPACT
    values[(size_t) paramIndex (Param::masterGain)] = 0.0f;
    values[(size_t) paramIndex (Param::ampSustain)] = std::max (values[(size_t) paramIndex (Param::ampSustain)], 0.2f);
    values[(size_t) paramIndex (Param::shapeMix)] = std::max (values[(size_t) paramIndex (Param::shapeMix)], 0.5f);
}

} // namespace am

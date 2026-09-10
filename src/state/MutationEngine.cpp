#include "MutationEngine.h"

namespace am
{

//==============================================================================
// DNA
//
// Three things decide how far a parameter is allowed to travel:
//
//   safeRange        the hard walls — outside them the instrument stops being
//                    playable (runaway feedback, a note that never arrives,
//                    a source that has gone silent).
//   preferredRegion  where the parameter sounds like ANTI-MATR. SUBTLE never
//                    pushes a value further out of it, EVOLVE occasionally
//                    jumps inside it and EXTREME redraws from it, which is
//                    what makes a major restructure safe rather than a lottery.
//   weight           per parameter x per DNA category: SHAPE and EVOLVE carry
//                    the identity of a patch and move most, SPACE and PITCH
//                    move least so a mutant still sits in the same mix and
//                    the same key.
//==============================================================================

namespace
{
    struct Region { float lo, hi; };

    /** The musical region of a parameter; defaults to the middle of its safe range. */
    Region preferred (Param p, float lo, float hi) noexcept
    {
        switch (p)
        {
            // ---- SHAPE: the identity controls are allowed to use their whole span
            case Param::shapeForm:         return { 0.00f, 1.00f };
            case Param::shapeDensity:      return { 0.15f, 0.90f };
            case Param::shapeMass:         return { 0.10f, 0.88f };
            case Param::shapeTension:      return { 0.18f, 0.85f };
            case Param::shapeDecay:        return { 0.15f, 0.88f };
            case Param::shapeSurface:      return { 0.00f, 0.68f };
            case Param::shapeCoupling:     return { 0.08f, 0.70f };
            case Param::shapeDistribution: return { 0.20f, 0.80f };
            case Param::shapeBlend:        return { 0.00f, 1.00f };
            case Param::shapeExcite:       return { 0.28f, 0.92f };
            case Param::shapeStrike:       return { 0.15f, 0.95f };
            case Param::shapeMix:          return { 0.45f, 1.00f };
            case Param::shapeStereo:       return { 0.20f, 0.95f };
            case Param::shapeKeytrack:     return { 0.85f, 1.00f };

            // ---- EVOLVE: the operators are dramatic; the top of their range is not musical
            case Param::evolveBend:        return { 0.00f, 0.70f };
            case Param::evolveMelt:        return { 0.00f, 0.65f };
            case Param::evolveTear:        return { 0.00f, 0.65f };
            case Param::evolveMagnet:      return { 0.00f, 0.85f };
            case Param::evolveGravity:     return { 0.25f, 0.75f };
            case Param::evolveScatter:     return { 0.00f, 0.55f };
            case Param::evolveCrush:       return { 0.00f, 0.45f };
            case Param::evolveSpeed:       return { 0.05f, 0.70f };
            case Param::evolveMotion:      return { 0.00f, 0.65f };

            // ---- amplitude: a mutant still has to speak when a key goes down
            case Param::ampAttack:         return { 0.001f, 1.20f };
            case Param::ampDecay:          return { 0.05f, 4.00f };
            case Param::ampSustain:        return { 0.00f, 1.00f };
            case Param::ampRelease:        return { 0.08f, 4.00f };

            // ---- sources: never mutate the energy away
            case Param::waveLevel:
            case Param::dustLevel:
            case Param::impactLevel:
            case Param::sampleLevel:
            case Param::gestureLevel:      return { 0.55f, 1.00f };
            case Param::waveDetune:        return { 0.02f, 0.55f };
            case Param::waveFM:
            case Param::wavePM:
            case Param::waveAM:
            case Param::waveRing:
            case Param::waveSync:          return { 0.00f, 0.55f };
            case Param::dustDensity:       return { 0.15f, 0.90f };
            case Param::dustGrain:         return { 0.05f, 0.75f };
            case Param::impactLength:      return { 0.05f, 0.70f };
            case Param::impactRate:        return { 0.00f, 0.55f };
            case Param::gesturePressure:   return { 0.25f, 0.85f };
            case Param::gestureSpeed:      return { 0.15f, 0.85f };
            case Param::gestureRoughness:  return { 0.05f, 0.75f };
            case Param::sampleStart:       return { 0.00f, 0.45f };
            case Param::sampleEnd:         return { 0.55f, 1.00f };

            // ---- FRACTURE: engaged, but never a wall of feedback
            case Param::fractureAmount:      return { 0.20f, 0.85f };
            case Param::fractureMix:         return { 0.30f, 1.00f };
            case Param::fractureFeedback:    return { 0.00f, 0.60f };
            case Param::fractureSpread:      return { 0.05f, 0.80f };
            case Param::fractureRandom:      return { 0.00f, 0.60f };
            case Param::fractureProbability: return { 0.45f, 1.00f };

            // ---- SPACE: a mutant stays in the same room
            case Param::spaceMix:           return { 0.10f, 0.60f };
            case Param::spaceSize:          return { 0.15f, 0.90f };
            case Param::spaceFeedback:      return { 0.10f, 0.65f };
            case Param::spaceDistDrive:     return { 0.05f, 0.55f };
            case Param::spaceDelayFeedback: return { 0.10f, 0.65f };
            case Param::spaceReverbDecay:   return { 0.15f, 0.85f };

            default: break;
        }

        // Everything else prefers the central 80 % of its safe range.
        const float span = hi - lo;
        return { lo + 0.10f * span, hi - 0.10f * span };
    }

    /** How freely a DNA category may move. SHAPE and EVOLVE are the patch; SPACE and PITCH are context. */
    float categoryWeight (MutationCategory c) noexcept
    {
        switch (c)
        {
            case MutationCategory::Shape:    return 1.00f;
            case MutationCategory::Evolve:   return 1.00f;
            case MutationCategory::Source:   return 0.85f;
            case MutationCategory::Chaos:    return 0.80f;
            case MutationCategory::Fracture: return 0.70f;
            case MutationCategory::Movement: return 0.60f;
            case MutationCategory::Space:    return 0.45f;
            case MutationCategory::Pitch:    return 0.30f;
            default:                         return 0.00f;
        }
    }

    /** How each strength draws: a walk, a walk with jumps, or a redraw. */
    struct StrengthProfile
    {
        float sigma;             ///< gaussian move as a fraction of the safe span
        float jumpProb;          ///< chance of redrawing inside the preferred region
        float jumpAmount;        ///< how far a jump travels (1 = land on the new value)
        float choiceProb;        ///< chance a choice or integer steps
        float boolProb;
        bool  clampToPreferred;  ///< SUBTLE never pushes a value further out of its region
    };

    StrengthProfile profileFor (MutationStrength s) noexcept
    {
        switch (s)
        {
            case MutationStrength::Subtle:  return { 0.045f, 0.00f, 0.00f, 0.04f, 0.02f, true };
            case MutationStrength::Evolve:  return { 0.150f, 0.28f, 0.55f, 0.20f, 0.10f, false };
            case MutationStrength::Extreme: return { 0.090f, 0.85f, 0.85f, 0.50f, 0.28f, false };
        }
        return { 0.045f, 0.0f, 0.0f, 0.04f, 0.02f, true };
    }

    inline float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }

    /**
        Post-conditions every mutant must satisfy, whatever the dice said.

        These only ever clamp: a patch that was already safe is untouched, which
        is what lets a category-masked mutation leave the rest of the patch alone.
    */
    void enforceSafety (ParamValues& v) noexcept
    {
        const auto value = [&v] (Param p) { return v[(size_t) paramIndex (p)]; };
        const auto write = [&v] (Param p, float x) { v[(size_t) paramIndex (p)] = ParameterRegistry::get (p).clampValue (x); };

        // SINGLE mode: a mutant never hides its source behind a silent layer.
        write (Param::sourceMode, 0.0f);

        // The selected source must still produce energy.
        static constexpr Param levels[] = { Param::waveLevel, Param::dustLevel, Param::impactLevel,
                                            Param::sampleLevel, Param::gestureLevel };
        const int source = juce::jlimit (0, 4, (int) std::lround (value (Param::sourceSelected)));
        write (levels[source], juce::jmax (0.4f, value (levels[source])));

        // A percussive envelope may not also have a two second attack.
        if (value (Param::ampSustain) < 0.25f)
            write (Param::ampAttack, juce::jmin (value (Param::ampAttack), 1.0f));

        // A long ring plus heavy coupling plus heavy surface is a screaming resonator.
        if (value (Param::shapeDecay) > 0.85f)
        {
            write (Param::shapeCoupling, juce::jmin (value (Param::shapeCoupling), 0.65f));
            write (Param::shapeSurface, juce::jmin (value (Param::shapeSurface), 0.65f));
        }

        // Regeneration limits (the engine survives more than this, the music does not).
        write (Param::fractureFeedback, juce::jmin (value (Param::fractureFeedback), 0.80f));
        write (Param::spaceFeedback, juce::jmin (value (Param::spaceFeedback), 0.80f));
        write (Param::spaceDelayFeedback, juce::jmin (value (Param::spaceDelayFeedback), 0.80f));

        // Wet, not drowned.
        write (Param::spaceMix, juce::jmin (value (Param::spaceMix), 0.70f));
    }
}

//==============================================================================
void MutationEngine::safeRange (Param p, float& lo, float& hi) noexcept
{
    const auto& d = ParameterRegistry::get (p);
    lo = d.min; hi = d.max;

    switch (p)
    {
        // ---- regeneration
        case Param::fractureFeedback:
        case Param::spaceFeedback:
        case Param::spaceDelayFeedback:  hi = 0.80f; break;

        // ---- pitch: a mutant stays in the same key and the same octave region
        case Param::masterTranspose:     lo = -12.0f; hi = 12.0f; break;
        case Param::waveOctave:          lo = -2.0f; hi = 1.0f; break;
        case Param::waveSemi:            lo = -12.0f; hi = 12.0f; break;
        case Param::masterFine:
        case Param::waveFine:            lo = -50.0f; hi = 50.0f; break;
        case Param::shapePitch:
        case Param::dustPitch:
        case Param::samplePitch:
        case Param::fracturePitch:
        case Param::spaceGrainPitch:     lo = -12.0f; hi = 12.0f; break;
        case Param::shapeKeytrack:       lo = 0.75f; break;

        // ---- amplitude: the note has to arrive and it has to end
        case Param::ampAttack:           hi = 2.50f; break;
        case Param::ampDecay:            hi = 6.00f; break;
        case Param::ampRelease:          hi = 8.00f; break;

        // ---- the energy never disappears
        case Param::waveLevel:
        case Param::dustLevel:
        case Param::impactLevel:
        case Param::sampleLevel:
        case Param::gestureLevel:        lo = 0.40f; break;
        case Param::waveUnison:          hi = 6.0f; break;
        case Param::gesturePressure:     lo = 0.15f; break;
        case Param::sampleEnd:           lo = 0.25f; break;

        // ---- Matter stays a resonator rather than an oscillator
        case Param::shapeDecay:          hi = 0.94f; break;
        case Param::shapeCoupling:       hi = 0.82f; break;
        case Param::shapeSurface:        hi = 0.85f; break;
        case Param::shapeMix:            lo = 0.10f; break;

        // ---- Evolve extremes are for the player, not for the dice
        case Param::evolveTear:          hi = 0.85f; break;
        case Param::evolveCrush:         hi = 0.75f; break;
        case Param::evolveScatter:       hi = 0.75f; break;

        // ---- movement stays inside a musical rate range
        case Param::lfo1Rate: case Param::lfo2Rate: case Param::lfo3Rate: case Param::lfo4Rate:
        case Param::chaos1Rate: case Param::chaos2Rate: case Param::chaos3Rate: case Param::chaos4Rate:
        case Param::fractureRate:        hi = 20.0f; break;
        case Param::env1Attack: case Param::env2Attack: case Param::env3Attack: case Param::env4Attack:
        case Param::env1Decay:  case Param::env2Decay:  case Param::env3Decay:  case Param::env4Decay:
                                         hi = 8.0f; break;
        case Param::env1Release: case Param::env2Release: case Param::env3Release: case Param::env4Release:
                                         hi = 12.0f; break;

        // ---- Space: loud, wet and driven are all bounded
        case Param::spaceMix:            hi = 0.70f; break;
        case Param::spaceDistDrive:      hi = 0.70f; break;
        case Param::spaceGrainMix:       hi = 0.70f; break;
        case Param::spaceCompAmount:     hi = 0.75f; break;
        case Param::spaceEqLow:
        case Param::spaceEqMid:
        case Param::spaceEqHigh:         lo = -8.0f; hi = 8.0f; break;

        default: break;
    }
}

float MutationEngine::weight (Param p) noexcept
{
    const auto& d = ParameterRegistry::get (p);
    float w = categoryWeight (d.mutation);

    switch (p)
    {
        // The six Shape macros and the four headline operators are the mutation.
        case Param::shapeDensity: case Param::shapeForm: case Param::shapeMass:
        case Param::shapeTension: case Param::shapeDecay: case Param::shapeSurface:
        case Param::evolveBend: case Param::evolveMelt: case Param::evolveTear: case Param::evolveMagnet:
            break;

        // Structure: worth changing, but less often than the macros.
        case Param::shapeTopology: case Param::shapeMaterialA: case Param::shapeMaterialB:
        case Param::shapeBlend: case Param::shapeCoupling: case Param::shapeDistribution:
            w *= 0.8f; break;

        // Controls that decide whether the patch works at all.
        case Param::shapeMix: case Param::shapeKeytrack: case Param::shapeStereo:
        case Param::waveLevel: case Param::dustLevel: case Param::impactLevel:
        case Param::sampleLevel: case Param::gestureLevel:
            w *= 0.4f; break;

        case Param::ampSustain: case Param::ampVelocity: case Param::ampAttack: case Param::ampRelease:
            w *= 0.5f; break;

        case Param::masterTranspose: case Param::waveOctave: case Param::masterGlide:
            w *= 0.35f; break;

        default: break;
    }

    return juce::jlimit (0.0f, 1.0f, w);
}

//==============================================================================
void MutationEngine::mutate (ParamValues& values, MutationStrength strength, uint32_t seed, CategoryMask categories)
{
    Rng rng (seed);
    const auto profile = profileFor (strength);

    for (const auto& d : ParameterRegistry::all())
    {
        if (d.mutation == MutationCategory::None) continue;
        if ((categories & maskFor (d.mutation)) == 0) continue;

        // Pitch is structural: it only moves once the player has asked for more than a relative.
        if (d.mutation == MutationCategory::Pitch && strength == MutationStrength::Subtle) continue;

        const size_t i = (size_t) paramIndex (d.param);
        float lo, hi;
        safeRange (d.param, lo, hi);
        if (hi <= lo) continue;

        const float w = weight (d.param);
        if (w <= 0.0f) continue;

        const auto region = preferred (d.param, lo, hi);
        const float span = hi - lo;
        float v = values[i];

        switch (d.kind)
        {
            case ParamKind::Float:
            {
                const float start = v;
                v += rng.nextGaussian() * profile.sigma * w * span;

                // A jump redraws inside the preferred region instead of wandering
                // out of it: this is what makes EXTREME a restructure and not noise.
                if (profile.jumpProb > 0.0f && rng.chance (profile.jumpProb * w))
                {
                    const float target = rng.nextRange (region.lo, region.hi);
                    v = lerp (v, target, juce::jlimit (0.0f, 1.0f, profile.jumpAmount * (0.5f + 0.5f * rng.nextFloat())));
                }

                v = juce::jlimit (lo, hi, v);

                // SUBTLE never pushes a value further outside its region than it already was.
                if (profile.clampToPreferred)
                    v = juce::jlimit (juce::jmin (region.lo, start), juce::jmax (region.hi, start), v);
                break;
            }

            case ParamKind::Int:
            {
                if (rng.chance (profile.choiceProb * w))
                {
                    const float step = juce::jmax (1.0f, std::round (span * profile.sigma * 2.0f));
                    v = std::round (v + rng.nextGaussian() * step);
                }
                break;
            }

            case ParamKind::Choice:
            {
                if (rng.chance (profile.choiceProb * w))
                {
                    const int n = d.numChoices();
                    if (n > 1)
                    {
                        // A neighbouring choice at EVOLVE, anywhere at EXTREME: adjacent
                        // materials, modes and topologies are relatives, distant ones are not.
                        if (strength == MutationStrength::Extreme)
                            v = (float) rng.nextInt (n);
                        else
                            v = (float) juce::jlimit (0, n - 1, (int) std::lround (v) + (rng.chance (0.5f) ? 1 : -1));
                    }
                }
                break;
            }

            case ParamKind::Bool:
            {
                if (rng.chance (profile.boolProb * w))
                    v = v >= 0.5f ? 0.0f : 1.0f;
                break;
            }
        }

        values[i] = d.clampValue (juce::jlimit (lo, hi, v));
    }

    enforceSafety (values);
}

//==============================================================================
void MutationEngine::randomize (ParamValues& values, uint32_t seed)
{
    ParameterRegistry::fillDefaults (values);
    Rng rng (seed ^ 0xA5A5A5A5u);

    // A random patch starts from a random *playable* object: the structural
    // choices first, then every continuous control from its preferred region.
    values[(size_t) paramIndex (Param::sourceSelected)] = (float) rng.nextInt (5);
    values[(size_t) paramIndex (Param::shapeMaterialA)] = (float) rng.nextInt (8);
    values[(size_t) paramIndex (Param::shapeMaterialB)] = (float) rng.nextInt (8);
    values[(size_t) paramIndex (Param::shapeTopology)]  = (float) rng.nextInt (6);
    values[(size_t) paramIndex (Param::spaceType)]      = (float) rng.nextInt (8);

    for (const auto& d : ParameterRegistry::all())
    {
        if (d.mutation == MutationCategory::None) continue;
        const size_t i = (size_t) paramIndex (d.param);
        float lo, hi;
        safeRange (d.param, lo, hi);
        if (hi <= lo) continue;

        const auto region = preferred (d.param, lo, hi);

        switch (d.kind)
        {
            case ParamKind::Float:
            {
                // Mostly the preferred region, occasionally the whole safe range.
                const float v = rng.chance (0.82f) ? rng.nextRange (region.lo, region.hi)
                                                   : rng.nextRange (lo, hi);
                values[i] = d.clampValue (v);
                break;
            }
            case ParamKind::Int:
                values[i] = d.clampValue (std::round (rng.nextRange (region.lo, region.hi)));
                break;
            case ParamKind::Choice:
                if (rng.chance (0.6f)) values[i] = (float) rng.nextInt (d.numChoices());
                break;
            case ParamKind::Bool:
                if (rng.chance (0.3f)) values[i] = values[i] >= 0.5f ? 0.0f : 1.0f;
                break;
        }
    }

    // A random patch is always immediately playable.
    values[(size_t) paramIndex (Param::masterGain)] = 0.0f;
    values[(size_t) paramIndex (Param::shapeMix)] = juce::jmax (0.5f, values[(size_t) paramIndex (Param::shapeMix)]);
    enforceSafety (values);
}

} // namespace am

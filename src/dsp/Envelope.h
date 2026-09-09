#pragma once

#include "core/Types.h"

namespace am
{

/**
    Analog-style ADSR with exponential segments.

    Attack aims above 1.0 so it reaches full level in the requested time with
    a convex curve; decay and release are exponential approaches. `curve`
    blends toward a more linear response (0 = very exponential, 1 = near linear).
*/
class ADSREnvelope
{
public:
    enum class Phase : uint8_t { Idle, Attack, Decay, Sustain, Release, Kill };

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        killCoeff = (float) std::exp (-1.0 / (0.002 * sr)); // ~2 ms fade for voice stealing
        recompute();
    }

    void setParameters (float attackSeconds, float decaySeconds, float sustainLevel, float releaseSeconds, float curve01) noexcept
    {
        if (attackSeconds != attackTime || decaySeconds != decayTime || sustainLevel != sustain
            || releaseSeconds != releaseTime || curve01 != curve)
        {
            attackTime = attackSeconds; decayTime = decaySeconds; sustain = clamp01 (sustainLevel);
            releaseTime = releaseSeconds; curve = clamp01 (curve01);
            recompute();
        }
    }

    void noteOn() noexcept
    {
        phase = Phase::Attack;
        if (attackTime <= 0.0005f) { level = 1.0f; phase = Phase::Decay; }
    }

    void noteOff() noexcept
    {
        if (phase != Phase::Idle && phase != Phase::Kill)
            phase = Phase::Release;
    }

    /** Fast fade used when a voice is stolen. */
    void kill() noexcept { if (phase != Phase::Idle) phase = Phase::Kill; }

    void reset() noexcept { level = 0.0f; phase = Phase::Idle; }

    bool isActive() const noexcept { return phase != Phase::Idle; }
    bool isReleasing() const noexcept { return phase == Phase::Release || phase == Phase::Kill; }
    Phase getPhase() const noexcept { return phase; }
    float getLevel() const noexcept { return level; }

    inline float next() noexcept
    {
        switch (phase)
        {
            case Phase::Attack:
                level = attackTarget + attackCoeff * (level - attackTarget);
                if (level >= 1.0f) { level = 1.0f; phase = Phase::Decay; }
                break;

            case Phase::Decay:
                level = decayTarget + decayCoeff * (level - decayTarget);
                if (level <= sustain + 0.0005f) { level = sustain; phase = Phase::Sustain; }
                break;

            case Phase::Sustain:
                level = sustain;
                break;

            case Phase::Release:
                level = releaseTarget + releaseCoeff * (level - releaseTarget);
                if (level <= kSilenceThreshold) { level = 0.0f; phase = Phase::Idle; }
                break;

            case Phase::Kill:
                level *= killCoeff;
                if (level <= kSilenceThreshold) { level = 0.0f; phase = Phase::Idle; }
                break;

            case Phase::Idle:
            default:
                level = 0.0f;
                break;
        }
        return level;
    }

    /** Multiplies the buffer by the envelope, sample by sample. */
    void applyTo (float* l, float* r, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const float g = next();
            l[i] *= g;
            if (r != nullptr) r[i] *= g;
        }
    }

private:
    void recompute() noexcept
    {
        // Overshoot targets make exponential segments finish in the requested time.
        const float shapeExp = lerp (0.05f, 1.5f, curve);       // attack aims at 1 + shapeExp
        attackTarget  = 1.0f + shapeExp;
        attackCoeff   = coeffFor (attackTime, std::log ((attackTarget) / shapeExp));

        const float decayRange = std::max (1.0f - sustain, 0.001f);
        const float decayUndershoot = lerp (0.01f, 0.5f, curve) * decayRange;
        decayTarget   = sustain - decayUndershoot;
        decayCoeff    = coeffFor (decayTime, std::log ((1.0f - decayTarget) / decayUndershoot));

        const float releaseUndershoot = lerp (0.001f, 0.2f, curve);
        releaseTarget = -releaseUndershoot;
        releaseCoeff  = coeffFor (releaseTime, std::log ((1.0f + releaseUndershoot) / releaseUndershoot));
    }

    /** Coefficient so that an exponential approach covers `logRatio` nats in `seconds`. */
    float coeffFor (float seconds, float logRatio) const noexcept
    {
        const double samples = std::max (1.0, (double) seconds * sr);
        return (float) std::exp (-(double) logRatio / samples);
    }

    double sr = 48000.0;
    float attackTime = 0.005f, decayTime = 0.25f, sustain = 1.0f, releaseTime = 0.4f, curve = 0.5f;
    float attackTarget = 1.3f, attackCoeff = 0.99f;
    float decayTarget = 0.0f, decayCoeff = 0.99f;
    float releaseTarget = 0.0f, releaseCoeff = 0.99f;
    float killCoeff = 0.99f;
    float level = 0.0f;
    Phase phase = Phase::Idle;
};

} // namespace am

#pragma once

#include "FXCommon.h"

namespace am::fx
{

/**
    GRANULAR DELAY — clouds of pitched grains read out of a delay buffer.

    Grains are spawned at a rate set by DENSITY, each one a Hann-windowed
    read head that starts somewhere behind the write pointer, plays at a rate
    set by PITCH (plus a few cents of scatter) and is panned by a seeded RNG,
    so a mono source turns into a wide, breathing cloud. Regeneration writes
    the cloud back into the buffer through a soft saturator, which lets DUST
    build up huge granular tails that can never run away.

    All randomness comes from a seeded `am::Rng`: the same patch renders the
    same cloud every time.
*/
class GranularDelay
{
public:
    static constexpr int kMaxGrains = 28;
    static constexpr float kMaxRegen = 0.86f;

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        const int maxSamples = (int) (sampleRate * 3.0) + 64;
        for (auto& line : lines) line.prepare (maxSamples);
        bufferSize = lines[0].capacity();

        for (size_t i = 0; i < window.size(); ++i)
            window[i] = 0.5f - 0.5f * std::cos (kTwoPi * (double) i / (double) (window.size() - 1));

        mix.prepare (sampleRate, 25.0f);
        regen.prepare (sampleRate, 40.0f);
        reset();
    }

    void reset()
    {
        for (auto& line : lines) line.clear();
        for (auto& g : grains) g.active = false;
        countdown = 0;
        rng.reseed (0x6D4145u);
        feedbackL = feedbackR = 0.0f;
        clamped = false;
    }

    void setParams (float size01, float density01, float pitchSemis, float mix01,
                    float regen01, float sizeScale) noexcept
    {
        grainMs = clampf (expMap (clampf (size01, 0.0f, 1.0f), 25.0f, 420.0f) * sizeScale, 8.0f, 900.0f);
        density = clampf (density01, 0.0f, 1.0f);
        rate = std::pow (2.0f, clampf (pitchSemis, -24.0f, 24.0f) / 12.0f);
        spread = clampf (sizeScale, 0.35f, 2.5f);
        mix.setTarget (clampf (mix01, 0.0f, 1.0f));
        const float requested = clampf (regen01, 0.0f, 4.0f);
        clamped = requested > kMaxRegen;
        regen.setTarget (std::min (requested, kMaxRegen));
    }

    bool feedbackClamped() const noexcept { return clamped; }

    void process (float* l, float* r, int n) noexcept
    {
        const float grainsPerSecond = expMap (density, 5.0f, 70.0f);
        const int interval = std::max (16, (int) ((float) sr / grainsPerSecond));

        for (int i = 0; i < n; ++i)
        {
            const float m = mix.next();
            const float fb = regen.next();

            lines[0].write (l[i] + softLimit (fb * feedbackL));
            lines[1].write (r[i] + softLimit (fb * feedbackR));

            if (--countdown <= 0)
            {
                spawn();
                countdown = interval + rng.nextInt (std::max (1, interval / 3));
            }

            float wetL = 0.0f, wetR = 0.0f;
            const double writeIndex = (double) lines[0].writeIndex();

            for (auto& g : grains)
            {
                if (! g.active) continue;

                const float phase = g.age / g.length;
                const float w = windowAt (phase);
                const double pos = writeIndex - g.offset + g.age * (double) g.rate;
                const float a = lines[0].readAt (pos);
                const float b = lines[1].readAt (pos);
                const float src = a + (b - a) * g.source;
                wetL += src * w * g.panL;
                wetR += src * w * g.panR;

                g.age += 1.0f;
                if (g.age >= g.length) g.active = false;
            }

            wetL *= kVoiceScale;
            wetR *= kVoiceScale;
            feedbackL = wetL;
            feedbackR = wetR;

            l[i] += m * (wetL - l[i]);
            r[i] += m * (wetR - r[i]);
        }
    }

private:
    struct Grain
    {
        bool  active = false;
        float age = 0.0f, length = 1.0f, rate = 1.0f;
        float panL = 0.7f, panR = 0.7f, source = 0.5f;
        double offset = 0.0;
    };

    inline float windowAt (float phase) const noexcept
    {
        const float p = clampf (phase, 0.0f, 1.0f) * (float) (window.size() - 1);
        const int i = (int) p;
        const float frac = p - (float) i;
        const float a = window[(size_t) i];
        const float b = window[(size_t) std::min (i + 1, (int) window.size() - 1)];
        return a + frac * (b - a);
    }

    void spawn() noexcept
    {
        for (auto& g : grains)
        {
            if (g.active) continue;

            const float detune = 1.0f + 0.004f * rng.nextBipolar();
            g.rate = clampf (rate * detune, 0.05f, 8.0f);
            g.length = clampf (grainMs * (1.0f + 0.25f * rng.nextBipolar()) * 0.001f * (float) sr,
                               32.0f, (float) bufferSize * 0.2f);
            g.age = 0.0f;

            // Stay behind the write head for the whole grain, even when pitching up.
            const float catchUp = g.length * std::max (0.0f, g.rate - 1.0f);
            const float base = (0.06f + 0.30f * rng.nextFloat() * spread) * (float) sr;
            g.offset = (double) clampf (base + catchUp + 256.0f, 256.0f, (float) bufferSize * 0.9f);

            const float pan = 0.5f + 0.5f * rng.nextBipolar() * clampf (0.35f + 0.65f * spread * 0.5f, 0.0f, 1.0f);
            const float angle = clampf (pan, 0.0f, 1.0f) * (float) kPi * 0.5f;
            g.panL = std::cos (angle) * 1.35f;
            g.panR = std::sin (angle) * 1.35f;
            g.source = rng.nextFloat();
            g.active = true;
            return;
        }
    }

    static constexpr float kVoiceScale = 0.55f;

    double sr = 48000.0;
    int bufferSize = 8;
    DelayLine lines[2];
    std::array<float, 1025> window {};
    std::array<Grain, kMaxGrains> grains {};
    Rng rng { 0x6D4145u };
    SmoothParam mix, regen;
    float grainMs = 120.0f, density = 0.5f, rate = 1.0f, spread = 1.0f;
    float feedbackL = 0.0f, feedbackR = 0.0f;
    int countdown = 0;
    bool clamped = false;
};

} // namespace am::fx

#pragma once

#include "FXCommon.h"

namespace am::fx
{

/**
    CHORUS — three modulated delay voices per channel.

    Voices sit at 9 / 15 / 23 ms and are swept by sine LFOs that are spread
    both across voices (0, 1/3, 2/3 of a cycle) and across the stereo field
    (right channel a half cycle behind), which gives a wide, slowly breathing
    image rather than a narrow flanger. The SIZE macro stretches the base
    delays and the modulation depth together, so a big Space also has a
    slower, deeper shimmer.
*/
class Chorus
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        const int maxSamples = (int) (sampleRate * 0.12) + 8;
        for (auto& line : lines) line.prepare (maxSamples);

        rate.prepare (sampleRate, 40.0f);
        depth.prepare (sampleRate, 30.0f);
        mix.prepare (sampleRate, 20.0f);
        size.prepare (sampleRate, 60.0f);

        for (int c = 0; c < 2; ++c)
            for (int v = 0; v < kVoices; ++v)
            {
                auto& lfo = lfos[(size_t) (c * kVoices + v)];
                lfo.prepare (sampleRate);
                lfo.reset ((float) v / (float) kVoices + (c == 1 ? 0.5f : 0.0f));
            }
        reset();
    }

    void reset()
    {
        for (auto& line : lines) line.clear();
        for (auto& f : damp) { f.prepare (sr); f.setCutoff (9000.0f); }
        // The voice LFOs are what make the chorus a chorus, and their phases are state: carried
        // across a reset they modulate the first moments of the next patch from wherever the last
        // one left them. Put them back on the spread they are prepared with.
        for (int c = 0; c < 2; ++c)
            for (int v = 0; v < kVoices; ++v)
                lfos[(size_t) (c * kVoices + v)].reset ((float) v / (float) kVoices + (c == 1 ? 0.5f : 0.0f));
    }

    void setParams (float rateHz, float depth01, float mix01, float sizeScale) noexcept
    {
        rate.setTarget (clampf (rateHz, 0.01f, 10.0f));
        depth.setTarget (clampf (depth01, 0.0f, 1.0f));
        mix.setTarget (clampf (mix01, 0.0f, 1.0f));
        size.setTarget (clampf (sizeScale, 0.35f, 2.5f));
    }

    void process (float* l, float* r, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const float rateHz = rate.next();
            const float d = depth.next();
            const float m = mix.next();
            const float scale = size.next();

            const float in[2] { l[i], r[i] };
            float out[2] { 0.0f, 0.0f };

            for (int c = 0; c < 2; ++c)
            {
                lines[(size_t) c].write (in[c] + 0.12f * crossFeed[(size_t) c]);
                float sum = 0.0f;
                for (int v = 0; v < kVoices; ++v)
                {
                    auto& lfo = lfos[(size_t) (c * kVoices + v)];
                    lfo.setRate (rateHz * kVoiceRate[v]);
                    const float mod = lfo.next();
                    const float base = kBaseMs[v] * scale;
                    const float sweep = kDepthMs[v] * d * scale;
                    const float samples = (base + sweep * mod) * 0.001f * (float) sr;
                    sum += lines[(size_t) c].readHermite (samples);
                }
                sum *= 1.0f / (float) kVoices;
                sum = damp[(size_t) c].lp (sum);
                crossFeed[(size_t) c] = sum;
                out[c] = in[c] + m * (sum - in[c]);
            }

            l[i] = out[0];
            r[i] = out[1];
        }
    }

private:
    static constexpr int kVoices = 3;
    static constexpr float kBaseMs[kVoices]  { 9.0f, 15.0f, 23.0f };
    static constexpr float kDepthMs[kVoices] { 3.5f, 5.0f, 7.0f };
    static constexpr float kVoiceRate[kVoices] { 1.0f, 0.77f, 1.31f };

    double sr = 48000.0;
    DelayLine lines[2];
    LFO lfos[2 * kVoices];
    OnePoleTPT damp[2];
    SmoothParam rate, depth, mix, size;
    float crossFeed[2] {};
};

} // namespace am::fx

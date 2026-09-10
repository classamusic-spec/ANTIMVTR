#pragma once

#include "FXCommon.h"

namespace am::fx
{

/**
    DELAY — stereo, tempo-syncable, with a tone filter inside the loop.

    Feedback is cross-coupled (L feeds R and R feeds L), so repeats bounce
    across the image instead of piling up in one channel; the first repeat
    still lands exactly on the delay time in both channels, which keeps
    synced patterns readable. The loop filter is a low-pass and a high-pass
    driven from one TONE control: dark repeats fall away into the mud, bright
    repeats thin out like tape.

    Feedback is hard-clamped below unity — `feedbackClamped()` tells the rack
    when the request had to be limited so it can raise a safety event.
*/
class Delay
{
public:
    static constexpr int kNumDivisions = 13;
    static constexpr float kMaxSeconds = 2.0f;
    static constexpr float kMaxFeedback = 0.94f;

    /** Musical divisions in beats (quarter note = 1 beat). */
    static float divisionBeats (int index) noexcept
    {
        static constexpr float beats[kNumDivisions]
        {
            0.125f,      // 1/32
            0.166667f,   // 1/16T
            0.25f,       // 1/16
            0.333333f,   // 1/8T
            0.375f,      // 1/16D
            0.5f,        // 1/8
            0.666667f,   // 1/4T
            0.75f,       // 1/8D
            1.0f,        // 1/4
            1.333333f,   // 1/2T
            1.5f,        // 1/4D
            2.0f,        // 1/2
            4.0f         // 1/1
        };
        return beats[juce::jlimit (0, kNumDivisions - 1, index)];
    }

    static int divisionIndex (float time01) noexcept
    {
        return juce::jlimit (0, kNumDivisions - 1, (int) std::lround (clampf (time01, 0.0f, 1.0f) * (kNumDivisions - 1)));
    }

    /** The delay time the module will use, in seconds — the tests call this too. */
    static float timeSeconds (float time01, bool sync, double bpm, float sizeScale) noexcept
    {
        float seconds;
        if (sync)
        {
            const double safeBpm = juce::jlimit (20.0, 300.0, bpm > 1.0 ? bpm : 120.0);
            seconds = (float) (divisionBeats (divisionIndex (time01)) * 60.0 / safeBpm);
        }
        else
        {
            seconds = expMap (time01, 0.01f, 1.6f);
        }
        return clampf (seconds * sizeScale, 0.002f, kMaxSeconds);
    }

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        const int maxSamples = (int) (sampleRate * (double) kMaxSeconds) + 64;
        for (auto& line : lines) line.prepare (maxSamples);

        for (int c = 0; c < 2; ++c)
        {
            loopLow[c].prepare (sampleRate);
            loopHigh[c].prepare (sampleRate);
        }
        timeL.prepare (sampleRate, 120.0f);
        timeR.prepare (sampleRate, 120.0f);
        feedback.prepare (sampleRate, 30.0f);
        mix.prepare (sampleRate, 20.0f);
        reset();
    }

    void reset()
    {
        for (auto& line : lines) line.clear();
        for (int c = 0; c < 2; ++c) { loopLow[c].reset(); loopHigh[c].reset(); }
        clamped = false;
    }

    void setParams (float time01, bool sync, double bpm, float feedback01, float tone01,
                    float mix01, float sizeScale, bool snapTime) noexcept
    {
        const float seconds = timeSeconds (time01, sync, bpm, sizeScale);
        const float samples = seconds * (float) sr;
        // A touch of stereo offset keeps wide sources from collapsing.
        timeL.setTarget (samples);
        timeR.setTarget (samples * 1.0f);
        if (snapTime) { timeL.reset (samples); timeR.reset (samples); }

        const float requested = clampf (feedback01, 0.0f, 4.0f);
        clamped = requested > kMaxFeedback;
        feedback.setTarget (std::min (requested, kMaxFeedback));
        mix.setTarget (clampf (mix01, 0.0f, 1.0f));

        const float t = clampf (tone01, 0.0f, 1.0f);
        for (int c = 0; c < 2; ++c)
        {
            loopLow[c].setCutoff (expMap (t, 600.0f, 17000.0f));
            loopHigh[c].setCutoff (expMap (t, 25.0f, 420.0f));
        }
    }

    bool feedbackClamped() const noexcept { return clamped; }

    void process (float* l, float* r, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const float dl = timeL.next();
            const float dr = timeR.next();
            const float fb = feedback.next();
            const float m = mix.next();

            const float tapL = lines[0].readHermite (dl);
            const float tapR = lines[1].readHermite (dr);

            float fL = loopHigh[0].hp (loopLow[0].lp (tapL));
            float fR = loopHigh[1].hp (loopLow[1].lp (tapR));
            // Headroom for the pile-up a sustained note creates, then a soft ceiling.
            fL = softLimit (fL, 0.9f, 1.6f);
            fR = softLimit (fR, 0.9f, 1.6f);

            // Partial input normalisation: raising FEEDBACK lengthens the repeats
            // instead of just piling energy up until something has to squash it.
            const float writeGain = 1.0f - 0.35f * fb;
            lines[0].write (l[i] * writeGain + fb * fR);
            lines[1].write (r[i] * writeGain + fb * fL);

            l[i] += m * (tapL - l[i]);
            r[i] += m * (tapR - r[i]);
        }
    }

private:
    double sr = 48000.0;
    DelayLine lines[2];
    OnePoleTPT loopLow[2], loopHigh[2];
    SmoothParam timeL, timeR, feedback, mix;
    bool clamped = false;
};

} // namespace am::fx

#pragma once

#include "FXCommon.h"

namespace am::fx
{

/**
    Three-band EQ for the wet path: low shelf (180 Hz), mid peak (1 kHz,
    moderate Q) and high shelf (4.5 kHz), all in dB. Gains are smoothed at
    block rate and the coefficients are only rebuilt when something actually
    moved, so automating the EQ costs nothing and never zips.
*/
class EQ
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        low.prepare (sampleRate, 40.0f);
        mid.prepare (sampleRate, 40.0f);
        high.prepare (sampleRate, 40.0f);
        low.reset (0.0f); mid.reset (0.0f); high.reset (0.0f);
        cached[0] = cached[1] = cached[2] = 1.0e9f;
        reset();
    }

    void reset()
    {
        for (auto& f : lowShelf) f.reset();
        for (auto& f : midPeak) f.reset();
        for (auto& f : highShelf) f.reset();
    }

    void setParams (float lowDb, float midDb, float highDb) noexcept
    {
        low.setTarget (clampf (lowDb, -24.0f, 24.0f));
        mid.setTarget (clampf (midDb, -24.0f, 24.0f));
        high.setTarget (clampf (highDb, -24.0f, 24.0f));
    }

    bool isNeutral() const noexcept
    {
        return std::abs (low.target()) < 0.01f && std::abs (mid.target()) < 0.01f && std::abs (high.target()) < 0.01f
            && std::abs (low.current()) < 0.01f && std::abs (mid.current()) < 0.01f && std::abs (high.current()) < 0.01f;
    }

    void process (float* l, float* r, int n) noexcept
    {
        low.skip (n); mid.skip (n); high.skip (n);
        updateCoefficients();

        for (int c = 0; c < 2; ++c)
        {
            float* buf = c == 0 ? l : r;
            for (int i = 0; i < n; ++i)
                buf[i] = highShelf[(size_t) c].process (midPeak[(size_t) c].process (lowShelf[(size_t) c].process (buf[i])));
        }
    }

private:
    void updateCoefficients() noexcept
    {
        const float values[3] { low.current(), mid.current(), high.current() };
        if (std::abs (values[0] - cached[0]) < 0.005f
            && std::abs (values[1] - cached[1]) < 0.005f
            && std::abs (values[2] - cached[2]) < 0.005f)
            return;

        for (int i = 0; i < 3; ++i) cached[i] = values[i];
        for (int c = 0; c < 2; ++c)
        {
            lowShelf[(size_t) c].setLowShelf (sr, 180.0f, 0.72f, values[0]);
            midPeak[(size_t) c].setPeak (sr, 1000.0f, 0.85f, values[1]);
            highShelf[(size_t) c].setHighShelf (sr, 4500.0f, 0.72f, values[2]);
        }
    }

    double sr = 48000.0;
    SmoothParam low, mid, high;
    Biquad lowShelf[2], midPeak[2], highShelf[2];
    float cached[3] { 1.0e9f, 1.0e9f, 1.0e9f };
};

} // namespace am::fx

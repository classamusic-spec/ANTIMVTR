#pragma once

#include "FXCommon.h"

namespace am::fx
{

/**
    FREQUENCY SHIFTER — single sideband (Bode style), not a pitch shifter.

    A pair of 4-section allpass chains forms an approximate Hilbert transform
    (their outputs stay ~90 degrees apart from roughly 20 Hz to 20 kHz); the
    analytic signal is then multiplied with a quadrature oscillator, which
    slides the whole spectrum up or down by a fixed number of Hz. Harmonic
    relationships break, which is exactly what makes MACHINE metallic and
    NEBULA slightly alien.

    `shiftHz()` is the public control mapping — the tests use it to predict
    where a sine will land.
*/
class FrequencyShifter
{
public:
    FrequencyShifter() { configure(); }

    static constexpr float kMaxShiftHz = 500.0f;

    /** amount -1..1 → Hz. Squared curve: fine control around zero. */
    static float shiftHz (float amount) noexcept
    {
        const float a = clampf (amount, -1.0f, 1.0f);
        return (a < 0.0f ? -1.0f : 1.0f) * a * a * kMaxShiftHz;
    }

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        shift.prepare (sampleRate, 30.0f);
        mix.prepare (sampleRate, 20.0f);
        reset();
    }

    void reset()
    {
        for (auto& c : chains) c.reset();
        phase = 0.0;
        for (auto& d : delayed) d = 0.0f;
    }

    void setParams (float amount, float mix01) noexcept
    {
        shift.setTarget (shiftHz (amount));
        mix.setTarget (clampf (mix01, 0.0f, 1.0f));
    }

    void process (float* l, float* r, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const float hz = shift.next();
            const float m = mix.next();

            phase += (double) hz / sr;
            phase -= std::floor (phase);
            const float cosine = fastSin01 ((float) phase + 0.25f);
            const float sine   = fastSin01 ((float) phase);

            for (int c = 0; c < 2; ++c)
            {
                float* buf = c == 0 ? l : r;
                const float x = buf[i];

                float a = chains[(size_t) (c * 2)].process (x);        // in-phase branch
                const float b = chains[(size_t) (c * 2 + 1)].process (x);  // quadrature branch
                const float inPhase = delayed[(size_t) c];             // branch A needs one sample of delay
                delayed[(size_t) c] = a;
                a = inPhase;

                const float wet = a * cosine + b * sine;
                buf[i] = x + m * (wet - x);
            }
        }
    }

private:
    /** Four cascaded second-order allpass sections: y[n] = k*(x[n] + y[n-2]) - x[n-2]. */
    struct AllpassChain
    {
        void set (const float* coefficients) noexcept
        {
            for (int i = 0; i < 4; ++i) k[i] = coefficients[i];
        }

        void reset() noexcept
        {
            for (auto& v : x1) v = 0.0f;
            for (auto& v : x2) v = 0.0f;
            for (auto& v : y1) v = 0.0f;
            for (auto& v : y2) v = 0.0f;
        }

        inline float process (float in) noexcept
        {
            for (int i = 0; i < 4; ++i)
            {
                const float out = k[i] * (in + y2[i]) - x2[i];
                x2[i] = x1[i]; x1[i] = in;
                y2[i] = y1[i]; y1[i] = sanitise (out);
                in = out;
            }
            return in;
        }

        float k[4] { 0.0f, 0.0f, 0.0f, 0.0f };
        float x1[4] {}, x2[4] {}, y1[4] {}, y2[4] {};
    };

    // Classic wide-band Hilbert pair coefficients (squared pole radii).
    static constexpr float kBranchA[4] { 0.6923878f,     0.9360654322959f, 0.9882295226860f, 0.9987488452737f };
    static constexpr float kBranchB[4] { 0.4021921162426f, 0.8561710882420f, 0.9722909545651f, 0.9952884791278f };

    void configure() noexcept
    {
        for (int c = 0; c < 2; ++c)
        {
            chains[(size_t) (c * 2)].set (kBranchA);
            chains[(size_t) (c * 2 + 1)].set (kBranchB);
        }
    }

    double sr = 48000.0, phase = 0.0;
    AllpassChain chains[4];
    float delayed[2] {};
    SmoothParam shift, mix;
};

} // namespace am::fx

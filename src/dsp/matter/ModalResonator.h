#pragma once

#include "core/Types.h"
#include <array>

namespace am
{

/**
    sin/cos for w in [0, pi] (quadrant reduction + Taylor, max error ~2e-7).
    Used for the per-block coefficient update of every node.
*/
inline void fastSinCos (float w, float& sn, float& cs) noexcept
{
    const int   k  = (int) (w * 0.636619772f + 0.5f);   // nearest multiple of pi/2
    const float x  = w - (float) k * 1.57079632679f;
    const float x2 = x * x;
    const float sx = x * (1.0f + x2 * (-1.0f / 6.0f + x2 * (1.0f / 120.0f + x2 * (-1.0f / 5040.0f + x2 * (1.0f / 362880.0f)))));
    const float cx = 1.0f + x2 * (-0.5f + x2 * (1.0f / 24.0f + x2 * (-1.0f / 720.0f + x2 * (1.0f / 40320.0f))));
    switch (k & 3)
    {
        case 0:  sn = sx;  cs = cx;  break;
        case 1:  sn = cx;  cs = -sx; break;
        case 2:  sn = -sx; cs = -cx; break;
        default: sn = -cx; cs = sx;  break;
    }
}

/**
    MODAL BANK — structure-of-arrays bank of coupled-form resonators.

    Each node is a rotating phasor (x, y):

        x' = c·x − s·y + in        c = r·cos ω,  s = r·sin ω
        y' = s·x + c·y             r = pole radius (1 − damping), ω = 2π f / sr

    The impulse response of x is exactly rⁿ·cos(ωn): frequency and decay are
    accurate by construction, the state magnitude sqrt(x²+y²) is the modal
    amplitude, and changing ω never changes the amplitude — frequency glides
    and Evolve warps are click-free. The form is well conditioned at low
    frequencies where direct-form biquads lose precision.

    Coefficients (c, s) and output gains are ramped in kSub-sample steps
    toward per-block targets. Coupling is an antisymmetric stencil on the x
    states (two strided bands, an optional hub and a few explicit edges),
    scaled so the coupled system is provably contractive (see
    finalizeCoupling). The per-sample loops are lane-structured (kLanes)
    so they vectorise without fast-math.
*/
class ModalBank
{
public:
    static constexpr int kLanes  = 4;
    static constexpr int kPad    = 64;                            ///< zero padding around the states for the coupling stencil
    static constexpr int kSub    = 16;                            ///< coefficient ramp granularity in samples
    static constexpr int kStride = kMaxMatterNodes + 2 * kPad;
    static constexpr int kMaxExtraEdges = 8;

    struct ExtraEdge { int from = 0; int to = 0; float k = 0.0f; };

    ModalBank() { resetAll(); }

    void resetAll() noexcept
    {
        for (auto& v : xp) v = 0.0f;
        for (auto& v : yp) v = 0.0f;
        for (auto& v : cin) v = 0.0f;
        c.fill (0.0f); s.fill (0.0f); gl.fill (0.0f); gr.fill (0.0f);
        cT.fill (0.0f); sT.fill (0.0f); glT.fill (0.0f); grT.fill (0.0f);
        dc.fill (0.0f); ds.fill (0.0f); dgl.fill (0.0f); dgr.fill (0.0f);
        a.fill (0.0f); b.fill (0.0f);
        clearCoupling();
    }

    /** Clears the resonator states only (targets and coefficients are kept). */
    void resetStates() noexcept
    {
        for (auto& v : xp) v = 0.0f;
        for (auto& v : yp) v = 0.0f;
    }

    void setCount (int n) noexcept
    {
        n = std::max (kLanes, std::min (kMaxMatterNodes, n));
        n -= n % kLanes;
        count = n;
    }

    int getCount() const noexcept { return count; }

    // ---- per-block targets (ramped)
    float* targetCos() noexcept   { return cT.data(); }
    float* targetSin() noexcept   { return sT.data(); }
    float* targetGainL() noexcept { return glT.data(); }
    float* targetGainR() noexcept { return grT.data(); }

    // ---- per-block immediate values
    float* inputGain() noexcept   { return a.data(); }   ///< gain of the excitation into each node
    float* strikeGain() noexcept  { return b.data(); }   ///< gain of the strike pulse into each node

    /** Sets the current coefficients/gains to the targets without ramping (note start, reactivation). */
    void snapToTargets() noexcept
    {
        c = cT; s = sT; gl = glT; gr = grT;
    }

    // ---- state access
    float x (int i) const noexcept { return xp[(size_t) (kPad + i)]; }
    float y (int i) const noexcept { return yp[(size_t) (kPad + i)]; }
    void  setState (int i, float xv, float yv) noexcept { xp[(size_t) (kPad + i)] = xv; yp[(size_t) (kPad + i)] = yv; }

    /** Modal amplitudes sqrt(x²+y²) for every node. */
    void amplitudes (float* dest) const noexcept
    {
        const float* __restrict x0 = xp.data() + kPad;
        const float* __restrict y0 = yp.data() + kPad;
        for (int i = 0; i < count; ++i)
            dest[i] = std::sqrt (x0[i] * x0[i] + y0[i] * y0[i]);
    }

    /** Sum of squared state magnitudes (fast finiteness probe). */
    float stateEnergy() const noexcept
    {
        const float* __restrict x0 = xp.data() + kPad;
        const float* __restrict y0 = yp.data() + kPad;
        float e = 0.0f;
        for (int i = 0; i < count; ++i) e += x0[i] * x0[i] + y0[i] * y0[i];
        return e;
    }

    /** Resets every node whose state is non-finite. Returns how many were reset. */
    int scrubNonFinite() noexcept
    {
        int bad = 0;
        float* __restrict x0 = xp.data() + kPad;
        float* __restrict y0 = yp.data() + kPad;
        for (int i = 0; i < count; ++i)
        {
            if (! std::isfinite (x0[i]) || ! std::isfinite (y0[i]))
            {
                x0[i] = 0.0f; y0[i] = 0.0f; ++bad;
            }
        }
        return bad;
    }

    /** Scales down any node whose amplitude exceeds `limit`. Returns how many. */
    int clampRunaway (float limit) noexcept
    {
        int n = 0;
        float* __restrict x0 = xp.data() + kPad;
        float* __restrict y0 = yp.data() + kPad;
        const float limit2 = limit * limit;
        for (int i = 0; i < count; ++i)
        {
            const float e = x0[i] * x0[i] + y0[i] * y0[i];
            if (e > limit2)
            {
                const float g = limit * 0.5f / std::sqrt (e);
                x0[i] *= g; y0[i] *= g; ++n;
            }
        }
        return n;
    }

    /** Zeroes nodes whose amplitude fell below `threshold` (denormal prevention). */
    void flushQuiet (float threshold) noexcept
    {
        float* __restrict x0 = xp.data() + kPad;
        float* __restrict y0 = yp.data() + kPad;
        const float t2 = threshold * threshold;
        for (int i = 0; i < count; ++i)
        {
            const float e = x0[i] * x0[i] + y0[i] * y0[i];
            if (e < t2) { x0[i] = 0.0f; y0[i] = 0.0f; }
        }
    }

    // ---- coupling
    void clearCoupling() noexcept
    {
        for (auto& v : kAp) v = 0.0f;
        for (auto& v : kBp) v = 0.0f;
        hubK.fill (0.0f);
        strideA = 0; strideB = 0; hub = -1; numExtra = 0;
        couplingActive = false; bandBActive = false; hubActive = false;
        couplingScale = 1.0f;
    }

    /** Edge strengths for band A: edge (i, i + stride) has strength kA[i]. Stride 0 disables. */
    void setBandA (int stride, const float* k) noexcept { setBand (kAp, strideA, stride, k); }
    void setBandB (int stride, const float* k) noexcept { setBand (kBp, strideB, stride, k); }

    /** Hub (star) coupling: edge (hub, i) with strength k[i]; k[hub] is ignored. */
    void setHub (int hubIndex, const float* k) noexcept
    {
        hub = hubIndex;
        for (int i = 0; i < count; ++i) hubK[(size_t) i] = (i == hubIndex || k == nullptr) ? 0.0f : k[i];
    }

    void setExtraEdges (const ExtraEdge* edges, int n) noexcept
    {
        numExtra = std::min (n, kMaxExtraEdges);
        for (int i = 0; i < numExtra; ++i) extra[(size_t) i] = edges[i];
    }

    /**
        Applies the stability bound. With D = per-node damping/rotation
        (‖D‖ = rMax) and K the antisymmetric coupling matrix, the coupled
        update is D·(I + K) and ‖I + K‖₂ = sqrt(1 + σ²) where σ ≤ max row
        sum of |K|. The system is contractive when rMax·sqrt(1 + σ²) < 1,
        i.e. σ < sqrt(1/rMax² − 1). Strengths are scaled so that σ stays at
        70% of that bound: energy can never grow through coupling.
        Returns the scale that was applied (1 = unconstrained).
    */
    float finalizeCoupling (float rMax) noexcept
    {
        float rowMax = 0.0f;
        hubActive = hub >= 0 && hub < count;
        bandBActive = strideB > 0;
        float hubRow = 0.0f;
        for (int i = 0; i < count; ++i)
        {
            float row = 0.0f;
            if (strideA > 0) row += std::abs (kAp[(size_t) (kPad + i)]) + std::abs (kAp[(size_t) (kPad + i - strideA)]);
            if (strideB > 0) row += std::abs (kBp[(size_t) (kPad + i)]) + std::abs (kBp[(size_t) (kPad + i - strideB)]);
            if (hubActive) { row += std::abs (hubK[(size_t) i]); hubRow += std::abs (hubK[(size_t) i]); }
            rowMax = std::max (rowMax, row);
        }
        for (int e = 0; e < numExtra; ++e) rowMax = std::max (rowMax, std::abs (extra[(size_t) e].k) * 2.0f);
        rowMax = std::max (rowMax, hubRow);

        couplingActive = rowMax > 0.0f;
        if (! couplingActive) { couplingScale = 1.0f; return 1.0f; }

        rMax = std::min (0.9999999f, std::max (0.0f, rMax));
        const float bound = 0.7f * std::sqrt (std::max (0.0f, 1.0f / (rMax * rMax) - 1.0f));
        couplingScale = rowMax > bound ? bound / rowMax : 1.0f;
        if (couplingScale < 1.0f)
        {
            for (auto& v : kAp) v *= couplingScale;
            for (auto& v : kBp) v *= couplingScale;
            for (auto& v : hubK) v *= couplingScale;
            for (int e = 0; e < numExtra; ++e) extra[(size_t) e].k *= couplingScale;
        }
        return couplingScale;
    }

    bool isCouplingActive() const noexcept { return couplingActive; }
    float lastCouplingScale() const noexcept { return couplingScale; }

    /**
        Renders n samples. `exc` is the (mono) excitation, `strike` the strike
        pulse (may be null), output is written (overwrite) to outL/outR.
    */
    void process (const float* exc, const float* strike, float* outL, float* outR, int n) noexcept
    {
        if (n <= 0) return;
        const int numSub = (n + kSub - 1) / kSub;
        const float inv = 1.0f / (float) numSub;
        const int N = count;

        // Per-sub-block deltas toward the targets.
        for (int i = 0; i < N; ++i)
        {
            dc[(size_t) i]  = (cT[(size_t) i]  - c[(size_t) i])  * inv;
            ds[(size_t) i]  = (sT[(size_t) i]  - s[(size_t) i])  * inv;
            dgl[(size_t) i] = (glT[(size_t) i] - gl[(size_t) i]) * inv;
            dgr[(size_t) i] = (grT[(size_t) i] - gr[(size_t) i]) * inv;
        }

        for (int start = 0; start < n; start += kSub)
        {
            const int len = std::min (kSub, n - start);
            for (int i = 0; i < N; ++i)
            {
                c[(size_t) i]  += dc[(size_t) i];
                s[(size_t) i]  += ds[(size_t) i];
                gl[(size_t) i] += dgl[(size_t) i];
                gr[(size_t) i] += dgr[(size_t) i];
            }
            if (couplingActive)
                renderSamples<true> (exc + start, strike != nullptr ? strike + start : nullptr, outL + start, outR + start, len);
            else
                renderSamples<false> (exc + start, strike != nullptr ? strike + start : nullptr, outL + start, outR + start, len);
        }

        // Land exactly on the targets (removes accumulated rounding).
        for (int i = 0; i < N; ++i)
        {
            c[(size_t) i] = cT[(size_t) i]; s[(size_t) i] = sT[(size_t) i];
            gl[(size_t) i] = glT[(size_t) i]; gr[(size_t) i] = grT[(size_t) i];
        }
    }

private:
    template <bool Coupled>
    void renderSamples (const float* __restrict exc, const float* __restrict strike,
                        float* __restrict outL, float* __restrict outR, int len) noexcept
    {
        float* __restrict x0 = xp.data() + kPad;
        float* __restrict y0 = yp.data() + kPad;
        const float* __restrict cc = c.data();
        const float* __restrict ss = s.data();
        const float* __restrict aa = a.data();
        const float* __restrict bb = b.data();
        const float* __restrict gL = gl.data();
        const float* __restrict gR = gr.data();
        float* __restrict ci = cin.data() + kPad;
        const float* __restrict kA = kAp.data() + kPad;
        const float* __restrict kB = kBp.data() + kPad;
        const float* __restrict hk = hubK.data();
        const int N = count;
        const int sA = strideA, sB = strideB;

        for (int t = 0; t < len; ++t)
        {
            const float u  = exc[t];
            const float st = strike != nullptr ? strike[t] : 0.0f;

            if constexpr (Coupled)
            {
                // Antisymmetric stencil on the previous states: node i receives
                // k·x from the higher end of each edge and −k·x from the lower end.
                const float xh = hubActive ? x0[hub] : 0.0f;
                float hubAcc[kLanes] = {};
                if (bandBActive)
                {
                    for (int i = 0; i < N; i += kLanes)
                        for (int l = 0; l < kLanes; ++l)
                        {
                            const int j = i + l;
                            ci[j] = kA[j] * x0[j + sA] - kA[j - sA] * x0[j - sA]
                                  + kB[j] * x0[j + sB] - kB[j - sB] * x0[j - sB]
                                  - hk[j] * xh;
                            hubAcc[l] += hk[j] * x0[j];
                        }
                }
                else
                {
                    for (int i = 0; i < N; i += kLanes)
                        for (int l = 0; l < kLanes; ++l)
                        {
                            const int j = i + l;
                            ci[j] = kA[j] * x0[j + sA] - kA[j - sA] * x0[j - sA] - hk[j] * xh;
                            hubAcc[l] += hk[j] * x0[j];
                        }
                }
                if (hubActive) ci[hub] += hubAcc[0] + hubAcc[1] + hubAcc[2] + hubAcc[3];
                for (int e = 0; e < numExtra; ++e)
                {
                    const auto& ed = extra[(size_t) e];
                    ci[ed.from] += ed.k * x0[ed.to];
                    ci[ed.to]   -= ed.k * x0[ed.from];
                }
            }

            float accL[kLanes] = {};
            float accR[kLanes] = {};
            for (int i = 0; i < N; i += kLanes)
            {
                for (int l = 0; l < kLanes; ++l)
                {
                    const int j = i + l;
                    const float xi = x0[j], yi = y0[j];
                    float in = u * aa[j] + st * bb[j];
                    if constexpr (Coupled) in += ci[j];
                    const float xn = cc[j] * xi - ss[j] * yi + in;
                    const float yn = ss[j] * xi + cc[j] * yi;
                    x0[j] = xn; y0[j] = yn;
                    accL[l] += xn * gL[j];
                    accR[l] += xn * gR[j];
                }
            }
            outL[t] = (accL[0] + accL[1]) + (accL[2] + accL[3]);
            outR[t] = (accR[0] + accR[1]) + (accR[2] + accR[3]);
        }
    }

    void setBand (std::array<float, kStride>& dest, int& strideVar, int stride, const float* k) noexcept
    {
        for (auto& v : dest) v = 0.0f;
        if (stride <= 0 || stride >= kPad || k == nullptr) { strideVar = 0; return; }
        strideVar = stride;
        for (int i = 0; i + stride < count; ++i) dest[(size_t) (kPad + i)] = k[i];
    }

    alignas (32) std::array<float, kStride> xp {}, yp {}, cin {}, kAp {}, kBp {};
    alignas (32) std::array<float, kMaxMatterNodes> c {}, s {}, gl {}, gr {};
    alignas (32) std::array<float, kMaxMatterNodes> cT {}, sT {}, glT {}, grT {};
    alignas (32) std::array<float, kMaxMatterNodes> dc {}, ds {}, dgl {}, dgr {};
    alignas (32) std::array<float, kMaxMatterNodes> a {}, b {}, hubK {};
    std::array<ExtraEdge, kMaxExtraEdges> extra {};

    int count = kLanes;
    int strideA = 0, strideB = 0, hub = -1, numExtra = 0;
    bool couplingActive = false, bandBActive = false, hubActive = false;
    float couplingScale = 1.0f;
};

} // namespace am

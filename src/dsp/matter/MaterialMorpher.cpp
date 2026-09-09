#include "MaterialMorpher.h"
#include "core/FastMath.h"

namespace am
{

namespace
{
    /** Van der Corput sequence (base 2) — evenly spread ranks for the Distribution control. */
    float vanDerCorput (uint32_t i) noexcept
    {
        uint32_t r = 0;
        for (int b = 0; b < 16; ++b) r = (r << 1) | ((i >> b) & 1u);
        return (float) r / 65536.0f;
    }

    inline float smoothstep01 (float t) noexcept
    {
        t = clamp01 (t);
        return t * t * (3.0f - 2.0f * t);
    }
}

void MaterialMorpher::rebuild (uint32_t seed, MaterialType a, MaterialType b, int nodeCount) noexcept
{
    curSeed = seed; matAType = a; matBType = b; count = nodeCount; built = true;
    profA = materialProfile (a);
    profB = materialProfile (b);
    buildStochasticStructure (seed, stochastic);
    buildMaterialStructure (a, seed, matA);
    buildMaterialStructure (b, seed + 0x51u, matB);

    Rng rng (hashSeed (seed, 0xA0C3E5u));
    for (int i = 0; i < kMaxMatterNodes; ++i)
    {
        jitter[(size_t) i]     = rng.nextBipolar();
        panRnd[(size_t) i]     = rng.nextBipolar();
        wobblePhase[(size_t) i] = rng.nextFloat();
        wobbleRate[(size_t) i]  = 0.5f + rng.nextFloat();
        ripple[(size_t) i]     = rng.nextFloat();
        rankSpread[(size_t) i] = vanDerCorput ((uint32_t) i) * (float) count;
    }
    jitter[0] = 0.0f; panRnd[0] = 0.0f; rankSpread[0] = 0.0f;
}

void MaterialMorpher::computeTargets (const Input& in, MatterNode* nodes) noexcept
{
    const auto& lib = StructureLibrary::get();
    blended = blendProfiles (profA, profB, in.blend);
    const MaterialProfile& P = blended;

    // ---- FORM: position between two anchors, with a plateau at each anchor.
    const float formPos = clamp01 (in.form) * (float) ((int) FormAnchor::Count - 1);
    int k = std::min ((int) FormAnchor::Count - 2, (int) formPos);
    const float tRaw = formPos - (float) k;
    const float t = smoothstep01 (tRaw);
    const StructureTable* tabs[(int) FormAnchor::Count];
    for (int i = 0; i < (int) FormAnchor::Count - 1; ++i) tabs[i] = &lib.anchor ((FormAnchor) i);
    tabs[(int) FormAnchor::Stochastic] = &stochastic;
    const StructureTable& T0 = *tabs[k];
    const StructureTable& T1 = *tabs[k + 1];

    // ---- global quantities
    const float mass    = clamp01 (in.mass);
    const float tension = clamp01 (in.tension);
    const float density = clamp01 (in.density);
    const float surface = clamp01 (in.surface);
    const float distr   = clamp01 (in.distribution);
    const float excite  = clamp01 (in.excite);
    const float kt      = clamp01 (in.keytrack);
    const double fNote  = std::max ((double) kMinFrequencyHz, in.noteFrequency);
    const float fBase   = kt >= 0.999f ? (float) fNote
                                       : (float) (std::pow (fNote, (double) kt) * std::pow ((double) kReferenceHz, 1.0 - (double) kt));
    const float pitchMul = pow2 (in.pitchSemis / 12.0f);
    const float fMax    = kMaxFrequencyRatio * (float) in.sampleRate;
    const float sr      = (float) in.sampleRate;

    const float gamma    = pow2 ((tension - 0.5f) * 0.7f);                 // interval expansion / compression
    const float stretchB = tension > 0.5f ? (tension - 0.5f) * 0.004f : 0.0f;
    const float t60Base  = 0.05f * pow2 (clamp01 (in.decay) * 8.6f) * P.t60Scale * pow2 ((mass - 0.4f) * 1.0f);
    const float slope    = P.dampingSlope + (mass - 0.4f) * 0.8f - (tension - 0.5f) * 0.4f;
    const float tilt     = P.weightSlope + (mass - 0.4f) * 1.2f - (distr - 0.5f) * 0.8f;
    const float excTilt  = (1.0f - excite) * 1.6f - P.exciteTilt * 0.5f + (mass - 0.4f) * 0.6f;
    const float nl       = surface * P.nonlinearity;
    const float detune   = surface * P.detune;
    const float pan      = clamp01 (in.stereo) * P.stereoWidth;
    const int   N        = count;
    const int   modal    = N - kBodyNodes;
    const float activeCount = 1.0f + (float) (modal - 1) * std::pow (density, 1.4f);
    const float wobbleAdv = P.wobbleRate * in.blockSeconds;

    float sumW2 = 0.0f;
    lastT60 = 0.0f;

    for (int i = 0; i < N; ++i)
    {
        MatterNode& n = nodes[i];
        float lr, w, t60, e, p;

        if (i >= modal)
        {
            // Body modes: sub-fundamental thump that grows with MASS.
            lr  = i == modal ? -1.0f : -0.5f;
            w   = P.bodyWeight * std::max (0.0f, mass - 0.25f) * 1.4f * (i == modal ? 1.0f : 0.6f);
            t60 = t60Base * 0.5f;
            e   = 1.0f;
            p   = 0.0f;
        }
        else
        {
            const float l0 = T0.log2Ratio[(size_t) i], l1 = T1.log2Ratio[(size_t) i];
            lr = l0 + (l1 - l0) * t;
            const float w0 = T0.weight[(size_t) i], w1 = T1.weight[(size_t) i];
            w = w0 + (w1 - w0) * t;

            // Material pull toward its own structure (A→B blended in the log domain).
            const float ml = matA.log2Ratio[(size_t) i] + (matB.log2Ratio[(size_t) i] - matA.log2Ratio[(size_t) i]) * in.blend;
            const float mw = matA.weight[(size_t) i] + (matB.weight[(size_t) i] - matA.weight[(size_t) i]) * in.blend;
            lr += (ml - lr) * P.structurePull;
            w  += (mw - w) * P.structurePull;

            // TENSION: expand/compress intervals, stiffness stretch when tight.
            lr *= gamma;
            if (stretchB > 0.0f && lr > 0.0f)
                lr += 0.5f * std::log2 (1.0f + stretchB * pow2 (2.0f * lr));

            // SURFACE micro detune, material wobble.
            lr += detune * jitter[(size_t) i];
            if (P.wobble > 0.0f)
            {
                float& ph = wobblePhase[(size_t) i];
                ph += wobbleAdv * wobbleRate[(size_t) i];
                ph -= std::floor (ph);
                lr += P.wobble * fastSin01 (ph);
            }

            const float lrPos = std::max (0.0f, lr);

            // DENSITY / DISTRIBUTION: which nodes are alive.
            const float fi = (float) i;
            const float rank = distr < 0.5f ? fi + (rankSpread[(size_t) i] - fi) * (distr * 2.0f)
                                            : rankSpread[(size_t) i] + ((i == 0 ? 0.0f : (float) (modal - i)) - rankSpread[(size_t) i]) * (distr * 2.0f - 1.0f);
            const float alive = smoothstep01 ((activeCount - rank) / 6.0f + 0.5f);

            w *= alive * pow2 (-tilt * lrPos) * (1.0f - 0.35f * P.weightRipple * ripple[(size_t) i]);
            t60 = t60Base * pow2 (-slope * lr);
            e   = pow2 (-excTilt * lrPos);

            // Stereo: fundamental centred, pattern by material.
            const float spread = std::min (1.0f, fi / 6.0f);
            switch (P.stereoPattern)
            {
                case StereoPattern::Alternate: p = ((i & 1) != 0 ? 1.0f : -1.0f) * (0.6f + 0.4f * std::abs (panRnd[(size_t) i])); break;
                case StereoPattern::Narrow:    p = 0.5f * panRnd[(size_t) i]; break;
                default:                       p = panRnd[(size_t) i]; break;
            }
            p *= pan * spread;
        }

        // Amplitude-dependent behaviour (block rate): hardening/softening pitch and nonlinear damping.
        float f = fBase * pow2 (lr) * pitchMul;
        if (nl > 0.0f)
        {
            const float amp = std::min (1.0f, n.energy * n.energy * 156.0f);   // ≈ (A / 0.08)²
            f   *= pow2 (P.hardening * nl * 0.12f * amp);
            t60 /= 1.0f + nl * 3.0f * amp;
        }

        const bool ok = std::isfinite (f) && f >= kMinFrequencyHz && f < fMax && w > 1.0e-4f;
        if (! ok) w = 0.0f;
        f = std::min (fMax, std::max (kMinFrequencyHz, std::isfinite (f) ? f : kMinFrequencyHz));
        t60 = std::min (40.0f, std::max (0.004f, t60));
        const float u = std::min (0.5f, 6.9077553f / (t60 * sr));
        const float damping = u * (1.0f - u * (0.5f - u * (1.0f / 6.0f)));   // 1 − e^(−u)

        n.frequency = n.targetFrequency = f;
        n.ratio = pow2 (lr);
        n.weight = std::min (1.5f, w);
        n.damping = damping;
        n.pan = std::max (-1.0f, std::min (1.0f, p));
        n.nonlinearity = nl;
        n.excitation = std::min (2.0f, e);
        n.active = ok;
        sumW2 += n.weight * n.weight;
        if (i == 0) lastT60 = t60;
    }

    lastNorm = 1.0f / std::sqrt (std::max (1.0f, sumW2));
}

} // namespace am

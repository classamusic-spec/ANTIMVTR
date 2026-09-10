#include "MaterialProfile.h"
#include <algorithm>

namespace am
{

namespace
{
    constexpr int kCandidates = 640;

    struct Candidate { float ratio; float weight; };

    /** Sorts candidates by ratio and writes the first kMaxMatterNodes into the table (ratio 1 first). */
    void commit (Candidate* cand, int n, StructureTable& out)
    {
        std::sort (cand, cand + n, [] (const Candidate& a, const Candidate& b) { return a.ratio < b.ratio; });
        const float base = cand[0].ratio;
        for (int i = 0; i < kMaxMatterNodes; ++i)
        {
            const Candidate& c = cand[std::min (i, n - 1)];
            const float r = i < n ? c.ratio / base : cand[n - 1].ratio / base * (float) std::pow (1.05, i - n + 1);
            out.log2Ratio[(size_t) i] = std::log2 (std::max (r, 1.0e-3f));
            out.weight[(size_t) i]    = i < n ? std::clamp (c.weight, 0.0f, 1.0f) : 0.02f;
        }
        out.log2Ratio[0] = 0.0f;
        out.weight[0] = 1.0f;
    }

    void fillStretchedHarmonic (StructureTable& out, float B, float weightExp)
    {
        for (int i = 0; i < kMaxMatterNodes; ++i)
        {
            const float n = (float) (i + 1);
            const float r = n * std::sqrt (1.0f + B * n * n);
            out.log2Ratio[(size_t) i] = std::log2 (r);
            out.weight[(size_t) i]    = std::pow (n, -weightExp);
        }
    }

    /** McMahon's asymptotic expansion for the n-th zero of J_m. */
    double besselZero (int m, int n)
    {
        const double beta = (n + 0.5 * m - 0.25) * kPi;
        const double mu = 4.0 * m * m;
        const double b8 = 8.0 * beta;
        return beta - (mu - 1.0) / b8
                    - 4.0 * (mu - 1.0) * (7.0 * mu - 31.0) / (3.0 * b8 * b8 * b8)
                    - 32.0 * (mu - 1.0) * (83.0 * mu * mu - 982.0 * mu + 3779.0) / (15.0 * std::pow (b8, 5.0));
    }

    void fillMembrane (StructureTable& out)
    {
        Candidate cand[kCandidates];
        int n = 0;
        const double j01 = besselZero (0, 1);
        for (int m = 0; m <= 16 && n < kCandidates; ++m)
            for (int k = 1; k <= 12 && n < kCandidates; ++k)
            {
                const float r = (float) (besselZero (m, k) / j01);
                // A strike near the centre favours axisymmetric (m = 0) modes.
                const float w = std::pow (r, -0.4f) / (1.0f + 0.4f * (float) m);
                cand[n++] = { r, w };
            }
        commit (cand, n, out);
    }

    void fillMetallic (StructureTable& out)
    {
        Candidate cand[kCandidates];
        int n = 0;
        for (int k = 1; k <= 40 && n + 4 <= kCandidates; ++k)
        {
            const float fk = (float) k;
            // free bar family (2k+1)²/9, a stiffer second family, a plate-like third and a sparse fourth
            cand[n++] = { std::pow ((2.0f * fk + 1.0f) / 3.0f, 2.0f), std::pow (fk, -0.25f) };
            cand[n++] = { 1.32f * std::pow (fk, 1.72f), 0.7f * std::pow (fk, -0.35f) };
            cand[n++] = { 2.05f * std::pow (fk, 1.55f) * (1.0f + 0.04f * std::sin (4.3f * fk)), 0.55f * std::pow (fk, -0.4f) };
            cand[n++] = { 3.1f * std::pow (fk, 1.4f), 0.45f * std::pow (fk, -0.5f) };
        }
        commit (cand, n, out);
    }

    void fillInharmonic (StructureTable& out)
    {
        for (int i = 0; i < kMaxMatterNodes; ++i)
        {
            const float n = (float) (i + 1);
            const float r = std::pow (n, 1.28f) * (1.0f + 0.07f * std::sin (1.9f * n + 0.4f));
            out.log2Ratio[(size_t) i] = std::log2 (r);
            out.weight[(size_t) i]    = std::pow (n, -0.5f) * (1.0f + 0.25f * std::sin (2.1f * n));
        }
        out.log2Ratio[0] = 0.0f;
        out.weight[0] = 1.0f;
    }

    void fillCrystalline (StructureTable& out)
    {
        Candidate cand[kCandidates];
        int n = 0;
        const float g2 = 6.0f / std::sqrt (5.0f);   // wine-glass (2,0) mode reference
        for (int k = 2; k <= 60 && n + 3 <= kCandidates; ++k)
        {
            const float fk = (float) k;
            const float g = (fk * (fk * fk - 1.0f) / std::sqrt (fk * fk + 1.0f)) / g2;   // shell mode family
            const float w = std::pow (fk - 1.0f, -0.3f);
            cand[n++] = { g, w };
            cand[n++] = { g * 1.004f, 0.5f * w };                 // near-degenerate twin → slow shimmer
            cand[n++] = { 1.93f * std::pow (g, 0.97f), 0.35f * w }; // second family
        }
        commit (cand, n, out);
    }
}

//==============================================================================
const StructureLibrary& StructureLibrary::get()
{
    static const StructureLibrary lib;
    return lib;
}

StructureLibrary::StructureLibrary()
{
    fillStretchedHarmonic (anchors[(size_t) FormAnchor::Harmonic], 0.0f, 0.6f);
    fillStretchedHarmonic (anchors[(size_t) FormAnchor::Stretched], 0.0015f, 0.65f);
    fillMembrane (anchors[(size_t) FormAnchor::Membrane]);
    fillMetallic (anchors[(size_t) FormAnchor::Metallic]);
    fillInharmonic (anchors[(size_t) FormAnchor::Inharmonic]);
    fillCrystalline (anchors[(size_t) FormAnchor::Crystalline]);
    buildStochasticStructure (7, anchors[(size_t) FormAnchor::Stochastic]);
}

void buildStochasticStructure (uint32_t seed, StructureTable& out) noexcept
{
    // Sorted by construction: cumulative random gaps in the log2 domain
    // spanning ~5.5 octaves, with occasional tight clusters.
    Rng rng (hashSeed (seed, 0x5706A57u));
    const float span = 5.5f;
    const float meanGap = span / (float) kMaxMatterNodes;
    float acc = 0.0f;
    out.log2Ratio[0] = 0.0f;
    out.weight[0] = 1.0f;
    for (int i = 1; i < kMaxMatterNodes; ++i)
    {
        const float u = rng.nextFloat();
        float gap;
        if (u < 0.18f)       gap = meanGap * 0.08f * rng.nextFloat();       // cluster partner
        else if (u < 0.30f)  gap = meanGap * (2.0f + 3.0f * rng.nextFloat()); // hole
        else                 gap = meanGap * (0.4f + 1.4f * rng.nextFloat());
        acc += gap;
        out.log2Ratio[(size_t) i] = acc;
        out.weight[(size_t) i] = (0.25f + 0.75f * rng.nextFloat()) * std::exp2 (-0.45f * acc);
    }
}

void buildMaterialStructure (MaterialType type, uint32_t seed, StructureTable& out) noexcept
{
    const auto& lib = StructureLibrary::get();
    const auto& p = materialProfile (type);
    Rng rng (hashSeed (seed, 0x3A7E51A1u + (uint32_t) type));

    switch (type)
    {
        case MaterialType::String:
        case MaterialType::Wood:
        case MaterialType::Organic:
        case MaterialType::Custom:
            for (int i = 0; i < kMaxMatterNodes; ++i)
            {
                const float n = (float) (i + 1);
                out.log2Ratio[(size_t) i] = std::log2 (n * std::sqrt (1.0f + p.stretch * n * n));
                out.weight[(size_t) i] = std::pow (n, -0.65f);
            }
            if (type == MaterialType::Organic)
                for (int i = 0; i < kMaxMatterNodes; ++i)   // warped, slightly compressed partials
                    out.log2Ratio[(size_t) i] *= 0.96f + 0.06f * std::sin (0.9f * (float) i);
            break;

        case MaterialType::Liquid:
        {
            // Dense, soft-spaced set in the low 3.5 octaves with random clusters.
            float acc = 0.0f;
            const float meanGap = 3.6f / (float) kMaxMatterNodes;
            for (int i = 1; i < kMaxMatterNodes; ++i)
            {
                const float u = rng.nextFloat();
                acc += u < 0.25f ? meanGap * 0.15f * rng.nextFloat() : meanGap * (0.5f + 1.2f * rng.nextFloat());
                out.log2Ratio[(size_t) i] = acc;
                out.weight[(size_t) i] = (0.4f + 0.6f * rng.nextFloat()) * std::exp2 (-0.6f * acc);
            }
            out.log2Ratio[0] = 0.0f;
            break;
        }

        case MaterialType::Void:
        {
            // Very sparse: a sub-octave, then modes roughly ×2.2 apart.
            out.log2Ratio[0] = 0.0f;
            out.log2Ratio[1] = -1.0f;
            for (int i = 2; i < kMaxMatterNodes; ++i)
                out.log2Ratio[(size_t) i] = 1.14f * (float) (i - 1) + 0.2f * rng.nextBipolar();
            for (int i = 0; i < kMaxMatterNodes; ++i)
                out.weight[(size_t) i] = std::exp2 (-0.35f * std::max (0.0f, out.log2Ratio[(size_t) i]));
            out.weight[1] = 0.7f;
            break;
        }

        default:
            out = lib.anchor (p.ownStructure);
            break;
    }

    // Common post-processing: span, jitter, ripple. The fundamental stays put.
    for (int i = 1; i < kMaxMatterNodes; ++i)
    {
        float l = out.log2Ratio[(size_t) i] * p.ratioSpan;
        if (p.ratioJitter > 0.0f) l += p.ratioJitter * rng.nextBipolar();
        out.log2Ratio[(size_t) i] = l;
        if (p.weightRipple > 0.0f)
            out.weight[(size_t) i] *= 1.0f - p.weightRipple * rng.nextFloat();
    }
    out.log2Ratio[0] = 0.0f;
    out.weight[0] = 1.0f;
}

//==============================================================================
namespace
{
    std::array<MaterialProfile, (size_t) MaterialType::Count> makeProfiles()
    {
        std::array<MaterialProfile, (size_t) MaterialType::Count> t {};

        {
            auto& m = t[(size_t) MaterialType::Crystal];
            m.name = "CRYSTAL"; m.ownStructure = FormAnchor::Crystalline; m.structurePull = 0.55f;
            m.t60Scale = 1.7f; m.dampingSlope = 0.12f;
            m.weightSlope = -0.25f; m.weightRipple = 0.1f;
            m.couplingScale = 0.5f; m.bandBWeight = 0.3f;
            m.nonlinearity = 0.15f; m.hardening = 1.0f;
            m.stereoWidth = 1.0f; m.stereoPattern = StereoPattern::Random;
            m.exciteTilt = 0.45f; m.strikeContact = 0.22f; m.strikeNoise = 0.12f; m.bodyWeight = 0.05f;
            m.wobble = 0.0f; m.detune = 0.004f; m.grain = 0.1f;
        }
        {
            auto& m = t[(size_t) MaterialType::Metal];
            m.name = "METAL"; m.ownStructure = FormAnchor::Metallic; m.structurePull = 0.55f;
            m.t60Scale = 1.3f; m.dampingSlope = 0.3f;
            m.weightSlope = -0.15f; m.weightRipple = 0.25f;
            m.couplingScale = 1.0f; m.bandBWeight = 1.0f;
            m.nonlinearity = 0.6f; m.hardening = 1.0f;
            m.stereoWidth = 1.0f; m.stereoPattern = StereoPattern::Random;
            m.exciteTilt = 0.5f; m.strikeContact = 0.3f; m.strikeNoise = 0.35f; m.bodyWeight = 0.2f;
            m.wobble = 0.0f; m.detune = 0.012f; m.grain = 0.2f;
        }
        {
            auto& m = t[(size_t) MaterialType::Organic];
            m.name = "ORGANIC"; m.ownStructure = FormAnchor::Inharmonic; m.structurePull = 0.5f;
            m.stretch = 0.0008f; m.ratioJitter = 0.05f;
            m.t60Scale = 0.5f; m.dampingSlope = 0.9f;
            m.weightSlope = 0.2f; m.weightRipple = 0.4f;
            m.couplingScale = 0.7f; m.bandBWeight = 0.5f;
            m.nonlinearity = 0.4f; m.hardening = -1.0f;
            m.stereoWidth = 0.6f; m.stereoPattern = StereoPattern::Random;
            m.exciteTilt = -0.3f; m.strikeContact = 1.4f; m.strikeNoise = 0.6f; m.bodyWeight = 0.35f;
            m.wobble = 0.006f; m.wobbleRate = 0.7f; m.detune = 0.02f; m.grain = 0.6f;
        }
        {
            auto& m = t[(size_t) MaterialType::Liquid];
            m.name = "LIQUID"; m.ownStructure = FormAnchor::Stochastic; m.structurePull = 0.55f;
            m.t60Scale = 0.55f; m.dampingSlope = 0.8f;
            m.weightSlope = 0.3f; m.weightRipple = 0.3f;
            m.couplingScale = 1.0f; m.bandBWeight = 0.8f;
            m.nonlinearity = 0.5f; m.hardening = -1.0f;
            m.stereoWidth = 0.9f; m.stereoPattern = StereoPattern::Random;
            m.exciteTilt = -0.6f; m.strikeContact = 3.5f; m.strikeNoise = 0.7f; m.bodyWeight = 0.5f;
            m.wobble = 0.03f; m.wobbleRate = 1.6f; m.detune = 0.025f; m.grain = 0.8f;
        }
        {
            auto& m = t[(size_t) MaterialType::Membrane];
            m.name = "MEMBRANE"; m.ownStructure = FormAnchor::Membrane; m.structurePull = 0.6f;
            m.t60Scale = 0.35f; m.dampingSlope = 1.1f;
            m.weightSlope = 0.1f; m.weightRipple = 0.15f;
            m.couplingScale = 0.8f; m.bandBWeight = 0.4f;
            m.nonlinearity = 0.7f; m.hardening = -1.0f;
            m.stereoWidth = 0.8f; m.stereoPattern = StereoPattern::Alternate;
            m.exciteTilt = -0.4f; m.strikeContact = 1.8f; m.strikeNoise = 0.55f; m.bodyWeight = 0.6f;
            m.wobble = 0.0f; m.detune = 0.01f; m.grain = 0.4f;
        }
        {
            auto& m = t[(size_t) MaterialType::String];
            m.name = "STRING"; m.ownStructure = FormAnchor::Stretched; m.structurePull = 0.65f;
            m.stretch = 0.0003f;
            m.t60Scale = 1.0f; m.dampingSlope = 0.55f;
            m.weightSlope = 0.0f; m.weightRipple = 0.1f;
            m.couplingScale = 0.4f; m.bandBWeight = 0.2f;
            m.nonlinearity = 0.3f; m.hardening = 1.0f;
            m.stereoWidth = 0.3f; m.stereoPattern = StereoPattern::Narrow;
            m.exciteTilt = 0.0f; m.strikeContact = 0.55f; m.strikeNoise = 0.3f; m.bodyWeight = 0.15f;
            m.wobble = 0.0f; m.detune = 0.006f; m.grain = 0.3f;
        }
        {
            auto& m = t[(size_t) MaterialType::Wood];
            m.name = "WOOD"; m.ownStructure = FormAnchor::Stretched; m.structurePull = 0.55f;
            m.stretch = 0.02f; m.ratioJitter = 0.02f;
            m.t60Scale = 0.25f; m.dampingSlope = 1.2f;
            m.weightSlope = 0.4f; m.weightRipple = 0.3f;
            m.couplingScale = 0.5f; m.bandBWeight = 0.3f;
            m.nonlinearity = 0.3f; m.hardening = -1.0f;
            m.stereoWidth = 0.5f; m.stereoPattern = StereoPattern::Alternate;
            m.exciteTilt = -0.2f; m.strikeContact = 0.9f; m.strikeNoise = 0.45f; m.bodyWeight = 0.4f;
            m.wobble = 0.0f; m.detune = 0.012f; m.grain = 0.4f;
        }
        {
            auto& m = t[(size_t) MaterialType::Void];
            m.name = "VOID"; m.ownStructure = FormAnchor::Crystalline; m.structurePull = 0.7f;
            m.t60Scale = 2.5f; m.dampingSlope = 0.05f;
            m.weightSlope = 0.2f; m.weightRipple = 0.2f;
            m.couplingScale = 0.6f; m.bandBWeight = 0.5f;
            m.nonlinearity = 0.2f; m.hardening = -1.0f;
            m.stereoWidth = 1.0f; m.stereoPattern = StereoPattern::Random;
            m.exciteTilt = -0.5f; m.strikeContact = 5.0f; m.strikeNoise = 0.3f; m.bodyWeight = 0.8f;
            m.wobble = 0.01f; m.wobbleRate = 0.2f; m.detune = 0.02f; m.grain = 0.5f;
        }
        {
            auto& m = t[(size_t) MaterialType::Custom];
            m.name = "CUSTOM"; m.ownStructure = FormAnchor::Harmonic; m.structurePull = 0.0f;
            m.t60Scale = 0.8f; m.dampingSlope = 0.6f;
            m.couplingScale = 0.6f; m.bandBWeight = 0.4f;
            m.nonlinearity = 0.3f; m.hardening = 0.0f;
            m.stereoWidth = 0.6f; m.stereoPattern = StereoPattern::Random;
            m.exciteTilt = 0.0f; m.strikeContact = 0.8f; m.strikeNoise = 0.3f; m.bodyWeight = 0.2f;
            m.detune = 0.01f; m.grain = 0.3f;
        }
        return t;
    }

    const std::array<MaterialProfile, (size_t) MaterialType::Count>& profiles()
    {
        static const auto table = makeProfiles();
        return table;
    }
}

const MaterialProfile& materialProfile (MaterialType type) noexcept
{
    const auto i = std::min ((size_t) type, (size_t) MaterialType::Count - 1);
    return profiles()[i];
}

const char* materialName (MaterialType type) noexcept
{
    return materialProfile (type).name;
}

MaterialProfile blendProfiles (const MaterialProfile& a, const MaterialProfile& b, float t) noexcept
{
    t = clamp01 (t);
    MaterialProfile r = t < 0.5f ? a : b;
    auto mix = [t] (float x, float y) { return x + (y - x) * t; };
    r.structurePull = mix (a.structurePull, b.structurePull);
    r.stretch       = mix (a.stretch, b.stretch);
    r.ratioJitter   = mix (a.ratioJitter, b.ratioJitter);
    r.ratioSpan     = mix (a.ratioSpan, b.ratioSpan);
    r.t60Scale      = std::exp2 (mix (std::log2 (a.t60Scale), std::log2 (b.t60Scale)));
    r.dampingSlope  = mix (a.dampingSlope, b.dampingSlope);
    r.weightSlope   = mix (a.weightSlope, b.weightSlope);
    r.weightRipple  = mix (a.weightRipple, b.weightRipple);
    r.couplingScale = mix (a.couplingScale, b.couplingScale);
    r.bandBWeight   = mix (a.bandBWeight, b.bandBWeight);
    r.nonlinearity  = mix (a.nonlinearity, b.nonlinearity);
    r.hardening     = mix (a.hardening, b.hardening);
    r.stereoWidth   = mix (a.stereoWidth, b.stereoWidth);
    r.exciteTilt    = mix (a.exciteTilt, b.exciteTilt);
    r.strikeContact = std::exp2 (mix (std::log2 (a.strikeContact), std::log2 (b.strikeContact)));
    r.strikeNoise   = mix (a.strikeNoise, b.strikeNoise);
    r.bodyWeight    = mix (a.bodyWeight, b.bodyWeight);
    r.wobble        = mix (a.wobble, b.wobble);
    r.wobbleRate    = mix (a.wobbleRate, b.wobbleRate);
    r.detune        = mix (a.detune, b.detune);
    r.grain         = mix (a.grain, b.grain);
    return r;
}

} // namespace am

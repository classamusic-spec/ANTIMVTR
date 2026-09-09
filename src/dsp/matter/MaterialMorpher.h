#pragma once

#include "MaterialProfile.h"
#include "MatterNode.h"

namespace am
{

/**
    MATERIAL MORPHER — computes the per-node targets of a voice from the
    Shape controls, the note and the blended material A→B.

    The structure is interpolated (log-frequency ratios, weights, decays,
    excitation, stereo) — never rendered audio. Seeded tables (stochastic
    anchor, material sets, jitters, pan pattern) are rebuilt only when the
    seed, the materials or the node count change; computeTargets() runs
    every block and is cheap (a few polynomial evaluations per node).
*/
class MaterialMorpher
{
public:
    static constexpr int kBodyNodes = 2;          ///< the last two nodes are sub-fundamental "body" modes
    static constexpr float kReferenceHz = 261.6256f; ///< C4: structure reference when keytrack < 1

    struct Input
    {
        float form = 0.3f, density = 0.5f, mass = 0.4f, tension = 0.5f, decay = 0.5f, surface = 0.2f;
        float blend = 0.0f, distribution = 0.5f, stereo = 0.6f, excite = 0.5f, keytrack = 1.0f, pitchSemis = 0.0f;
        double noteFrequency = 261.6256;
        double sampleRate = 48000.0;
        float  blockSeconds = 0.0f;     ///< advances the slow wobble
        int    nodeCount = 64;
    };

    bool needsRebuild (uint32_t seed, MaterialType a, MaterialType b, int nodeCount) const noexcept
    {
        return ! built || seed != curSeed || a != matAType || b != matBType || nodeCount != count;
    }

    void rebuild (uint32_t seed, MaterialType a, MaterialType b, int nodeCount) noexcept;

    /**
        Writes frequency / targetFrequency / ratio / weight / damping / pan /
        nonlinearity / excitation / active for every node (cluster and
        couplingCount are left alone). Reads nodes[i].energy for the
        amplitude-dependent nonlinearity.
    */
    void computeTargets (const Input& in, MatterNode* nodes) noexcept;

    const MaterialProfile& profile() const noexcept { return blended; }
    float fundamentalT60() const noexcept { return lastT60; }
    float outputNormalisation() const noexcept { return lastNorm; }

    /** Accurate 2^x for |x| < 64 (error ~1e-6). */
    static inline float pow2 (float x) noexcept
    {
        x = std::max (-60.0f, std::min (60.0f, x));
        const int   ip = (int) std::floor (x);
        const float f  = x - (float) ip;
        const float p  = 1.0f + f * (0.69314718f + f * (0.24022651f + f * (0.05550411f + f * (0.00961813f + f * (0.00133336f + f * 0.00015403f)))));
        return std::ldexp (p, ip);
    }

private:
    StructureTable stochastic, matA, matB;
    std::array<float, kMaxMatterNodes> jitter {}, panRnd {}, rankSpread {}, wobblePhase {}, wobbleRate {}, ripple {};
    MaterialProfile profA, profB, blended;
    uint32_t curSeed = 0;
    MaterialType matAType = MaterialType::Custom, matBType = MaterialType::Custom;
    int count = 0;
    bool built = false;
    float lastT60 = 0.0f, lastNorm = 1.0f;
};

} // namespace am

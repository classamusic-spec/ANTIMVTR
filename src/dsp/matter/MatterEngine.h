#pragma once

#include "dsp/RenderContext.h"
#include "dev/diagnostics/DiagnosticSnapshot.h"
#include "core/RealtimeUtils.h"
#include "MatterNode.h"
#include "ModalResonator.h"
#include "MaterialProfile.h"
#include "MaterialMorpher.h"
#include "MatterTopology.h"

namespace am
{

/**
    MATTER ENGINE — the technological centerpiece of ANTI-MATR.

    Matter is a dynamic graph of resonating nodes excited by the Source (and
    by a velocity-scaled strike at note-on). The public Shape controls
    (Density, Form, Mass, Tension, Decay, Surface) and the material models
    drive the node distribution, damping, weighting, coupling and
    nonlinearity through MaterialMorpher; MatterTopology builds clusters
    and coupling edges; ModalBank renders the nodes.

    Per block: the node records (possibly modified by Evolve between
    blocks) are turned into resonator coefficients (frequencies glide,
    gains ramp), the block is rendered, node energies and safety are
    updated, and the material baseline for the next block is recomputed
    from the smoothed parameters (see MatterNode.h for the contract).
*/
class MatterEngine
{
public:
    using Node = MatterNode;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    /** Called when the voice starts (or re-targets in legato): (re)builds the node distribution for the note. */
    void noteOn (const NoteState& note, const ParamValues& params, Quality quality);
    void noteOff();

    /**
        Processes one block. `excL/excR` is the source excitation; the Matter
        response is written to `outL/outR` (overwrite). Shape parameters are
        read from `ctx` every block and followed smoothly.
    */
    void process (const float* excL, const float* excR, float* outL, float* outR, int n,
                  const RenderContext& ctx, const NoteState& note);

    /** Output-referred stored energy (0..1-ish). Used to decide when a released voice can end. */
    float energy() const noexcept { return currentEnergy; }
    bool isActive() const noexcept { return currentEnergy > kSilenceThreshold; }

    int numNodes() const noexcept { return nodeCount; }
    int activeNodes() const noexcept;
    int clusterCount() const noexcept { return topology.numClusters(); }
    uint32_t topologySeed() const noexcept { return seed; }

    Node& node (int i) noexcept { return nodes[(size_t) i]; }
    const Node& node (int i) const noexcept { return nodes[(size_t) i]; }

    /** Copies node state into the diagnostics record. Returns number of nodes written. */
    int fillDiagnostics (NodeDiag* dest, int maxNodes) const noexcept;

    /** Copies coupling edges into the diagnostics record. Returns number of edges written. */
    int fillEdgeDiagnostics (EdgeDiag* dest, int maxEdges) const noexcept;

    /** Average and maximum coupling strength over all edges (0 if none). */
    void couplingStats (float& average, float& maximum) const noexcept;

    // ---- introspection (tests, DSP LAB)
    const MaterialProfile& material() const noexcept { return morpher.profile(); }
    const MatterTopology& getTopology() const noexcept { return topology; }
    float renderedFrequency (int i) const noexcept { return renderFreq[(size_t) i]; }
    float lastCouplingScale() const noexcept { return bank.lastCouplingScale(); }

private:
    struct ShapeValues
    {
        MaterialMorpher::Input morph;
        float coupling = 0.3f, strike = 0.35f, surface = 0.2f, mass = 0.4f;
        int materialA = 0, materialB = 1, topologyType = 2;
        uint32_t seed = 7;
    };

    static ShapeValues readShape (const ParamValues& p) noexcept;
    void rebuildIfNeeded (const ShapeValues& v, int count) noexcept;
    void computeNextTargets (const ShapeValues& v, const NoteState& note, float blockSeconds) noexcept;
    void applyCoupling (const ShapeValues& v, float rMax) noexcept;
    void buildStrike (const NoteState& note, const ShapeValues& v) noexcept;
    void conditionExcitation (const float* excL, const float* excR, int n, const ShapeValues& v) noexcept;
    void reportSafety (const RenderContext& ctx, SafetyEvent e, int count) noexcept;

    std::array<Node, kMaxMatterNodes> nodes {};
    ModalBank bank;
    MaterialMorpher morpher;
    MatterTopology topology;

    std::array<float, kMaxMatterNodes> renderFreq {}, amps {}, outGain {}, edgeJitter {};
    std::array<float, kMaxMatterNodes> coupA {}, coupB {}, coupHub {};
    std::array<ModalBank::ExtraEdge, ModalBank::kMaxExtraEdges> extraScaled {};

    static constexpr int kStrikeBuf = 4096;
    static constexpr int kOutDelay  = 32;          ///< fixed delay of the output used for the surface interaction
    std::array<float, kStrikeBuf> strikeBuf {};
    std::array<float, kMaxBlockSize> excBuf {}, transBuf {}, strikeSig {};
    std::array<float, kOutDelay> outRing {};
    int strikePos = 0, strikeLen = 0, strikePulseLen = 0, outRingPos = 0;
    float strikeNorm = 1.0f;          ///< coherence normalisation of the strike (see process())
    float envFast = 0.0f, envSlow = 0.0f;   ///< excitation power followers behind the transient split
    bool  transientActive = false;

    int nodeCount = 0;
    uint32_t seed = 0;
    int lastTopology = -1, lastQualityNodes = 0;
    float currentEnergy = 0.0f;
    float lastEdgeScale = 0.0f;
    float lastCoupScale = 0.0f, lastCoupRMax = 0.0f, lastCoupJitter = -1.0f;
    int   lastCoupTopology = -1, topologyVersion = 0;
    double sr = 48000.0;
    float lpState = 0.0f;
    bool gate = false, snapNext = true, everStarted = false;
    Rng grainRng { 0x6A11u };
    AntiDenormal antiDenormal;
};

} // namespace am

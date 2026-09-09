#pragma once

#include "dsp/RenderContext.h"
#include "dev/diagnostics/DiagnosticSnapshot.h"

namespace am
{

/**
    MATTER ENGINE — the technological centerpiece of ANTI-MATR.

    Matter is a dynamic graph of resonating nodes excited by the Source. The
    public Shape controls (Density, Form, Mass, Tension, Decay, Surface) and
    the material models drive the node distribution, damping, weighting,
    coupling and nonlinearity.

    This header defines the stable interface used by the voice, the Evolve
    engine and the diagnostics. The Phase-0 implementation is a transparent
    pass-through so the rest of the instrument can be validated; the real
    modal graph is implemented in Phase 5/6 behind the same interface.
*/
class MatterEngine
{
public:
    /** One resonating node. Evolve operators manipulate these directly. */
    struct Node
    {
        float frequency       = 0.0f;   ///< current resonant frequency (Hz)
        float targetFrequency = 0.0f;   ///< frequency the material asks for (Hz), before Evolve
        float ratio           = 1.0f;   ///< target frequency / fundamental
        float weight          = 0.0f;   ///< amplitude weight 0..1 (0 = inactive)
        float damping         = 0.0f;   ///< per-sample energy loss factor
        float pan             = 0.0f;   ///< -1..1
        float nonlinearity    = 0.0f;   ///< 0..1
        float excitation      = 1.0f;   ///< how strongly the source excites this node
        float energy          = 0.0f;   ///< running energy estimate for diagnostics
        uint8_t cluster       = 0;
        uint8_t couplingCount = 0;
        bool  active          = false;
        // Resonator state (2-pole / SVF style), maintained by the implementation.
        float s1 = 0.0f, s2 = 0.0f;
        float g = 0.0f, r = 0.0f;
    };

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    /** Called when the voice starts: (re)builds the node distribution for the note. */
    void noteOn (const NoteState& note, const ParamValues& params, Quality quality);
    void noteOff();

    /**
        Processes one block. `excL/excR` is the source excitation; the Matter
        response is written to `outL/outR` (overwrite). The implementation may
        read `ctx` every block to follow Shape parameter changes smoothly.
    */
    void process (const float* excL, const float* excR, float* outL, float* outR, int n,
                  const RenderContext& ctx, const NoteState& note);

    /** Total stored energy (0..1-ish). Used to decide when a released voice can end. */
    float energy() const noexcept { return currentEnergy; }

    bool isActive() const noexcept { return currentEnergy > kSilenceThreshold; }

    int numNodes() const noexcept { return nodeCount; }
    int activeNodes() const noexcept;
    int clusterCount() const noexcept { return clusters; }
    uint32_t topologySeed() const noexcept { return seed; }

    Node& node (int i) noexcept { return nodes[(size_t) i]; }
    const Node& node (int i) const noexcept { return nodes[(size_t) i]; }

    /** Copies node state into the diagnostics record. Returns number of nodes written. */
    int fillDiagnostics (NodeDiag* dest, int maxNodes) const noexcept;

    /** Copies coupling edges into the diagnostics record. Returns number of edges written. */
    int fillEdgeDiagnostics (EdgeDiag* dest, int maxEdges) const noexcept;

    /** Average and maximum coupling strength over all edges (0 if none). */
    void couplingStats (float& average, float& maximum) const noexcept;

private:
    std::array<Node, kMaxMatterNodes> nodes {};
    int nodeCount = 0;
    int clusters  = 0;
    uint32_t seed = 0;
    float currentEnergy = 0.0f;
    double sr = 48000.0;
    bool gate = false;
};

} // namespace am

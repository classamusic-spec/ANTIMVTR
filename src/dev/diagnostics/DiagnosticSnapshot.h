#pragma once

#include "PerformanceProfiler.h"
#include "SafetyMonitor.h"

namespace am
{

/** Per-node diagnostic record published for the focus voice. */
struct NodeDiag
{
    float   frequency       = 0.0f;
    float   targetFrequency = 0.0f;
    float   energy          = 0.0f;
    float   weight          = 0.0f;
    float   damping         = 0.0f;
    float   pan             = 0.0f;
    float   nonlinearity    = 0.0f;
    float   excitation      = 0.0f;
    uint8_t cluster         = 0;
    uint8_t couplingCount   = 0;
    uint8_t active          = 0;
};

/** One coupling edge of the Matter graph, published for the focus voice. */
struct EdgeDiag
{
    uint8_t from     = 0;
    uint8_t to       = 0;
    uint8_t type     = 0;      ///< MatterEngine coupling type (implementation-defined enum)
    float   strength = 0.0f;
};

constexpr int kMaxMatterEdges = 512;

//==============================================================================
// EVOLVE diagnostics — filled by EvolveEngine::fillDiagnostics for the focus voice.

/** The eight Evolve operators in parameter order. Bit (1 << op) of DevControls::evolveBypassMask bypasses one. */
enum class EvolveOperator : uint8_t { Bend = 0, Melt, Tear, Magnet, Gravity, Scatter, Freeze, Crush, Count };
constexpr int kNumEvolveOperators = (int) EvolveOperator::Count;

inline constexpr uint32_t evolveBypassBit (EvolveOperator op) noexcept { return 1u << (uint32_t) op; }

inline const char* evolveOperatorName (EvolveOperator op) noexcept
{
    switch (op)
    {
        case EvolveOperator::Bend:    return "Bend";
        case EvolveOperator::Melt:    return "Melt";
        case EvolveOperator::Tear:    return "Tear";
        case EvolveOperator::Magnet:  return "Magnet";
        case EvolveOperator::Gravity: return "Gravity";
        case EvolveOperator::Scatter: return "Scatter";
        case EvolveOperator::Freeze:  return "Freeze";
        case EvolveOperator::Crush:   return "Crush";
        default:                      return "Unknown";
    }
}

/** What Evolve did to the focus voice's nodes in the last block. Trivially copyable. */
struct EvolveDiag
{
    float    amount[kNumEvolveOperators] {};  ///< applied amount per operator after bypass (Gravity: signed pull -1..1, Freeze: 0/1)
    int      activeNodes   = 0;               ///< active nodes the operators considered
    int      nodesMoved    = 0;               ///< nodes whose frequency, weight, damping or pan changed
    float    meanAbsCents  = 0.0f;            ///< mean |frequency shift| in cents over the active nodes
    float    maxAbsCents   = 0.0f;
    int      magnetLocked  = 0;               ///< active nodes within 5 cents of the magnet grid after the pull
    int      tearPairs     = 0;               ///< detuned twin pairs Tear created
    int      crushDropped  = 0;               ///< nodes Crush silenced
    uint8_t  freeze        = 0;
    uint8_t  bypassMask    = 0;               ///< the bypass mask honoured this block
    float    motionPhase   = 0.0f;            ///< 0..1
    float    motionRateHz  = 0.0f;
    float    fundamentalHz = 0.0f;            ///< the material fundamental the grids are relative to
};

struct StageLevels
{
    float rms  = 0.0f;
    float peak = 0.0f;
};

/**
    Complete engine diagnostic state, published once per block through a
    TripleBuffer. Read by DSP LAB. Must stay trivially copyable.
*/
struct DiagnosticSnapshot
{
    double   sampleRate   = 0.0;
    int      blockSize    = 0;
    uint8_t  quality      = (uint8_t) Quality::Normal;
    uint8_t  dryMode      = (uint8_t) DryMode::FullSynth;
    int      activeVoices = 0;
    int      maxVoices    = 0;
    uint64_t sampleTime   = 0;
    int      latencySamples = 0;

    StageLevels stages[(int) Stage::Count] {};
    SafetyMonitor::Counters safety;
    PerformanceProfiler::Stats perf;

    // Matter inspection of the focus voice (most recent or explicitly selected voice)
    int      focusVoice     = -1;
    int      focusNote      = -1;
    int      numNodes       = 0;
    int      activeNodes    = 0;
    int      clusterCount   = 0;
    uint32_t topologySeed   = 0;
    float    averageCoupling = 0.0f;
    float    maxCoupling     = 0.0f;
    float    matterEnergy    = 0.0f;
    uint8_t  materialA       = 0;
    uint8_t  materialB       = 0;
    float    materialBlend   = 0.0f;
    NodeDiag nodes[kMaxMatterNodes] {};
    int      numEdges = 0;
    EdgeDiag edges[kMaxMatterEdges] {};
    float    fundamentalHz = 0.0f;    ///< frequency the focus voice is playing (after bend/glide)

    // Evolve (focus voice)
    EvolveDiag evolve;

    // Fracture
    int   fractureFFTSize = 0;
    int   fractureHop     = 0;
    float fractureActivity = 0.0f;

    uint32_t eventsDropped = 0;
};

/**
    Lightweight state for the consumer-facing visualizers (central object,
    panel meters). Published every block. Must stay trivially copyable.
*/
struct VisualStateSnapshot
{
    static constexpr int kVisualNodes = 32;
    static constexpr int kBands = 16;

    float rmsL = 0.0f, rmsR = 0.0f, peak = 0.0f;
    float sourceRms = 0.0f;
    float matterRms = 0.0f;
    int   activeVoices = 0;
    int   activeNodes  = 0;
    int   clusterCount = 0;

    float density = 0.5f, form = 0.3f, mass = 0.4f, tension = 0.5f, decay = 0.5f, surface = 0.2f;
    float bend = 0.0f, melt = 0.0f, tear = 0.0f, magnet = 0.0f, gravity = 0.5f, scatter = 0.0f, crush = 0.0f;
    bool  freeze = false;
    float fractureActivity = 0.0f;
    bool  fractureOn = false;
    float spaceActivity = 0.0f;
    int   spaceType = 0;
    float pitchHz = 0.0f;
    float noteEnergy = 0.0f;      ///< 0..1 envelope-like energy of the most recent voice
    uint64_t sampleTime = 0;

    int   numVisualNodes = 0;
    float nodeFrequency[kVisualNodes] {};
    float nodeEnergy[kVisualNodes] {};
    float nodePan[kVisualNodes] {};
    uint8_t nodeCluster[kVisualNodes] {};
};

/** Developer controls written by DSP LAB and read by the engine (all atomics). */
struct DevControls
{
    std::atomic<int>  dryMode        { (int) DryMode::FullSynth };
    std::atomic<bool> bypassEvolve   { false };
    std::atomic<uint32_t> evolveBypassMask { 0 };   ///< bit (1 << EvolveOperator) bypasses one operator; 0 = nothing bypassed
    std::atomic<bool> bypassFracture { false };
    std::atomic<bool> bypassSpace    { false };
    std::atomic<int>  focusVoice     { -1 };   ///< -1 = most recently started voice
    std::atomic<bool> profiling      { true };
};

} // namespace am

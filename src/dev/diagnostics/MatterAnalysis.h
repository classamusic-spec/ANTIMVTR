#pragma once

#include "DiagnosticSnapshot.h"

#include <vector>

namespace am::dev
{

/**
    Analysis of a published Matter node/edge set (§71–73, §75).

    Pure functions over the diagnostic records — no GUI, no engine access, no
    audio thread. DSP LAB views draw what these produce and the unit tests
    verify the maths against synthetic distributions.
*/

/** Summary statistics of one node/edge set. */
struct MatterStats
{
    int   numNodes      = 0;
    int   activeNodes   = 0;
    int   clusterCount  = 0;    ///< number of distinct cluster ids among the nodes
    float minFrequency  = 0.0f;
    float maxFrequency  = 0.0f;
    float totalEnergy   = 0.0f;
    float totalWeight   = 0.0f;
    float meanRatio     = 0.0f; ///< mean of frequency / fundamental
    float meanAbsCents  = 0.0f; ///< mean |deviation from the nearest harmonic| in cents
    float maxAbsCents   = 0.0f;
    int   numEdges      = 0;
    float meanStrength  = 0.0f;
    float maxStrength   = 0.0f;
    float meanCoupling  = 0.0f; ///< mean couplingCount over the nodes
};

/** Signed distance in cents from `freq` to the nearest harmonic of `fundamental`.
    Returns 0 (and harmonic 0) when either frequency is not usable. */
float centsFromNearestHarmonic (float freq, float fundamental, int* harmonicOut = nullptr) noexcept;

/** Statistics over `numNodes` nodes and `numEdges` edges. Inactive nodes are
    included in the counts but only active, non-zero nodes contribute to the
    frequency / harmonicity figures. */
MatterStats analyseNodes (const NodeDiag* nodes, int numNodes,
                          const EdgeDiag* edges, int numEdges,
                          float fundamentalHz) noexcept;

/** Weight-weighted histogram of frequency/fundamental ratios.
    `bins` is resized to `numBins` and covers ratios [0, maxRatio]. Values are
    normalised so the largest bin is 1 (all-zero when there is nothing to show). */
void ratioHistogram (const NodeDiag* nodes, int numNodes, float fundamentalHz,
                     float maxRatio, int numBins, std::vector<float>& bins);

/** One laid-out point of the topology graph, in 0..1 view coordinates. */
struct GraphNode
{
    float x = 0.5f, y = 0.5f;
    float weight = 0.0f;        ///< 0..1, drives the dot size
    float energy = 0.0f;        ///< 0..1 (normalised against the strongest node)
    float frequency = 0.0f;
    uint8_t cluster = 0;
    bool  active = false;
};

/** Layout of the Matter graph (§72).

    Nodes are placed on a ring per cluster: the angle comes from the cluster
    (so clusters occupy wedges), the radius from the log frequency, so the
    picture stays stable while node frequencies move. In `clusterMode` every
    cluster collapses to a single point at its mean log frequency. */
std::vector<GraphNode> layoutTopology (const NodeDiag* nodes, int numNodes,
                                       bool clusterMode,
                                       float minHz = 20.0f, float maxHz = 20000.0f);

/** Difference between two captured node sets (before / after Evolve, §75). */
struct DistributionDelta
{
    int   nodesBefore = 0, nodesAfter = 0;
    int   clustersBefore = 0, clustersAfter = 0;
    int   movedNodes = 0;           ///< nodes whose frequency moved more than 1 cent
    float meanAbsCents = 0.0f;      ///< mean |frequency shift| in cents over matched nodes
    float maxAbsCents = 0.0f;
    float energyBefore = 0.0f, energyAfter = 0.0f;
    float weightBefore = 0.0f, weightAfter = 0.0f;
};

/** Compares two node sets index by index (the Evolve operators transform nodes
    in place, so index identity is meaningful). */
DistributionDelta compareDistributions (const NodeDiag* before, int numBefore,
                                        const NodeDiag* after, int numAfter) noexcept;

/** Normalised log position of `hz` inside [minHz, maxHz], clamped to 0..1. */
float logPosition (float hz, float minHz, float maxHz) noexcept;

} // namespace am::dev

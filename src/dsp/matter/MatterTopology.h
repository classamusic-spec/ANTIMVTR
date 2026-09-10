#pragma once

#include "ModalResonator.h"
#include "MatterCluster.h"

namespace am
{

/** Order matches the shape.topology choice list. */
enum class TopologyType : uint8_t { Chain = 0, Ring, Clusters, Lattice, Random, Star, Count };

/** Published as EdgeDiag::type. */
enum class EdgeType : uint8_t { BandA = 0, BandB, Closure, Hub, Random };

/**
    MATTER TOPOLOGY — builds the coupling graph and the cluster assignment
    for a node count, a topology type and a seed.

    The graph is realised as two strided bands (edge (i, i+stride) with a
    per-edge base strength), an optional hub and a few explicit edges — the
    exact form ModalBank renders — plus a flat edge list for diagnostics.
    Base strengths are 0..1; the engine scales them per block.
*/
class MatterTopology
{
public:
    static constexpr int kMaxEdges = 512;
    static constexpr int kMaxClusters = 16;

    struct Edge { uint8_t from, to; EdgeType type; float base; };

    void build (TopologyType type, uint32_t seed, int nodeCount, int clusterCount, float bandBWeight) noexcept;

    int  nodeCount() const noexcept { return count; }
    int  strideA() const noexcept { return sA; }
    int  strideB() const noexcept { return sB; }
    int  hub() const noexcept { return hubIndex; }
    const float* bandA() const noexcept { return kA.data(); }
    const float* bandB() const noexcept { return kB.data(); }
    const float* hubStrength() const noexcept { return kHub.data(); }
    const ModalBank::ExtraEdge* extraEdges() const noexcept { return extras.data(); }
    int  numExtraEdges() const noexcept { return numExtra; }

    int  numClusters() const noexcept { return clusterCount; }
    const MatterCluster& cluster (int i) const noexcept { return clusters[(size_t) i]; }
    uint8_t clusterOf (int node) const noexcept { return clusterId[(size_t) node]; }
    uint8_t couplingCount (int node) const noexcept { return degree[(size_t) node]; }

    int  numEdges() const noexcept { return edgeCount; }
    const Edge& edge (int i) const noexcept { return edges[(size_t) i]; }

private:
    void addEdge (int from, int to, EdgeType type, float base) noexcept;

    int count = 0, sA = 0, sB = 0, hubIndex = -1, numExtra = 0, clusterCount = 0, edgeCount = 0;
    std::array<float, kMaxMatterNodes> kA {}, kB {}, kHub {};
    std::array<ModalBank::ExtraEdge, ModalBank::kMaxExtraEdges> extras {};
    std::array<uint8_t, kMaxMatterNodes> clusterId {}, degree {};
    std::array<MatterCluster, kMaxClusters> clusters {};
    std::array<Edge, kMaxEdges> edges {};
};

} // namespace am

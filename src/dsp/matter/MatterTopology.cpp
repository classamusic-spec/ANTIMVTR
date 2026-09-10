#include "MatterTopology.h"
#include "core/Random.h"

namespace am
{

void MatterTopology::addEdge (int from, int to, EdgeType type, float base) noexcept
{
    if (edgeCount >= kMaxEdges || base <= 0.0f || from < 0 || to < 0 || from >= count || to >= count || from == to)
        return;
    edges[(size_t) edgeCount++] = { (uint8_t) from, (uint8_t) to, type, base };
    if (degree[(size_t) from] < 255) ++degree[(size_t) from];
    if (degree[(size_t) to] < 255)   ++degree[(size_t) to];
}

void MatterTopology::build (TopologyType type, uint32_t seed, int nodeCount, int clusterCountIn, float bandBWeight) noexcept
{
    count = std::max (ModalBank::kLanes, std::min (kMaxMatterNodes, nodeCount));
    kA.fill (0.0f); kB.fill (0.0f); kHub.fill (0.0f);
    degree.fill (0); clusterId.fill (0);
    sA = sB = 0; hubIndex = -1; numExtra = 0; edgeCount = 0;

    // ---- clusters: contiguous index ranges (index order = importance order)
    clusterCount = std::max (1, std::min (kMaxClusters, clusterCountIn));
    if (clusterCount > count) clusterCount = count;
    for (int c = 0; c < clusterCount; ++c)
    {
        const int first = c * count / clusterCount;
        const int last  = (c + 1) * count / clusterCount;
        clusters[(size_t) c] = { (uint8_t) c, (uint8_t) first, (uint8_t) (last - first) };
        for (int i = first; i < last; ++i) clusterId[(size_t) i] = (uint8_t) c;
    }

    Rng rng (hashSeed (seed, 0x70B0106Fu + (uint32_t) type));
    // Upper nodes couple a little less; edges touching the fundamental are weak so a sustained
    // source can still lock to it (the hub of a STAR is exempt: coupling to the fundamental is its point).
    auto taper = [] (int i) { return (i == 0 ? 0.25f : 1.0f) / (1.0f + 0.015f * (float) i); };
    const float wB = clamp01 (bandBWeight);

    switch (type)
    {
        case TopologyType::Chain:
            sA = 1;
            for (int i = 0; i + 1 < count; ++i) kA[(size_t) i] = taper (i);
            break;

        case TopologyType::Ring:
            sA = 1;
            for (int i = 0; i + 1 < count; ++i) kA[(size_t) i] = taper (i);
            extras[(size_t) numExtra++] = { count - 1, 0, 0.25f };
            break;

        case TopologyType::Clusters:
            sA = 1; sB = 2;
            for (int i = 0; i + 1 < count; ++i)
                if (clusterId[(size_t) i] == clusterId[(size_t) (i + 1)]) kA[(size_t) i] = taper (i);
            for (int i = 0; i + 2 < count; ++i)
                if (clusterId[(size_t) i] == clusterId[(size_t) (i + 2)]) kB[(size_t) i] = wB * taper (i);
            break;

        case TopologyType::Lattice:
        {
            sA = 1;
            sB = std::max (2, std::min (ModalBank::kPad - 1, (int) std::lround (std::sqrt ((double) count))));
            for (int i = 0; i + 1 < count; ++i)
                if ((i + 1) % sB != 0) kA[(size_t) i] = taper (i);         // rows
            for (int i = 0; i + sB < count; ++i) kB[(size_t) i] = wB * taper (i); // columns
            break;
        }

        case TopologyType::Random:
        {
            sA = 1;
            sB = 3 + rng.nextInt (std::max (1, std::min (ModalBank::kPad - 4, count / 3 - 2)));
            for (int i = 0; i + 1 < count; ++i)
                if (rng.chance (0.6f)) kA[(size_t) i] = (0.3f + 0.7f * rng.nextFloat()) * taper (i);
            for (int i = 0; i + sB < count; ++i)
                if (rng.chance (0.35f)) kB[(size_t) i] = wB * (0.3f + 0.7f * rng.nextFloat()) * taper (i);
            for (int e = 0; e < 4 && numExtra < ModalBank::kMaxExtraEdges; ++e)
            {
                const int from = rng.nextInt (count), to = rng.nextInt (count);
                if (from != to) extras[(size_t) numExtra++] = { from, to, 0.4f + 0.6f * rng.nextFloat() };
            }
            break;
        }

        case TopologyType::Star:
        default:
            hubIndex = 0;
            for (int i = 1; i < count; ++i) kHub[(size_t) i] = 0.8f * taper (i);
            sA = 1;
            for (int i = 0; i + 1 < count; ++i) kA[(size_t) i] = 0.3f * taper (i);
            break;
    }

    // ---- flat edge list (diagnostics) and degrees
    if (sA > 0) for (int i = 0; i + sA < count; ++i) addEdge (i, i + sA, EdgeType::BandA, kA[(size_t) i]);
    if (sB > 0) for (int i = 0; i + sB < count; ++i) addEdge (i, i + sB, EdgeType::BandB, kB[(size_t) i]);
    if (hubIndex >= 0) for (int i = 0; i < count; ++i) addEdge (hubIndex, i, EdgeType::Hub, kHub[(size_t) i]);
    for (int e = 0; e < numExtra; ++e)
        addEdge (extras[(size_t) e].from, extras[(size_t) e].to, type == TopologyType::Ring ? EdgeType::Closure : EdgeType::Random, extras[(size_t) e].k);
}

} // namespace am

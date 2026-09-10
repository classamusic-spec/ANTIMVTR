#include "MatterAnalysis.h"

#include <algorithm>
#include <cmath>

namespace am::dev
{

float logPosition (float hz, float minHz, float maxHz) noexcept
{
    if (! (hz > 0.0f) || ! (minHz > 0.0f) || ! (maxHz > minHz))
        return 0.0f;
    const float u = std::log (juce::jmax (minHz, hz) / minHz) / std::log (maxHz / minHz);
    return juce::jlimit (0.0f, 1.0f, u);
}

float centsFromNearestHarmonic (float freq, float fundamental, int* harmonicOut) noexcept
{
    if (harmonicOut != nullptr) *harmonicOut = 0;
    if (! std::isfinite (freq) || ! std::isfinite (fundamental) || freq <= 0.0f || fundamental <= 0.0f)
        return 0.0f;

    const float ratio = freq / fundamental;
    int harmonic = (int) std::lround (ratio);
    if (harmonic < 1) harmonic = 1;
    if (harmonicOut != nullptr) *harmonicOut = harmonic;

    const float reference = fundamental * (float) harmonic;
    return 1200.0f * std::log2 (freq / reference);
}

MatterStats analyseNodes (const NodeDiag* nodes, int numNodes,
                          const EdgeDiag* edges, int numEdges,
                          float fundamentalHz) noexcept
{
    MatterStats s;
    if (nodes == nullptr) numNodes = 0;
    s.numNodes = juce::jmax (0, numNodes);

    bool clusterSeen[256] {};
    double ratioSum = 0.0, centsSum = 0.0, couplingSum = 0.0;
    int measured = 0;

    for (int i = 0; i < s.numNodes; ++i)
    {
        const auto& n = nodes[i];
        if (n.active != 0) ++s.activeNodes;
        if (! clusterSeen[n.cluster]) { clusterSeen[n.cluster] = true; ++s.clusterCount; }

        s.totalEnergy += std::isfinite (n.energy) ? n.energy : 0.0f;
        s.totalWeight += std::isfinite (n.weight) ? n.weight : 0.0f;
        couplingSum += (double) n.couplingCount;

        if (! std::isfinite (n.frequency) || n.frequency <= 0.0f)
            continue;

        if (measured == 0) { s.minFrequency = s.maxFrequency = n.frequency; }
        else
        {
            s.minFrequency = std::min (s.minFrequency, n.frequency);
            s.maxFrequency = std::max (s.maxFrequency, n.frequency);
        }

        if (fundamentalHz > 0.0f)
        {
            ratioSum += (double) (n.frequency / fundamentalHz);
            const float cents = std::abs (centsFromNearestHarmonic (n.frequency, fundamentalHz));
            centsSum += (double) cents;
            s.maxAbsCents = std::max (s.maxAbsCents, cents);
        }
        ++measured;
    }

    if (measured > 0 && fundamentalHz > 0.0f)
    {
        s.meanRatio    = (float) (ratioSum / (double) measured);
        s.meanAbsCents = (float) (centsSum / (double) measured);
    }
    if (s.numNodes > 0)
        s.meanCoupling = (float) (couplingSum / (double) s.numNodes);

    s.numEdges = edges != nullptr ? juce::jmax (0, numEdges) : 0;
    if (s.numEdges > 0)
    {
        double sum = 0.0;
        for (int i = 0; i < s.numEdges; ++i)
        {
            const float st = std::isfinite (edges[i].strength) ? edges[i].strength : 0.0f;
            sum += (double) st;
            s.maxStrength = std::max (s.maxStrength, st);
        }
        s.meanStrength = (float) (sum / (double) s.numEdges);
    }
    return s;
}

void ratioHistogram (const NodeDiag* nodes, int numNodes, float fundamentalHz,
                     float maxRatio, int numBins, std::vector<float>& bins)
{
    numBins = juce::jlimit (1, 512, numBins);
    bins.assign ((size_t) numBins, 0.0f);
    if (nodes == nullptr || numNodes <= 0 || ! (fundamentalHz > 0.0f) || ! (maxRatio > 0.0f))
        return;

    float largest = 0.0f;
    for (int i = 0; i < numNodes; ++i)
    {
        const auto& n = nodes[i];
        if (! std::isfinite (n.frequency) || n.frequency <= 0.0f)
            continue;

        const float ratio = n.frequency / fundamentalHz;
        if (ratio > maxRatio) continue;

        int bin = (int) (ratio / maxRatio * (float) numBins);
        bin = juce::jlimit (0, numBins - 1, bin);
        const float w = std::isfinite (n.weight) ? juce::jmax (0.0f, n.weight) : 0.0f;
        bins[(size_t) bin] += juce::jmax (w, 0.05f);   // a node with no weight still counts
        largest = std::max (largest, bins[(size_t) bin]);
    }

    if (largest > 0.0f)
        for (auto& b : bins) b /= largest;
}

std::vector<GraphNode> layoutTopology (const NodeDiag* nodes, int numNodes,
                                       bool clusterMode, float minHz, float maxHz)
{
    std::vector<GraphNode> out;
    if (nodes == nullptr || numNodes <= 0)
        return out;

    // Map the cluster ids present onto a dense 0..n-1 range so the wedges are even.
    std::array<int, 256> clusterSlot {};
    clusterSlot.fill (-1);
    int numClusters = 0;
    for (int i = 0; i < numNodes; ++i)
        if (clusterSlot[nodes[i].cluster] < 0)
            clusterSlot[nodes[i].cluster] = numClusters++;

    const float twoPi = (float) kTwoPi;
    float maxEnergy = 0.0f;
    for (int i = 0; i < numNodes; ++i)
        if (std::isfinite (nodes[i].energy))
            maxEnergy = std::max (maxEnergy, nodes[i].energy);
    const float energyScale = maxEnergy > 1.0e-9f ? 1.0f / maxEnergy : 0.0f;

    if (clusterMode)
    {
        struct Acc { double logSum = 0.0; double energy = 0.0; double weight = 0.0; int count = 0; int active = 0; uint8_t id = 0; };
        std::vector<Acc> acc ((size_t) juce::jmax (1, numClusters));
        for (int i = 0; i < numNodes; ++i)
        {
            const auto& n = nodes[i];
            const int slot = clusterSlot[n.cluster];
            if (slot < 0) continue;
            auto& a = acc[(size_t) slot];
            a.id = n.cluster;
            a.logSum += (double) logPosition (n.frequency, minHz, maxHz);
            a.energy += std::isfinite (n.energy) ? (double) n.energy : 0.0;
            a.weight += std::isfinite (n.weight) ? (double) n.weight : 0.0;
            a.active += n.active != 0 ? 1 : 0;
            ++a.count;
        }

        out.reserve (acc.size());
        for (size_t c = 0; c < acc.size(); ++c)
        {
            const auto& a = acc[c];
            if (a.count == 0) continue;
            const float radius = 0.16f + 0.32f * (float) (a.logSum / (double) a.count);
            const float angle = twoPi * ((float) c + 0.5f) / (float) acc.size();
            GraphNode g;
            g.x = 0.5f + radius * std::cos (angle);
            g.y = 0.5f + radius * std::sin (angle);
            g.weight = juce::jlimit (0.0f, 1.0f, (float) (a.weight / (double) a.count));
            g.energy = juce::jlimit (0.0f, 1.0f, (float) a.energy * energyScale);
            g.frequency = 0.0f;
            g.cluster = a.id;
            g.active = a.active > 0;
            out.push_back (g);
        }
        return out;
    }

    // Per-cluster running index so nodes of one cluster spread across its wedge.
    std::vector<int> seen ((size_t) juce::jmax (1, numClusters), 0);
    std::vector<int> total ((size_t) juce::jmax (1, numClusters), 0);
    for (int i = 0; i < numNodes; ++i)
        if (const int slot = clusterSlot[nodes[i].cluster]; slot >= 0)
            ++total[(size_t) slot];

    out.reserve ((size_t) numNodes);
    for (int i = 0; i < numNodes; ++i)
    {
        const auto& n = nodes[i];
        const int slot = juce::jmax (0, clusterSlot[n.cluster]);
        const int index = seen[(size_t) slot]++;
        const int count = juce::jmax (1, total[(size_t) slot]);

        const float wedge = twoPi / (float) juce::jmax (1, numClusters);
        // Spread inside the wedge, leaving a small gap between clusters.
        const float within = count > 1 ? ((float) index / (float) (count - 1) - 0.5f) * 0.78f : 0.0f;
        const float angle = wedge * ((float) slot + 0.5f) + within * wedge;
        const float radius = 0.13f + 0.35f * logPosition (n.frequency, minHz, maxHz);

        GraphNode g;
        g.x = 0.5f + radius * std::cos (angle);
        g.y = 0.5f + radius * std::sin (angle);
        g.weight = juce::jlimit (0.0f, 1.0f, std::isfinite (n.weight) ? n.weight : 0.0f);
        g.energy = juce::jlimit (0.0f, 1.0f, (std::isfinite (n.energy) ? n.energy : 0.0f) * energyScale);
        g.frequency = n.frequency;
        g.cluster = n.cluster;
        g.active = n.active != 0;
        out.push_back (g);
    }
    return out;
}

DistributionDelta compareDistributions (const NodeDiag* before, int numBefore,
                                        const NodeDiag* after, int numAfter) noexcept
{
    DistributionDelta d;
    if (before == nullptr) numBefore = 0;
    if (after == nullptr)  numAfter = 0;
    d.nodesBefore = juce::jmax (0, numBefore);
    d.nodesAfter  = juce::jmax (0, numAfter);

    bool clusterA[256] {}, clusterB[256] {};
    for (int i = 0; i < d.nodesBefore; ++i)
    {
        if (! clusterA[before[i].cluster]) { clusterA[before[i].cluster] = true; ++d.clustersBefore; }
        d.energyBefore += std::isfinite (before[i].energy) ? before[i].energy : 0.0f;
        d.weightBefore += std::isfinite (before[i].weight) ? before[i].weight : 0.0f;
    }
    for (int i = 0; i < d.nodesAfter; ++i)
    {
        if (! clusterB[after[i].cluster]) { clusterB[after[i].cluster] = true; ++d.clustersAfter; }
        d.energyAfter += std::isfinite (after[i].energy) ? after[i].energy : 0.0f;
        d.weightAfter += std::isfinite (after[i].weight) ? after[i].weight : 0.0f;
    }

    const int matched = std::min (d.nodesBefore, d.nodesAfter);
    double sum = 0.0;
    int counted = 0;
    for (int i = 0; i < matched; ++i)
    {
        const float a = before[i].frequency, b = after[i].frequency;
        if (! std::isfinite (a) || ! std::isfinite (b) || a <= 0.0f || b <= 0.0f)
            continue;
        const float cents = std::abs (1200.0f * std::log2 (b / a));
        sum += (double) cents;
        d.maxAbsCents = std::max (d.maxAbsCents, cents);
        if (cents > 1.0f) ++d.movedNodes;
        ++counted;
    }
    if (counted > 0)
        d.meanAbsCents = (float) (sum / (double) counted);
    return d;
}

} // namespace am::dev

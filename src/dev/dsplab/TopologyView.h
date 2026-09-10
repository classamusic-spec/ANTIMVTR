#pragma once

#include "LabWidgets.h"
#include "MatterCapture.h"
#include "dev/diagnostics/MatterAnalysis.h"

namespace am::dev
{

/**
    TOPOLOGY VIEW (§72)

    The Matter graph drawn from the published node and edge records: nodes are
    points laid out by cluster (wedge) and frequency (radius), coupling edges
    are lines whose brightness follows their strength. The cluster mode
    collapses every cluster to a single point so a large graph stays readable.
*/
class TopologyView : public LabPanel
{
public:
    TopologyView();

    /** Points at the arrays to draw. The caller keeps them alive. */
    void setGraph (const NodeDiag* nodes, int numNodes, const EdgeDiag* edges, int numEdges);
    void setClusterMode (bool shouldCollapseClusters);
    bool clusterMode() const noexcept { return collapseClusters; }

    void paint (juce::Graphics& g) override;

private:
    void rebuild();

    const NodeDiag* nodes = nullptr;
    const EdgeDiag* edges = nullptr;
    int numNodes = 0, numEdges = 0;
    bool collapseClusters = false;
    std::vector<GraphNode> points;
    std::vector<int> clusterSlotOfNode;   ///< node index -> index into `points` in cluster mode
};

} // namespace am::dev

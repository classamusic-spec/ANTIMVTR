#include "TopologyView.h"

namespace am::dev
{

using namespace am::ui;

namespace
{
    juce::Colour clusterColour (uint8_t cluster) noexcept
    {
        static const juce::Colour palette[] =
        {
            Theme::cyan, Theme::violet, Theme::amber, Theme::magenta,
            Theme::blue, Theme::ivory, Theme::cyan.brighter (0.3f), Theme::violet.darker (0.2f)
        };
        return palette[cluster % (uint8_t) (sizeof (palette) / sizeof (palette[0]))];
    }
}

TopologyView::TopologyView() : LabPanel ("Topology") {}

void TopologyView::setGraph (const NodeDiag* n, int nodeCount, const EdgeDiag* e, int edgeCount)
{
    nodes = n;
    numNodes = juce::jmax (0, nodeCount);
    edges = e;
    numEdges = juce::jmax (0, edgeCount);
    rebuild();
    repaint();
}

void TopologyView::setClusterMode (bool shouldCollapseClusters)
{
    if (collapseClusters == shouldCollapseClusters)
        return;
    collapseClusters = shouldCollapseClusters;
    rebuild();
    repaint();
}

void TopologyView::rebuild()
{
    points = layoutTopology (nodes, numNodes, collapseClusters);

    // In cluster mode edges have to be aggregated onto the collapsed points.
    clusterSlotOfNode.assign ((size_t) juce::jmax (0, numNodes), -1);
    if (! collapseClusters || nodes == nullptr)
        return;

    for (int i = 0; i < numNodes; ++i)
        for (size_t p = 0; p < points.size(); ++p)
            if (points[p].cluster == nodes[i].cluster)
            {
                clusterSlotOfNode[(size_t) i] = (int) p;
                break;
            }
}

void TopologyView::paint (juce::Graphics& g)
{
    setSubtitle (collapseClusters ? "CLUSTER GRAPH" : "NODE GRAPH");
    LabPanel::paint (g);

    auto area = contentBoundsF().reduced (4.0f);
    if (points.empty())
    {
        plot::emptyState (g, area, numNodes > 0 ? "laying out" : "no nodes published");
        return;
    }

    const float size = juce::jmin (area.getWidth(), area.getHeight());
    const auto square = juce::Rectangle<float> (size, size).withCentre (area.getCentre());
    auto toPixels = [&] (const GraphNode& p)
    {
        return juce::Point<float> (square.getX() + p.x * square.getWidth(),
                                   square.getY() + p.y * square.getHeight());
    };

    // Frequency rings (the radius axis).
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    for (float r : { 0.13f, 0.30f, 0.48f })
        g.drawEllipse (juce::Rectangle<float> (size * r * 2.0f, size * r * 2.0f).withCentre (square.getCentre()), 1.0f);

    // Edges first so the nodes sit on top.
    int drawnEdges = 0;
    if (edges != nullptr && numEdges > 0)
    {
        float strongest = 0.0f;
        for (int i = 0; i < numEdges; ++i)
            if (std::isfinite (edges[i].strength))
                strongest = juce::jmax (strongest, std::abs (edges[i].strength));
        const float scale = strongest > 1.0e-9f ? 1.0f / strongest : 0.0f;

        for (int i = 0; i < numEdges; ++i)
        {
            const auto& e = edges[i];
            int a = e.from, b = e.to;
            if (collapseClusters)
            {
                if (a >= numNodes || b >= numNodes) continue;
                a = clusterSlotOfNode[(size_t) a];
                b = clusterSlotOfNode[(size_t) b];
                if (a < 0 || b < 0 || a == b) continue;
            }
            if (a < 0 || b < 0 || a >= (int) points.size() || b >= (int) points.size())
                continue;

            const float strength = juce::jlimit (0.0f, 1.0f, std::abs (e.strength) * scale);
            const auto p0 = toPixels (points[(size_t) a]);
            const auto p1 = toPixels (points[(size_t) b]);
            g.setColour (clusterColour (points[(size_t) a].cluster).withAlpha (0.10f + 0.55f * strength));
            g.drawLine (p0.x, p0.y, p1.x, p1.y, 0.7f + 1.1f * strength);
            ++drawnEdges;
        }
    }

    for (const auto& p : points)
    {
        const auto centre = toPixels (p);
        const float radius = 1.6f + 4.0f * p.weight + 2.5f * p.energy;
        const auto colour = clusterColour (p.cluster);
        const auto dot = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);

        if (p.energy > 0.02f)
            draw::glowEllipse (g, dot, colour, 4.0f + 10.0f * p.energy, p.energy);
        g.setColour (p.active ? colour : colour.withAlpha (0.28f));
        g.fillEllipse (dot);
    }

    auto footer = area.removeFromBottom (11.0f);
    plot::caption (g, juce::String (collapseClusters ? (int) points.size() : numNodes)
                      + (collapseClusters ? " CLUSTERS  " : " NODES  ") + juce::String (drawnEdges) + " EDGES",
                   footer, Theme::textDim, 8.5f);
    plot::caption (g, "RADIUS = LOG FREQ   ANGLE = CLUSTER", footer, Theme::textDim, 8.5f,
                   juce::Justification::centredRight);

    if (numEdges == 0)
        plot::caption (g, "NO COUPLING EDGES PUBLISHED", area.removeFromTop (11.0f), Theme::textDim, 8.5f,
                       juce::Justification::centredRight);
}

} // namespace am::dev

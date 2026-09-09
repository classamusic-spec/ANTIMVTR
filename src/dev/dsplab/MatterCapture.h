#pragma once

#include "dev/diagnostics/DiagnosticSnapshot.h"

#include <juce_core/juce_core.h>
#include <array>

namespace am::dev
{

/**
    A frozen copy of the Matter node / edge arrays of one diagnostic snapshot.

    The MATTER tab captures BEFORE and AFTER states so a change (an Evolve
    operator, a Shape move, a new topology seed) can be compared side by side;
    the EVOLVE tab reads the same two captures (§73, §75).
*/
struct MatterCapture
{
    bool         valid = false;
    juce::String label;
    juce::String taken;             ///< wall clock time the capture was taken

    int      numNodes     = 0;
    int      activeNodes  = 0;
    int      clusterCount = 0;
    int      numEdges     = 0;
    int      focusVoice   = -1;
    int      focusNote    = -1;
    uint32_t topologySeed = 0;
    float    fundamentalHz   = 0.0f;
    float    matterEnergy    = 0.0f;
    float    averageCoupling = 0.0f;
    float    maxCoupling     = 0.0f;
    uint8_t  materialA = 0, materialB = 0;
    float    materialBlend = 0.0f;

    std::array<NodeDiag, kMaxMatterNodes> nodes {};
    std::array<EdgeDiag, kMaxMatterEdges> edges {};

    void captureFrom (const DiagnosticSnapshot& s, juce::String captureLabel)
    {
        label = std::move (captureLabel);
        taken = juce::Time::getCurrentTime().formatted ("%H:%M:%S");
        numNodes = juce::jlimit (0, kMaxMatterNodes, s.numNodes);
        numEdges = juce::jlimit (0, kMaxMatterEdges, s.numEdges);
        activeNodes = s.activeNodes;
        clusterCount = s.clusterCount;
        focusVoice = s.focusVoice;
        focusNote = s.focusNote;
        topologySeed = s.topologySeed;
        fundamentalHz = s.fundamentalHz;
        matterEnergy = s.matterEnergy;
        averageCoupling = s.averageCoupling;
        maxCoupling = s.maxCoupling;
        materialA = s.materialA;
        materialB = s.materialB;
        materialBlend = s.materialBlend;
        for (int i = 0; i < numNodes; ++i) nodes[(size_t) i] = s.nodes[i];
        for (int i = 0; i < numEdges; ++i) edges[(size_t) i] = s.edges[i];
        valid = true;
    }

    void clear() { *this = MatterCapture(); }

    juce::String describe() const
    {
        if (! valid) return "empty";
        return label + "  " + taken + "   " + juce::String (numNodes) + " nodes / "
             + juce::String (numEdges) + " edges   " + juce::String (fundamentalHz, 1) + " Hz";
    }
};

/** The two capture slots shared by the MATTER and EVOLVE tabs. */
struct MatterCaptureStore
{
    MatterCapture before, after;
};

} // namespace am::dev

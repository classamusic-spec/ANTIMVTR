#include "MatterEngine.h"
#include "core/RealtimeUtils.h"

namespace am
{

void MatterEngine::prepare (double sampleRate, int)
{
    sr = sampleRate;
    reset();
}

void MatterEngine::reset()
{
    for (auto& n : nodes) n = Node();
    nodeCount = 0;
    clusters = 0;
    currentEnergy = 0.0f;
    gate = false;
}

void MatterEngine::noteOn (const NoteState& note, const ParamValues& params, Quality quality)
{
    // Phase 0: keep a trivial single-node description so diagnostics and the
    // visualizer have something truthful to display. The Phase 5 modal
    // implementation replaces this with the material-driven distribution.
    gate = true;
    seed = (uint32_t) std::lround (paramValue (params, Param::shapeSeed));
    nodeCount = 1;
    clusters = 1;
    auto& n = nodes[0];
    n = Node();
    n.frequency = n.targetFrequency = (float) note.frequency;
    n.ratio = 1.0f;
    n.weight = 1.0f;
    n.active = true;
    juce::ignoreUnused (quality);
}

void MatterEngine::noteOff()
{
    gate = false;
}

void MatterEngine::process (const float* excL, const float* excR, float* outL, float* outR, int n,
                            const RenderContext& ctx, const NoteState& note)
{
    // Pass-through: the "material" is transparent until the modal engine lands.
    juce::FloatVectorOperations::copy (outL, excL, n);
    juce::FloatVectorOperations::copy (outR, excR, n);

    LevelMeter meter;
    meter.measureStereo (outL, outR, n);
    currentEnergy = meter.rms;

    if (nodeCount > 0)
    {
        auto& node = nodes[0];
        node.frequency = node.targetFrequency = (float) note.frequency;
        node.energy = currentEnergy;
        node.active = gate || currentEnergy > kSilenceThreshold;
    }
    juce::ignoreUnused (ctx);
}

int MatterEngine::activeNodes() const noexcept
{
    int a = 0;
    for (int i = 0; i < nodeCount; ++i)
        if (nodes[(size_t) i].active) ++a;
    return a;
}

int MatterEngine::fillDiagnostics (NodeDiag* dest, int maxNodes) const noexcept
{
    const int count = std::min (nodeCount, maxNodes);
    for (int i = 0; i < count; ++i)
    {
        const auto& s = nodes[(size_t) i];
        auto& d = dest[i];
        d.frequency = s.frequency; d.targetFrequency = s.targetFrequency;
        d.energy = s.energy; d.weight = s.weight; d.damping = s.damping;
        d.pan = s.pan; d.nonlinearity = s.nonlinearity; d.excitation = s.excitation;
        d.cluster = s.cluster; d.couplingCount = s.couplingCount; d.active = s.active ? 1 : 0;
    }
    return count;
}

} // namespace am

#include "ControlGraph.h"

namespace am
{

ControlGraph::ControlGraph()
{
    ParameterRegistry::fillDefaults (effective);
    lastBase = effective;
    smoothed = effective;
    modulation.fill (0.0f);
    lastModulation.fill (0.0f);
}

void ControlGraph::prepare (double sampleRate, int)
{
    sr = sampleRate;
    for (const auto& d : ParameterRegistry::all())
    {
        const float ms = ParameterRegistry::smoothingMs (d.smoothing);
        const size_t i = (size_t) paramIndex (d.param);
        coeffPerSample[i] = ms > 0.0f ? (float) std::exp (-1.0 / (sampleRate * (double) ms * 0.001)) : 0.0f;
    }
}

void ControlGraph::resetTo (const ParamValues& base)
{
    lastBase = base;
    smoothed = base;
    effective = base;
    modulation.fill (0.0f);
    lastModulation.fill (0.0f);
}

void ControlGraph::update (const ParamValues& base, int numSamples)
{
    const auto& table = ParameterRegistry::all();
    for (size_t i = 0; i < (size_t) kNumParams; ++i)
    {
        const auto& d = table[i];
        const float target = base[i];
        lastBase[i] = target;

        float v;
        if (coeffPerSample[i] > 0.0f)
        {
            const float k = std::pow (coeffPerSample[i], (float) numSamples);
            smoothed[i] = target + k * (smoothed[i] - target);
            if (std::abs (smoothed[i] - target) < 1.0e-6f) smoothed[i] = target;
            v = smoothed[i];
        }
        else
        {
            smoothed[i] = target;
            v = target;
        }

        const float mod = modulation[i];
        lastModulation[i] = mod;
        modulation[i] = 0.0f;
        v += mod;

        effective[i] = d.clampValue (v);
    }
}

} // namespace am

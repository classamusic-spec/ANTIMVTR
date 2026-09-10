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
    cachedBlockSize = -1;
}

void ControlGraph::resetTo (const ParamValues& base)
{
    ++gen;
    lastBase = base;
    smoothed = base;
    effective = base;
    modulation.fill (0.0f);
    lastModulation.fill (0.0f);
}

void ControlGraph::update (const ParamValues& base, int numSamples)
{
    const auto& table = ParameterRegistry::all();
    bool changed = false;

    // The per-slice decay factor only depends on the slice length: cache it so
    // control-rate modulation (slices of 64 samples) costs no pow() per parameter.
    if (numSamples != cachedBlockSize)
    {
        cachedBlockSize = numSamples;
        for (size_t i = 0; i < (size_t) kNumParams; ++i)
            blockCoeff[i] = coeffPerSample[i] > 0.0f ? std::pow (coeffPerSample[i], (float) numSamples) : 0.0f;
    }

    for (size_t i = 0; i < (size_t) kNumParams; ++i)
    {
        const auto& d = table[i];
        const float target = base[i];
        lastBase[i] = target;

        float v;
        if (coeffPerSample[i] > 0.0f)
        {
            const float k = blockCoeff[i];
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

        const float e = d.clampValue (v);
        changed |= (e != effective[i]);
        effective[i] = e;
    }

    if (changed) ++gen;
}

} // namespace am

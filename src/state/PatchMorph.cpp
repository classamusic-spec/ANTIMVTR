#include "PatchMorph.h"

#include <cmath>

namespace am
{

ParamValues PatchMorph::interpolate (const ParamValues& a, const ParamValues& b, float t) noexcept
{
    t = std::isfinite (t) ? std::clamp (t, 0.0f, 1.0f) : 0.0f;
    if (t <= 0.0f) return a;   // the ends are exact: the normalised round trip is not bit-exact on skewed ranges
    if (t >= 1.0f) return b;
    ParamValues out = a;
    for (const auto& d : ParameterRegistry::all())
    {
        const auto i = (size_t) paramIndex (d.param);
        const float va = d.clampValue (a[i]);
        const float vb = d.clampValue (b[i]);
        float v = va;
        switch (d.kind)
        {
            case ParamKind::Float:
            {
                const float na = d.toNormalised (va), nb = d.toNormalised (vb);
                v = d.fromNormalised (na + (nb - na) * t);
                break;
            }
            case ParamKind::Int:
                v = std::round (va + (vb - va) * t);
                break;
            case ParamKind::Choice:
            case ParamKind::Bool:
                v = t < 0.5f ? va : vb;
                break;
        }
        out[i] = d.clampValue (std::isfinite (v) ? v : va);
    }
    return out;
}

PatchState PatchMorph::interpolate (const PatchState& a, const PatchState& b, float t)
{
    t = std::isfinite (t) ? std::clamp (t, 0.0f, 1.0f) : 0.0f;
    PatchState out = t < 0.5f ? a : b;
    out.params = interpolate (a.params, b.params, t);
    if (t > 0.0f && t < 1.0f)
    {
        const int percent = (int) std::lround (t * 100.0f);
        out.meta.name = a.meta.name + " \xe2\x86\x94 " + b.meta.name + " " + juce::String (percent) + "%";
        for (const auto& tag : (t < 0.5f ? b : a).meta.tags)
            out.meta.tags.addIfNotAlreadyThere (tag);
    }
    return out;
}

} // namespace am

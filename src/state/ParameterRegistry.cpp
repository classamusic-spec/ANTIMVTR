#include "ParameterRegistry.h"

#include <unordered_map>
#include <string>

namespace am
{

namespace
{
    std::array<ParamDesc, kNumParams> buildTable()
    {
        std::array<ParamDesc, kNumParams> table {};
        int index = 0;

       #define AM_PARAM_ROW(e, idStr, nameStr, grp, kindV, mn, mx, df, skewV, unitStr, choicesStr, mod, mut, smooth) \
        { \
            ParamDesc d; \
            d.param = Param::e; d.id = idStr; d.name = nameStr; d.group = ParamGroup::grp; \
            d.kind = ParamKind::kindV; d.min = mn; d.max = mx; d.defaultValue = df; d.skew = skewV; \
            d.unit = unitStr; d.choices = choicesStr; d.modulatable = mod; \
            d.mutation = MutationCategory::mut; d.smoothing = SmoothingKind::smooth; \
            if (d.kind == ParamKind::Choice) { d.min = 0.0f; d.max = (float) (d.numChoices() - 1); } \
            table[(size_t) index++] = d; \
        }
        ANTIMATR_PARAMETER_LIST (AM_PARAM_ROW)
       #undef AM_PARAM_ROW

        return table;
    }

    const std::array<ParamDesc, kNumParams>& table()
    {
        static const auto t = buildTable();
        return t;
    }

    const std::unordered_map<std::string, Param>& idMap()
    {
        static const auto map = []
        {
            std::unordered_map<std::string, Param> m;
            for (const auto& d : table())
                m.emplace (d.id, d.param);
            return m;
        }();
        return map;
    }
}

//==============================================================================
int ParamDesc::numChoices() const noexcept
{
    if (kind != ParamKind::Choice) return 0;
    int n = 1;
    for (const char* c = choices; *c != 0; ++c)
        if (*c == '|') ++n;
    return n;
}

float ParamDesc::clampValue (float v) const noexcept
{
    if (! std::isfinite (v)) return defaultValue;
    v = juce::jlimit (min, max, v);
    if (isDiscrete()) v = std::round (v);
    return v;
}

float ParamDesc::toNormalised (float value) const noexcept
{
    if (max <= min) return 0.0f;
    const float proportion = juce::jlimit (0.0f, 1.0f, (value - min) / (max - min));
    if (skew == 1.0f || proportion <= 0.0f) return proportion;
    return std::pow (proportion, skew);
}

float ParamDesc::fromNormalised (float norm) const noexcept
{
    norm = juce::jlimit (0.0f, 1.0f, norm);
    const float proportion = (skew == 1.0f || norm <= 0.0f) ? norm : std::pow (norm, 1.0f / skew);
    return clampValue (min + (max - min) * proportion);
}

juce::String ParamDesc::formatValue (float value) const
{
    switch (kind)
    {
        case ParamKind::Bool:   return value >= 0.5f ? "ON" : "OFF";
        case ParamKind::Choice:
        {
            juce::StringArray items;
            items.addTokens (juce::String (choices), "|", "");
            const int i = juce::jlimit (0, items.size() - 1, (int) std::lround (value));
            return items.size() > 0 ? items[i] : juce::String (i);
        }
        case ParamKind::Int:
        {
            juce::String s ((int) std::lround (value));
            if (unit[0] != 0) s << " " << unit;
            return s;
        }
        case ParamKind::Float:
        default:
        {
            juce::String s;
            const float absMax = std::max (std::abs (min), std::abs (max));
            if (absMax >= 100.0f)      s = juce::String (value, 0);
            else if (absMax >= 10.0f)  s = juce::String (value, 1);
            else                       s = juce::String (value, 2);
            if (unit[0] != 0) s << " " << unit;
            return s;
        }
    }
}

//==============================================================================
const ParamDesc& ParameterRegistry::get (Param p) noexcept
{
    return table()[(size_t) paramIndex (p)];
}

const std::array<ParamDesc, kNumParams>& ParameterRegistry::all() noexcept
{
    return table();
}

std::optional<Param> ParameterRegistry::fromID (std::string_view id) noexcept
{
    const auto& m = idMap();
    const auto it = m.find (std::string (id));
    if (it == m.end()) return std::nullopt;
    return it->second;
}

void ParameterRegistry::fillDefaults (ParamValues& values) noexcept
{
    for (const auto& d : table())
        values[(size_t) paramIndex (d.param)] = d.defaultValue;
}

ParamValues ParameterRegistry::defaults() noexcept
{
    ParamValues v {};
    fillDefaults (v);
    return v;
}

std::vector<Param> ParameterRegistry::inGroup (ParamGroup g)
{
    std::vector<Param> out;
    for (const auto& d : table())
        if (d.group == g) out.push_back (d.param);
    return out;
}

const char* ParameterRegistry::groupName (ParamGroup g) noexcept
{
    switch (g)
    {
        case ParamGroup::Master:   return "Master";
        case ParamGroup::Amp:      return "Amp";
        case ParamGroup::Source:   return "Source";
        case ParamGroup::Wave:     return "Wave";
        case ParamGroup::Dust:     return "Dust";
        case ParamGroup::Impact:   return "Impact";
        case ParamGroup::Sample:   return "Sample";
        case ParamGroup::Gesture:  return "Gesture";
        case ParamGroup::Shape:    return "Shape";
        case ParamGroup::Evolve:   return "Evolve";
        case ParamGroup::Fracture: return "Fracture";
        case ParamGroup::Space:    return "Space";
        case ParamGroup::Mod:      return "Mod";
        case ParamGroup::Macro:    return "Macro";
        default:                   return "Unknown";
    }
}

const char* ParameterRegistry::mutationName (MutationCategory m) noexcept
{
    switch (m)
    {
        case MutationCategory::None:     return "None";
        case MutationCategory::Source:   return "Source";
        case MutationCategory::Shape:    return "Shape";
        case MutationCategory::Evolve:   return "Evolve";
        case MutationCategory::Fracture: return "Fracture";
        case MutationCategory::Movement: return "Movement";
        case MutationCategory::Space:    return "Space";
        case MutationCategory::Pitch:    return "Pitch";
        case MutationCategory::Chaos:    return "Chaos";
        default:                         return "Unknown";
    }
}

float ParameterRegistry::smoothingMs (SmoothingKind k) noexcept
{
    switch (k)
    {
        case SmoothingKind::Fast:   return 3.0f;
        case SmoothingKind::Medium: return 20.0f;
        case SmoothingKind::Slow:   return 80.0f;
        default:                    return 0.0f;
    }
}

juce::String ParameterRegistry::validate()
{
    juce::String problems;
    std::unordered_map<std::string, int> seen;
    int index = 0;

    for (const auto& d : table())
    {
        if (d.param != paramFromIndex (index))
            problems << "Enum/table mismatch at " << d.id << "\n";

        if (seen.count (d.id) > 0)
            problems << "Duplicate parameter id: " << d.id << "\n";
        seen[d.id] = 1;

        if (d.max < d.min)
            problems << "Inverted range: " << d.id << "\n";

        if (d.defaultValue < d.min || d.defaultValue > d.max)
            problems << "Default out of range: " << d.id << "\n";

        if (d.kind == ParamKind::Choice && d.numChoices() < 2)
            problems << "Choice with fewer than 2 options: " << d.id << "\n";

        if (d.skew <= 0.0f)
            problems << "Non-positive skew: " << d.id << "\n";

        ++index;
    }

    return problems;
}

} // namespace am

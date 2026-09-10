#include "ModRouting.h"

namespace am
{

namespace
{
    struct SourceInfo
    {
        const char*    id;
        const char*    name;
        ModSourceGroup group;
        bool           bipolar;
    };

    // Order must match the ModSource enum exactly.
    constexpr SourceInfo kSourceTable[] =
    {
        { "none",      "None",         ModSourceGroup::Note,     false },
        { "lfo1",      "LFO 1",        ModSourceGroup::LFO,      true  },
        { "lfo2",      "LFO 2",        ModSourceGroup::LFO,      true  },
        { "lfo3",      "LFO 3",        ModSourceGroup::LFO,      true  },
        { "lfo4",      "LFO 4",        ModSourceGroup::LFO,      true  },
        { "env1",      "Env 1",        ModSourceGroup::Envelope, false },
        { "env2",      "Env 2",        ModSourceGroup::Envelope, false },
        { "env3",      "Env 3",        ModSourceGroup::Envelope, false },
        { "env4",      "Env 4",        ModSourceGroup::Envelope, false },
        { "chaos1",    "Chaos 1",      ModSourceGroup::Chaos,    true  },
        { "chaos2",    "Chaos 2",      ModSourceGroup::Chaos,    true  },
        { "chaos3",    "Chaos 3",      ModSourceGroup::Chaos,    true  },
        { "chaos4",    "Chaos 4",      ModSourceGroup::Chaos,    true  },
        { "macro1",    "Macro 1",      ModSourceGroup::Macro,    false },
        { "macro2",    "Macro 2",      ModSourceGroup::Macro,    false },
        { "macro3",    "Macro 3",      ModSourceGroup::Macro,    false },
        { "macro4",    "Macro 4",      ModSourceGroup::Macro,    false },
        { "macro5",    "Macro 5",      ModSourceGroup::Macro,    false },
        { "macro6",    "Macro 6",      ModSourceGroup::Macro,    false },
        { "macro7",    "Macro 7",      ModSourceGroup::Macro,    false },
        { "macro8",    "Macro 8",      ModSourceGroup::Macro,    false },
        { "velocity",  "Velocity",     ModSourceGroup::Note,     false },
        { "keytrack",  "Key Track",    ModSourceGroup::Note,     true  },
        { "pressure",  "Pressure",     ModSourceGroup::Note,     false },
        { "modwheel",  "Mod Wheel",    ModSourceGroup::Note,     false },
        { "pitchbend", "Pitch Bend",   ModSourceGroup::Note,     true  },
        { "timbre",    "Timbre",       ModSourceGroup::Note,     false },
        { "noterandom","Note Random",  ModSourceGroup::Note,     true  },
        { "gate",      "Gate",         ModSourceGroup::Note,     false },
    };

    static_assert (sizeof (kSourceTable) / sizeof (kSourceTable[0]) == (size_t) kNumModSources,
                   "ModSource table out of sync with the enum");

    inline const SourceInfo& info (ModSource s) noexcept
    {
        const int i = (int) s;
        return kSourceTable[(size_t) (i >= 0 && i < kNumModSources ? i : 0)];
    }

    /** The retrigger parameter of an LFO slot (0-based). */
    inline Param lfoRetrigParam (int slot) noexcept
    {
        switch (slot)
        {
            case 0:  return Param::lfo1Retrig;
            case 1:  return Param::lfo2Retrig;
            case 2:  return Param::lfo3Retrig;
            default: return Param::lfo4Retrig;
        }
    }
}

//==============================================================================
const char* modSourceId (ModSource s) noexcept   { return info (s).id; }
const char* modSourceName (ModSource s) noexcept { return info (s).name; }
ModSourceGroup modSourceGroup (ModSource s) noexcept { return info (s).group; }
bool modSourceIsBipolar (ModSource s) noexcept   { return info (s).bipolar; }

const char* modSourceGroupName (ModSourceGroup g) noexcept
{
    switch (g)
    {
        case ModSourceGroup::LFO:      return "LFO";
        case ModSourceGroup::Envelope: return "ENVELOPE";
        case ModSourceGroup::Chaos:    return "CHAOS";
        case ModSourceGroup::Macro:    return "MACRO";
        case ModSourceGroup::Note:     return "NOTE";
        default:                       return "";
    }
}

ModSource modSourceFromId (juce::StringRef id) noexcept
{
    for (int i = 1; i < kNumModSources; ++i)
        if (id.text.compare (juce::CharPointer_UTF8 (kSourceTable[(size_t) i].id)) == 0)
            return (ModSource) i;
    return ModSource::None;
}

int modSourceSlot (ModSource s) noexcept
{
    const int i = (int) s;
    if (i >= (int) ModSource::LFO1   && i <= (int) ModSource::LFO4)   return i - (int) ModSource::LFO1;
    if (i >= (int) ModSource::Env1   && i <= (int) ModSource::Env4)   return i - (int) ModSource::Env1;
    if (i >= (int) ModSource::Chaos1 && i <= (int) ModSource::Chaos4) return i - (int) ModSource::Chaos1;
    if (i >= (int) ModSource::Macro1 && i <= (int) ModSource::Macro8) return i - (int) ModSource::Macro1;
    return -1;
}

bool modSourceIsPerVoice (ModSource s, const ParamValues& params) noexcept
{
    switch (modSourceGroup (s))
    {
        case ModSourceGroup::Envelope: return true;
        case ModSourceGroup::Note:     return s != ModSource::None;
        case ModSourceGroup::LFO:      return paramBool (params, lfoRetrigParam (modSourceSlot (s)));
        default:                       return false;   // chaos + macros are global
    }
}

//==============================================================================
bool ModRoutingTable::isValid (const ModRouting& r) noexcept
{
    if (r.source == ModSource::None || (int) r.source >= kNumModSources) return false;
    if ((int) r.target < 0 || (int) r.target >= kNumParams) return false;
    if (! ParameterRegistry::get (r.target).modulatable) return false;
    if (! std::isfinite (r.depth) || ! std::isfinite (r.curve)) return false;
    return true;
}

ModRouting ModRoutingTable::sanitised (ModRouting r) noexcept
{
    r.depth = std::isfinite (r.depth) ? juce::jlimit (-1.0f, 1.0f, r.depth) : 0.0f;
    r.curve = std::isfinite (r.curve) ? juce::jlimit (-1.0f, 1.0f, r.curve) : 0.0f;
    return r;
}

int ModRoutingTable::indexOf (ModSource s, Param target) const noexcept
{
    for (int i = 0; i < count; ++i)
        if (routings[(size_t) i].source == s && routings[(size_t) i].target == target)
            return i;
    return -1;
}

int ModRoutingTable::countForTarget (Param target) const noexcept
{
    int n = 0;
    for (int i = 0; i < count; ++i)
        if (routings[(size_t) i].target == target && routings[(size_t) i].enabled) ++n;
    return n;
}

int ModRoutingTable::numEnabled() const noexcept
{
    int n = 0;
    for (int i = 0; i < count; ++i)
        if (routings[(size_t) i].enabled) ++n;
    return n;
}

void ModRoutingTable::sort() noexcept
{
    // Insertion sort: deterministic, allocation free and the table is tiny.
    for (int i = 1; i < count; ++i)
    {
        const ModRouting key = routings[(size_t) i];
        const auto less = [] (const ModRouting& a, const ModRouting& b) noexcept
        {
            if (a.source != b.source) return (int) a.source < (int) b.source;
            return (int) a.target < (int) b.target;
        };
        int j = i - 1;
        while (j >= 0 && less (key, routings[(size_t) j]))
        {
            routings[(size_t) (j + 1)] = routings[(size_t) j];
            --j;
        }
        routings[(size_t) (j + 1)] = key;
    }
}

int ModRoutingTable::add (const ModRouting& in) noexcept
{
    const ModRouting r = sanitised (in);
    if (! isValid (r) || isFull()) return -1;
    if (indexOf (r.source, r.target) >= 0) return -1;   // no duplicates
    routings[(size_t) count++] = r;
    sort();
    return indexOf (r.source, r.target);
}

bool ModRoutingTable::remove (int index) noexcept
{
    if (index < 0 || index >= count) return false;
    for (int i = index; i < count - 1; ++i) routings[(size_t) i] = routings[(size_t) (i + 1)];
    routings[(size_t) --count] = ModRouting{};
    return true;
}

bool ModRoutingTable::setDepth (int index, float depth) noexcept
{
    if (index < 0 || index >= count || ! std::isfinite (depth)) return false;
    routings[(size_t) index].depth = juce::jlimit (-1.0f, 1.0f, depth);
    return true;
}

bool ModRoutingTable::setCurve (int index, float curve) noexcept
{
    if (index < 0 || index >= count || ! std::isfinite (curve)) return false;
    routings[(size_t) index].curve = juce::jlimit (-1.0f, 1.0f, curve);
    return true;
}

bool ModRoutingTable::setBipolar (int index, bool b) noexcept
{
    if (index < 0 || index >= count) return false;
    routings[(size_t) index].bipolar = b;
    return true;
}

bool ModRoutingTable::setEnabled (int index, bool b) noexcept
{
    if (index < 0 || index >= count) return false;
    routings[(size_t) index].enabled = b;
    return true;
}

bool ModRoutingTable::operator== (const ModRoutingTable& o) const noexcept
{
    if (count != o.count) return false;
    for (int i = 0; i < count; ++i)
        if (routings[(size_t) i] != o.routings[(size_t) i]) return false;
    return true;
}

//==============================================================================
juce::var ModRoutingTable::toVar() const
{
    juce::Array<juce::var> list;
    for (int i = 0; i < count; ++i)
    {
        const auto& r = routings[(size_t) i];
        auto* o = new juce::DynamicObject();
        o->setProperty ("src", juce::String (modSourceId (r.source)));
        o->setProperty ("dst", juce::String (ParameterRegistry::get (r.target).id));
        o->setProperty ("depth", r.depth);
        o->setProperty ("curve", r.curve);
        o->setProperty ("bipolar", r.bipolar);
        o->setProperty ("on", r.enabled);
        list.add (juce::var (o));
    }

    auto* root = new juce::DynamicObject();
    root->setProperty ("routings", list);
    return juce::var (root);
}

ModRoutingTable ModRoutingTable::fromVar (const juce::var& v, juce::String* warnings)
{
    ModRoutingTable table;
    const auto note = [warnings] (const juce::String& text)
    {
        if (warnings != nullptr) *warnings << text << "\n";
    };

    const juce::var* listVar = nullptr;
    if (auto* obj = v.getDynamicObject())
    {
        if (obj->hasProperty ("routings")) listVar = &obj->getProperty ("routings");
    }
    else if (v.isArray())
    {
        listVar = &v;
    }

    if (listVar == nullptr || ! listVar->isArray()) return table;

    for (const auto& item : *listVar->getArray())
    {
        auto* o = item.getDynamicObject();
        if (o == nullptr) { note ("mod routing: not an object"); continue; }

        ModRouting r;
        const auto srcId = o->getProperty ("src").toString();
        r.source = modSourceFromId (srcId);
        if (r.source == ModSource::None) { note ("mod routing: unknown source '" + srcId + "'"); continue; }

        const auto dstId = o->getProperty ("dst").toString();
        const auto p = ParameterRegistry::fromID (dstId.toStdString());
        if (! p.has_value()) { note ("mod routing: unknown target '" + dstId + "'"); continue; }
        r.target = *p;

        r.depth   = (float) (double) o->getProperty ("depth");
        r.curve   = o->hasProperty ("curve") ? (float) (double) o->getProperty ("curve") : 0.0f;
        r.bipolar = o->hasProperty ("bipolar") ? (bool) o->getProperty ("bipolar") : true;
        r.enabled = o->hasProperty ("on") ? (bool) o->getProperty ("on") : true;

        if (! isValid (r)) { note ("mod routing: '" + dstId + "' is not modulatable"); continue; }
        if (table.add (r) < 0) note ("mod routing: '" + srcId + " -> " + dstId + "' dropped (duplicate or table full)");
    }
    return table;
}

} // namespace am

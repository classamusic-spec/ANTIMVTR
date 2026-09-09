#include "StateManager.h"

namespace am
{

namespace
{
    juce::var paramsToVar (const ParamValues& values)
    {
        auto* obj = new juce::DynamicObject();
        for (const auto& d : ParameterRegistry::all())
        {
            const float v = values[(size_t) paramIndex (d.param)];
            if (d.isDiscrete())
                obj->setProperty (d.id, (int) std::lround (v));
            else
                obj->setProperty (d.id, (double) v);
        }
        return juce::var (obj);
    }

    void varToParams (const juce::var& v, ParamValues& values, juce::String* warnings)
    {
        ParameterRegistry::fillDefaults (values);
        auto* obj = v.getDynamicObject();
        if (obj == nullptr) return;

        for (const auto& prop : obj->getProperties())
        {
            const auto id = prop.name.toString();
            const auto p = ParameterRegistry::fromID (id.toStdString());
            if (! p.has_value())
            {
                if (warnings != nullptr) *warnings << "Unknown parameter: " << id << "\n";
                continue;
            }
            const auto& d = ParameterRegistry::get (*p);
            values[(size_t) paramIndex (*p)] = d.clampValue ((float) (double) prop.value);
        }
    }

    juce::var stringArrayToVar (const juce::StringArray& a)
    {
        juce::Array<juce::var> arr;
        for (const auto& s : a) arr.add (s);
        return juce::var (arr);
    }

    juce::StringArray varToStringArray (const juce::var& v)
    {
        juce::StringArray out;
        if (auto* arr = v.getArray())
            for (const auto& item : *arr) out.add (item.toString());
        return out;
    }
}

//==============================================================================
juce::var StateManager::toVar (const PatchState& s)
{
    auto* root = new juce::DynamicObject();

    auto* meta = new juce::DynamicObject();
    meta->setProperty ("schema",   PatchState::kSchemaVersion);
    meta->setProperty ("plugin",   s.meta.pluginVersion.isNotEmpty() ? s.meta.pluginVersion : juce::String (ANTIMATR_VERSION_STRING));
    meta->setProperty ("name",     s.meta.name);
    meta->setProperty ("author",   s.meta.author);
    meta->setProperty ("category", s.meta.category);
    meta->setProperty ("tags",     stringArrayToVar (s.meta.tags));
    meta->setProperty ("comment",  s.meta.comment);

    root->setProperty ("format",   "ANTI-MATR");
    root->setProperty ("meta",     juce::var (meta));
    root->setProperty ("params",   paramsToVar (s.params));

    auto setIfValid = [root] (const char* key, const juce::var& v) { if (! v.isVoid()) root->setProperty (key, v); };
    setIfValid ("matter",   s.matter);
    setIfValid ("fracture", s.fracture);
    setIfValid ("mod",      s.mod);
    setIfValid ("space",    s.space);
    setIfValid ("sample",   s.sample);
    setIfValid ("ab",       s.ab);
    setIfValid ("dna",      s.dna);
    setIfValid ("seeds",    s.seeds);
    setIfValid ("extra",    s.extra);
    setIfValid ("ui",       s.ui);

    return juce::var (root);
}

juce::String StateManager::toJson (const PatchState& state, bool pretty)
{
    return juce::JSON::toString (toVar (state), ! pretty);
}

int StateManager::migrate (juce::DynamicObject& root, int fromVersion)
{
    int version = fromVersion;

    // Future migrations go here, each bumping `version` by one:
    // if (version == 1) { ... transform ... ; version = 2; }

    juce::ignoreUnused (root);
    return version;
}

bool StateManager::fromVar (const juce::var& v, PatchState& out, juce::String* warnings)
{
    auto* root = v.getDynamicObject();
    if (root == nullptr) return false;

    if (root->getProperty ("format").toString() != "ANTI-MATR")
    {
        if (warnings != nullptr) *warnings << "Not an ANTI-MATR document\n";
        return false;
    }

    PatchState s;
    const auto metaVar = root->getProperty ("meta");
    int schema = 1;
    if (auto* meta = metaVar.getDynamicObject())
    {
        schema               = (int) meta->getProperty ("schema");
        s.meta.pluginVersion = meta->getProperty ("plugin").toString();
        s.meta.name          = meta->getProperty ("name").toString();
        s.meta.author        = meta->getProperty ("author").toString();
        s.meta.category      = meta->getProperty ("category").toString();
        s.meta.tags          = varToStringArray (meta->getProperty ("tags"));
        s.meta.comment       = meta->getProperty ("comment").toString();
    }
    if (schema < 1) schema = 1;

    if (schema > PatchState::kSchemaVersion)
    {
        if (warnings != nullptr)
            *warnings << "Document schema " << schema << " is newer than supported " << PatchState::kSchemaVersion << "; loading best effort\n";
    }
    else if (schema < PatchState::kSchemaVersion)
    {
        migrate (*root, schema);
    }
    s.meta.schemaVersion = PatchState::kSchemaVersion;

    varToParams (root->getProperty ("params"), s.params, warnings);

    s.matter   = root->getProperty ("matter");
    s.fracture = root->getProperty ("fracture");
    s.mod      = root->getProperty ("mod");
    s.space    = root->getProperty ("space");
    s.sample   = root->getProperty ("sample");
    s.ab       = root->getProperty ("ab");
    s.dna      = root->getProperty ("dna");
    s.seeds    = root->getProperty ("seeds");
    s.extra    = root->getProperty ("extra");
    s.ui       = root->getProperty ("ui");

    out = std::move (s);
    return true;
}

bool StateManager::fromJson (const juce::String& json, PatchState& out, juce::String* warnings)
{
    juce::var parsed;
    const auto result = juce::JSON::parse (json, parsed);
    if (result.failed())
    {
        if (warnings != nullptr) *warnings << "JSON parse error: " << result.getErrorMessage() << "\n";
        return false;
    }
    return fromVar (parsed, out, warnings);
}

juce::MemoryBlock StateManager::toBinary (const PatchState& state)
{
    juce::MemoryOutputStream mos;
    mos.writeString (toJson (state, false));
    return mos.getMemoryBlock();
}

bool StateManager::fromBinary (const void* data, size_t size, PatchState& out, juce::String* warnings)
{
    if (data == nullptr || size == 0) return false;
    juce::MemoryInputStream mis (data, size, false);
    const auto json = mis.readString();
    return fromJson (json, out, warnings);
}

juce::ValueTree StateManager::toParameterTree (const ParamValues& values, const juce::Identifier& rootType)
{
    juce::ValueTree tree (rootType);
    for (const auto& d : ParameterRegistry::all())
    {
        juce::ValueTree p ("PARAM");
        p.setProperty ("id", juce::String (d.id), nullptr);
        p.setProperty ("value", (double) values[(size_t) paramIndex (d.param)], nullptr);
        tree.appendChild (p, nullptr);
    }
    return tree;
}

void StateManager::fromParameterTree (const juce::ValueTree& tree, ParamValues& values)
{
    ParameterRegistry::fillDefaults (values);
    for (const auto& child : tree)
    {
        if (! child.hasType ("PARAM")) continue;
        const auto p = ParameterRegistry::fromID (child.getProperty ("id").toString().toStdString());
        if (! p.has_value()) continue;
        const auto& d = ParameterRegistry::get (*p);
        values[(size_t) paramIndex (*p)] = d.clampValue ((float) (double) child.getProperty ("value"));
    }
}

} // namespace am

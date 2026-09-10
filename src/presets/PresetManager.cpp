#include "PresetManager.h"
#include "FactoryContent.h"
#include "dsp/fx/SpacePresets.h"

namespace am
{

PresetManager::PresetManager()
{
    registerBuiltIns();
}

PatchState PresetManager::initPatch()
{
    PatchState s;
    s.meta.name = "Init";
    s.meta.category = "INIT";
    SpacePresets::apply (paramChoice (s.params, Param::spaceType), s.params);
    return s;
}

void PresetManager::addFactory (FactoryPreset preset)
{
    factory.push_back (std::move (preset));
}

void PresetManager::registerBuiltIns()
{
    // "Init" is the blank object every other patch is a departure from; the
    // reference library itself lives in FactoryContent.cpp.
    addFactory ({ "Init", "INIT", { "basic" }, [] (PatchState& s) { s = initPatch(); } });
    FactoryContent::registerAll (*this);
}

PatchState PresetManager::buildFactory (int index) const
{
    PatchState s = initPatch();
    if (index >= 0 && index < (int) factory.size())
    {
        const auto& f = factory[(size_t) index];
        const auto beforeBuild = s.params;
        f.build (s);

        // Every factory patch carries the curated rack of its Space type unless
        // the builder set the rack itself: the curated values are applied first
        // and then the individual rack parameters the builder touched are put
        // back, so a patch can lean on a Space and still tighten one module.
        const auto afterBuild = s.params;
        SpacePresets::apply (paramChoice (s.params, Param::spaceType), s.params);
        for (const auto& d : ParameterRegistry::all())
        {
            if (d.group != ParamGroup::Space) continue;
            const size_t i = (size_t) paramIndex (d.param);
            if (afterBuild[i] != beforeBuild[i])
                s.params[i] = afterBuild[i];
        }

        s.meta.name = f.name;
        s.meta.category = f.category;
        s.meta.tags = f.tags;
        s.meta.author = "ANTI-MATR";
    }
    return s;
}

int PresetManager::findFactory (const juce::String& name) const
{
    for (int i = 0; i < (int) factory.size(); ++i)
        if (factory[(size_t) i].name.equalsIgnoreCase (name))
            return i;
    return -1;
}

juce::File PresetManager::userPresetDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
            .getChildFile ("ANTI-MATR").getChildFile ("Presets");
}

juce::Array<juce::File> PresetManager::userPresetFiles() const
{
    juce::Array<juce::File> files;
    const auto dir = userPresetDirectory();
    if (dir.isDirectory())
        files = dir.findChildFiles (juce::File::findFiles, true, "*.antimatr.json");
    return files;
}

bool PresetManager::saveUserPreset (const PatchState& state, const juce::File& file, juce::String* error) const
{
    if (! file.getParentDirectory().createDirectory())
    {
        if (error != nullptr) *error = "Could not create " + file.getParentDirectory().getFullPathName();
        return false;
    }
    PatchState toSave = state;
    toSave.ui = juce::var();
    if (! file.replaceWithText (StateManager::toJson (toSave, true)))
    {
        if (error != nullptr) *error = "Could not write " + file.getFullPathName();
        return false;
    }
    return true;
}

bool PresetManager::loadUserPreset (const juce::File& file, PatchState& out, juce::String* error) const
{
    if (! file.existsAsFile())
    {
        if (error != nullptr) *error = "Missing file " + file.getFullPathName();
        return false;
    }
    juce::String warnings;
    if (! StateManager::fromJson (file.loadFileAsString(), out, &warnings))
    {
        if (error != nullptr) *error = warnings;
        return false;
    }
    return true;
}

} // namespace am

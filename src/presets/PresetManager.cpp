#include "PresetManager.h"
#include "dsp/fx/SpacePresets.h"

namespace am
{

namespace
{
    inline void set (PatchState& s, Param p, float v) { s.params[(size_t) paramIndex (p)] = ParameterRegistry::get (p).clampValue (v); }
}

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
    // Phase 0 reference patches. The full factory library is generated in
    // src/presets/FactoryContent.cpp in later phases; these ensure the
    // preset path works end to end from the very first build.
    addFactory ({ "Init", "INIT", { "basic" }, [] (PatchState& s) { s = initPatch(); } });

    addFactory ({ "Void Bloom", "PAD", { "pad", "evolving", "cinematic" }, [] (PatchState& s)
    {
        s.meta.name = "Void Bloom"; s.meta.category = "PAD"; s.meta.tags = { "pad", "evolving", "cinematic" };
        set (s, Param::ampAttack, 0.8f);  set (s, Param::ampRelease, 2.5f);
        set (s, Param::waveUnison, 4);    set (s, Param::waveDetune, 0.22f); set (s, Param::waveSpread, 0.8f);
        set (s, Param::shapeDensity, 0.65f); set (s, Param::shapeForm, 0.35f); set (s, Param::shapeMass, 0.55f);
        set (s, Param::shapeTension, 0.4f);  set (s, Param::shapeDecay, 0.7f); set (s, Param::shapeSurface, 0.25f);
        set (s, Param::evolveMelt, 0.3f);    set (s, Param::evolveMagnet, 0.4f);
        set (s, Param::spaceType, 0); set (s, Param::spaceMix, 0.45f); set (s, Param::spaceSize, 0.7f);
    }});

    addFactory ({ "Carbon Bass", "BASS", { "bass", "organic" }, [] (PatchState& s)
    {
        s.meta.name = "Carbon Bass"; s.meta.category = "BASS"; s.meta.tags = { "bass", "organic" };
        set (s, Param::ampAttack, 0.002f); set (s, Param::ampDecay, 0.4f); set (s, Param::ampSustain, 0.7f); set (s, Param::ampRelease, 0.15f);
        set (s, Param::waveOctave, -1);   set (s, Param::shapeDensity, 0.3f); set (s, Param::shapeForm, 0.15f);
        set (s, Param::shapeMass, 0.8f);   set (s, Param::shapeTension, 0.35f); set (s, Param::shapeDecay, 0.35f);
        set (s, Param::shapeSurface, 0.45f); set (s, Param::masterMode, 2);    set (s, Param::masterGlide, 0.08f);
        set (s, Param::spaceType, 2); set (s, Param::spaceMix, 0.12f);
    }});

    addFactory ({ "Crystal Ghost", "KEYS", { "keys", "bells", "crystal" }, [] (PatchState& s)
    {
        s.meta.name = "Crystal Ghost"; s.meta.category = "KEYS"; s.meta.tags = { "keys", "bells", "crystal" };
        set (s, Param::sourceSelected, 2); // IMPACT
        set (s, Param::ampAttack, 0.001f); set (s, Param::ampDecay, 1.5f); set (s, Param::ampSustain, 0.0f); set (s, Param::ampRelease, 1.2f);
        set (s, Param::shapeMaterialA, 0); set (s, Param::shapeDensity, 0.45f); set (s, Param::shapeForm, 0.75f);
        set (s, Param::shapeMass, 0.25f);  set (s, Param::shapeTension, 0.7f); set (s, Param::shapeDecay, 0.85f);
        set (s, Param::shapeSurface, 0.1f); set (s, Param::evolveMagnet, 0.6f);
        set (s, Param::spaceType, 6); set (s, Param::spaceMix, 0.4f);
    }});
}

PatchState PresetManager::buildFactory (int index) const
{
    PatchState s = initPatch();
    if (index >= 0 && index < (int) factory.size())
    {
        const auto& f = factory[(size_t) index];
        f.build (s);
        // Every factory patch carries the curated rack of its Space type unless the builder set the rack itself.
        SpacePresets::apply (paramChoice (s.params, Param::spaceType), s.params);
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

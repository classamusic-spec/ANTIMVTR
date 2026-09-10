#include <juce_core/juce_core.h>

#include "dev/diagnostics/PresetValidator.h"
#include "dsp/SynthEngine.h"
#include "dsp/fracture/Fragment.h"
#include "presets/FactoryContent.h"
#include "presets/PresetManager.h"
#include "state/ModRouting.h"
#include "state/MutationEngine.h"
#include "state/PatchMorph.h"
#include "state/StateManager.h"

using namespace am;
using namespace am::dev;

namespace
{
    /** Everything the factory bank promises about one rendered preset. */
    juce::StringArray judge (const PresetValidationResult& r, const juce::String& category)
    {
        juce::StringArray problems = r.issues;
        const auto& spec = FactoryContent::categorySpec (category);

        if (r.rms < spec.minRms)
            problems.add ("RMS " + juce::String (r.rms, 5) + " below the " + juce::String (spec.category)
                          + " floor " + juce::String (spec.minRms, 5));
        if (r.rms > spec.maxRms)
            problems.add ("RMS " + juce::String (r.rms, 5) + " above the " + juce::String (spec.category)
                          + " ceiling " + juce::String (spec.maxRms, 5));
        if (r.peak < spec.minPeak)
            problems.add ("Peak " + juce::String (r.peak, 4) + " below the " + juce::String (spec.category)
                          + " floor " + juce::String (spec.minPeak, 4));
        if (r.peak > spec.maxPeak)
            problems.add ("Peak " + juce::String (r.peak, 4) + " above the " + juce::String (spec.category)
                          + " ceiling " + juce::String (spec.maxPeak, 4));
        return problems;
    }

    /** A short render: enough to catch silence, clipping, NaN and instability. */
    PresetValidatorOptions quickOptions()
    {
        PresetValidatorOptions o;
        o.holdSeconds = 0.6;
        o.releaseSeconds = 0.5;
        o.blockSize = 128;
        return o;
    }

    /** Renders a mutated patch through a private engine (the validator path, without the registry). */
    struct MutationRender
    {
        float peak = 0.0f, rms = 0.0f, dc = 0.0f;
        int   nonFinite = 0;
        uint32_t safety = 0;
    };

    MutationRender renderPatch (SynthEngine& engine, const PatchState& patch,
                                const PresetValidatorOptions& options, int midiNote, bool prepared = false)
    {
        MutationRender out;
        const double sr = options.sampleRate;
        const int blockSize = options.blockSize;

        // Hold the note long enough for its own attack: a two second swell is
        // not silent, it is slow, and the window has to be able to tell them apart.
        const double hold = juce::jmax (options.holdSeconds,
                                        (double) paramValue (patch.params, Param::ampAttack) + 0.30);
        const int holdSamples = (int) (hold * sr);
        const int totalSamples = holdSamples + (int) (options.releaseSeconds * sr);

        if (! prepared) engine.prepare (sr, blockSize);
        engine.reset();
        engine.control().resetTo (patch.params);
        {
            auto table = patch.fracture.isVoid() ? FractureTable::makeDefault() : FractureTable::fromVar (patch.fracture);
            engine.fractureEngine().publishTable (std::make_unique<FractureTable> (table));
            auto routings = patch.mod.isVoid() ? ModRoutingTable() : ModRoutingTable::fromVar (patch.mod);
            engine.modulationEngine().publishRoutings (std::make_unique<ModRoutingTable> (routings));
        }
        auto& diag = engine.diagnostics();
        diag.safety.reset();
        diag.events.drain ([] (const EngineEvent&) {});

        juce::AudioBuffer<float> block (2, blockSize);
        const TransportInfo transport;
        double sum = 0.0, dcSum = 0.0;
        int counted = 0, position = 0;
        bool noteSent = false, releaseSent = false;

        while (position < totalSamples)
        {
            const int n = std::min (blockSize, totalSamples - position);
            block.setSize (2, n, false, false, true);
            block.clear();

            juce::MidiBuffer midi;
            if (! noteSent)                                            { midi.addEvent (juce::MidiMessage::noteOn (1, midiNote, 0.85f), 0); noteSent = true; }
            else if (! releaseSent && position + n > holdSamples)       { midi.addEvent (juce::MidiMessage::noteOff (1, midiNote), juce::jlimit (0, n - 1, holdSamples - position)); releaseSent = true; }

            engine.process (block, midi, patch.params, transport);

            const float* l = block.getReadPointer (0);
            const float* r = block.getReadPointer (1);
            for (int i = 0; i < n; ++i)
            {
                if (! std::isfinite (l[i]) || ! std::isfinite (r[i])) { ++out.nonFinite; continue; }
                out.peak = juce::jmax (out.peak, std::abs (l[i]), std::abs (r[i]));
                const double m = 0.5 * ((double) l[i] + (double) r[i]);
                sum += m * m;
                dcSum += m;
                ++counted;
            }
            position += n;
        }

        out.rms = (float) std::sqrt (sum / juce::jmax (1, counted));
        out.dc = (float) (dcSum / juce::jmax (1, counted));
        out.safety = engine.diagnostics().safety.snapshot().total;
        return out;
    }
}

//==============================================================================
class FactoryContentTests : public juce::UnitTest
{
public:
    FactoryContentTests() : juce::UnitTest ("Factory content", "factory") {}

    void runTest() override
    {
        PresetManager presets;
        const int count = presets.numFactoryPresets();

        beginTest ("the bank covers the categories the spec names");
        {
            expect (count >= 31, "at least 30 reference patches plus Init, got " + juce::String (count));

            juce::StringArray names;
            std::map<juce::String, int> perCategory;
            for (int i = 0; i < count; ++i)
            {
                const auto& f = presets.factoryPreset (i);
                expect (f.name.isNotEmpty(), "preset " + juce::String (i) + " has no name");
                expect (! names.contains (f.name, true), "duplicate preset name: " + f.name);
                names.add (f.name);

                expect (FactoryContent::isKnownCategory (f.category), f.name + " declares unknown category " + f.category);
                if (f.category != "INIT")
                    expect (f.tags.size() >= 2, f.name + " needs tags that describe the sound");
                ++perCategory[f.category];
            }

            for (const auto& spec : FactoryContent::categories())
            {
                const juce::String category (spec.category);
                if (category == "INIT") continue;
                expect (perCategory[category] >= 2, "category " + category + " needs at least two patches");
            }

            // The reference patches the spec calls out by name.
            const char* required[] = { "Void Bloom", "Liquid Teeth", "Carbon Bass", "Crystal Ghost", "Broken Choir",
                                       "Titanium Skin", "Gravity Drone", "Glass Creature", "Membrane Sky",
                                       "Fractured Voice", "Impossible String", "Metal Bloom", "Frozen Machine",
                                       "Electric Organism", "Dust Piano", "Nebula Pad", "Torn Bass", "Magnet Bells",
                                       "Organic Circuit", "Anti-String" };
            for (const auto* name : required)
                expect (presets.findFactory (name) >= 0, juce::String ("missing reference patch: ") + name);
        }

        beginTest ("every preset builds deterministically and stays in range");
        {
            FactoryContent::resetRejectedRoutings();
            for (int i = 0; i < count; ++i)
            {
                const auto a = presets.buildFactory (i);
                const auto b = presets.buildFactory (i);
                expect (a.params == b.params, a.meta.name + " does not build identically twice");
                expectEquals (StateManager::toJson (a, false), StateManager::toJson (b, false),
                              a.meta.name + " does not serialise identically twice");

                for (const auto& d : ParameterRegistry::all())
                {
                    const float v = a.params[(size_t) paramIndex (d.param)];
                    expect (std::isfinite (v), a.meta.name + " " + d.id + " is not finite");
                    if (d.kind == ParamKind::Choice)
                        expect (v >= 0.0f && v <= (float) (d.numChoices() - 1), a.meta.name + " " + d.id + " choice out of range");
                    else
                        expect (v >= d.min && v <= d.max, a.meta.name + " " + d.id + " out of range");
                }
            }
            expectEquals (FactoryContent::rejectedRoutings(), 0, "the modulation matrix rejected a factory routing");
        }

        beginTest ("presets carry usable modulation, fracture and sample sections");
        {
            int withFracture = 0, withSample = 0, withMods = 0, macrosWired = 0;
            for (int i = 0; i < count; ++i)
            {
                const auto s = presets.buildFactory (i);
                const juce::String name = s.meta.name;

                if (! s.mod.isVoid())
                {
                    juce::String warnings;
                    const auto routings = ModRoutingTable::fromVar (s.mod, &warnings);
                    expect (warnings.isEmpty(), name + " modulation warnings: " + warnings);
                    if (routings.size() > 0)
                    {
                        ++withMods;
                        bool macroRouted = false;
                        for (const auto& r : routings)
                        {
                            expect (ModRoutingTable::isValid (r), name + " has an invalid routing");
                            expect (std::abs (r.depth) <= 1.0f, name + " routing depth out of range");
                            expect (ParameterRegistry::get (r.target).modulatable, name + " routes to a non-modulatable target");
                            if (modSourceGroup (r.source) == ModSourceGroup::Macro) macroRouted = true;
                        }
                        if (macroRouted) ++macrosWired;
                        expect (routings.size() >= 4, name + " should move: it only has "
                                                       + juce::String (routings.size()) + " routings");
                    }
                }

                if (paramBool (s.params, Param::fractureOn))
                {
                    ++withFracture;
                    expect (! s.fracture.isVoid(), name + " turns FRACTURE on but carries no table");
                    const auto table = FractureTable::fromVar (s.fracture);
                    expect (table.numFragments >= 1 && table.numFragments <= kMaxFractureFragments);
                    expect (table.numSteps >= 1 && table.numSteps <= kMaxSequencerSteps);
                    float openGate = 0.0f;
                    for (int step = 0; step < table.numSteps; ++step)
                        openGate = juce::jmax (openGate, table.steps[(size_t) step].gate);
                    expect (openGate > 0.0f, name + " has a Fracture pattern that never opens");
                }

                if (paramChoice (s.params, Param::sourceSelected) == 3)
                {
                    ++withSample;
                    expect (! s.sample.isVoid(), name + " uses SAMPLE but references no sample");
                    auto* reference = s.sample.getDynamicObject();
                    expect (reference != nullptr && reference->hasProperty ("builtIn"), name + " must use a built-in sample");
                    expect (reference->getProperty ("path").toString().isEmpty(), name + " must not reference a file on disk");
                }
            }

            logMessage ("modulated " + juce::String (withMods) + ", macros wired " + juce::String (macrosWired)
                        + ", fracture " + juce::String (withFracture) + ", sample " + juce::String (withSample));
            expect (withMods >= count - 1, "every patch except Init carries modulation");
            expect (macrosWired >= count - 1, "every patch except Init wires its macros");
            expect (withFracture >= 6, "the bank must use FRACTURE");
            expect (withSample >= 3, "the bank must use the built-in samples");
        }

        beginTest ("every source, material and Space is represented");
        {
            std::set<int> sources, materials, spaces, topologies, gestures;
            for (int i = 0; i < count; ++i)
            {
                const auto s = presets.buildFactory (i);
                sources.insert (paramChoice (s.params, Param::sourceSelected));
                materials.insert (paramChoice (s.params, Param::shapeMaterialA));
                materials.insert (paramChoice (s.params, Param::shapeMaterialB));
                spaces.insert (paramChoice (s.params, Param::spaceType));
                topologies.insert (paramChoice (s.params, Param::shapeTopology));
                if (paramChoice (s.params, Param::sourceSelected) == 4)
                    gestures.insert (paramChoice (s.params, Param::gestureMode));
            }
            expectEquals ((int) sources.size(), 5, "all five sources must be used");
            expect ((int) materials.size() >= 8, "the bank must use every material");
            expect ((int) spaces.size() >= 7, "the bank must use nearly every Space");
            expectEquals ((int) topologies.size(), 6, "all six topologies must be used");
            expect ((int) gestures.size() >= 5, "the GESTURE patches must use different modes");
        }

        beginTest ("JSON round trip preserves parameters and every section");
        {
            for (int i = 0; i < count; ++i)
            {
                const auto s = presets.buildFactory (i);
                PatchState back;
                juce::String warnings;
                expect (StateManager::fromJson (StateManager::toJson (s), back, &warnings), s.meta.name + ": " + warnings);
                expect (back.params == s.params, "parameters differ after the round trip for " + s.meta.name);
                expectEquals (back.meta.name, s.meta.name);
                expectEquals (back.meta.category, s.meta.category);
                expectEquals (back.meta.tags.size(), s.meta.tags.size(), s.meta.name + " lost its tags");

                const auto modA = ModRoutingTable::fromVar (s.mod);
                const auto modB = ModRoutingTable::fromVar (back.mod);
                expect (modA == modB, s.meta.name + " lost modulation routings across JSON");

                if (! s.fracture.isVoid())
                {
                    const auto ta = FractureTable::fromVar (s.fracture);
                    const auto tb = FractureTable::fromVar (back.fracture);
                    expectEquals (tb.numFragments, ta.numFragments, s.meta.name + " fracture fragments");
                    expectEquals (tb.numSteps, ta.numSteps, s.meta.name + " fracture steps");
                    for (int f = 0; f < kMaxFractureFragments; ++f)
                    {
                        expectWithinAbsoluteError (tb.fragments[(size_t) f].pitch, ta.fragments[(size_t) f].pitch, 1.0e-4f);
                        expectWithinAbsoluteError (tb.fragments[(size_t) f].delay, ta.fragments[(size_t) f].delay, 1.0e-4f);
                        expectWithinAbsoluteError (tb.fragments[(size_t) f].feedback, ta.fragments[(size_t) f].feedback, 1.0e-4f);
                    }
                    for (int step = 0; step < kMaxSequencerSteps; ++step)
                    {
                        expectEquals ((int) tb.steps[(size_t) step].mask, (int) ta.steps[(size_t) step].mask);
                        expectWithinAbsoluteError (tb.steps[(size_t) step].gate, ta.steps[(size_t) step].gate, 1.0e-4f);
                    }
                }

                if (! s.sample.isVoid())
                {
                    auto* a = s.sample.getDynamicObject();
                    auto* b = back.sample.getDynamicObject();
                    expect (b != nullptr, s.meta.name + " lost its sample reference");
                    if (a != nullptr && b != nullptr)
                        expectEquals ((int) b->getProperty ("builtIn"), (int) a->getProperty ("builtIn"),
                                      s.meta.name + " sample reference changed");
                }
            }
        }

        beginTest ("every preset passes the validator at its category level");
        {
            auto engine = std::make_unique<SynthEngine>();
            PresetValidatorOptions options;   // the standard render: 2 s held, 1.5 s tail

            juce::StringArray failures;
            for (int i = 0; i < count; ++i)
            {
                const auto r = PresetValidator::validateOne (presets, i, options, *engine);
                const auto problems = judge (r, r.category);

                logMessage (r.name.paddedRight (' ', 20) + r.category.paddedRight (' ', 12)
                            + "peak " + juce::String (r.peak, 3)
                            + "  rms " + juce::String (r.rms, 4)
                            + "  dc " + juce::String (r.dc, 5)
                            + "  centroid " + juce::String ((int) r.centroidHz) + " Hz"
                            + "  cpu " + juce::String (r.cpuAvgPercent, 1) + "%"
                            + (problems.isEmpty() ? "" : "   <-- " + problems.joinIntoString ("; ")));

                if (! problems.isEmpty()) failures.add (r.name + ": " + problems.joinIntoString ("; "));
            }

            expect (failures.isEmpty(), "presets failed the gate:\n   " + failures.joinIntoString ("\n   "));
        }

        beginTest ("every preset is playable across the keyboard");
        {
            auto engine = std::make_unique<SynthEngine>();
            auto options = quickOptions();

            juce::StringArray failures;
            for (const int note : { 36, 60, 84 })
            {
                options.midiNote = note;
                for (int i = 0; i < count; ++i)
                {
                    const auto r = PresetValidator::validateOne (presets, i, options, *engine);
                    juce::StringArray problems;
                    if (r.nonFinite > 0)          problems.add ("non-finite");
                    if (r.peak > 1.0f)            problems.add ("peak " + juce::String (r.peak, 3));
                    if (r.rms < 1.0e-4f)          problems.add ("inaudible (rms " + juce::String (r.rms, 7) + ")");
                    if (r.safetyTotal > 0)        problems.add ("safety " + juce::String ((int) r.safetyTotal));
                    if (std::abs (r.dc) > 0.02f)  problems.add ("dc " + juce::String (r.dc, 4));
                    if (! problems.isEmpty())
                        failures.add (r.name + " @ note " + juce::String (note) + ": " + problems.joinIntoString (", "));
                }
            }
            expect (failures.isEmpty(), "keyboard range problems:\n   " + failures.joinIntoString ("\n   "));
        }
    }
};

static FactoryContentTests factoryContentTests;

//==============================================================================
class FactoryMutationTests : public juce::UnitTest
{
public:
    FactoryMutationTests() : juce::UnitTest ("Factory mutation", "factory") {}

    void runTest() override
    {
        PresetManager presets;
        const int count = presets.numFactoryPresets();
        const MutationStrength strengths[] = { MutationStrength::Subtle, MutationStrength::Evolve, MutationStrength::Extreme };

        beginTest ("200 seeds x 3 strengths on every preset stay inside the safe ranges");
        {
            juce::Random random (0x5EED);
            juce::StringArray failures;

            for (int i = 0; i < count && failures.isEmpty(); ++i)
            {
                const auto patch = presets.buildFactory (i);
                for (int seedIndex = 0; seedIndex < 200 && failures.isEmpty(); ++seedIndex)
                {
                    const uint32_t seed = (uint32_t) random.nextInt();
                    for (const auto strength : strengths)
                    {
                        auto values = patch.params;
                        MutationEngine::mutate (values, strength, seed);

                        for (const auto& d : ParameterRegistry::all())
                        {
                            const float v = values[(size_t) paramIndex (d.param)];
                            float lo = d.min, hi = d.max;
                            MutationEngine::safeRange (d.param, lo, hi);
                            if (! std::isfinite (v) || v < lo - 1.0e-4f || v > hi + 1.0e-4f)
                            {
                                failures.add (patch.meta.name + " seed " + juce::String ((int) seed) + " " + d.id
                                              + " = " + juce::String (v) + " outside " + juce::String (lo) + ".." + juce::String (hi));
                                break;
                            }
                        }
                        if (! failures.isEmpty()) break;

                        // Structural guarantees: the mutant must still make and shape sound.
                        expectAudibleStructure (values, patch.meta.name, seed, failures);
                        if (! failures.isEmpty()) break;
                    }
                }
            }
            expect (failures.isEmpty(), failures.joinIntoString ("\n   "));
        }

        beginTest ("SUBTLE keeps close relatives, EXTREME restructures");
        {
            float subtleDistance = 0.0f, evolveDistance = 0.0f, extremeDistance = 0.0f;
            int samples = 0;

            for (int i = 0; i < count; ++i)
            {
                const auto patch = presets.buildFactory (i);
                for (uint32_t seed = 1; seed <= 12; ++seed)
                {
                    subtleDistance  += distance (patch.params, seed, MutationStrength::Subtle);
                    evolveDistance  += distance (patch.params, seed, MutationStrength::Evolve);
                    extremeDistance += distance (patch.params, seed, MutationStrength::Extreme);
                    ++samples;
                }
            }

            subtleDistance /= (float) samples;
            evolveDistance /= (float) samples;
            extremeDistance /= (float) samples;
            logMessage ("mean normalised move: subtle " + juce::String (subtleDistance, 4)
                        + "  evolve " + juce::String (evolveDistance, 4)
                        + "  extreme " + juce::String (extremeDistance, 4));

            expect (subtleDistance > 0.002f, "SUBTLE must actually change something");
            expect (subtleDistance < 0.05f, "SUBTLE must stay a close relative");
            expect (evolveDistance > subtleDistance * 1.8f, "EVOLVE must move meaningfully further than SUBTLE");
            expect (extremeDistance > evolveDistance * 1.4f, "EXTREME must restructure");
        }

        beginTest ("category masks only move the DNA they are given");
        {
            const auto patch = presets.buildFactory (presets.findFactory ("Metal Bloom"));
            auto values = patch.params;
            MutationEngine::mutate (values, MutationStrength::Extreme, 4242,
                                    MutationEngine::maskFor (MutationCategory::Shape));

            for (const auto& d : ParameterRegistry::all())
            {
                const float before = patch.params[(size_t) paramIndex (d.param)];
                const float after = values[(size_t) paramIndex (d.param)];
                if (d.mutation != MutationCategory::Shape && d.param != Param::sourceMode)
                    expectWithinAbsoluteError (after, before, 1.0e-6f, juce::String (d.id) + " moved outside its category");
            }

            bool shapeMoved = false;
            for (const auto& d : ParameterRegistry::all())
                if (d.mutation == MutationCategory::Shape
                    && std::abs (values[(size_t) paramIndex (d.param)] - patch.params[(size_t) paramIndex (d.param)]) > 1.0e-4f)
                    shapeMoved = true;
            expect (shapeMoved, "a SHAPE mutation must change SHAPE");
        }

        beginTest ("every preset survives 200 random mutations, rendered");
        {
            auto engine = std::make_unique<SynthEngine>();
            PresetValidatorOptions options;
            options.holdSeconds = 0.28;
            options.releaseSeconds = 0.20;
            options.blockSize = 512;
            engine->prepare (options.sampleRate, options.blockSize);

            juce::Random random ((juce::int64) 0xB10D5EEDLL);
            juce::StringArray failures;
            float worstPeak = 0.0f, quietestRms = 1.0f;
            const auto started = juce::Time::getMillisecondCounter();

            for (int i = 0; i < count; ++i)
            {
                const auto base = presets.buildFactory (i);
                for (int n = 0; n < 200; ++n)
                {
                    auto patch = base;
                    const auto strength = strengths[n % 3];
                    const uint32_t seed = (uint32_t) random.nextInt();
                    MutationEngine::mutate (patch.params, strength, seed);

                    const auto r = renderPatch (*engine, patch, options, 60, true);
                    worstPeak = juce::jmax (worstPeak, r.peak);
                    quietestRms = juce::jmin (quietestRms, r.rms);

                    juce::StringArray problems;
                    if (r.nonFinite > 0)         problems.add ("non-finite " + juce::String (r.nonFinite));
                    if (r.peak > 1.0f)           problems.add ("peak " + juce::String (r.peak, 3));
                    if (r.rms < 1.0e-4f)         problems.add ("silent (rms " + juce::String (r.rms, 8) + ")");
                    if (std::abs (r.dc) > 0.02f) problems.add ("dc " + juce::String (r.dc, 4));
                    if (r.safety > 0)            problems.add ("safety " + juce::String ((int) r.safety));

                    if (! problems.isEmpty())
                        failures.add (base.meta.name + " strength " + juce::String ((int) strength)
                                      + " seed " + juce::String ((int) seed) + ": " + problems.joinIntoString (", "));
                }
            }

            logMessage (juce::String (count * 200) + " mutations rendered in "
                        + juce::String ((juce::Time::getMillisecondCounter() - started) / 1000) + " s: worst peak "
                        + juce::String (worstPeak, 3) + ", quietest rms " + juce::String (quietestRms, 6));
            expect (failures.isEmpty(), "mutations failed the gate:\n   " + failures.joinIntoString ("\n   "));
        }

        beginTest ("one preset survives 200 consecutive mutations of its own child");
        {
            auto engine = std::make_unique<SynthEngine>();
            auto options = quickOptions();
            auto patch = presets.buildFactory (presets.findFactory ("Void Bloom"));
            juce::Random random (0xDEEDDEED);
            juce::StringArray failures;

            for (int n = 0; n < 200 && failures.isEmpty(); ++n)
            {
                MutationEngine::mutate (patch.params, strengths[n % 3], (uint32_t) random.nextInt());
                if (n % 20 != 0) continue;              // render every twentieth generation

                const auto r = renderPatch (*engine, patch, options, 60);
                if (r.nonFinite > 0 || r.peak > 1.0f || r.rms < 1.0e-4f || r.safety > 0)
                    failures.add ("generation " + juce::String (n) + ": peak " + juce::String (r.peak, 3)
                                  + " rms " + juce::String (r.rms, 6) + " nonFinite " + juce::String (r.nonFinite)
                                  + " safety " + juce::String ((int) r.safety));
            }
            expect (failures.isEmpty(), "a mutation lineage went bad:\n   " + failures.joinIntoString ("\n   "));
        }

        beginTest ("randomize always produces a playable patch");
        {
            auto engine = std::make_unique<SynthEngine>();
            auto options = quickOptions();
            juce::StringArray failures;

            for (uint32_t seed = 1; seed <= 40; ++seed)
            {
                PatchState patch = PresetManager::initPatch();
                MutationEngine::randomize (patch.params, seed);
                const auto r = renderPatch (*engine, patch, options, 60);
                if (r.nonFinite > 0 || r.peak > 1.0f || r.rms < 1.0e-4f || r.safety > 0)
                    failures.add ("seed " + juce::String ((int) seed) + ": peak " + juce::String (r.peak, 3)
                                  + " rms " + juce::String (r.rms, 6) + " nonFinite " + juce::String (r.nonFinite)
                                  + " safety " + juce::String ((int) r.safety));
            }
            expect (failures.isEmpty(), "randomize produced unusable patches:\n   " + failures.joinIntoString ("\n   "));
        }
    }

private:
    void expectAudibleStructure (const ParamValues& v, const juce::String& name, uint32_t seed, juce::StringArray& failures)
    {
        const int source = paramChoice (v, Param::sourceSelected);
        static const Param levels[] = { Param::waveLevel, Param::dustLevel, Param::impactLevel,
                                        Param::sampleLevel, Param::gestureLevel };
        const float level = paramValue (v, levels[juce::jlimit (0, 4, source)]);
        const float strike = paramValue (v, Param::shapeStrike);
        const float mix = paramValue (v, Param::shapeMix);

        const juce::String where = name + " seed " + juce::String ((int) seed) + ": ";
        if (level < 0.2f && strike * mix < 0.1f)
            failures.add (where + "no energy left (level " + juce::String (level, 3) + ", strike " + juce::String (strike, 3) + ")");
        if (paramValue (v, Param::ampAttack) > 2.0f && paramValue (v, Param::ampSustain) < 0.2f)
            failures.add (where + "the note never arrives (attack " + juce::String (paramValue (v, Param::ampAttack), 2) + ")");
        if (paramValue (v, Param::fractureFeedback) > 0.9f || paramValue (v, Param::spaceDelayFeedback) > 0.9f)
            failures.add (where + "runaway feedback");
    }

    static float distance (const ParamValues& base, uint32_t seed, MutationStrength strength)
    {
        auto values = base;
        MutationEngine::mutate (values, strength, seed);

        float sum = 0.0f;
        int counted = 0;
        for (const auto& d : ParameterRegistry::all())
        {
            if (d.mutation == MutationCategory::None) continue;
            const float span = juce::jmax (1.0e-6f, d.max - d.min);
            const float move = std::abs (values[(size_t) paramIndex (d.param)] - base[(size_t) paramIndex (d.param)]) / span;
            sum += d.kind == ParamKind::Choice ? (move > 0.0f ? 1.0f : 0.0f) : move;
            ++counted;
        }
        return counted > 0 ? sum / (float) counted : 0.0f;
    }
};

static FactoryMutationTests factoryMutationTests;

//==============================================================================
class FactoryMorphTests : public juce::UnitTest
{
public:
    FactoryMorphTests() : juce::UnitTest ("Factory morph", "factory") {}

    void runTest() override
    {
        PresetManager presets;
        const int count = presets.numFactoryPresets();

        beginTest ("A/B morph between any two factory presets stays in range");
        {
            std::vector<PatchState> bank;
            bank.reserve ((size_t) count);
            for (int i = 0; i < count; ++i)
                bank.push_back (presets.buildFactory (i));

            juce::StringArray failures;
            for (int a = 0; a < count && failures.isEmpty(); ++a)
            {
                for (int b = a + 1; b < count && failures.isEmpty(); ++b)
                {
                    for (const float t : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
                    {
                        const auto morphed = PatchMorph::interpolate (bank[(size_t) a].params, bank[(size_t) b].params, t);
                        for (const auto& d : ParameterRegistry::all())
                        {
                            const float v = morphed[(size_t) paramIndex (d.param)];
                            const bool ok = std::isfinite (v)
                                            && v >= d.min - 1.0e-4f && v <= d.max + 1.0e-4f
                                            && (d.kind != ParamKind::Choice || v <= (float) (d.numChoices() - 1) + 1.0e-4f);
                            if (! ok)
                            {
                                failures.add (bank[(size_t) a].meta.name + " -> " + bank[(size_t) b].meta.name
                                              + " at t = " + juce::String (t) + ": " + d.id + " = " + juce::String (v));
                                break;
                            }
                        }
                        if (! failures.isEmpty()) break;
                    }
                }
            }
            expect (failures.isEmpty(), failures.joinIntoString ("\n   "));

            // The ends are exact, the middle is genuinely in between.
            const auto& a = bank[1], & b = bank[(size_t) count - 1];
            expect (PatchMorph::interpolate (a.params, b.params, 0.0f) == a.params);
            expect (PatchMorph::interpolate (a.params, b.params, 1.0f) == b.params);
        }

        beginTest ("morphed patches render safely at the quarter points");
        {
            auto engine = std::make_unique<SynthEngine>();
            auto options = quickOptions();
            juce::StringArray failures;

            for (int a = 0; a + 1 < count; ++a)
            {
                const auto first = presets.buildFactory (a);
                const auto second = presets.buildFactory ((a + 7) % count);
                for (const float t : { 0.25f, 0.5f, 0.75f })
                {
                    auto morphed = PatchMorph::interpolate (first, second, t);
                    const auto r = renderPatch (*engine, morphed, options, 60);
                    if (r.nonFinite > 0 || r.peak > 1.0f || r.safety > 0)
                        failures.add (first.meta.name + " -> " + second.meta.name + " at t = " + juce::String (t)
                                      + ": peak " + juce::String (r.peak, 3) + " nonFinite " + juce::String (r.nonFinite)
                                      + " safety " + juce::String ((int) r.safety));
                }
            }
            expect (failures.isEmpty(), "morph render problems:\n   " + failures.joinIntoString ("\n   "));
        }
    }
};

static FactoryMorphTests factoryMorphTests;

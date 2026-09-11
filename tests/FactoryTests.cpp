#include <juce_core/juce_core.h>

#include "FactoryBankSampling.h"

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
using namespace am::factorytest;

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

    //==========================================================================
    // FIXED BUDGETS.
    //
    // These are totals for the WHOLE BANK and they do not grow with it: 36
    // patches and 300 patches cost the same here. What grows is the per-preset
    // work (the validator gate and the keyboard range), because that is what
    // stops a broken patch shipping and it has to see every patch.
    //==========================================================================
    constexpr int kMutationRenderBudget = 900;   ///< rendered mutations across the bank
    constexpr double kMutationHoldCap    = 0.8;  ///< seconds; a mutant may ask for a 2.5 s attack
    constexpr int kMorphExtraPairs      = 400;   ///< sampled pairs on top of the structural ones
    constexpr int kMorphRenderBudget    = 300;   ///< rendered morphs across the bank

}

//==============================================================================
class FactoryContentTests : public juce::UnitTest
{
public:
    FactoryContentTests() : juce::UnitTest ("Factory content", "factory") {}

    void runTest() override
    {
        // `count` is the real library. `scaled` is the same library presented at
        // the size ANTIMATR_FACTORY_SCALE asks for, so the cost of the two
        // full-coverage render tests at 300 patches can be measured today
        // rather than guessed at. Identity checks always use the real bank.
        ScaledBank bank;
        PresetManager& presets = bank.manager();
        const int count = bank.realSize();
        const int scaled = bank.size();
        if (bank.isScaled())
            logMessage ("SCALE MEASUREMENT: presenting " + juce::String (scaled) + " presets from a bank of " + juce::String (count));

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
            // Full coverage, deliberately: this is the gate that stops a broken
            // patch shipping, so it renders every single preset however big the
            // bank gets. The cost is managed by preparing each engine once and
            // by spreading the renders over the machine's cores — not by doing
            // less work per preset.
            const Stopwatch clock;
            PresetValidatorOptions options;   // the standard render: 2 s held, 1.5 s tail
            options.engineAlreadyPrepared = true;
            options.flagCpu = false;          // judged below, on its own, where the number means something

            std::vector<juce::String> report ((size_t) scaled);
            std::vector<float> cpu ((size_t) scaled, 0.0f);
            const auto failures = parallelPresetSweep (scaled, options,
                [&] (SynthEngine& engine, int i, juce::StringArray& problemsFor)
                {
                    const auto r = PresetValidator::validateOne (presets, bank.realIndex (i), options, engine);
                    const auto problems = judge (r, r.category);
                    cpu[(size_t) i] = r.cpuAvgPercent;

                    report[(size_t) i] = r.name.paddedRight (' ', 20) + r.category.paddedRight (' ', 12)
                                       + "peak " + juce::String (r.peak, 3)
                                       + "  rms " + juce::String (r.rms, 4)
                                       + "  dc " + juce::String (r.dc, 5)
                                       + "  centroid " + juce::String ((int) r.centroidHz) + " Hz"
                                       + "  cpu " + juce::String (r.cpuAvgPercent, 1) + "%"
                                       + (problems.isEmpty() ? "" : "   <-- " + problems.joinIntoString ("; "));

                    if (! problems.isEmpty()) problemsFor.add (r.name + ": " + problems.joinIntoString ("; "));
                });

            for (const auto& line : report) logMessage (line);
            logMessage ("validator gate: " + juce::String (scaled) + " presets rendered in " + clock.elapsed()
                        + " on " + juce::String (testThreadCount()) + " threads");

            // The CPU budget, judged where the number is worth something. Four
            // renders at once inflate every reading, so anything that looks over
            // budget is re-rendered ALONE and judged on that: the gate is exactly
            // as strict as it was when the whole sweep ran one preset at a time,
            // and in a healthy bank this costs nothing because the list is empty.
            juce::StringArray expensive;
            {
                auto engine = std::make_unique<SynthEngine>();
                PresetValidatorOptions alone;      // defaults, CPU flagged
                engine->prepare (alone.sampleRate, alone.blockSize);
                alone.engineAlreadyPrepared = true;

                int rechecked = 0;
                for (int i = 0; i < scaled; ++i)
                {
                    if (cpu[(size_t) i] <= alone.cpuLimit) continue;
                    ++rechecked;
                    const auto r = PresetValidator::validateOne (presets, bank.realIndex (i), alone, *engine);
                    if (r.cpuAvgPercent > alone.cpuLimit)
                        expensive.add (r.name + ": CPU " + juce::String (r.cpuAvgPercent, 1) + "% above "
                                       + juce::String (alone.cpuLimit, 0) + "% rendered on its own");
                }
                if (rechecked > 0)
                    logMessage ("re-measured " + juce::String (rechecked) + " preset(s) alone for the CPU budget, "
                                + juce::String (expensive.size()) + " over it");
            }

            expect (failures.isEmpty(), "presets failed the gate:\n   " + failures.joinIntoString ("\n   "));
            expect (expensive.isEmpty(), "presets are too expensive to play:\n   " + expensive.joinIntoString ("\n   "));
        }

        beginTest ("every preset is playable across the keyboard");
        {
            // Full coverage, every preset, at both ends of the keyboard: a patch
            // that only behaves in the middle is not finished.
            //
            // The middle is not rendered again here. The gate above renders every
            // preset at note 60 for three and a half seconds and judges it against
            // its category window, which is a strictly harder test than anything
            // this one applies — so note 60 would be 300 renders spent re-asking a
            // question that has just been answered. The guard below keeps that
            // true if anyone changes the validator's note.
            expect (PresetValidatorOptions().midiNote == 60,
                    "the validator gate no longer covers note 60 — put it back in the keyboard sweep");

            // The parameter bounds and the JSON round trip are covered above too,
            // so this renders directly instead of going through validateOne and
            // repeating them per note.
            const Stopwatch clock;
            const auto options = safetyRenderOptions();

            const auto failures = parallelPresetSweep (scaled, options,
                [&] (SynthEngine& engine, int i, juce::StringArray& problemsFor)
                {
                    for (const int note : { 36, 84 })
                    {
                        const auto r = renderPatch (engine, bank.patch (i), options, note, true);
                        juce::StringArray problems;
                        if (r.nonFinite > 0)          problems.add ("non-finite");
                        if (r.peak > 1.0f)            problems.add ("peak " + juce::String (r.peak, 3));
                        if (r.rms < 1.0e-4f)          problems.add ("inaudible (rms " + juce::String (r.rms, 7) + ")");
                        if (r.safety > 0)             problems.add ("safety " + juce::String ((int) r.safety));
                        if (std::abs (r.dc) > 0.02f)  problems.add ("dc " + juce::String (r.dc, 4));
                        if (! problems.isEmpty())
                            problemsFor.add (bank.name (i) + " @ note " + juce::String (note) + ": " + problems.joinIntoString (", "));
                    }
                });
            logMessage ("keyboard range: " + juce::String (scaled * 2) + " renders in " + clock.elapsed());
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
            // Kept at full coverage: no render here, just the mutation and a
            // scan of the parameter table, so 300 presets cost a few seconds.
            // The seeds are positional (hashed from the draw index) so a
            // failure names a seed that can be re-run on its own.
            const Stopwatch clock;
            juce::StringArray failures;

            for (int i = 0; i < count && failures.isEmpty(); ++i)
            {
                const auto patch = presets.buildFactory (i);
                for (int seedIndex = 0; seedIndex < 200 && failures.isEmpty(); ++seedIndex)
                {
                    const uint32_t seed = seedFor (0x5EEDu, 0, seedIndex);
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
            logMessage (juce::String (count * 200 * 3) + " mutations range-checked in " + clock.elapsed());
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

        beginTest ("a fixed budget of rendered mutations, spread across the whole bank");
        {
            // WHAT THIS TESTS is the mutation engine: that no (patch, strength,
            // seed) combination it can produce renders to something unsafe.
            // Two hundred renders per preset answered that question 7,200 times
            // for 36 patches and would answer it 60,000 times for 300 — the same
            // question, at ten times the price. A fixed budget spread round-robin
            // over the bank draws from every patch and every strength, and the
            // suite costs the same whether the bank is 36 patches or 3,000.
            const Stopwatch clock;
            ScaledBank bank;
            const auto options = mutationRenderOptions();

            const auto plan = mutationPlan (bank.size(), kMutationRenderBudget);
            std::vector<RenderResult> rendered (plan.size());

            const auto failures = parallelPresetSweep ((int) plan.size(), options,
                [&] (SynthEngine& engine, int k, juce::StringArray& problemsFor)
                {
                    const auto& draw = plan[(size_t) k];
                    auto patch = bank.patch (draw.preset);
                    const auto strength = strengths[draw.strength];
                    MutationEngine::mutate (patch.params, strength, draw.seed);

                    const auto r = renderPatch (engine, patch, options, 60, true, nullptr, nullptr, kMutationHoldCap);
                    rendered[(size_t) k] = r;

                    juce::StringArray problems;
                    if (r.nonFinite > 0)         problems.add ("non-finite " + juce::String (r.nonFinite));
                    if (r.peak > 1.0f)           problems.add ("peak " + juce::String (r.peak, 3));
                    // A mutant whose attack ran past the cap has not finished
                    // arriving, so how quiet it is says nothing. Whether the note
                    // arrives at all is the range check's job, above.
                    if (! r.truncated && r.rms < 1.0e-4f)
                                                 problems.add ("silent (rms " + juce::String (r.rms, 8) + ")");
                    if (std::abs (r.dc) > 0.02f) problems.add ("dc " + juce::String (r.dc, 4));
                    if (r.safety > 0)            problems.add ("safety " + juce::String ((int) r.safety));

                    if (! problems.isEmpty())
                        problemsFor.add (bank.name (draw.preset) + " strength " + juce::String ((int) strength)
                                         + " seed " + juce::String ((int) draw.seed)
                                         + " (draw " + juce::String (draw.index) + " of " + juce::String ((int) plan.size()) + ")"
                                         + ": " + problems.joinIntoString (", "));
                });

            float worstPeak = 0.0f, quietestRms = 1.0f;
            for (const auto& r : rendered)
            {
                worstPeak = juce::jmax (worstPeak, r.peak);
                quietestRms = juce::jmin (quietestRms, r.rms);
            }

            const int perPreset = bank.size() > 0 ? (int) plan.size() / bank.size() : 0;
            logMessage (juce::String ((int) plan.size()) + " mutations rendered across " + juce::String (bank.size())
                        + " presets (" + juce::String (perPreset) + "+ each) in " + clock.elapsed()
                        + ": worst peak " + juce::String (worstPeak, 3) + ", quietest rms " + juce::String (quietestRms, 6));
            expect ((int) plan.size() >= bank.size(), "the budget must reach every preset at least once");
            expect (failures.isEmpty(), "mutations failed the gate:\n   " + failures.joinIntoString ("\n   "));
        }

        beginTest ("one preset survives 200 consecutive mutations of its own child");
        {
            auto engine = std::make_unique<SynthEngine>();
            const auto options = safetyRenderOptions();
            engine->prepare (options.sampleRate, options.blockSize);
            auto patch = presets.buildFactory (presets.findFactory ("Void Bloom"));
            juce::Random random (0xDEEDDEED);
            juce::StringArray failures;

            for (int n = 0; n < 200 && failures.isEmpty(); ++n)
            {
                MutationEngine::mutate (patch.params, strengths[n % 3], (uint32_t) random.nextInt());
                if (n % 20 != 0) continue;              // render every twentieth generation

                const auto r = renderPatch (*engine, patch, options, 60, true);
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
            const auto options = safetyRenderOptions();
            engine->prepare (options.sampleRate, options.blockSize);
            juce::StringArray failures;

            for (uint32_t seed = 1; seed <= 40; ++seed)
            {
                PatchState patch = PresetManager::initPatch();
                MutationEngine::randomize (patch.params, seed);
                const auto r = renderPatch (*engine, patch, options, 60, true);
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
        ScaledBank bank;
        const int count = bank.size();

        std::vector<juce::String> categoryOf;
        categoryOf.reserve ((size_t) count);
        for (int i = 0; i < count; ++i) categoryOf.push_back (bank.category (i));

        beginTest ("A/B morph stays in range across a deterministic sample of pairs");
        {
            // WHAT THIS TESTS is PatchMorph: that no two patches can be blended
            // into a value outside its parameter's range. Every pair is 630
            // combinations at 36 presets and 44,850 at 300 — seventy times the
            // work to re-ask a question that only depends on which PARAMETERS
            // disagree, not on which patches carry them. The plan keeps the
            // coverage that finds a disagreement: every neighbour (which, since
            // the bank is registered in category order, includes every category
            // boundary in the library), one pair for every combination of
            // categories, a long-range partner for every preset, and a seeded
            // sample on top. Every preset is in at least four pairs; the cost is
            // linear in the size of the bank.
            const Stopwatch clock;
            const auto pairs = morphPairs (count, categoryOf, kMorphExtraPairs, kMorphPairMaster);

            std::vector<int> appearances ((size_t) count, 0);
            juce::StringArray failures;
            for (const auto& pair : pairs)
            {
                ++appearances[(size_t) pair.a];
                ++appearances[(size_t) pair.b];
                if (! failures.isEmpty()) continue;

                for (const float t : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
                {
                    const auto morphed = PatchMorph::interpolate (bank.patch (pair.a).params, bank.patch (pair.b).params, t);
                    for (const auto& d : ParameterRegistry::all())
                    {
                        const float v = morphed[(size_t) paramIndex (d.param)];
                        const bool ok = std::isfinite (v)
                                        && v >= d.min - 1.0e-4f && v <= d.max + 1.0e-4f
                                        && (d.kind != ParamKind::Choice || v <= (float) (d.numChoices() - 1) + 1.0e-4f);
                        if (! ok)
                        {
                            failures.add (bank.name (pair.a) + " -> " + bank.name (pair.b)
                                          + " (" + pair.why + " pair, indices " + juce::String (pair.a) + "/" + juce::String (pair.b) + ")"
                                          + " at t = " + juce::String (t) + ": " + d.id + " = " + juce::String (v));
                            break;
                        }
                    }
                    if (! failures.isEmpty()) break;
                }
            }

            int leastCovered = count > 0 ? appearances[0] : 0;
            for (const int n : appearances) leastCovered = juce::jmin (leastCovered, n);
            logMessage (juce::String ((int) pairs.size()) + " pairs over " + juce::String (count)
                        + " presets (every preset in at least " + juce::String (leastCovered) + ") in " + clock.elapsed());
            expect (leastCovered >= 1, "the pair plan missed a preset entirely");
            expect (failures.isEmpty(), failures.joinIntoString ("\n   "));

            // The ends are exact, the middle is genuinely in between.
            const auto& a = bank.patch (1), & b = bank.patch (count - 1);
            expect (PatchMorph::interpolate (a.params, b.params, 0.0f) == a.params);
            expect (PatchMorph::interpolate (a.params, b.params, 1.0f) == b.params);
        }

        beginTest ("a fixed budget of morphed patches renders safely at the quarter points");
        {
            const Stopwatch clock;
            const auto options = safetyRenderOptions();
            const auto plan = morphRenderPlan (count, kMorphRenderBudget);

            const auto failures = parallelPresetSweep ((int) plan.size(), options,
                [&] (SynthEngine& engine, int k, juce::StringArray& problemsFor)
                {
                    const auto& draw = plan[(size_t) k];
                    auto morphed = PatchMorph::interpolate (bank.patch (draw.a), bank.patch (draw.b), draw.t);
                    const auto r = renderPatch (engine, morphed, options, 60, true);
                    if (r.nonFinite > 0 || r.peak > 1.0f || r.safety > 0)
                        problemsFor.add (bank.name (draw.a) + " -> " + bank.name (draw.b) + " at t = " + juce::String (draw.t)
                                         + " (draw " + juce::String (draw.index) + ")"
                                         + ": peak " + juce::String (r.peak, 3) + " nonFinite " + juce::String (r.nonFinite)
                                         + " safety " + juce::String ((int) r.safety));
                });
            logMessage (juce::String ((int) plan.size()) + " morphs rendered in " + clock.elapsed());
            expect (failures.isEmpty(), "morph render problems:\n   " + failures.joinIntoString ("\n   "));
        }
    }
};

static FactoryMorphTests factoryMorphTests;

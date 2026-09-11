#pragma once

#include <juce_core/juce_core.h>

#include "dev/diagnostics/PresetValidator.h"
#include "dsp/SynthEngine.h"
#include "dsp/fracture/Fragment.h"
#include "dsp/source/SampleData.h"
#include "presets/FactoryContent.h"
#include "presets/PresetManager.h"
#include "state/ModRouting.h"
#include "state/MutationEngine.h"
#include "state/StateManager.h"

#include <algorithm>
#include <atomic>
#include <ctime>
#include <thread>
#include <utility>
#include <vector>

/**
    SHARED MACHINERY FOR THE FACTORY SUITE

    The factory bank grows from 36 patches to 300. Two of the tests that
    covered it were written when "run it on everything" was cheap: 200
    rendered mutations per preset (60,000 renders at 300) and a morph check
    over every pair (44,850 pairs at 300). Both are combinatorial and both
    test an engine, not a patch — so they get a FIXED TOTAL BUDGET spread
    deterministically across the whole bank instead.

    Everything here is seeded from a compile-time constant and derived with a
    pure hash, so a run on Windows CI draws exactly the same work as a run on
    this machine and any failure prints the numbers needed to reproduce it on
    its own.

    Cheap per-preset checks (the validator gate, the keyboard range) are NOT
    sampled: they are what stops a broken patch shipping and they still cover
    every single preset.
*/
namespace am::factorytest
{

//==============================================================================
/**
    Cost of one section, so a regression shows up in the log instead of in CI.

    Both numbers, because they answer different questions. WALL is what the
    person waiting sees and depends on how many cores are free; CPU is the work
    actually done and is the only figure that survives a shared machine — a
    build running alongside the suite doubles the first and leaves the second
    alone. Compare CPU when you are judging whether a change made the suite
    cheaper.
*/
struct Stopwatch
{
    uint32_t startedWall = juce::Time::getMillisecondCounter();
    std::clock_t startedCpu = std::clock();

    juce::String elapsed() const
    {
        const double wall = (juce::Time::getMillisecondCounter() - startedWall) / 1000.0;
        const double cpu = (double) (std::clock() - startedCpu) / (double) CLOCKS_PER_SEC;
        return juce::String (wall, 1) + " s wall / " + juce::String (cpu, 1) + " s cpu";
    }
};

/** splitmix32 finaliser — cheap, portable and well mixed. Every sampled seed comes from here. */
inline uint32_t mix32 (uint32_t x) noexcept
{
    x += 0x9E3779B9u;
    x = (x ^ (x >> 16)) * 0x85EBCA6Bu;
    x = (x ^ (x >> 13)) * 0xC2B2AE35u;
    return x ^ (x >> 16);
}

/** A seed for draw `k` of stream `stream`, rooted in `master`. Pure: same everywhere, always. */
inline uint32_t seedFor (uint32_t master, int stream, int k) noexcept
{
    return mix32 (master ^ mix32 ((uint32_t) stream * 0x27220A95u + 0x9E3779B9u) ^ mix32 ((uint32_t) k * 0x165667B1u));
}

/** Uniform integer in [0, range) from a hashed seed. */
inline int pick (uint32_t master, int stream, int k, int range) noexcept
{
    return range <= 0 ? 0 : (int) (seedFor (master, stream, k) % (uint32_t) range);
}

//==============================================================================
/**
    The factory bank as the heavy tests see it.

    Normally this is just the real library. Set ANTIMATR_FACTORY_SCALE=300 and
    it presents a bank of that size by repeating the real patches, so the cost
    of the suite at the target bank size can be MEASURED rather than
    extrapolated. Only the tests whose runtime is being managed use it; the
    identity checks (names, categories, uniqueness) always use the real bank.
*/
class ScaledBank
{
public:
    ScaledBank()
    {
        realCount = presets.numFactoryPresets();
        virtualCount = realCount;

        const auto env = juce::SystemStats::getEnvironmentVariable ("ANTIMATR_FACTORY_SCALE", juce::String()).trim();
        if (env.isNotEmpty())
            virtualCount = juce::jmax (realCount, env.getIntValue());

        cache.reserve ((size_t) realCount);
        for (int i = 0; i < realCount; ++i)
            cache.push_back (presets.buildFactory (i));
    }

    int  size() const noexcept        { return virtualCount; }
    int  realSize() const noexcept    { return realCount; }
    bool isScaled() const noexcept    { return virtualCount != realCount; }

    int realIndex (int i) const noexcept { return realCount > 0 ? i % realCount : 0; }

    const PatchState& patch (int i) const { return cache[(size_t) realIndex (i)]; }
    juce::String category (int i) const   { return presets.factoryPreset (realIndex (i)).category.toUpperCase(); }

    /** Unambiguous even when the bank is repeated for a scale measurement. */
    juce::String name (int i) const
    {
        const auto n = presets.factoryPreset (realIndex (i)).name;
        return isScaled() ? n + " #" + juce::String (i / juce::jmax (1, realCount)) : n;
    }

    PresetManager& manager() noexcept { return presets; }

private:
    PresetManager presets;
    std::vector<PatchState> cache;
    int realCount = 0, virtualCount = 0;
};

//==============================================================================
// RENDER PROFILES
//
// Three, and the difference between them is the point.
//
//   * The CATEGORY GATE keeps the validator's own defaults — 2 s held, 1.5 s of
//     tail, 128-sample blocks — because those numbers are what the category RMS
//     and peak windows were calibrated against and what `scripts/render.sh`
//     reports back to an author. Nothing here may change them.
//
//   * A SAFETY RENDER asks a different question: does this make sound, does it
//     stay inside the rails, does it stay finite. Nothing about it is compared
//     against a calibrated window, so it renders less audio in bigger blocks.
//     That is where the per-preset cost of a 300-patch bank is bought back.
//
//   * The MUTATION render is shorter still: the mutants are drawn in bulk and
//     all that is asked of them is that they do not blow up.
//
// Both short profiles still stretch the held note to cover the patch's own
// attack — a two second swell is slow, not silent, and a fixed window that
// could not tell them apart would fail every pad in the bank.
//==============================================================================

/** Short render: audible, finite, inside the rails. Not compared to any window. */
inline dev::PresetValidatorOptions safetyRenderOptions()
{
    dev::PresetValidatorOptions o;
    o.holdSeconds = 0.35;
    o.releaseSeconds = 0.25;
    o.blockSize = 512;
    return o;
}

/** Shorter still, for the bulk mutation and morph draws. */
inline dev::PresetValidatorOptions mutationRenderOptions()
{
    dev::PresetValidatorOptions o;
    o.holdSeconds = 0.22;
    o.releaseSeconds = 0.15;
    o.blockSize = 512;
    return o;
}

/** Long enough for the envelope descriptor of the uniqueness fingerprint to have a shape. */
inline dev::PresetValidatorOptions fingerprintOptions()
{
    dev::PresetValidatorOptions o;
    o.holdSeconds = 0.45;
    o.releaseSeconds = 0.35;
    o.blockSize = 512;
    return o;
}

//==============================================================================
/** One rendered mutation / morph: the numbers the gates judge. */
struct RenderResult
{
    float    peak = 0.0f, rms = 0.0f, dc = 0.0f;
    int      nonFinite = 0;
    uint32_t safety = 0;
    int      numSamples = 0;
    bool     truncated = false;   ///< the hold was capped, so `rms` is not the whole note
};

/**
    Renders one patch through `engine` exactly the way the validator does —
    fracture table, modulation routings and the patch's own built-in sample —
    and optionally keeps the audio for analysis.

    Pass `enginePrepared` when the caller already prepared the engine at these
    options, which is what makes a budget of renders cheap: prepare() is the
    expensive part and it is identical for every patch.
*/
inline RenderResult renderPatch (SynthEngine& engine, const PatchState& patch,
                                 const dev::PresetValidatorOptions& options, int midiNote,
                                 bool enginePrepared = false,
                                 std::vector<float>* captureL = nullptr,
                                 std::vector<float>* captureR = nullptr,
                                 double maxHoldSeconds = 0.0)
{
    RenderResult out;
    const double sr = options.sampleRate;
    const int blockSize = options.blockSize;

    // Hold the note long enough for its own attack: a two second swell is not
    // silent, it is slow, and the window has to be able to tell them apart.
    //
    // A caller that only wants to know whether the audio stays finite and inside
    // the rails can cap that with `maxHoldSeconds`. The mutation draws do: the
    // engine lets a mutant have a two and a half second attack, and rendering
    // all of it for every draw would triple the cost of the budget to answer a
    // question about the first block. `truncated` says the cap applied, so the
    // caller knows not to read anything into how quiet the result is.
    const double wanted = juce::jmax (options.holdSeconds,
                                      (double) paramValue (patch.params, Param::ampAttack) + 0.30);
    const double hold = maxHoldSeconds > 0.0 ? juce::jmin (wanted, maxHoldSeconds) : wanted;
    out.truncated = hold < wanted - 1.0e-9;

    const int holdSamples = (int) (hold * sr);
    const int totalSamples = holdSamples + (int) (options.releaseSeconds * sr);

    if (! enginePrepared) engine.prepare (sr, blockSize);
    engine.reset();
    engine.control().resetTo (patch.params);
    {
        auto table = patch.fracture.isVoid() ? FractureTable::makeDefault() : FractureTable::fromVar (patch.fracture);
        engine.fractureEngine().publishTable (std::make_unique<FractureTable> (table));
        auto routings = patch.mod.isVoid() ? ModRoutingTable() : ModRoutingTable::fromVar (patch.mod);
        engine.modulationEngine().publishRoutings (std::make_unique<ModRoutingTable> (routings));

        // A SAMPLE patch is its sample: publish the one it references or two
        // patches that differ only in that choice measure identically.
        int builtIn = 0;
        if (auto* reference = patch.sample.getDynamicObject())
        {
            if (reference->hasProperty ("builtIn")) builtIn = (int) reference->getProperty ("builtIn");
            else                                    builtIn = juce::jmax (0, BuiltInSamples::indexOf (reference->getProperty ("name").toString()));
        }
        engine.publishSample (BuiltInSamples::create (juce::jlimit (0, BuiltInSamples::count() - 1, builtIn)));
    }
    auto& diag = engine.diagnostics();
    diag.safety.reset();
    diag.events.drain ([] (const EngineEvent&) {});

    if (captureL != nullptr) { captureL->assign ((size_t) totalSamples, 0.0f); }
    if (captureR != nullptr) { captureR->assign ((size_t) totalSamples, 0.0f); }

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
        if (! noteSent)                                       { midi.addEvent (juce::MidiMessage::noteOn (1, midiNote, options.velocity), 0); noteSent = true; }
        else if (! releaseSent && position + n > holdSamples)  { midi.addEvent (juce::MidiMessage::noteOff (1, midiNote), juce::jlimit (0, n - 1, holdSamples - position)); releaseSent = true; }

        engine.process (block, midi, patch.params, transport);

        const float* l = block.getReadPointer (0);
        const float* r = block.getReadPointer (1);
        if (captureL != nullptr) std::copy (l, l + n, captureL->begin() + position);
        if (captureR != nullptr) std::copy (r, r + n, captureR->begin() + position);

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

    out.numSamples = totalSamples;
    out.rms = (float) std::sqrt (sum / juce::jmax (1, counted));
    out.dc = (float) (dcSum / juce::jmax (1, counted));
    out.safety = engine.diagnostics().safety.snapshot().total;
    return out;
}

//==============================================================================
/** Worker threads the per-preset render sweeps may use. ANTIMATR_TEST_THREADS=1 serialises them. */
inline int testThreadCount()
{
    const auto env = juce::SystemStats::getEnvironmentVariable ("ANTIMATR_TEST_THREADS", juce::String()).trim();
    if (env.isNotEmpty()) return juce::jlimit (1, 32, env.getIntValue());
    return juce::jlimit (1, 8, juce::SystemStats::getNumCpus());
}

/**
    Runs `body (engine, index, failures)` for every preset, across the cores.

    The per-preset gates — the validator level check and the keyboard range —
    are the two tests that MUST stay at full coverage as the bank grows, which
    means their cost grows with it: 300 presets is 300 full renders plus 900
    short ones however carefully the rest of the suite is sampled. Renders are
    independent, so they are spread over the machine instead.

    Each worker owns its SynthEngine and prepares it once. Failures are written
    into a slot per preset, never appended to a shared list, so the report comes
    out in preset order and is identical however the work happened to be
    scheduled — set ANTIMATR_TEST_THREADS=1 and the output does not change.
*/
template <typename Body>
juce::StringArray parallelPresetSweep (int count, const dev::PresetValidatorOptions& options, Body body)
{
    juce::StringArray merged;
    if (count <= 0) return merged;

    std::vector<juce::StringArray> perPreset ((size_t) count);
    const int workers = juce::jlimit (1, count, testThreadCount());
    std::atomic<int> next { 0 };

    auto work = [&]
    {
        auto engine = std::make_unique<SynthEngine>();
        engine->prepare (options.sampleRate, options.blockSize);
        for (;;)
        {
            const int i = next.fetch_add (1, std::memory_order_relaxed);
            if (i >= count) break;
            body (*engine, i, perPreset[(size_t) i]);
        }
    };

    if (workers <= 1)
    {
        work();
    }
    else
    {
        std::vector<std::thread> threads;
        threads.reserve ((size_t) workers);
        for (int w = 0; w < workers; ++w) threads.emplace_back (work);
        for (auto& t : threads) t.join();
    }

    for (const auto& failures : perPreset) merged.addArray (failures);
    return merged;
}

//==============================================================================
// SAMPLING PLANS
//
// Every plan is a pure function of the bank size: no state, no clock, no
// system random. Two runs of the same binary on the same bank draw the same
// work, and the failure messages carry the draw index and seed.
//==============================================================================

/** Master seeds. Changing one re-rolls that plan's coverage; keep them still unless you mean it. */
inline constexpr uint32_t kMutationMaster  = 0xB10D5EEDu;
inline constexpr uint32_t kMorphPairMaster = 0x0A11A1CEu;

/** One rendered mutation. */
struct MutationDraw
{
    int      index = 0;        ///< position in the plan, printed on failure
    int      preset = 0;
    int      strength = 0;     ///< index into { Subtle, Evolve, Extreme }
    uint32_t seed = 0;
};

/**
    A fixed total of rendered mutations, spread round-robin over the bank.

    The thing under test is the mutation engine, so what matters is how many
    (patch, strength, seed) combinations get rendered — not that each patch
    gets its own two hundred. Round-robin guarantees EVERY preset is drawn
    (whenever the budget reaches the bank size) and that the draws per preset
    differ by at most one; rotating the strength by the preset index means
    three passes over the bank cover all three strengths on every patch.
*/
inline std::vector<MutationDraw> mutationPlan (int count, int budget, uint32_t master = kMutationMaster)
{
    std::vector<MutationDraw> plan;
    if (count <= 0 || budget <= 0) return plan;
    plan.reserve ((size_t) budget);

    for (int k = 0; k < budget; ++k)
    {
        MutationDraw d;
        d.index = k;
        d.preset = k % count;
        d.strength = ((k / count) + d.preset) % 3;
        d.seed = seedFor (master, 1, k);
        plan.push_back (d);
    }
    return plan;
}

/** A pair of presets to morph between. */
struct MorphPair
{
    int a = 0, b = 0;
    const char* why = "";   ///< why the pair is in the plan, printed on failure
};

/**
    Pair coverage without the pairs.

    O(n²) is 44,850 pairs at 300 presets and buys almost nothing: what can go
    wrong in a morph is a parameter pair whose ranges or kinds disagree, and
    that shows up on any pair that crosses the disagreement. The plan keeps
    the coverage that matters and drops the rest:

      * every neighbour (i, i+1) plus the wrap — every preset appears twice,
        and because the bank is registered in category order every CATEGORY
        BOUNDARY in the library is one of these pairs;
      * one representative pair for every unordered pair of categories, so
        PAD→CINEMATIC is tested even though they are nowhere near each other
        (fixed cost: 66 pairs for twelve categories);
      * a long-range partner (i, i + n/2) for every preset, so no preset is
        only ever morphed with its neighbours;
      * `extra` deterministic random pairs on top.

    Cost is O(n), every preset appears in at least four pairs, and the plan is
    identical on every machine.
*/
inline std::vector<MorphPair> morphPairs (int count, const std::vector<juce::String>& categoryOf,
                                          int extra, uint32_t master)
{
    std::vector<MorphPair> pairs;
    if (count < 2) return pairs;

    std::vector<uint8_t> seen;
    const size_t cells = (size_t) count * (size_t) count;
    const bool trackSeen = cells <= 4u * 1024u * 1024u;
    if (trackSeen) seen.assign (cells, 0);

    auto add = [&] (int a, int b, const char* why)
    {
        if (a == b) return;
        if (a > b) std::swap (a, b);
        if (trackSeen)
        {
            auto& flag = seen[(size_t) a * (size_t) count + (size_t) b];
            if (flag) return;
            flag = 1;
        }
        pairs.push_back ({ a, b, why });
    };

    for (int i = 0; i < count; ++i)
        add (i, (i + 1) % count, "neighbour");

    // First preset of each category, so every category boundary is crossed once.
    {
        std::vector<int> firstOf;
        juce::StringArray seenCategories;
        for (int i = 0; i < count && i < (int) categoryOf.size(); ++i)
            if (! seenCategories.contains (categoryOf[(size_t) i], true))
            {
                seenCategories.add (categoryOf[(size_t) i]);
                firstOf.push_back (i);
            }
        for (size_t a = 0; a < firstOf.size(); ++a)
            for (size_t b = a + 1; b < firstOf.size(); ++b)
                add (firstOf[a], firstOf[b], "category boundary");
    }

    for (int i = 0; i < count; ++i)
        add (i, (i + juce::jmax (1, count / 2)) % count, "long range");

    for (int k = 0; k < juce::jmax (0, extra); ++k)
        add (pick (master, 2, k * 2, count), pick (master, 2, k * 2 + 1, count), "sampled");

    return pairs;
}

/** One rendered morph. */
struct MorphRenderDraw
{
    int   index = 0;
    int   a = 0, b = 0;
    float t = 0.5f;
};

/**
    A fixed total of rendered morphs.

    Round-robin on `a` so every preset is an endpoint of at least one rendered
    morph as soon as the budget reaches the bank size; the partner walks away
    from it by a stride that changes each pass, and t cycles through the
    quarter points.
*/
inline std::vector<MorphRenderDraw> morphRenderPlan (int count, int budget)
{
    std::vector<MorphRenderDraw> plan;
    if (count < 2 || budget <= 0) return plan;
    plan.reserve ((size_t) budget);

    static const float ts[] = { 0.25f, 0.5f, 0.75f };
    for (int k = 0; k < budget; ++k)
    {
        MorphRenderDraw d;
        d.index = k;
        d.a = k % count;
        const int pass = k / count;
        const int stride = 1 + (pass * 7 + 6) % juce::jmax (1, count - 1);
        d.b = (d.a + stride) % count;
        if (d.b == d.a) d.b = (d.a + 1) % count;
        d.t = ts[k % 3];
        plan.push_back (d);
    }
    return plan;
}

} // namespace am::factorytest

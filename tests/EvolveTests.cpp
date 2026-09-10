#include <juce_core/juce_core.h>
#include "dsp/matter/MatterEngine.h"
#include "dsp/evolve/EvolveEngine.h"
#include "dsp/SynthEngine.h"
#include "dev/diagnostics/Diagnostics.h"

using namespace am;

namespace
{
    /** Matter + Evolve for one voice, driven like AntiMatrVoice does: apply() then process() per block. */
    struct Harness
    {
        MatterEngine matter;
        EvolveEngine evolve;
        ParamValues params = ParameterRegistry::defaults();
        NoteState note;
        RenderContext ctx;
        double sr;
        int block;
        std::vector<float> excL, excR, outL, outR;

        Harness (double sampleRate, int blockSize, Diagnostics* d = nullptr) : sr (sampleRate), block (blockSize)
        {
            excL.assign ((size_t) blockSize, 0.0f); excR.assign ((size_t) blockSize, 0.0f);
            outL.assign ((size_t) blockSize, 0.0f); outR.assign ((size_t) blockSize, 0.0f);
            matter.prepare (sr, blockSize);
            evolve.prepare (sr, blockSize);
            ctx.sampleRate = sr; ctx.numSamples = blockSize; ctx.params = &params; ctx.diagnostics = d;
            // A rigid, wobble-free, surface-free object: the baseline is identical from block to block.
            set (Param::shapeSurface, 0.0f);
        }

        void set (Param p, float v) { params[(size_t) paramIndex (p)] = v; }

        void start (int midiNote, uint32_t noteId = 1, float velocity = 0.8f, Quality q = Quality::Normal)
        {
            note = NoteState();
            note.midiNote = midiNote; note.velocity = velocity; note.gate = true; note.noteId = noteId;
            note.baseFrequency = note.frequency = midiNoteToHz (midiNote);
            matter.noteOn (note, params, q);
            evolve.noteOn (note, params);
        }

        /** Evolve only: the node records now hold what Matter would render next. */
        void applyOnly() { ctx.numSamples = block; evolve.apply (matter, ctx, note); }

        /** One full block: Evolve, then Matter with silence (strike only) or a sine excitation. */
        void step (float sineHz = 0.0f, float sineAmp = 0.0f, double* phase = nullptr)
        {
            for (int i = 0; i < block; ++i)
            {
                float s = 0.0f;
                if (sineAmp > 0.0f && phase != nullptr) { s = sineAmp * (float) std::sin (*phase); *phase += kTwoPi * sineHz / sr; }
                excL[(size_t) i] = excR[(size_t) i] = s;
            }
            ctx.numSamples = block;
            evolve.apply (matter, ctx, note);
            matter.process (excL.data(), excR.data(), outL.data(), outR.data(), block, ctx, note);
        }

        std::vector<float> render (double seconds, float sineHz = 0.0f, float sineAmp = 0.0f)
        {
            const int total = (int) (seconds * sr);
            std::vector<float> out; out.reserve ((size_t) total + (size_t) block);
            double ph = 0.0;
            for (int pos = 0; pos < total; pos += block)
            {
                step (sineHz, sineAmp, &ph);
                for (int i = 0; i < block; ++i) out.push_back (0.5f * (outL[(size_t) i] + outR[(size_t) i]));
            }
            return out;
        }

        float fundamental() const { return (float) note.frequency; }
        int numNodes() const { return matter.numNodes(); }
        const MatterNode& node (int i) const { return matter.node (i); }
        bool live (int i) const { return node (i).active && node (i).weight > 0.0f; }
        int modalCount() const { return matter.numNodes() - MaterialMorpher::kBodyNodes; }
    };

    using Nodes = std::array<MatterNode, kMaxMatterNodes>;

    Nodes snapshot (const Harness& h)
    {
        Nodes n {};
        for (int i = 0; i < h.numNodes(); ++i) n[(size_t) i] = h.node (i);
        return n;
    }

    bool sameBits (float a, float b) { return std::memcmp (&a, &b, sizeof (float)) == 0; }

    /** Bit-identical comparison of every field Evolve is allowed to write (plus the informational ones). */
    bool identical (const MatterNode& a, const MatterNode& b)
    {
        return sameBits (a.frequency, b.frequency) && sameBits (a.targetFrequency, b.targetFrequency) && sameBits (a.ratio, b.ratio)
            && sameBits (a.weight, b.weight) && sameBits (a.damping, b.damping) && sameBits (a.pan, b.pan)
            && sameBits (a.nonlinearity, b.nonlinearity) && sameBits (a.excitation, b.excitation)
            && a.cluster == b.cluster && a.couplingCount == b.couplingCount && a.active == b.active;
    }

    double cents (double f, double ref) { return 1200.0 * std::log2 (f / ref); }

    bool allFinite (const std::vector<float>& v) { for (float x : v) if (! std::isfinite (x)) return false; return true; }
    float peakOf (const std::vector<float>& v) { float p = 0.0f; for (float x : v) p = std::max (p, std::abs (x)); return p; }
    float rmsOf (const std::vector<float>& v, int start, int end)
    {
        double s = 0.0; int n = 0;
        for (int i = std::max (0, start); i < std::min (end, (int) v.size()); ++i) { s += (double) v[(size_t) i] * v[(size_t) i]; ++n; }
        return n > 0 ? (float) std::sqrt (s / n) : 0.0f;
    }

    /** Spectral height of node i as the engine defines it: log2 ratio over a fixed span of four octaves. */
    float heightOf (const Harness& h, const Nodes& base, int i)
    {
        return std::clamp (std::log2 (base[(size_t) i].targetFrequency / h.fundamental()) / 4.0f, 0.0f, 1.0f);
    }
}

class EvolveTests : public juce::UnitTest
{
public:
    EvolveTests() : juce::UnitTest ("Evolve engine", "evolve") {}

    void runTest() override
    {
        beginTest ("Zero amounts (gravity 0.5, freeze off) leave every node bit-identical, block after block");
        {
            for (int material : { 0, 1, 3, 5 })
                for (auto q : { Quality::Eco, Quality::Normal, Quality::Ultra })
                {
                    Harness h (48000.0, 128);
                    h.set (Param::shapeMaterialA, (float) material); h.set (Param::shapeSurface, 0.3f);
                    h.set (Param::evolveMotion, 1.0f); h.set (Param::evolveSpeed, 1.0f);   // motion alone must not move anything
                    h.start (60, 1, 0.8f, q);
                    for (int b = 0; b < 12; ++b)
                    {
                        const auto before = snapshot (h);
                        h.applyOnly();
                        for (int i = 0; i < h.numNodes(); ++i)
                            expect (identical (before[(size_t) i], h.node (i)), "node " + juce::String (i) + " changed with all operators at zero");
                        h.step();
                    }
                    EvolveDiag d; h.evolve.fillDiagnostics (d);
                    expectEquals (d.nodesMoved, 0);
                }
        }

        beginTest ("BEND: lever around the pivot, up to bendRange octaves, monotonic in amount");
        {
            for (float amount : { 0.5f, 1.0f })
            {
                // Pivot 0: the fundamental stays, everything above rises; the top node reaches amount * range octaves.
                Harness h (48000.0, 128);
                h.set (Param::evolveBendPivot, 0.0f); h.set (Param::evolveBendRange, 1.0f); h.set (Param::evolveBendCurve, 0.0f);
                h.set (Param::evolveMotion, 0.0f); h.set (Param::evolveBend, amount);
                h.start (48);    // low enough that the top partial stays well below Nyquist after a +1 octave bend
                const auto base = snapshot (h);
                h.applyOnly();
                float topShift = 0.0f, topBase = 0.0f; int topIndex = -1;
                for (int i = 0; i < h.modalCount(); ++i)
                {
                    if (! (base[(size_t) i].active && base[(size_t) i].weight > 0.0f)) continue;
                    const double c = cents (h.node (i).frequency, base[(size_t) i].targetFrequency);
                    expect (c >= -0.01, "node below its baseline with pivot 0: " + juce::String (c));
                    // Linear curve: the shift is amount * range * height, exactly.
                    expectWithinAbsoluteError ((float) c, amount * 1200.0f * heightOf (h, base, i), 6.0f, "bend law for node " + juce::String (i));
                    if (base[(size_t) i].targetFrequency > topBase) { topBase = base[(size_t) i].targetFrequency; topShift = (float) c; topIndex = i; }
                }
                expect (topIndex >= 0 && heightOf (h, base, topIndex) >= 1.0f, "the top of this object should sit four octaves up");
                expectWithinAbsoluteError (topShift, amount * 1200.0f, 12.0f, "top node shift at bend " + juce::String (amount));
                expectWithinAbsoluteError ((float) cents (h.node (0).frequency, base[0].targetFrequency), 0.0f, 0.5f, "fundamental must stay put at pivot 0");

                // Pivot 1: the top stays, everything below sinks; the fundamental drops amount * range octaves.
                Harness g (48000.0, 128);
                g.set (Param::evolveBendPivot, 1.0f); g.set (Param::evolveBendRange, 1.0f); g.set (Param::evolveBendCurve, 0.0f);
                g.set (Param::evolveMotion, 0.0f); g.set (Param::evolveBend, amount);
                g.start (48);
                const auto gb = snapshot (g);
                g.applyOnly();
                for (int i = 0; i < g.modalCount(); ++i)
                    if (g.live (i)) expect (cents (g.node (i).frequency, gb[(size_t) i].targetFrequency) <= 0.01, "node above its baseline with pivot 1");
                expectWithinAbsoluteError ((float) cents (g.node (0).frequency, gb[0].targetFrequency), -amount * 1200.0f, 12.0f, "fundamental drop at pivot 1");
            }
            // Curve: exponential distribution moves the middle less than the linear one.
            float midLinear = 0.0f, midExp = 0.0f;
            for (int pass = 0; pass < 2; ++pass)
            {
                Harness h (48000.0, 128);
                h.set (Param::evolveBendPivot, 0.0f); h.set (Param::evolveBendRange, 1.0f); h.set (Param::evolveBendCurve, pass == 0 ? 0.0f : 1.0f);
                h.set (Param::evolveMotion, 0.0f); h.set (Param::evolveBend, 1.0f);
                h.start (60);
                const auto base = snapshot (h);
                h.applyOnly();
                float best = 1.0f; int mid = -1;
                for (int i = 0; i < h.modalCount(); ++i)
                    if (h.live (i) && std::abs (heightOf (h, base, i) - 0.5f) < best) { best = std::abs (heightOf (h, base, i) - 0.5f); mid = i; }
                expect (mid >= 0);
                (pass == 0 ? midLinear : midExp) = (float) cents (h.node (mid).frequency, base[(size_t) mid].targetFrequency);
            }
            expect (midExp < midLinear * 0.6f, "bend curve 1 should move the middle much less: " + juce::String (midLinear) + " vs " + juce::String (midExp));
        }

        beginTest ("MELT: high partials sag, damp faster and fade; the fundamental keeps its pitch; stronger at 1 than 0.5");
        {
            float previousDrop = 0.0f;
            for (float amount : { 0.5f, 1.0f })
            {
                Harness h (48000.0, 128);
                h.set (Param::shapeDensity, 0.8f); h.set (Param::evolveMotion, 0.0f); h.set (Param::evolveMelt, amount);
                h.start (60);
                const auto base = snapshot (h);
                h.applyOnly();
                float dropSum = 0.0f; int highNodes = 0;
                for (int i = 1; i < h.modalCount(); ++i)
                {
                    if (! h.live (i) || heightOf (h, base, i) < 0.4f) continue;
                    const auto& n = h.node (i);
                    const auto& b = base[(size_t) i];
                    expect (n.frequency < b.targetFrequency, "high partial did not sag");
                    expect (n.damping > b.damping, "high partial did not damp faster");
                    expect (n.weight < b.weight, "high partial did not fade");
                    dropSum += (float) -cents (n.frequency, b.targetFrequency); ++highNodes;
                }
                expect (highNodes >= 4, "not enough high partials to judge");
                const float meanDrop = dropSum / (float) std::max (1, highNodes);
                expect (meanDrop > previousDrop, "melt 1 should sag more than melt 0.5");
                expectWithinAbsoluteError ((float) cents (h.node (0).frequency, base[0].targetFrequency), 0.0f, 0.5f, "the fundamental keeps its pitch");
                previousDrop = meanDrop;
            }
        }

        beginTest ("TEAR: detuned twin pairs appear, more at 1 than at 0.5, nonlinearity rises, energy is conserved");
        {
            auto closePairs = [] (const Harness& h, double maxCents)
            {
                int pairs = 0;
                for (int i = 0; i < h.numNodes(); ++i)
                    for (int j = i + 1; j < h.numNodes(); ++j)
                        if (h.live (i) && h.live (j) && std::abs (cents (h.node (i).frequency, h.node (j).frequency)) < maxCents) ++pairs;
                return pairs;
            };
            int previousPairs = -1;
            for (float amount : { 0.0f, 0.5f, 1.0f })
            {
                Harness h (48000.0, 128);
                h.set (Param::shapeMaterialA, 8.0f); h.set (Param::shapeMaterialB, 8.0f);   // CUSTOM: harmonic, no close pairs by itself
                h.set (Param::shapeDensity, 0.5f); h.set (Param::evolveMotion, 0.0f); h.set (Param::evolveTear, amount);
                h.start (60);
                const auto base = snapshot (h);
                float weightBefore = 0.0f;
                for (int i = 0; i < h.numNodes(); ++i) weightBefore += base[(size_t) i].weight * base[(size_t) i].weight;
                h.applyOnly();
                const int pairs = closePairs (h, 40.0);
                expect (pairs > previousPairs, "tear " + juce::String (amount) + " should create more twin pairs than the previous amount: " + juce::String (pairs));
                previousPairs = pairs;
                EvolveDiag d; h.evolve.fillDiagnostics (d);
                if (amount > 0.0f)
                {
                    expect (d.tearPairs >= 1, "tearPairs not reported");
                    expect (pairs >= d.tearPairs, "every reported twin should be a close pair");
                    // The fundamental is torn first: its twin sits within 20 cents and shares its damping order of magnitude.
                    bool twinOfRoot = false;
                    for (int i = 1; i < h.numNodes(); ++i)
                        if (h.live (i) && std::abs (cents (h.node (i).frequency, h.node (0).frequency)) < 20.0) twinOfRoot = true;
                    expect (twinOfRoot, "the fundamental has no twin");
                    expect (h.node (0).nonlinearity > base[0].nonlinearity, "nonlinearity did not rise");
                    float weightAfter = 0.0f;
                    for (int i = 0; i < h.numNodes(); ++i) weightAfter += h.node (i).weight * h.node (i).weight;
                    // The twins' own (least important) weights are sacrificed; the paired power itself is conserved.
                    expect (weightAfter / weightBefore > 0.7f && weightAfter / weightBefore < 1.05f,
                            "tear should roughly conserve the weight power: " + juce::String (weightAfter / weightBefore));
                }
            }
        }

        beginTest ("MAGNET: every target grid, 5 cents at 1, partial pull at 0.5");
        {
            for (int target = 0; target < 7; ++target)
                for (int material : { 0, 1 })      // CRYSTAL and METAL: inharmonic objects
                {
                    Harness base (48000.0, 128);
                    base.set (Param::shapeMaterialA, (float) material); base.set (Param::shapeForm, 0.7f); base.set (Param::shapeDensity, 0.8f);
                    base.start (60);
                    base.applyOnly();
                    double baseDist = 0.0; int count = 0;
                    for (int i = 0; i < base.numNodes(); ++i)
                    {
                        if (! base.live (i)) continue;
                        const float lr = std::log2 (base.node (i).frequency / base.fundamental());
                        baseDist += std::abs (EvolveEngine::magnetGridLog2 (target, lr) - lr) * 1200.0; ++count;
                    }
                    baseDist /= std::max (1, count);
                    expect (baseDist > 8.0, "the baseline object should be off the grid for this test: " + juce::String (baseDist));

                    for (float amount : { 0.5f, 1.0f })
                    {
                        Harness h (48000.0, 128);
                        h.set (Param::shapeMaterialA, (float) material); h.set (Param::shapeForm, 0.7f); h.set (Param::shapeDensity, 0.8f);
                        h.set (Param::evolveMagnetTarget, (float) target); h.set (Param::evolveMagnet, amount); h.set (Param::evolveMotion, 0.0f);
                        h.start (60);
                        h.applyOnly();
                        double dist = 0.0; int n = 0; double worst = 0.0;
                        for (int i = 0; i < h.numNodes(); ++i)
                        {
                            if (! h.live (i)) continue;
                            const float lr = std::log2 (h.node (i).frequency / h.fundamental());
                            const double c = std::abs (EvolveEngine::magnetGridLog2 (target, lr) - lr) * 1200.0;
                            dist += c; ++n; worst = std::max (worst, c);
                        }
                        dist /= std::max (1, n);
                        const juce::String tag = " (target " + juce::String (target) + ", material " + juce::String (material) + ")";
                        if (amount >= 1.0f)
                            expect (worst < 5.0, "partial farther than 5 cents from the grid at magnet 1: " + juce::String (worst) + tag);
                        else
                            expect (dist < baseDist * 0.7 && dist > baseDist * 0.2, "magnet 0.5 should pull halfway: " + juce::String (baseDist) + " -> " + juce::String (dist) + tag);
                        EvolveDiag d; h.evolve.fillDiagnostics (d);
                        if (amount >= 1.0f) expectEquals (d.magnetLocked, d.activeNodes, "every active node should be locked" + tag);
                    }
                }
            // Grid definitions: pure fifth, 12-TET triads, harmonic rounding.
            expectWithinAbsoluteError (EvolveEngine::magnetGridLog2 (1, 0.6f), std::log2 (1.5f), 1.0e-5f);
            expectWithinAbsoluteError (EvolveEngine::magnetGridLog2 (2, 0.3f), 4.0f / 12.0f, 1.0e-5f);
            expectWithinAbsoluteError (EvolveEngine::magnetGridLog2 (3, 0.3f), 3.0f / 12.0f, 1.0e-5f);
            expectWithinAbsoluteError (EvolveEngine::magnetGridLog2 (0, 1.4f), 1.0f, 1.0e-5f);
            expectWithinAbsoluteError (EvolveEngine::magnetGridLog2 (0, 1.6f), 2.0f, 1.0e-5f);
            expectWithinAbsoluteError (EvolveEngine::magnetGridLog2 (6, std::log2 (5.4f)), std::log2 (5.0f), 1.0e-4f);
            expectWithinAbsoluteError (EvolveEngine::magnetGridLog2 (6, std::log2 (0.55f)), std::log2 (0.5f), 1.0e-4f);
        }

        beginTest ("GRAVITY: 1 sinks weight and excitation and damps the top; 0 lifts; 0.5 is an exact identity");
        {
            auto balance = [] (const Harness& h, const Nodes& base, float& topWeight, float& lowWeight, float& topDamping, float& topExc)
            {
                topWeight = lowWeight = topDamping = topExc = 0.0f;
                for (int i = 0; i < h.modalCount(); ++i)
                {
                    if (! h.live (i)) continue;
                    const float hh = heightOf (h, base, i);
                    if (hh > 0.6f) { topWeight += h.node (i).weight; topDamping += h.node (i).damping; topExc += h.node (i).excitation; }
                    else if (hh < 0.2f) lowWeight += h.node (i).weight;
                }
            };
            Harness ref (48000.0, 128); ref.set (Param::shapeDensity, 0.9f); ref.start (60);
            const auto base = snapshot (ref);
            ref.applyOnly();
            float tw0, lw0, td0, te0; balance (ref, base, tw0, lw0, td0, te0);

            for (float g : { 1.0f, 0.75f })
            {
                Harness h (48000.0, 128); h.set (Param::shapeDensity, 0.9f); h.set (Param::evolveGravity, g); h.start (60);
                h.applyOnly();
                float tw, lw, td, te; balance (h, base, tw, lw, td, te);
                expect (tw / lw < tw0 / lw0 * 0.7f, "gravity " + juce::String (g) + " should sink the weight balance");
                expect (td > td0 * 1.2f, "gravity " + juce::String (g) + " should damp the top faster");
                expect (te < te0, "gravity " + juce::String (g) + " should pull excitation down");
                expect (h.node (0).weight >= base[0].weight, "the fundamental should not lose weight when sinking");
            }
            for (float g : { 0.0f, 0.25f })
            {
                Harness h (48000.0, 128); h.set (Param::shapeDensity, 0.9f); h.set (Param::evolveGravity, g); h.start (60);
                h.applyOnly();
                float tw, lw, td, te; balance (h, base, tw, lw, td, te);
                expect (tw / lw > tw0 / lw0 * 1.5f, "gravity " + juce::String (g) + " should lift the weight balance");
                expect (h.node (0).weight < base[0].weight, "the fundamental should lose weight when lifting");
                expect (h.node (0).damping > base[0].damping, "the fundamental should decay faster when lifting");
                expect (td > td0, "lifting should still decay the top slightly faster");
            }
            {
                Harness h (48000.0, 128); h.set (Param::shapeDensity, 0.9f); h.set (Param::evolveGravity, 0.5f); h.start (60);
                const auto b = snapshot (h);
                h.applyOnly();
                for (int i = 0; i < h.numNodes(); ++i) expect (identical (b[(size_t) i], h.node (i)), "gravity 0.5 is not neutral");
            }
        }

        beginTest ("SCATTER: deterministic per seed, different between seeds, moves frequency/weight/pan, more at 1");
        {
            auto run = [] (int seed, uint32_t noteId, float amount, Nodes& base)
            {
                Harness h (48000.0, 128);
                h.set (Param::shapeDensity, 0.8f); h.set (Param::evolveMotion, 0.0f);
                h.set (Param::evolveScatterSeed, (float) seed); h.set (Param::evolveScatter, amount);
                h.start (60, noteId);
                base = snapshot (h);
                h.applyOnly();
                return snapshot (h);
            };
            Nodes b1, b2, b3, b4, b5;
            const auto a = run (3, 1, 1.0f, b1);
            const auto b = run (3, 1, 1.0f, b2);
            const auto c = run (4, 1, 1.0f, b3);
            const auto d = run (3, 2, 1.0f, b4);
            const auto e = run (3, 1, 0.5f, b5);
            int same = 0, differSeed = 0, differNote = 0, movedFull = 0, movedHalf = 0, pans = 0, weights = 0;
            double centsFull = 0.0, centsHalf = 0.0, rootCents = 0.0;
            const int N = 64;
            for (int i = 0; i < N; ++i)
            {
                if (! (b1[(size_t) i].active && b1[(size_t) i].weight > 0.0f)) continue;
                if (identical (a[(size_t) i], b[(size_t) i])) ++same;
                if (std::abs (cents (a[(size_t) i].frequency, c[(size_t) i].frequency)) > 1.0) ++differSeed;
                if (std::abs (cents (a[(size_t) i].frequency, d[(size_t) i].frequency)) > 1.0) ++differNote;
                const double cf = std::abs (cents (a[(size_t) i].frequency, b1[(size_t) i].targetFrequency));
                const double ch = std::abs (cents (e[(size_t) i].frequency, b5[(size_t) i].targetFrequency));
                if (cf > 1.0) ++movedFull;
                if (ch > 1.0) ++movedHalf;
                centsFull += cf; centsHalf += ch;
                if (i == 0) rootCents = cf;
                if (std::abs (a[(size_t) i].pan - b1[(size_t) i].pan) > 0.01f) ++pans;
                if (std::abs (a[(size_t) i].weight - b1[(size_t) i].weight) > 1.0e-4f) ++weights;
                expect (cf <= 300.0 + 0.01, "scatter must stay within ±3 semitones: " + juce::String (cf));
            }
            const int live = [&] { int n = 0; for (int i = 0; i < N; ++i) if (b1[(size_t) i].active && b1[(size_t) i].weight > 0.0f) ++n; return n; }();
            expectEquals (same, live, "same seed and note must reproduce bit-identically");
            expect (differSeed > live / 2, "different seeds should differ: " + juce::String (differSeed) + " of " + juce::String (live));
            expect (differNote > live / 2, "different notes should differ: " + juce::String (differNote) + " of " + juce::String (live));
            expect (movedFull > live * 3 / 4 && movedHalf > live / 2, "scatter should move most nodes");
            expect (centsFull > centsHalf * 1.5, "scatter 1 should move more than 0.5");
            expect (rootCents <= 60.5, "the fundamental scatters at most 60 cents: " + juce::String (rootCents));
            expect (pans > live / 2 && weights > live / 2, "pans and weights should scatter too");
        }

        beginTest ("FREEZE: damping floored, motion clock stopped, finite and non-silent over 20 s of processing");
        {
            Harness h (48000.0, 256);
            h.set (Param::evolveFreeze, 1.0f); h.set (Param::evolveSpeed, 1.0f); h.set (Param::evolveMotion, 1.0f); h.set (Param::evolveBend, 0.5f);
            h.start (60);
            const auto base = snapshot (h);
            h.applyOnly();
            const float floorD = EvolveEngine::freezeDamping (48000.0);
            for (int i = 0; i < h.numNodes(); ++i)
            {
                if (! h.live (i)) continue;
                expect (h.node (i).damping <= base[(size_t) i].damping + 1.0e-12f, "freeze increased damping");
                expect (h.node (i).damping <= floorD + 1.0e-9f, "damping above the freeze floor");
                expect (h.node (i).excitation < base[(size_t) i].excitation, "freeze should reduce excitation");
            }
            expect (h.evolve.isFrozen());
            const float phaseBefore = h.evolve.motionPhase();
            auto out = h.render (20.0);
            expectWithinAbsoluteError (h.evolve.motionPhase(), phaseBefore, 1.0e-6f, "motion phase moved while frozen");
            expect (allFinite (out), "non-finite output while frozen");
            expect (peakOf (out) < 4.0f, "runaway while frozen");
            const float early = rmsOf (out, 48000, 96000), late = rmsOf (out, (int) out.size() - 48000, (int) out.size());
            expect (late > 1.0e-3f, "a frozen object went silent: " + juce::String (late));
            // The damping floor (2e-6) is a 72 s T60 at 48 kHz: at most ~15 dB over these 18 s.
            expect (late > early * 0.12f, "a frozen object decayed too much: " + juce::String (early) + " -> " + juce::String (late));
            expect (late <= early * 1.5f + 1.0e-6f, "a frozen object grew: " + juce::String (early) + " -> " + juce::String (late));

            // The same object unfrozen decays normally.
            Harness u (48000.0, 256);
            u.start (60);
            auto ref = u.render (20.0);
            expect (rmsOf (ref, (int) ref.size() - 48000, (int) ref.size()) < late * 0.01f, "the unfrozen object should have decayed");
        }

        beginTest ("CRUSH: coarse frequency grid, thinned and quantised weights, nonlinearity, stepped decay");
        {
            for (float amount : { 0.5f, 1.0f })
            {
                Harness h (48000.0, 128);
                h.set (Param::shapeDensity, 0.9f); h.set (Param::evolveMotion, 0.0f); h.set (Param::evolveCrush, amount);
                h.start (48);
                const auto base = snapshot (h);
                h.applyOnly();
                const float step = EvolveEngine::crushGridOctaves (amount);
                int liveBefore = 0, liveAfter = 0;
                for (int i = 0; i < h.numNodes(); ++i)
                {
                    if (! (base[(size_t) i].active && base[(size_t) i].weight > 0.0f)) continue;
                    ++liveBefore;
                    const auto& n = h.node (i);
                    if (n.weight > 0.0f) ++liveAfter;
                    if (n.frequency >= 0.4f * 48000.0f) continue;   // faded at the top of the band, not on the grid by design
                    const double lr = std::log2 (n.frequency / h.fundamental());
                    const double off = std::abs (lr - std::round (lr / step) * step) * 1200.0;
                    expect (off < 1.0, "node off the crush grid by " + juce::String (off) + " cents");
                    expect (n.nonlinearity > base[(size_t) i].nonlinearity, "crush should add nonlinearity");
                }
                EvolveDiag d; h.evolve.fillDiagnostics (d);
                expect (liveAfter < liveBefore, "crush " + juce::String (amount) + " should thin the node count: " + juce::String (liveBefore) + " -> " + juce::String (liveAfter));
                expectEquals (d.crushDropped, liveBefore - liveAfter);
            }
            // Digital decay: between two hold ticks the written weight of the ringing fundamental rises to hold its
            // level while the modal amplitude decays, then drops at the tick — a staircase, not a slope.
            {
                Harness h (48000.0, 128);
                h.set (Param::evolveMotion, 0.0f); h.set (Param::evolveCrush, 1.0f);
                h.start (60);
                std::vector<float> written;
                for (int b = 0; b < 200; ++b)     // 0.53 s: several 160 ms holds
                {
                    h.applyOnly();
                    written.push_back (h.node (0).weight);
                    h.step();
                }
                int rises = 0, drops = 0;
                for (size_t b = 30; b + 4 < written.size(); ++b)
                {
                    if (written[b + 1] > written[b] * 1.002f) ++rises;
                    if (written[b + 4] < written[b] * 0.85f) ++drops;      // a level step, smoothed over ~8 ms
                }
                expect (rises > 60, "the held level should lift the weight while the node decays: " + juce::String (rises));
                expect (drops >= 1, "at least one level step down expected: " + juce::String (drops));
                // ...and the output level really is held: fit the slope of the 10 ms level track over 100 ms windows.
                // Somewhere inside a hold the crushed ring is flat; the plain object falls ~3.5 dB per 100 ms everywhere.
                auto slopes = [] (const std::vector<float>& out, float& flattest, float& steepestFlat)
                {
                    const int win = 480;
                    std::vector<double> track;
                    for (int start = 0; start + win <= (int) out.size(); start += win)
                        track.push_back (20.0 * std::log10 (rmsOf (out, start, start + win) + 1.0e-9));
                    flattest = -1.0e9f; steepestFlat = 1.0e9f;
                    for (size_t s0 = 2; s0 + 10 <= track.size(); ++s0)
                    {
                        double sx = 0, sy = 0, sxx = 0, sxy = 0;
                        for (int k = 0; k < 10; ++k) { sx += k; sy += track[s0 + (size_t) k]; sxx += k * k; sxy += k * track[s0 + (size_t) k]; }
                        const double slope = (10 * sxy - sx * sy) / (10 * sxx - sx * sx) * 10.0;   // dB per 100 ms
                        if (track[s0] < -80.0) continue;
                        flattest = std::max (flattest, (float) slope);
                        steepestFlat = std::min (steepestFlat, (float) slope);
                    }
                };
                float crushedFlattest, crushedMin, plainFlattest, plainMin;
                slopes (h.render (0.6), crushedFlattest, crushedMin);
                Harness p (48000.0, 128); p.start (60);
                for (int b = 0; b < 200; ++b) p.step();
                slopes (p.render (0.6), plainFlattest, plainMin);
                expect (crushedFlattest > -1.0f, "a crushed ring should hold its level inside a hold: flattest slope " + juce::String (crushedFlattest) + " dB/100ms");
                expect (plainFlattest < -2.5f, "the plain ring should fall steadily: flattest slope " + juce::String (plainFlattest) + " dB/100ms");
            }
        }

        beginTest ("Re-applied every block, never accumulating: identical node values on consecutive blocks");
        {
            Harness h (48000.0, 128);
            h.set (Param::evolveMotion, 0.0f);
            h.set (Param::evolveBend, 0.7f); h.set (Param::evolveMelt, 0.5f); h.set (Param::evolveTear, 0.5f);
            h.set (Param::evolveMagnet, 0.5f); h.set (Param::evolveGravity, 0.8f); h.set (Param::evolveScatter, 0.5f);
            h.start (60);
            h.applyOnly();
            const auto first = snapshot (h);
            for (int b = 0; b < 40; ++b)
            {
                h.step();
                h.applyOnly();
                for (int i = 0; i < h.numNodes(); ++i)
                {
                    const auto& n = h.node (i);
                    const auto& f = first[(size_t) i];
                    expect (sameBits (n.frequency, f.frequency) && sameBits (n.weight, f.weight) && sameBits (n.damping, f.damping)
                            && sameBits (n.pan, f.pan) && sameBits (n.excitation, f.excitation) && sameBits (n.nonlinearity, f.nonlinearity),
                            "node " + juce::String (i) + " drifted on block " + juce::String (b));
                }
            }
        }

        beginTest ("Bypass mask: a bypassed operator is an identity; diagnostics report the applied amounts");
        {
            Diagnostics diag;
            Harness h (48000.0, 128, &diag);
            h.set (Param::evolveBend, 1.0f); h.set (Param::evolveMotion, 0.0f);
            h.start (60);
            diag.dev.evolveBypassMask.store (evolveBypassBit (EvolveOperator::Bend));
            const auto base = snapshot (h);
            h.applyOnly();
            for (int i = 0; i < h.numNodes(); ++i) expect (identical (base[(size_t) i], h.node (i)), "bypassed bend moved a node");
            EvolveDiag d; h.evolve.fillDiagnostics (d);
            expectEquals (d.amount[(int) EvolveOperator::Bend], 0.0f);
            expectEquals ((int) d.bypassMask, (int) evolveBypassBit (EvolveOperator::Bend));
            diag.dev.evolveBypassMask.store (0u);
            h.applyOnly();
            h.evolve.fillDiagnostics (d);
            expectEquals (d.amount[(int) EvolveOperator::Bend], 1.0f);
            expect (d.nodesMoved > 0 && d.meanAbsCents > 10.0f && d.maxAbsCents >= d.meanAbsCents, "diagnostics should describe the bend");
            expectWithinAbsoluteError (d.fundamentalHz, h.fundamental(), 0.01f);
            expectWithinAbsoluteError (d.motionRateHz, EvolveEngine::motionRateHz (0.3f), 1.0e-6f);
        }

        beginTest ("Motion: SPEED/MOTION drift the active operators over time without leaving their range");
        {
            expectWithinAbsoluteError (EvolveEngine::motionRateHz (0.0f), 0.02f, 1.0e-6f);
            expectWithinAbsoluteError (EvolveEngine::motionRateHz (1.0f), 8.0f, 1.0e-4f);
            Harness h (48000.0, 128);
            h.set (Param::evolveSpeed, 1.0f); h.set (Param::evolveMotion, 1.0f); h.set (Param::evolveScatter, 0.5f); h.set (Param::evolveBend, 0.5f);
            h.set (Param::evolveBendPivot, 0.0f); h.set (Param::evolveBendRange, 0.5f);
            h.start (60);
            h.applyOnly();
            const auto first = snapshot (h);
            int changed = 0;
            for (int b = 0; b < 30; ++b) h.step();
            h.applyOnly();
            for (int i = 0; i < h.numNodes(); ++i)
            {
                if (! h.live (i)) continue;
                const double c = cents (h.node (i).frequency, first[(size_t) i].frequency);
                if (std::abs (c) > 0.5) ++changed;
                expect (std::abs (cents (h.node (i).frequency, first[(size_t) i].targetFrequency)) < 1200.0, "motion pushed a node out of range");
            }
            expect (changed > 4, "motion should have drifted the nodes: " + juce::String (changed));
            expect (h.evolve.motionPhase() >= 0.0f && h.evolve.motionPhase() < 1.0f);
        }

        beginTest ("Extreme combinations at 44.1/48/88.2/96 kHz, blocks 32-1024: finite, no resonator resets");
        {
            Diagnostics diag;
            int trial = 0;
            for (double sr : { 44100.0, 48000.0, 88200.0, 96000.0 })
                for (int block : { 32, 128, 1024 })
                {
                    Harness h (sr, block, &diag);
                    const bool ultra = trial % 5 == 4;
                    for (auto p : { Param::evolveBend, Param::evolveMelt, Param::evolveTear, Param::evolveMagnet, Param::evolveScatter,
                                    Param::evolveCrush, Param::evolveSpeed, Param::evolveMotion, Param::evolveBendRange, Param::evolveBendCurve })
                        h.set (p, 1.0f);
                    h.set (Param::evolveGravity, trial % 2 == 0 ? 1.0f : 0.0f);
                    h.set (Param::evolveFreeze, trial % 3 == 0 ? 1.0f : 0.0f);
                    h.set (Param::evolveBendPivot, (float) (trial % 4) / 3.0f);
                    h.set (Param::evolveMagnetTarget, (float) (trial % 7));
                    h.set (Param::shapeDensity, 1.0f); h.set (Param::shapeCoupling, 1.0f); h.set (Param::shapeSurface, 1.0f); h.set (Param::shapeDecay, 1.0f);
                    h.set (Param::shapeMaterialA, (float) (trial % 9));
                    h.start (trial % 2 == 0 ? 24 : 108, 1, 1.0f, ultra ? Quality::Ultra : Quality::Normal);
                    expectEquals (h.numNodes(), ultra ? 128 : 64);
                    auto out = h.render (1.0, 440.0f, 1.0f);
                    expect (allFinite (out), "non-finite output in trial " + juce::String (trial));
                    expect (peakOf (out) < 8.0f, "runaway in trial " + juce::String (trial) + ": " + juce::String (peakOf (out)));
                    h.applyOnly();                        // the bounds apply to what Evolve writes, not to Matter's next baseline
                    for (int i = 0; i < h.numNodes(); ++i)
                    {
                        const auto& n = h.node (i);
                        if (! h.live (i) && ! (n.active && n.weight > 0.0f)) continue;   // inactive nodes are left untouched
                        expect (n.frequency >= 20.0f && n.frequency <= 0.45f * (float) sr + 0.01f, "frequency out of bounds");
                        expect (n.damping >= 2.0e-6f && n.damping <= 1.0f, "damping out of bounds");
                        expect (n.weight >= 0.0f && n.pan >= -1.0f && n.pan <= 1.0f && n.excitation >= 0.0f && n.excitation <= 2.0f
                                && n.nonlinearity >= 0.0f && n.nonlinearity <= 2.0f, "bounds violated");
                    }
                    ++trial;
                }
            expectEquals ((int) diag.safety.count (SafetyEvent::ResonatorReset), 0);
            expectEquals ((int) diag.safety.count (SafetyEvent::NaN), 0);
            expectEquals ((int) diag.safety.count (SafetyEvent::InvalidFrequency), 0);
            expectEquals ((int) diag.safety.count (SafetyEvent::InvalidCoefficient), 0);
            expectEquals ((int) diag.safety.count (SafetyEvent::FeedbackClamp), 0);
        }

        beginTest ("NaN / infinite parameters are ignored, the nodes stay valid");
        {
            Harness h (48000.0, 128);
            h.set (Param::evolveBend, std::numeric_limits<float>::quiet_NaN());
            h.set (Param::evolveMelt, std::numeric_limits<float>::infinity());
            h.set (Param::evolveGravity, -std::numeric_limits<float>::infinity());
            h.set (Param::evolveScatterSeed, std::numeric_limits<float>::quiet_NaN());
            h.set (Param::evolveTear, 1.0f);
            h.start (60);
            auto out = h.render (0.5);
            expect (allFinite (out));
            for (int i = 0; i < h.numNodes(); ++i)
                expect (std::isfinite (h.node (i).frequency) && std::isfinite (h.node (i).weight) && std::isfinite (h.node (i).damping), "non-finite node");
        }

        beginTest ("Full engine: a 16-note chord with everything at 1 (dry Matter + Evolve) is finite and free of resets");
        {
            SynthEngine engine;
            engine.prepare (48000.0, 128);
            auto params = ParameterRegistry::defaults();
            for (auto p : { Param::evolveBend, Param::evolveMelt, Param::evolveTear, Param::evolveMagnet, Param::evolveScatter,
                            Param::evolveCrush, Param::evolveSpeed, Param::evolveMotion, Param::evolveGravity, Param::evolveFreeze })
                params[(size_t) paramIndex (p)] = 1.0f;
            engine.control().resetTo (params);
            engine.diagnostics().dev.dryMode.store ((int) DryMode::MatterAndEvolve);
            const int total = 48000 * 2;
            juce::AudioBuffer<float> audio (2, total);
            audio.clear();
            TransportInfo transport;
            for (int pos = 0; pos < total; pos += 128)
            {
                juce::MidiBuffer midi;
                if (pos == 0) for (int i = 0; i < 16; ++i) midi.addEvent (juce::MidiMessage::noteOn (1, 40 + i * 3, 0.9f), i);
                if (pos == 128 * 300) for (int i = 0; i < 16; ++i) midi.addEvent (juce::MidiMessage::noteOff (1, 40 + i * 3), 0);
                juce::AudioBuffer<float> chunk (audio.getArrayOfWritePointers(), 2, pos, 128);
                engine.process (chunk, midi, params, transport);
            }
            bool finite = true; float peak = 0.0f;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < total; ++i) { const float v = audio.getSample (ch, i); if (! std::isfinite (v)) finite = false; else peak = std::max (peak, std::abs (v)); }
            expect (finite, "non-finite engine output");
            expect (peak <= 1.0f, "peak above the ceiling");
            const auto& safety = engine.diagnostics().safety;
            expectEquals ((int) safety.count (SafetyEvent::ResonatorReset), 0);
            expectEquals ((int) safety.count (SafetyEvent::NaN), 0);
            expectEquals ((int) safety.count (SafetyEvent::Infinity), 0);
            expectEquals ((int) safety.count (SafetyEvent::InvalidCoefficient), 0);
            expectEquals ((int) safety.count (SafetyEvent::InvalidFrequency), 0);
            // The published snapshot carries the Evolve block for the focus voice.
            auto snapshotPtr = std::make_unique<DiagnosticSnapshot>();
            expect (engine.diagnostics().diagnosticSnapshots.read (*snapshotPtr), "no diagnostic snapshot published");
            expect (snapshotPtr->evolve.freeze == 1, "snapshot should report freeze");
            expectEquals (snapshotPtr->evolve.amount[(int) EvolveOperator::Bend], 1.0f);
            expect (snapshotPtr->evolve.activeNodes > 0 && snapshotPtr->evolve.nodesMoved > 0, "snapshot should describe the focus voice");
        }
    }
};

static EvolveTests evolveTests;

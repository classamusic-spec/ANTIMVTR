#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include "dsp/matter/MatterEngine.h"
#include "dsp/SynthEngine.h"
#include <chrono>

using namespace am;

namespace
{
    struct Harness
    {
        MatterEngine matter;
        ParamValues params = ParameterRegistry::defaults();
        NoteState note;
        RenderContext ctx;
        double sr;
        int block;
        std::vector<float> zeroL, zeroR, outL, outR;
        Diagnostics* diag = nullptr;

        Harness (double sampleRate, int blockSize, Diagnostics* d = nullptr) : sr (sampleRate), block (blockSize), diag (d)
        {
            zeroL.assign ((size_t) blockSize, 0.0f); zeroR.assign ((size_t) blockSize, 0.0f);
            outL.assign ((size_t) blockSize, 0.0f); outR.assign ((size_t) blockSize, 0.0f);
            matter.prepare (sr, blockSize);
            ctx.sampleRate = sr; ctx.numSamples = blockSize; ctx.params = &params; ctx.diagnostics = d;
        }

        void set (Param p, float v) { params[(size_t) paramIndex (p)] = v; }

        void start (int midiNote, float velocity = 0.8f, Quality q = Quality::Normal)
        {
            note = NoteState();
            note.midiNote = midiNote; note.velocity = velocity; note.gate = true; note.noteId = 1;
            note.baseFrequency = note.frequency = midiNoteToHz (midiNote);
            matter.noteOn (note, params, q);
        }

        /** Renders `seconds` of silence-driven output (strike only) into a mono vector. Optional sine excitation. */
        std::vector<float> render (double seconds, float sineHz = 0.0f, float sineAmp = 0.0f, bool stereoSum = true)
        {
            const int total = (int) (seconds * sr);
            std::vector<float> out; out.reserve ((size_t) total);
            std::vector<float> excL ((size_t) block), excR ((size_t) block);
            double ph = 0.0;
            for (int pos = 0; pos < total; pos += block)
            {
                const int n = std::min (block, total - pos);
                for (int i = 0; i < n; ++i)
                {
                    const float s = sineAmp > 0.0f ? sineAmp * (float) std::sin (ph) : 0.0f;
                    excL[(size_t) i] = excR[(size_t) i] = s;
                    ph += kTwoPi * sineHz / sr;
                }
                ctx.numSamples = n;
                matter.process (excL.data(), excR.data(), outL.data(), outR.data(), n, ctx, note);
                for (int i = 0; i < n; ++i) out.push_back (stereoSum ? 0.5f * (outL[(size_t) i] + outR[(size_t) i]) : outL[(size_t) i]);
            }
            return out;
        }
    };

    bool allFinite (const std::vector<float>& v) { for (float x : v) if (! std::isfinite (x)) return false; return true; }
    float peakOf (const std::vector<float>& v) { float p = 0.0f; for (float x : v) p = std::max (p, std::abs (x)); return p; }
    float rmsOf (const std::vector<float>& v, int start, int end)
    {
        double s = 0.0; int n = 0;
        for (int i = std::max (0, start); i < std::min (end, (int) v.size()); ++i) { s += (double) v[(size_t) i] * v[(size_t) i]; ++n; }
        return n > 0 ? (float) std::sqrt (s / n) : 0.0f;
    }

    /** Frequency of the dominant sinusoid via zero crossings with linear interpolation over [start, end). */
    double measureFrequency (const std::vector<float>& v, double sr, int start, int end)
    {
        double first = -1.0, last = -1.0; int count = 0;
        for (int i = std::max (1, start); i < std::min (end, (int) v.size()); ++i)
        {
            if (v[(size_t) (i - 1)] < 0.0f && v[(size_t) i] >= 0.0f)
            {
                const double frac = v[(size_t) (i - 1)] / (v[(size_t) (i - 1)] - v[(size_t) i]);
                const double t = (double) (i - 1) + frac;
                if (first < 0.0) first = t; else { last = t; ++count; }
            }
        }
        return count > 0 ? sr * (double) count / (last - first) : 0.0;
    }

    /** Time (s) for the envelope to fall 60 dB (or 20 dB → extrapolated) after `from`. */
    double measureT60 (const std::vector<float>& v, double sr, int from)
    {
        const int win = (int) (sr * 0.01);
        const float ref = rmsOf (v, from, from + win);
        if (ref <= 0.0f) return 0.0;
        double t20 = -1.0;
        for (int i = from; i + win <= (int) v.size(); i += win)
        {
            const float r = rmsOf (v, i, i + win);
            const double db = 20.0 * std::log10 (std::max (1.0e-12f, r) / ref);
            if (db <= -60.0) return (double) (i - from) / sr;
            if (t20 < 0.0 && db <= -20.0) t20 = (double) (i - from) / sr;
        }
        return t20 > 0.0 ? t20 * 3.0 : 1.0e9;
    }

    struct Spectrum
    {
        std::vector<float> mags; double binHz;
        double centroid() const { double n = 0, d = 0; for (size_t k = 1; k < mags.size(); ++k) { n += mags[k] * (double) k * binHz; d += mags[k]; } return d > 0 ? n / d : 0; }
        double flatness() const { double lg = 0, mean = 0; size_t c = 0; for (size_t k = 1; k < mags.size(); ++k) { lg += std::log (mags[k] + 1e-12); mean += mags[k]; ++c; } return std::exp (lg / c) / (mean / c + 1e-12); }
        /** Frequencies of the spectral peaks above `relativeDb` (relative to the strongest bin), ascending. */
        std::vector<double> peakFrequencies (double relativeDb) const
        {
            float mx = 0.0f; for (float m : mags) mx = std::max (mx, m);
            const float th = mx * (float) std::pow (10.0, relativeDb / 20.0);
            std::vector<double> out;
            for (size_t k = 2; k + 2 < mags.size(); ++k)
                if (mags[k] > th && mags[k] > mags[k - 1] && mags[k] > mags[k + 1]) out.push_back ((double) k * binHz);
            return out;
        }
        int peaks (double relativeDb) const
        {
            float mx = 0.0f; for (float m : mags) mx = std::max (mx, m);
            const float th = mx * (float) std::pow (10.0, relativeDb / 20.0);
            int n = 0;
            for (size_t k = 2; k + 2 < mags.size(); ++k)
                if (mags[k] > th && mags[k] > mags[k - 1] && mags[k] > mags[k + 1]) ++n;
            return n;
        }
    };

    Spectrum spectrumOf (const std::vector<float>& v, double sr, int start, int order = 13)
    {
        const int size = 1 << order;
        juce::dsp::FFT fft (order);
        std::vector<float> data ((size_t) size * 2, 0.0f);
        for (int i = 0; i < size; ++i) data[(size_t) i] = (size_t) (start + i) < v.size() ? v[(size_t) (start + i)] * (0.5f - 0.5f * (float) std::cos (kTwoPi * i / size)) : 0.0f;
        fft.performFrequencyOnlyForwardTransform (data.data(), true);
        Spectrum s; s.binHz = sr / size;
        s.mags.assign (data.begin(), data.begin() + size / 2);
        return s;
    }
}

class MatterTests : public juce::UnitTest
{
public:
    MatterTests() : juce::UnitTest ("Matter engine", "matter") {}

    void runTest() override
    {
        beginTest ("Resonator frequency accuracy at 44.1/48/88.2/96 kHz (±0.5%)");
        {
            for (double sr : { 44100.0, 48000.0, 88200.0, 96000.0 })
                for (int midi : { 36, 60, 84 })
                {
                    Harness h (sr, 128);
                    h.set (Param::shapeMaterialA, 8); h.set (Param::shapeMaterialB, 8);   // CUSTOM: form structure only
                    h.set (Param::shapeDensity, 0.0f);                                     // fundamental only
                    h.set (Param::shapeSurface, 0.0f); h.set (Param::shapeCoupling, 0.0f);
                    h.set (Param::shapeStrike, 1.0f); h.set (Param::shapeDecay, 0.8f); h.set (Param::shapeMass, 0.2f);
                    h.start (midi);
                    auto out = h.render (1.0);
                    expect (allFinite (out));
                    const double expected = midiNoteToHz (midi);
                    const double f = measureFrequency (out, sr, (int) (0.1 * sr), (int) (0.9 * sr));
                    expectWithinAbsoluteError (f, expected, expected * 0.005, "sr " + juce::String (sr) + " note " + juce::String (midi) + " got " + juce::String (f));
                    expectWithinAbsoluteError ((double) h.matter.renderedFrequency (0), expected, expected * 0.001);
                }
        }

        beginTest ("Decay control: T60 monotonic, decay 0 short, decay 1 several seconds, frequency dependent");
        {
            double previous = 0.0;
            for (float decay : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
            {
                Harness h (48000.0, 128);
                h.set (Param::shapeMaterialA, 5); h.set (Param::shapeMaterialB, 5);   // STRING
                h.set (Param::shapeDensity, 0.0f); h.set (Param::shapeSurface, 0.0f); h.set (Param::shapeCoupling, 0.0f);
                h.set (Param::shapeStrike, 1.0f); h.set (Param::shapeDecay, decay);
                h.start (60);
                auto out = h.render (decay >= 0.75f ? 12.0 : 4.0);
                const double t60 = measureT60 (out, 48000.0, 480);
                expect (t60 > previous, "T60 not monotonic at decay " + juce::String (decay) + ": " + juce::String (t60));
                if (decay == 0.0f) expect (t60 < 0.15, "decay 0 too long: " + juce::String (t60));
                if (decay == 1.0f) expect (t60 > 3.0, "decay 1 too short: " + juce::String (t60));
                previous = t60;
            }
            // Frequency dependence: high modes decay faster than the fundamental (MEMBRANE).
            Harness h (48000.0, 128);
            h.set (Param::shapeMaterialA, 4); h.set (Param::shapeMaterialB, 4);
            h.set (Param::shapeDensity, 1.0f); h.set (Param::shapeStrike, 1.0f); h.set (Param::shapeDecay, 0.6f);
            h.start (48);
            h.render (0.05);
            float t60Low = 0.0f, t60High = 0.0f, fLow = 0.0f, fHigh = 0.0f;
            for (int i = 0; i < h.matter.numNodes(); ++i)
            {
                const auto& n = h.matter.node (i);
                if (! n.active) continue;
                const float t60 = 6.9078f / (n.damping * 48000.0f);
                if (fLow == 0.0f || n.frequency < fLow) { fLow = n.frequency; t60Low = t60; }
                if (n.frequency > fHigh) { fHigh = n.frequency; t60High = t60; }
            }
            expect (fHigh > fLow * 4.0f && t60High < t60Low * 0.5f, "high modes should decay faster: " + juce::String (t60Low) + " vs " + juce::String (t60High));
        }

        beginTest ("Extreme parameters: finite, bounded, no runaway (all 0, all 1, random, 128 nodes)");
        {
            Diagnostics diag;
            juce::Random rnd (1234);
            for (int trial = 0; trial < 14; ++trial)
            {
                Harness h (48000.0, trial % 3 == 0 ? 32 : 128, &diag);
                const bool allOne = trial == 1, allZero = trial == 0;
                for (auto p : { Param::shapeDensity, Param::shapeForm, Param::shapeMass, Param::shapeTension, Param::shapeDecay,
                                Param::shapeSurface, Param::shapeBlend, Param::shapeCoupling, Param::shapeDistribution,
                                Param::shapeStereo, Param::shapeExcite, Param::shapeStrike, Param::shapeKeytrack })
                    h.set (p, allOne ? 1.0f : allZero ? 0.0f : rnd.nextFloat());
                h.set (Param::shapeMaterialA, allOne ? 8.0f : (float) rnd.nextInt (9));
                h.set (Param::shapeMaterialB, allOne ? 8.0f : (float) rnd.nextInt (9));
                h.set (Param::shapeTopology, (float) rnd.nextInt (6));
                h.set (Param::shapePitch, allOne ? 24.0f : allZero ? -24.0f : rnd.nextFloat() * 48.0f - 24.0f);
                h.set (Param::shapeSeed, (float) rnd.nextInt (9999));
                if (trial >= 2 && trial < 5) { h.set (Param::shapeCoupling, 1.0f); h.set (Param::shapeSurface, 1.0f); h.set (Param::shapeDecay, 1.0f); }
                h.start (trial % 2 == 0 ? 24 : 108, 1.0f, Quality::Ultra);
                expectEquals (h.matter.numNodes(), 128);
                auto out = h.render (2.0, 440.0f, 1.0f);
                expect (allFinite (out), "non-finite output in trial " + juce::String (trial));
                expect (peakOf (out) < 4.0f, "runaway in trial " + juce::String (trial) + " peak " + juce::String (peakOf (out)));
                // Energy must not grow once excitation has stopped (coupling / nonlinearity stability).
                h.note.gate = false;
                auto tail = h.render (2.0);
                const float early = rmsOf (tail, 0, 4800), late = rmsOf (tail, (int) tail.size() - 4800, (int) tail.size());
                expect (late <= early * 1.05f + 1.0e-6f, "energy grew after excitation stopped in trial " + juce::String (trial) + ": " + juce::String (early) + " -> " + juce::String (late));
            }
            expectEquals ((int) diag.safety.count (SafetyEvent::ResonatorReset), 0);
            expectEquals ((int) diag.safety.count (SafetyEvent::NaN), 0);
        }

        beginTest ("Determinism with a fixed seed; different seeds differ");
        {
            auto renderSeed = [] (int seed)
            {
                Harness h (48000.0, 128);
                h.set (Param::shapeMaterialA, 3); h.set (Param::shapeForm, 1.0f); h.set (Param::shapeSeed, (float) seed);
                h.set (Param::shapeSurface, 0.6f); h.set (Param::shapeTopology, 4.0f);
                h.start (60);
                return h.render (0.5, 261.63f, 0.5f);
            };
            auto a = renderSeed (42), b = renderSeed (42), c = renderSeed (43);
            bool identical = a.size() == b.size();
            for (size_t i = 0; identical && i < a.size(); ++i) identical = a[i] == b[i];
            expect (identical, "same seed must reproduce bit-identically");
            double diff = 0.0; for (size_t i = 0; i < a.size(); ++i) diff += std::abs ((double) a[i] - c[i]);
            expect (diff > 1.0, "different seeds should produce different output");
        }

        beginTest ("Energy decays to silence after excitation stops");
        {
            Harness h (48000.0, 128);
            h.set (Param::shapeDecay, 0.3f); h.set (Param::shapeStrike, 1.0f);
            h.start (60);
            h.render (0.5, 261.63f, 1.0f);
            expect (h.matter.energy() > 0.01f, "no energy while driven");
            h.note.gate = false;
            h.render (6.0);
            expect (h.matter.energy() < 1.0e-4f, "energy did not decay: " + juce::String (h.matter.energy()));
            auto tail = h.render (0.1);
            expect (rmsOf (tail, 0, (int) tail.size()) < 1.0e-4f);
        }

        beginTest ("Materials differ (CRYSTAL / METAL / MEMBRANE / ORGANIC / STRING / LIQUID)");
        {
            struct Stat { double centroid, flatness; int peaks; double t60; std::vector<double> ratios; };
            const double f0 = midiNoteToHz (48);
            std::vector<Stat> stats;
            for (int m : { 0, 1, 4, 2, 5, 3 })
            {
                Harness h (48000.0, 128);
                h.set (Param::shapeMaterialA, (float) m); h.set (Param::shapeMaterialB, (float) m);
                h.set (Param::shapeStrike, 1.0f); h.set (Param::shapeDensity, 0.7f); h.set (Param::shapeExcite, 0.9f);
                h.start (48);
                auto out = h.render (2.0);
                expect (allFinite (out));
                auto s = spectrumOf (out, 48000.0, 0);
                std::vector<double> ratios;
                for (double f : s.peakFrequencies (-40.0)) if (f > f0 * 1.25 && ratios.size() < 6) ratios.push_back (std::log2 (f / f0));
                stats.push_back ({ s.centroid(), s.flatness(), s.peaks (-40.0), measureT60 (out, 48000.0, 240), ratios });
                logMessage (juce::String (materialName ((MaterialType) m)) + ": centroid " + juce::String (s.centroid(), 0) + " Hz, flatness "
                            + juce::String (s.flatness(), 4) + ", peaks " + juce::String (s.peaks (-40.0)) + ", T60 " + juce::String (stats.back().t60, 2));
            }
            int distinct = 0;
            for (size_t i = 0; i < stats.size(); ++i)
                for (size_t j = i + 1; j < stats.size(); ++j)
                {
                    // Modal structure distance: mean |Δ log2 ratio| of the first partials above the fundamental.
                    double structure = 0.0; size_t common = std::min (stats[i].ratios.size(), stats[j].ratios.size());
                    for (size_t k = 0; k < common; ++k) structure += std::abs (stats[i].ratios[k] - stats[j].ratios[k]);
                    structure = common >= 3 ? structure / (double) common : 0.0;
                    const bool differs = std::abs (stats[i].centroid - stats[j].centroid) > 0.15 * std::max (stats[i].centroid, stats[j].centroid)
                                      || std::abs (stats[i].t60 - stats[j].t60) > 0.3 * std::max (stats[i].t60, stats[j].t60)
                                      || std::abs (stats[i].peaks - stats[j].peaks) > 3
                                      || structure > 0.06;
                    if (differs) ++distinct;
                }
            expectEquals (distinct, 15, "every pair of materials should differ in centroid, ring time or partial count");
            expect (stats[1].centroid > stats[2].centroid, "METAL should be brighter than MEMBRANE");
            expect (stats[0].t60 > stats[2].t60, "CRYSTAL should ring longer than MEMBRANE");
        }

        beginTest ("Form sweep changes the partial structure");
        {
            std::vector<std::vector<float>> freqs;
            for (float form : { 0.0f, 0.5f, 1.0f })
            {
                Harness h (48000.0, 128);
                h.set (Param::shapeMaterialA, 8); h.set (Param::shapeMaterialB, 8); h.set (Param::shapeForm, form);
                h.set (Param::shapeDensity, 0.5f); h.set (Param::shapeSurface, 0.0f);
                h.start (60);
                h.render (0.01);
                std::vector<float> f;
                for (int i = 0; i < 8; ++i) f.push_back (h.matter.node (i).ratio);
                freqs.push_back (f);
            }
            // Harmonic: integer ratios; other forms are not.
            expectWithinAbsoluteError (freqs[0][1], 2.0f, 0.02f);
            expectWithinAbsoluteError (freqs[0][2], 3.0f, 0.03f);
            expect (std::abs (freqs[2][1] - 2.0f) > 0.05f || std::abs (freqs[2][2] - 3.0f) > 0.05f, "stochastic form should not be harmonic");
            expect (std::abs (freqs[1][2] - freqs[0][2]) > 0.05f, "form 0.5 should move partials");
        }

        beginTest ("Mass changes weight, not pitch; Keytrack and Pitch honoured");
        {
            float f0[2] = {}, centroid[2] = {};
            for (int k = 0; k < 2; ++k)
            {
                Harness h (48000.0, 128);
                h.set (Param::shapeMaterialA, 1); h.set (Param::shapeMass, k == 0 ? 0.1f : 0.9f); h.set (Param::shapeStrike, 1.0f);
                h.start (60);
                auto out = h.render (1.0);
                f0[k] = h.matter.node (0).frequency;
                centroid[k] = (float) spectrumOf (out, 48000.0, 0).centroid();
            }
            expectWithinAbsoluteError (f0[1], f0[0], 0.5f, "mass must not transpose the fundamental");
            expect (centroid[1] < centroid[0] * 0.75f, "heavier mass should be darker: " + juce::String (centroid[0]) + " -> " + juce::String (centroid[1]));

            Harness h (48000.0, 128);
            h.set (Param::shapeKeytrack, 0.0f); h.set (Param::shapeSurface, 0.0f);
            h.start (84);
            h.render (0.01);
            expectWithinAbsoluteError (h.matter.node (0).targetFrequency, 261.6256f, 0.5f, "keytrack 0 should hold the reference pitch");
            h.set (Param::shapeKeytrack, 1.0f); h.set (Param::shapePitch, 12.0f);
            h.render (0.01);
            expectWithinAbsoluteError (h.matter.node (0).targetFrequency, (float) midiNoteToHz (96), 1.0f, "pitch +12 should double the fundamental");
        }

        beginTest ("Sample-rate consistency (44.1 vs 96 kHz: similar centroid and T60)");
        {
            double c[2] = {}, t[2] = {};
            int k = 0;
            for (double sr : { 44100.0, 96000.0 })
            {
                Harness h (sr, 128);
                h.set (Param::shapeMaterialA, 1); h.set (Param::shapeStrike, 1.0f); h.set (Param::shapeSurface, 0.0f); h.set (Param::shapeDecay, 0.6f);
                h.start (55);
                auto out = h.render (3.0);
                c[k] = spectrumOf (out, sr, 0, sr > 50000.0 ? 14 : 13).centroid();
                t[k] = measureT60 (out, sr, (int) (0.005 * sr));
                ++k;
            }
            expectWithinAbsoluteError (c[1], c[0], c[0] * 0.15, "centroid differs between sample rates: " + juce::String (c[0]) + " vs " + juce::String (c[1]));
            expectWithinAbsoluteError (t[1], t[0], t[0] * 0.2, "T60 differs between sample rates: " + juce::String (t[0]) + " vs " + juce::String (t[1]));
        }

        beginTest ("Coupling: edges published, energy bounded at coupling 1 with long decay");
        {
            Harness h (48000.0, 128);
            h.set (Param::shapeCoupling, 1.0f); h.set (Param::shapeDecay, 1.0f); h.set (Param::shapeTopology, 3.0f);
            h.set (Param::shapeMaterialA, 1); h.set (Param::shapeStrike, 1.0f);
            h.start (48);
            auto out = h.render (1.0);
            EdgeDiag edges[kMaxMatterEdges];
            const int n = h.matter.fillEdgeDiagnostics (edges, kMaxMatterEdges);
            expect (n > 32, "lattice should publish many edges");
            float avg = 0.0f, mx = 0.0f;
            h.matter.couplingStats (avg, mx);
            expect (avg > 0.0f && mx >= avg);
            expect (h.matter.lastCouplingScale() <= 1.0f);
            NodeDiag nodes[kMaxMatterNodes];
            h.matter.fillDiagnostics (nodes, kMaxMatterNodes);
            int coupled = 0; for (int i = 0; i < h.matter.numNodes(); ++i) if (nodes[i].couplingCount > 0) ++coupled;
            expect (coupled == h.matter.numNodes());
            h.note.gate = false;
            auto tail = h.render (4.0);
            expect (allFinite (tail));
            expect (rmsOf (tail, (int) tail.size() - 48000, (int) tail.size()) < rmsOf (tail, 0, 48000) * 1.01f + 1.0e-6f, "coupled system must not gain energy");
        }

        beginTest ("Parameter moves while ringing are click-free");
        {
            Harness h (48000.0, 64);
            h.set (Param::shapeStrike, 0.0f); h.set (Param::shapeDecay, 0.9f); h.set (Param::shapeMaterialA, 5);
            h.start (57);
            h.render (1.0, 220.0f, 1.0f);
            std::vector<float> all;
            for (int step = 0; step < 300; ++step)
            {
                const float t = (float) step / 300.0f;
                h.set (Param::shapeDensity, 0.2f + 0.7f * t); h.set (Param::shapeMass, 0.9f - 0.8f * t);
                h.set (Param::shapeForm, t); h.set (Param::shapeDecay, 0.9f - 0.5f * t); h.set (Param::shapeStereo, t);
                auto chunk = h.render (64.0 / 48000.0, 220.0f, 1.0f, false);
                all.insert (all.end(), chunk.begin(), chunk.end());
            }
            float maxStep = 0.0f, peak = peakOf (all);
            for (size_t i = 1; i < all.size(); ++i) maxStep = std::max (maxStep, std::abs (all[i] - all[i - 1]));
            expect (allFinite (all));
            expect (maxStep < peak * 0.6f, "discontinuity while sweeping: step " + juce::String (maxStep) + " peak " + juce::String (peak));
        }

        beginTest ("Full engine: 16-note chord stays finite and limited; strike alone rings with a silent source");
        {
            auto params = ParameterRegistry::defaults();
            params[(size_t) paramIndex (Param::waveLevel)] = 0.0f;
            params[(size_t) paramIndex (Param::shapeStrike)] = 1.0f;
            SynthEngine engine;
            engine.prepare (48000.0, 128);
            engine.control().resetTo (params);
            engine.diagnostics().dev.dryMode.store ((int) DryMode::MatterOnly);
            juce::AudioBuffer<float> buf (2, 48000);
            buf.clear();
            TransportInfo tr;
            for (int pos = 0; pos < 48000; pos += 128)
            {
                juce::MidiBuffer midi;
                if (pos == 0) for (int i = 0; i < 16; ++i) midi.addEvent (juce::MidiMessage::noteOn (1, 40 + i * 3, 0.9f), 0);
                juce::AudioBuffer<float> chunk (buf.getArrayOfWritePointers(), 2, pos, 128);
                engine.process (chunk, midi, params, tr);
            }
            float peak = 0.0f; bool finite = true; double sum = 0.0;
            for (int i = 0; i < 48000; ++i) { const float v = buf.getSample (0, i); if (! std::isfinite (v)) finite = false; else { peak = std::max (peak, std::abs (v)); sum += (double) v * v; } }
            expect (finite);
            expect (peak <= 1.0f);
            expect (std::sqrt (sum / 48000.0) > 0.01, "strike should ring with a silent source");
            expectEquals ((int) engine.diagnostics().safety.count (SafetyEvent::NaN), 0);
            expectEquals ((int) engine.diagnostics().safety.count (SafetyEvent::ResonatorReset), 0);
        }

        beginTest ("Legato replica (diagnostic)");
        {
            auto mono = ParameterRegistry::defaults();
            mono[(size_t) paramIndex (Param::ampAttack)] = 0.001f;
            mono[(size_t) paramIndex (Param::ampRelease)] = 0.05f;
            mono[(size_t) paramIndex (Param::masterMode)] = 2.0f;
            mono[(size_t) paramIndex (Param::masterGlide)] = 0.2f;
            SynthEngine engine;
            engine.prepare (48000.0, 128);
            engine.control().resetTo (mono);
            juce::AudioBuffer<float> audio (2, 48000);
            audio.clear();
            TransportInfo transport;
            juce::String trace;
            for (int pos = 0; pos < 48000; pos += 128)
            {
                juce::MidiBuffer midi;
                if (pos == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 48, 0.8f), 0);
                if (pos == 128 * 20) midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
                if (pos == 128 * 200) midi.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
                if (pos == 43200) midi.addEvent (juce::MidiMessage::noteOff (1, 48), 0);
                juce::AudioBuffer<float> chunk (audio.getArrayOfWritePointers(), 2, pos, 128);
                engine.process (chunk, midi, mono, transport);
                if (pos % (128 * 10) == 0 && pos <= 128 * 120)
                {
                    const int fv = engine.voiceManager().mostRecentVoice();
                    const auto& m = engine.voiceManager().voice (fv).matter();
                    trace += juce::String (pos / 128) + ":" + juce::String (engine.voiceManager().voice (fv).noteState().frequency, 1) + "/" + juce::String (m.renderedFrequency (0), 1) + "/" + juce::String (m.node (0).energy, 3) + " ";
                }
            }
            auto zc = [&] (int a, int b) { int c = 0; for (int i = a + 1; i < b; ++i) if ((audio.getSample (0, i - 1) < 0.0f) != (audio.getSample (0, i) < 0.0f)) ++c; return c; };
            logMessage ("legato replica crossings: early " + juce::String (zc (128 * 20, 128 * 20 + 2400)) + " late " + juce::String (zc (128 * 90, 128 * 90 + 2400)) + " back " + juce::String (zc (128 * 300, 128 * 300 + 2400)));
            logMessage ("block:noteHz/node0Hz/energy " + trace);
        }

        beginTest ("Benchmark: 16 voices x 64 nodes, 48 kHz / 128 samples (informational)");
        {
            std::vector<std::unique_ptr<Harness>> voices;
            for (int v = 0; v < 16; ++v)
            {
                voices.push_back (std::make_unique<Harness> (48000.0, 128));
                voices.back()->set (Param::shapeCoupling, 0.3f);
                voices.back()->start (40 + v * 3);
            }
            std::vector<float> excL (128), excR (128);
            for (int i = 0; i < 128; ++i) excL[(size_t) i] = excR[(size_t) i] = 0.5f * (float) std::sin (i * 0.1);
            const int blocks = 400;
            const auto t0 = std::chrono::steady_clock::now();
            for (int b = 0; b < blocks; ++b)
                for (auto& v : voices)
                    v->matter.process (excL.data(), excR.data(), v->outL.data(), v->outR.data(), 128, v->ctx, v->note);
            const auto t1 = std::chrono::steady_clock::now();
            const double perBlockUs = std::chrono::duration<double, std::micro> (t1 - t0).count() / blocks;
            const double budgetUs = 128.0 / 48000.0 * 1.0e6;
            logMessage ("Matter 16 voices x 64 nodes: " + juce::String (perBlockUs, 1) + " us per block = "
                        + juce::String (100.0 * perBlockUs / budgetUs, 1) + "% of the 128-sample budget");
            // Breakdown: the bank alone (uncoupled / coupled), 16 x 64 nodes.
            for (int coupled = 0; coupled < 2; ++coupled)
            {
                std::vector<std::unique_ptr<ModalBank>> banks;
                std::vector<float> k (64, 0.002f);
                for (int v = 0; v < 16; ++v)
                {
                    banks.push_back (std::make_unique<ModalBank>());
                    auto& bk = *banks.back();
                    bk.setCount (64);
                    for (int i = 0; i < 64; ++i)
                    {
                        float sn, cs; fastSinCos (0.01f * (float) (i + 1), sn, cs);
                        bk.targetCos()[i] = 0.9999f * cs; bk.targetSin()[i] = 0.9999f * sn;
                        bk.targetGainL()[i] = bk.targetGainR()[i] = 0.05f; bk.inputGain()[i] = 0.01f; bk.strikeGain()[i] = 0.1f;
                    }
                    bk.snapToTargets();
                    if (coupled) { bk.setBandA (1, k.data()); bk.setBandB (8, k.data()); bk.finalizeCoupling (0.9999f); }
                }
                std::vector<float> oL (128), oR (128);
                const auto b0 = std::chrono::steady_clock::now();
                for (int b = 0; b < blocks; ++b)
                    for (auto& bk : banks) bk->process (excL.data(), excR.data(), oL.data(), oR.data(), 128);
                const auto b1 = std::chrono::steady_clock::now();
                const double us = std::chrono::duration<double, std::micro> (b1 - b0).count() / blocks;
                logMessage (juce::String (coupled ? "  bank coupled:   " : "  bank uncoupled: ") + juce::String (us, 1) + " us per block ("
                            + juce::String (100.0 * us / budgetUs, 1) + "%), " + juce::String (us * 1000.0 / (16.0 * 64.0 * 128.0), 2) + " ns per node-sample");
            }
            expect (perBlockUs < budgetUs * 0.6, "Matter is too expensive: " + juce::String (perBlockUs) + " us per block");
        }
    }
};

static MatterTests matterTests;

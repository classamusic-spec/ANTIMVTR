#include <juce_core/juce_core.h>

#include <functional>
#include <set>

#include "dev/diagnostics/DiagnosticReport.h"
#include "dev/diagnostics/MatterAnalysis.h"
#include "dev/diagnostics/PresetValidator.h"
#include "dev/diagnostics/SignalMetrics.h"
#include "dev/diagnostics/StressTestGenerator.h"
#include "dsp/SynthEngine.h"

using namespace am;
using namespace am::dev;

namespace
{
    /** Builds a synthetic node set: `n` partials on a harmonic series with a
        controllable detune in cents and a cluster every `perCluster` nodes. */
    std::vector<NodeDiag> makeNodes (int n, float fundamental, float detuneCents = 0.0f, int perCluster = 4)
    {
        std::vector<NodeDiag> nodes ((size_t) n);
        for (int i = 0; i < n; ++i)
        {
            auto& d = nodes[(size_t) i];
            const float harmonic = (float) (i + 1);
            d.targetFrequency = fundamental * harmonic;
            d.frequency = d.targetFrequency * std::pow (2.0f, detuneCents / 1200.0f);
            d.weight = 1.0f / harmonic;
            d.energy = 0.5f / harmonic;
            d.damping = 0.001f * harmonic;
            d.pan = (i % 2 == 0) ? -0.3f : 0.3f;
            d.excitation = 1.0f;
            d.cluster = (uint8_t) (i / juce::jmax (1, perCluster));
            d.couplingCount = (uint8_t) juce::jmin (4, i);
            d.active = 1;
        }
        return nodes;
    }

    void fillSine (std::vector<float>& buffer, double sampleRate, double freq, float amplitude)
    {
        for (size_t i = 0; i < buffer.size(); ++i)
            buffer[i] = amplitude * (float) std::sin (kTwoPi * freq * (double) i / sampleRate);
    }
}

//==============================================================================
class DSPLabSignalTests : public juce::UnitTest
{
public:
    DSPLabSignalTests() : juce::UnitTest ("DSP LAB signal metrics", "dsplab") {}

    void runTest() override
    {
        beginTest ("SignalMetrics measures peak, RMS, crest and DC of a sine");
        {
            std::vector<float> l (4800), r (4800);
            fillSine (l, 48000.0, 100.0, 0.5f);
            r = l;
            const auto m = SignalMetrics::measure (l.data(), r.data(), (int) l.size());
            expectWithinAbsoluteError (m.peak, 0.5f, 0.01f);
            expectWithinAbsoluteError (m.rms, 0.5f / std::sqrt (2.0f), 0.01f);
            expectWithinAbsoluteError (m.crestFactor, std::sqrt (2.0f), 0.05f);
            expectWithinAbsoluteError (m.dc, 0.0f, 0.01f);
            expectEquals (m.nonFinite, 0);
            expectWithinAbsoluteError (m.correlation, 1.0f, 0.001f);
        }

        beginTest ("SignalMetrics reports DC, inverted correlation and non-finite samples");
        {
            std::vector<float> l (1024, 0.25f), r (1024, -0.25f);
            auto m = SignalMetrics::measure (l.data(), r.data(), (int) l.size());
            expectWithinAbsoluteError (m.dc, 0.0f, 1.0e-6f);          // L and R cancel
            expectWithinAbsoluteError (m.correlation, -1.0f, 0.001f);

            std::fill (r.begin(), r.end(), 0.25f);
            m = SignalMetrics::measure (l.data(), r.data(), (int) l.size());
            expectWithinAbsoluteError (m.dc, 0.25f, 1.0e-6f);

            l[10] = std::numeric_limits<float>::quiet_NaN();
            l[11] = std::numeric_limits<float>::infinity();
            m = SignalMetrics::measure (l.data(), r.data(), (int) l.size());
            expectEquals (m.nonFinite, 2);
            expect (std::isfinite (m.peak) && std::isfinite (m.rms), "metrics stay finite");
        }

        beginTest ("SignalMetrics on an empty or null window is zero");
        {
            const auto m = SignalMetrics::measure (nullptr, nullptr, 0);
            expectEquals (m.peak, 0.0f);
            expectEquals (m.rms, 0.0f);
            expectEquals (m.nonFinite, 0);
        }

        beginTest ("SpectrumMeasurement finds the centroid of a sine and rates flatness");
        {
            SpectrumMeasurement spectrum (11);
            expectEquals (spectrum.fftSize(), 2048);
            expectEquals (spectrum.numBins(), 1024);

            std::vector<float> tone (2048);
            fillSine (tone, 48000.0, 1000.0, 0.7f);
            const float centroid = spectrum.spectralCentroid (tone.data(), (int) tone.size(), 48000.0);
            expectWithinAbsoluteError (centroid, 1000.0f, 60.0f);

            const float toneFlatness = spectrum.spectralFlatness (tone.data(), (int) tone.size());

            juce::Random rng (1234);
            std::vector<float> noise (2048);
            for (auto& s : noise) s = (float) rng.nextDouble() * 2.0f - 1.0f;
            const float noiseFlatness = spectrum.spectralFlatness (noise.data(), (int) noise.size());

            expect (noiseFlatness > toneFlatness * 4.0f,
                    "noise flatness " + juce::String (noiseFlatness) + " should dwarf tone flatness " + juce::String (toneFlatness));

            const float higher = spectrum.spectralCentroid (noise.data(), (int) noise.size(), 48000.0);
            expect (higher > centroid, "broadband centroid should sit above a 1 kHz tone");
        }

        beginTest ("SpectrumMeasurement magnitudes peak in the right bin");
        {
            SpectrumMeasurement spectrum (11);
            std::vector<float> tone (2048);
            fillSine (tone, 48000.0, 3000.0, 0.5f);
            std::vector<float> mags;
            spectrum.magnitudes (tone.data(), (int) tone.size(), mags);
            expectEquals ((int) mags.size(), 1024);

            const auto peakBin = (int) std::distance (mags.begin(), std::max_element (mags.begin(), mags.end()));
            const int expectedBin = (int) std::lround (3000.0 / (48000.0 / 2048.0));
            expect (std::abs (peakBin - expectedBin) <= 2,
                    "peak bin " + juce::String (peakBin) + " vs expected " + juce::String (expectedBin));
        }

        beginTest ("SweepSummary aggregates points and counts dangerous ones");
        {
            std::vector<SweepPoint> points;
            for (int i = 0; i < 10; ++i)
            {
                SweepPoint p;
                p.normalised = (float) i / 9.0f;
                p.peak = 0.1f * (float) i;
                p.cpuPercent = 5.0f * (float) i;
                p.centroidHz = 500.0f + 100.0f * (float) i;
                points.push_back (p);
            }
            points[7].nonFinite = 3;
            points[8].safetyDelta = 2;

            const auto s = SweepSummary::of (points, 0.99f, 80.0f);
            expectEquals (s.numPoints, 10);
            expectWithinAbsoluteError (s.maxPeak, 0.9f, 1.0e-5f);
            expectWithinAbsoluteError (s.maxCpuPercent, 45.0f, 1.0e-4f);
            expectWithinAbsoluteError (s.minCentroidHz, 500.0f, 1.0e-4f);
            expectWithinAbsoluteError (s.maxCentroidHz, 1400.0f, 1.0e-4f);
            expectEquals (s.totalNonFinite, 3);
            expectEquals ((int) s.totalSafety, 2);
            expectEquals (s.dangerousPoints, 2);
            expect (points[7].dangerous() && points[8].dangerous() && ! points[0].dangerous());

            expectEquals (SweepSummary::of ({}).numPoints, 0);
        }
    }
};

static DSPLabSignalTests dspLabSignalTests;

//==============================================================================
class DSPLabMatterAnalysisTests : public juce::UnitTest
{
public:
    DSPLabMatterAnalysisTests() : juce::UnitTest ("DSP LAB matter analysis", "dsplab") {}

    void runTest() override
    {
        beginTest ("centsFromNearestHarmonic finds the harmonic and the deviation");
        {
            int harmonic = 0;
            expectWithinAbsoluteError (centsFromNearestHarmonic (440.0f, 440.0f, &harmonic), 0.0f, 1.0e-3f);
            expectEquals (harmonic, 1);

            expectWithinAbsoluteError (centsFromNearestHarmonic (1320.0f, 440.0f, &harmonic), 0.0f, 1.0e-3f);
            expectEquals (harmonic, 3);

            // +50 cents above the 2nd harmonic
            const float sharp = 880.0f * std::pow (2.0f, 50.0f / 1200.0f);
            expectWithinAbsoluteError (centsFromNearestHarmonic (sharp, 440.0f, &harmonic), 50.0f, 0.01f);
            expectEquals (harmonic, 2);

            // below the fundamental snaps to harmonic 1 and reports a negative deviation
            expect (centsFromNearestHarmonic (300.0f, 440.0f, &harmonic) < 0.0f);
            expectEquals (harmonic, 1);

            expectEquals (centsFromNearestHarmonic (0.0f, 440.0f), 0.0f);
            expectEquals (centsFromNearestHarmonic (440.0f, 0.0f), 0.0f);
            expectEquals (centsFromNearestHarmonic (std::numeric_limits<float>::quiet_NaN(), 440.0f), 0.0f);
        }

        beginTest ("analyseNodes summarises a harmonic distribution");
        {
            const auto nodes = makeNodes (16, 100.0f, 0.0f, 4);
            std::vector<EdgeDiag> edges;
            for (int i = 0; i < 15; ++i)
                edges.push_back ({ (uint8_t) i, (uint8_t) (i + 1), 0, 0.1f * (float) (i + 1) });

            const auto s = analyseNodes (nodes.data(), (int) nodes.size(), edges.data(), (int) edges.size(), 100.0f);
            expectEquals (s.numNodes, 16);
            expectEquals (s.activeNodes, 16);
            expectEquals (s.clusterCount, 4);
            expectWithinAbsoluteError (s.minFrequency, 100.0f, 0.01f);
            expectWithinAbsoluteError (s.maxFrequency, 1600.0f, 0.01f);
            expectWithinAbsoluteError (s.meanRatio, 8.5f, 0.01f);       // mean of 1..16
            expectWithinAbsoluteError (s.meanAbsCents, 0.0f, 1.0e-3f);
            expectEquals (s.numEdges, 15);
            expectWithinAbsoluteError (s.maxStrength, 1.5f, 1.0e-4f);
            expectWithinAbsoluteError (s.meanStrength, 0.8f, 1.0e-4f);
        }

        beginTest ("analyseNodes reports harmonicity deviation for a detuned set");
        {
            const auto nodes = makeNodes (8, 220.0f, 25.0f, 8);
            const auto s = analyseNodes (nodes.data(), (int) nodes.size(), nullptr, 0, 220.0f);
            expectWithinAbsoluteError (s.meanAbsCents, 25.0f, 0.5f);
            expectWithinAbsoluteError (s.maxAbsCents, 25.0f, 0.5f);
            expectEquals (s.clusterCount, 1);
            expectEquals (s.numEdges, 0);
        }

        beginTest ("analyseNodes tolerates non-finite and empty input");
        {
            auto nodes = makeNodes (4, 100.0f);
            nodes[1].frequency = std::numeric_limits<float>::quiet_NaN();
            nodes[2].energy = std::numeric_limits<float>::infinity();
            const auto s = analyseNodes (nodes.data(), 4, nullptr, 0, 100.0f);
            expect (std::isfinite (s.meanAbsCents) && std::isfinite (s.totalEnergy) && std::isfinite (s.meanRatio));

            const auto empty = analyseNodes (nullptr, 0, nullptr, 0, 0.0f);
            expectEquals (empty.numNodes, 0);
            expectEquals (empty.clusterCount, 0);
        }

        beginTest ("ratioHistogram bins harmonics into their integer slots");
        {
            const auto nodes = makeNodes (8, 100.0f, 0.0f, 8);
            std::vector<float> bins;
            ratioHistogram (nodes.data(), (int) nodes.size(), 100.0f, 16.0f, 64, bins);
            expectEquals ((int) bins.size(), 64);

            const float largest = *std::max_element (bins.begin(), bins.end());
            expectWithinAbsoluteError (largest, 1.0f, 1.0e-5f);

            int occupied = 0;
            for (auto b : bins) if (b > 0.0f) ++occupied;
            expectEquals (occupied, 8);

            // ratio 1 lands in bin floor(1/16 * 64) = 4
            expect (bins[4] > 0.0f, "the fundamental should occupy bin 4");

            std::vector<float> none;
            ratioHistogram (nodes.data(), 8, 0.0f, 16.0f, 32, none);
            expectEquals ((int) none.size(), 32);
            expectEquals (*std::max_element (none.begin(), none.end()), 0.0f);
        }

        beginTest ("layoutTopology keeps every point inside the view and honours clusters");
        {
            const auto nodes = makeNodes (32, 110.0f, 0.0f, 8);
            const auto points = layoutTopology (nodes.data(), (int) nodes.size(), false);
            expectEquals ((int) points.size(), 32);
            for (const auto& p : points)
            {
                expect (p.x >= 0.0f && p.x <= 1.0f, "x in range: " + juce::String (p.x));
                expect (p.y >= 0.0f && p.y <= 1.0f, "y in range: " + juce::String (p.y));
                expect (p.energy >= 0.0f && p.energy <= 1.0f);
                expect (p.weight >= 0.0f && p.weight <= 1.0f);
            }

            const auto clustered = layoutTopology (nodes.data(), (int) nodes.size(), true);
            expectEquals ((int) clustered.size(), 4);       // 32 nodes / 8 per cluster

            expect (layoutTopology (nullptr, 0, false).empty());
        }

        beginTest ("logPosition maps the decade axis");
        {
            expectWithinAbsoluteError (logPosition (20.0f, 20.0f, 20000.0f), 0.0f, 1.0e-5f);
            expectWithinAbsoluteError (logPosition (20000.0f, 20.0f, 20000.0f), 1.0f, 1.0e-5f);
            expectWithinAbsoluteError (logPosition (632.45f, 20.0f, 20000.0f), 0.5f, 0.01f);
            expectEquals (logPosition (0.0f, 20.0f, 20000.0f), 0.0f);
            expectWithinAbsoluteError (logPosition (1.0e9f, 20.0f, 20000.0f), 1.0f, 1.0e-5f);
        }

        beginTest ("compareDistributions measures node movement between captures");
        {
            const auto before = makeNodes (12, 200.0f, 0.0f, 4);
            auto after = before;
            for (size_t i = 0; i < after.size(); ++i)
                after[i].frequency *= std::pow (2.0f, 30.0f / 1200.0f);     // everything +30 cents
            after[0].frequency = before[0].frequency;                        // except the first

            const auto d = compareDistributions (before.data(), (int) before.size(), after.data(), (int) after.size());
            expectEquals (d.nodesBefore, 12);
            expectEquals (d.nodesAfter, 12);
            expectEquals (d.movedNodes, 11);
            expectWithinAbsoluteError (d.maxAbsCents, 30.0f, 0.01f);
            expectWithinAbsoluteError (d.meanAbsCents, 30.0f * 11.0f / 12.0f, 0.1f);
            expectEquals (d.clustersBefore, 3);
            expectEquals (d.clustersAfter, 3);
            expect (d.energyBefore > 0.0f);

            const auto none = compareDistributions (nullptr, 0, nullptr, 0);
            expectEquals (none.movedNodes, 0);
            expectEquals (none.meanAbsCents, 0.0f);
        }
    }
};

static DSPLabMatterAnalysisTests dspLabMatterAnalysisTests;

//==============================================================================
class DSPLabReportTests : public juce::UnitTest
{
public:
    DSPLabReportTests() : juce::UnitTest ("DSP LAB diagnostic report", "dsplab") {}

    void runTest() override
    {
        beginTest ("DiagnosticReport produces a parseable document with no audio");
        {
            auto in = DiagnosticReportInput::environment();
            in.presetName = "Void Bloom";
            in.presetTags = juce::StringArray { "dark", "metallic" };
            in.abSlot = 1;

            auto& s = in.snapshot;
            s.sampleRate = 48000.0;
            s.blockSize = 256;
            s.quality = (uint8_t) Quality::High;
            s.dryMode = (uint8_t) DryMode::MatterOnly;
            s.activeVoices = 5;
            s.maxVoices = 16;
            s.latencySamples = 128;
            s.numNodes = 42;
            s.activeNodes = 31;
            s.clusterCount = 6;
            s.numEdges = 77;
            s.topologySeed = 123456u;
            s.materialA = 2; s.materialB = 3; s.materialBlend = 0.4f;
            s.fundamentalHz = 261.63f;
            s.stages[(int) Stage::Master].rms = 0.25f;
            s.stages[(int) Stage::Master].peak = 0.7f;
            s.perf.totalAvgPercent = 12.5f;
            s.perf.avgPercent[(int) Subsystem::Matter] = 7.25f;
            s.perf.overruns = 2;
            s.safety.counts[(int) SafetyEvent::HardClip] = 4;
            s.safety.lastSubsystem[(int) SafetyEvent::HardClip] = (uint8_t) Subsystem::Master;
            s.safety.lastVoice[(int) SafetyEvent::HardClip] = 3;
            s.safety.total = 4;
            s.eventsDropped = 9;

            in.parameters[(size_t) paramIndex (Param::shapeDensity)] = 0.77f;
            in.extras.push_back ({ "sweepMaxPeak", 0.913 });

            const auto json = DiagnosticReport::toJson (in);
            expect (json.isNotEmpty());

            const auto parsed = juce::JSON::parse (json);
            expect (parsed.isObject(), "report parses back as an object");
            expect (parsed["containsAudio"].equals (false), "report declares it holds no audio");
            expectEquals (parsed["schema"].toString(), juce::String ("1"));
            expect (parsed["build"].toString().isNotEmpty());
            expect (parsed["os"].toString().isNotEmpty());
            expect (parsed["timestamp"].toString().isNotEmpty());

            const auto engine = parsed["engine"];
            expectEquals ((double) engine["sampleRate"], 48000.0);
            expectEquals ((int) engine["blockSize"], 256);
            expectEquals (engine["quality"].toString(), juce::String ("HIGH"));
            expectEquals (engine["dryMode"].toString(), juce::String ("MATTER ONLY"));
            expectEquals ((int) engine["eventsDropped"], 9);

            const auto matter = parsed["matter"];
            expectEquals ((int) matter["numNodes"], 42);
            expectEquals ((int) matter["clusterCount"], 6);
            expectEquals ((int) matter["topologySeed"], 123456);

            expectEquals ((int) parsed["cpu"]["overruns"], 2);
            expectWithinAbsoluteError ((float) (double) parsed["cpu"]["totalAvgPercent"], 12.5f, 0.01f);
            expectWithinAbsoluteError ((float) (double) parsed["cpu"]["Matter"]["avgPercent"], 7.25f, 0.01f);

            expectEquals ((int) parsed["safety"]["Hard clip"]["count"], 4);
            expectEquals (parsed["safety"]["Hard clip"]["lastSubsystem"].toString(), juce::String ("Master"));
            expectEquals ((int) parsed["safety"]["total"], 4);

            expectEquals (parsed["preset"]["name"].toString(), juce::String ("Void Bloom"));
            expectEquals (parsed["preset"]["abSlot"].toString(), juce::String ("B"));

            expectEquals ((int) parsed["numParameters"], kNumParams);
            expectEquals ((int) parsed["numChangedParameters"], 1);
            expect (parsed["changedParameters"].hasProperty ("shape.density"));
            expectWithinAbsoluteError ((float) (double) parsed["parameters"]["shape.density"], 0.77f, 1.0e-4f);
            expectWithinAbsoluteError ((float) (double) parsed["measurements"]["sweepMaxPeak"], 0.913f, 1.0e-4f);

            // No audio: the document must contain no arrays and no sample-carrying keys.
            std::function<void (const juce::var&)> assertNoAudio = [&] (const juce::var& v)
            {
                expect (! v.isArray(), "the report must not carry sample arrays");
                if (auto* obj = v.getDynamicObject())
                    for (const auto& prop : obj->getProperties())
                    {
                        const auto key = prop.name.toString();
                        expect (! key.containsIgnoreCase ("waveform") && ! key.containsIgnoreCase ("pcm")
                                && ! key.containsIgnoreCase ("spectrum") && ! key.containsIgnoreCase ("path"),
                                "unexpected key in the report: " + key);
                        assertNoAudio (prop.value);
                    }
            };
            assertNoAudio (parsed);

            const auto master = parsed["stageLevels"]["Master"];
            expectWithinAbsoluteError ((float) (double) master["rmsDb"], gainToDb (0.25f), 0.05f);
        }

        beginTest ("numActiveParameters counts changes against the defaults");
        {
            auto values = ParameterRegistry::defaults();
            expectEquals (DiagnosticReport::numActiveParameters (values), 0);
            values[(size_t) paramIndex (Param::shapeForm)] += 0.3f;
            values[(size_t) paramIndex (Param::evolveMelt)] += 0.2f;
            expectEquals (DiagnosticReport::numActiveParameters (values), 2);
        }

        beginTest ("DiagnosticReport writes to disk");
        {
            juce::TemporaryFile temp (".json");
            const auto in = DiagnosticReportInput::environment();
            const auto result = DiagnosticReport::writeTo (temp.getFile(), in);
            expect (result.wasOk(), result.getErrorMessage());
            expect (temp.getFile().existsAsFile());
            expect (temp.getFile().getSize() > 100);
            expect (juce::JSON::parse (temp.getFile().loadFileAsString()).isObject());
        }

        beginTest ("defaultFile lands in a writable directory with a json extension");
        {
            const auto f = DiagnosticReport::defaultFile();
            expectEquals (f.getFileExtension(), juce::String (".json"));
            expect (f.getFileName().startsWith ("antimatr-diagnostics-"));
        }
    }
};

static DSPLabReportTests dspLabReportTests;

//==============================================================================
class DSPLabStressTests : public juce::UnitTest
{
public:
    DSPLabStressTests() : juce::UnitTest ("DSP LAB stress generator", "dsplab") {}

    static std::vector<StressAction> runScenario (StressTest test, double seconds, double step,
                                                  const StressTestGenerator::Config& cfg,
                                                  StressTestGenerator* keep = nullptr)
    {
        StressTestGenerator local;
        auto& gen = keep != nullptr ? *keep : local;
        gen.start (test, cfg);
        std::vector<StressAction> actions;
        for (double t = 0.0; t <= seconds + 1.0e-9; t += step)
            gen.advance (t, actions);
        return actions;
    }

    void runTest() override
    {
        StressTestGenerator::Config cfg;
        cfg.numFactoryPresets = 6;
        cfg.maxVoices = 16;
        cfg.seed = 0xA11CE;

        beginTest ("every scenario has a name, a description and stays inside MIDI range");
        {
            for (int i = 0; i < (int) StressTest::Count; ++i)
            {
                const auto test = (StressTest) i;
                expect (juce::String (StressTestGenerator::name (test)).isNotEmpty());
                expect (juce::String (StressTestGenerator::description (test)).isNotEmpty());

                StressTestGenerator gen;
                const auto actions = runScenario (test, 6.0, 0.02, cfg, &gen);
                expect (! actions.empty(), juce::String (StressTestGenerator::name (test)) + " produced no actions");

                for (const auto& a : actions)
                {
                    expect (a.note >= 0 && a.note <= 127, "note in range");
                    expect (a.velocity >= 0.0f && a.velocity <= 1.0f, "velocity in range");
                    expect (a.bend >= -1.0f && a.bend <= 1.0f, "bend in range");
                    expect (a.normalised >= 0.0f && a.normalised <= 1.0f, "normalised parameter in range");
                    expect (a.preset >= 0 && a.preset < cfg.numFactoryPresets, "preset index in range");
                    if (a.kind == StressAction::Kind::Parameter)
                        expect (a.paramIndex >= 0 && a.paramIndex < kNumParams, "parameter index in range");
                    expect (a.time >= 0.0 && a.time <= 6.001, "action time inside the run");
                }

                // Stopping must leave nothing held.
                std::vector<StressAction> tail;
                gen.stop (tail);
                expectEquals (gen.notesHeld(), 0, juce::String (StressTestGenerator::name (test)) + " left notes held");
                expect (! gen.isRunning());
                expect (std::any_of (tail.begin(), tail.end(),
                                     [] (const StressAction& a) { return a.kind == StressAction::Kind::AllNotesOff; }),
                        "stop() emits an all-notes-off");
            }
        }

        beginTest ("note on / note off stay balanced across a run");
        {
            for (int i = 0; i < (int) StressTest::Count; ++i)
            {
                StressTestGenerator gen;
                auto actions = runScenario ((StressTest) i, 8.0, 0.01, cfg, &gen);
                gen.stop (actions);

                std::array<int, 128> depth {};
                for (const auto& a : actions)
                {
                    if (a.kind == StressAction::Kind::NoteOn)  ++depth[(size_t) a.note];
                    if (a.kind == StressAction::Kind::NoteOff) --depth[(size_t) a.note];
                    expect (depth[(size_t) a.note] >= 0 && depth[(size_t) a.note] <= 1,
                            "note " + juce::String (a.note) + " on/off nesting stays 0..1 in "
                            + StressTestGenerator::name ((StressTest) i));
                }
                for (auto d : depth)
                    expectEquals (d, 0, "all notes released after stop in " + juce::String (StressTestGenerator::name ((StressTest) i)));
            }
        }

        beginTest ("the schedule is deterministic and independent of the polling rate");
        {
            const auto slow = runScenario (StressTest::NoteFlood, 4.0, 0.05, cfg);
            const auto fast = runScenario (StressTest::NoteFlood, 4.0, 0.005, cfg);
            expectEquals ((int) slow.size(), (int) fast.size(), "the same actions regardless of poll rate");
            for (size_t i = 0; i < std::min (slow.size(), fast.size()); ++i)
            {
                expect (slow[i].kind == fast[i].kind);
                expectEquals (slow[i].note, fast[i].note);
                expectWithinAbsoluteError (slow[i].time, fast[i].time, 1.0e-9);
            }

            auto other = cfg; other.seed = 0xBEEF;
            const auto different = runScenario (StressTest::NoteFlood, 4.0, 0.05, other);
            bool anyDifferent = false;
            for (size_t i = 0; i < std::min (slow.size(), different.size()); ++i)
                if (slow[i].note != different[i].note) { anyDifferent = true; break; }
            expect (anyDifferent, "a different seed produces a different note sequence");
        }

        beginTest ("sustained note holds exactly one note");
        {
            StressTestGenerator gen;
            const auto actions = runScenario (StressTest::SustainedNote, 5.0, 0.02, cfg, &gen);
            expectEquals ((int) actions.size(), 1);
            expect (actions[0].kind == StressAction::Kind::NoteOn);
            expectEquals (gen.notesHeld(), 1);
            expectWithinAbsoluteError (gen.elapsed(), 5.0, 0.05);
        }

        beginTest ("the 16-note chord starts 16 distinct notes");
        {
            StressTestGenerator gen;
            const auto actions = runScenario (StressTest::Chord16, 2.0, 0.02, cfg, &gen);
            int noteOns = 0;
            std::set<int> notes;
            for (const auto& a : actions)
                if (a.kind == StressAction::Kind::NoteOn) { ++noteOns; notes.insert (a.note); }
            expectEquals (noteOns, 16);
            expectEquals ((int) notes.size(), 16);
            expectEquals (gen.notesHeld(), 16);
        }

        beginTest ("max polyphony respects the configured voice count");
        {
            StressTestGenerator gen;
            const auto actions = runScenario (StressTest::MaxPolyphony, 5.0, 0.02, cfg, &gen);
            int noteOns = 0;
            for (const auto& a : actions)
                if (a.kind == StressAction::Kind::NoteOn) ++noteOns;
            expectEquals (noteOns, cfg.maxVoices);
            expect (gen.duration() > 0.0, "max polyphony has a finite nominal duration");
        }

        beginTest ("extreme velocity alternates between the lowest and highest velocity");
        {
            const auto actions = runScenario (StressTest::ExtremeVelocity, 3.0, 0.02, cfg);
            float lowest = 1.0f, highest = 0.0f;
            for (const auto& a : actions)
                if (a.kind == StressAction::Kind::NoteOn)
                {
                    lowest = juce::jmin (lowest, a.velocity);
                    highest = juce::jmax (highest, a.velocity);
                }
            expectWithinAbsoluteError (lowest, 1.0f / 127.0f, 1.0e-4f);
            expectWithinAbsoluteError (highest, 1.0f, 1.0e-4f);
        }

        beginTest ("pitch bend sweeps the full range and returns to centre on stop");
        {
            StressTestGenerator gen;
            auto actions = runScenario (StressTest::PitchBendSweep, 4.0, 0.02, cfg, &gen);
            float lowest = 1.0f, highest = -1.0f;
            int bends = 0;
            for (const auto& a : actions)
                if (a.kind == StressAction::Kind::PitchBend)
                {
                    ++bends;
                    lowest = juce::jmin (lowest, a.bend);
                    highest = juce::jmax (highest, a.bend);
                }
            expect (bends > 100, "expected a dense bend stream, got " + juce::String (bends));
            expect (lowest < -0.98f && highest > 0.98f, "the sweep should reach both extremes");

            std::vector<StressAction> tail;
            gen.stop (tail);
            expect (std::any_of (tail.begin(), tail.end(), [] (const StressAction& a)
                    { return a.kind == StressAction::Kind::PitchBend && a.bend == 0.0f; }),
                    "stop() recentres the bend");
        }

        beginTest ("automation sweep drives the configured parameter across 0..1");
        {
            auto sweepCfg = cfg;
            sweepCfg.sweepParameter = paramIndex (Param::fractureAmount);
            const auto actions = runScenario (StressTest::AutomationSweep, 6.0, 0.02, sweepCfg);
            float lowest = 1.0f, highest = 0.0f;
            int writes = 0;
            for (const auto& a : actions)
                if (a.kind == StressAction::Kind::Parameter)
                {
                    ++writes;
                    expectEquals (a.paramIndex, paramIndex (Param::fractureAmount));
                    lowest = juce::jmin (lowest, a.normalised);
                    highest = juce::jmax (highest, a.normalised);
                }
            expect (writes > 100);
            expect (lowest < 0.05f && highest > 0.95f, "the parameter should traverse the whole range");

            // An out-of-range request falls back to the default sweep parameter.
            auto badCfg = cfg; badCfg.sweepParameter = 999999;
            const auto fallback = runScenario (StressTest::AutomationSweep, 1.0, 0.02, badCfg);
            for (const auto& a : fallback)
                if (a.kind == StressAction::Kind::Parameter)
                    expectEquals (a.paramIndex, StressTestGenerator::defaultSweepParameter());
        }

        beginTest ("macro motion moves all eight macros");
        {
            const auto actions = runScenario (StressTest::MacroMotion, 3.0, 0.02, cfg);
            std::set<int> touched;
            for (const auto& a : actions)
                if (a.kind == StressAction::Kind::Parameter)
                    touched.insert (a.paramIndex);
            expectEquals ((int) touched.size(), kNumMacros);
            for (int m = 0; m < kNumMacros; ++m)
                expect (touched.count (paramIndex (Param::macro1) + m) == 1, "macro " + juce::String (m + 1) + " is driven");
        }

        beginTest ("preset switch loop cycles through the factory bank");
        {
            const auto actions = runScenario (StressTest::PresetSwitchLoop, 6.0, 0.02, cfg);
            std::vector<int> order;
            for (const auto& a : actions)
                if (a.kind == StressAction::Kind::Preset)
                    order.push_back (a.preset);
            expect ((int) order.size() >= 6, "expected several preset switches");
            for (size_t i = 0; i < order.size(); ++i)
                expectEquals (order[i], (int) (i % (size_t) cfg.numFactoryPresets));
        }

        beginTest ("advance ignores time going backwards");
        {
            StressTestGenerator gen;
            gen.start (StressTest::NoteFlood, cfg);
            std::vector<StressAction> actions;
            gen.advance (1.0, actions);
            const auto after = actions.size();
            gen.advance (0.5, actions);
            expectEquals ((int) actions.size(), (int) after, "rewinding time produces nothing");
            gen.advance (1.5, actions);
            expect (actions.size() > after, "moving forward resumes the schedule");
        }
    }
};

static DSPLabStressTests dspLabStressTests;

//==============================================================================
class DSPLabPresetValidatorTests : public juce::UnitTest
{
public:
    DSPLabPresetValidatorTests() : juce::UnitTest ("DSP LAB preset validator", "dsplab") {}

    void runTest() override
    {
        PresetValidator::Options fast;
        fast.holdSeconds = 0.25;
        fast.releaseSeconds = 0.25;
        fast.blockSize = 128;

        PresetManager presets;
        auto engine = std::make_unique<SynthEngine>();

        beginTest ("validateOne checks bounds, JSON round trip and renders offline");
        {
            expect (presets.numFactoryPresets() > 0, "there are factory presets to validate");

            const auto r = PresetValidator::validateOne (presets, 0, fast, *engine);
            expectEquals (r.index, 0);
            expect (r.name.isNotEmpty());
            expect (r.parametersInRange, "factory preset 0 must stay inside the parameter ranges");
            expect (r.jsonRoundTrip, "factory preset 0 must survive a JSON round trip");
            expectEquals (r.nonFinite, 0, "offline render produced non-finite samples");
            expect (std::isfinite (r.peak) && std::isfinite (r.rms) && std::isfinite (r.dc));
            expect (r.peak <= 4.0f, "peak " + juce::String (r.peak) + " is wildly out of range");
            expect (r.renderSeconds >= 0.0);
            expect (r.centroidHz >= 0.0f);
        }

        beginTest ("validation runs the whole factory bank without instability");
        {
            int silent = 0, clipping = 0, safetyIssues = 0;
            juce::StringArray failures;

            for (int i = 0; i < presets.numFactoryPresets(); ++i)
            {
                const auto r = PresetValidator::validateOne (presets, i, fast, *engine);
                expectEquals (r.nonFinite, 0, r.name + " produced non-finite samples");
                expect (r.parametersInRange, r.name + " has out-of-range parameters");
                expect (r.jsonRoundTrip, r.name + " failed the JSON round trip");
                if (r.rms < fast.silenceRms) ++silent;
                if (r.peak > fast.peakLimit) ++clipping;
                if (r.safetyTotal > 0) ++safetyIssues;
                if (! r.passed()) failures.add (r.name + ": " + r.issues.joinIntoString ("; "));
            }

            logMessage ("Validated " + juce::String (presets.numFactoryPresets()) + " factory presets: "
                        + juce::String (silent) + " silent, " + juce::String (clipping) + " clipping, "
                        + juce::String (safetyIssues) + " with safety events");
            for (const auto& f : failures)
                logMessage ("  issue -> " + f);

            expectEquals (clipping, 0, "no factory preset may clip the master output");
            expectEquals (safetyIssues, 0, "no factory preset may raise safety events");
        }

        beginTest ("an out-of-range preset index fails cleanly");
        {
            const auto r = PresetValidator::validateOne (presets, 9999, fast, *engine);
            expect (! r.passed());
            expect (r.issues.joinIntoString (" ").containsIgnoreCase ("range"));
        }

        beginTest ("the background thread validates every preset and reports progress");
        {
            PresetValidator validator;
            expect (! validator.busy());
            expectEquals (validator.progress(), 0.0f);

            validator.startValidation (fast);
            const auto deadline = juce::Time::getMillisecondCounter() + 120000;
            while (validator.busy() && juce::Time::getMillisecondCounter() < deadline)
                juce::Thread::sleep (25);

            expect (! validator.busy(), "validation finished inside the timeout");
            const auto results = validator.results();
            expectEquals ((int) results.size(), validator.totalPresets());
            expectWithinAbsoluteError (validator.progress(), 1.0f, 1.0e-5f);
            expect (validator.summary().contains ("passed"));
            for (const auto& r : results)
                expectEquals (r.nonFinite, 0, r.name + " produced non-finite samples on the worker thread");
        }

        beginTest ("cancelling a validation is safe");
        {
            PresetValidator validator;
            validator.startValidation (fast);
            validator.cancelValidation();
            expect (! validator.busy());
        }
    }
};

static DSPLabPresetValidatorTests dspLabPresetValidatorTests;

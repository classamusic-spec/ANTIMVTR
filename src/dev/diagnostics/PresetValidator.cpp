#include "PresetValidator.h"

#include "dsp/SynthEngine.h"
#include "dsp/fracture/Fragment.h"
#include "dsp/source/SampleData.h"
#include "state/ModRouting.h"
#include "state/StateManager.h"

namespace am::dev
{

namespace
{
    /** Renders `numSamples` of the engine into the accumulating capture buffer. */
    struct OfflineRender
    {
        juce::AudioBuffer<float> block;
        std::vector<float> captureL, captureR;
        int written = 0;

        explicit OfflineRender (int blockSize, int capacity)
            : block (2, blockSize)
        {
            captureL.assign ((size_t) capacity, 0.0f);
            captureR.assign ((size_t) capacity, 0.0f);
        }

        void append (int numSamples)
        {
            const int n = std::min (numSamples, (int) captureL.size() - written);
            if (n <= 0) return;
            std::copy (block.getReadPointer (0), block.getReadPointer (0) + n, captureL.begin() + written);
            std::copy (block.getReadPointer (1), block.getReadPointer (1) + n, captureR.begin() + written);
            written += n;
        }
    };
}

PresetValidator::PresetValidator()
    : juce::Thread ("ANTI-MATR preset validator")
{
}

PresetValidator::~PresetValidator()
{
    stopThread (4000);
}

void PresetValidator::startValidation (const Options& options)
{
    cancelValidation();

    opts = options;
    {
        const juce::ScopedLock sl (lock);
        completed.clear();
    }
    total.store (presetManager.numFactoryPresets());
    done.store (0);
    startThread (juce::Thread::Priority::low);
}

void PresetValidator::cancelValidation()
{
    if (isThreadRunning())
        stopThread (4000);
}

bool PresetValidator::busy() const
{
    return isThreadRunning();
}

float PresetValidator::progress() const
{
    const int t = total.load();
    return t > 0 ? juce::jlimit (0.0f, 1.0f, (float) done.load() / (float) t) : 0.0f;
}

int PresetValidator::totalPresets() const
{
    return total.load() > 0 ? total.load() : presetManager.numFactoryPresets();
}

std::vector<PresetValidationResult> PresetValidator::results() const
{
    const juce::ScopedLock sl (lock);
    return completed;
}

juce::String PresetValidator::summary() const
{
    const auto r = results();
    if (r.empty())
        return busy() ? "Validating..." : "No results yet.";

    int passed = 0;
    float worstPeak = 0.0f, worstCpu = 0.0f;
    uint32_t safety = 0;
    int nonFinite = 0;
    for (const auto& one : r)
    {
        if (one.passed()) ++passed;
        worstPeak = juce::jmax (worstPeak, one.peak);
        worstCpu = juce::jmax (worstCpu, one.cpuAvgPercent);
        safety += one.safetyTotal;
        nonFinite += one.nonFinite;
    }
    return juce::String (passed) + " / " + juce::String ((int) r.size()) + " passed"
         + "   peak " + juce::String (worstPeak, 3)
         + "   cpu " + juce::String (worstCpu, 1) + "%"
         + "   safety " + juce::String ((int) safety)
         + "   non-finite " + juce::String (nonFinite)
         + (busy() ? "   (running)" : "");
}

void PresetValidator::run()
{
    auto engine = std::make_unique<SynthEngine>();
    const int count = presetManager.numFactoryPresets();

    for (int i = 0; i < count && ! threadShouldExit(); ++i)
    {
        auto result = validateOne (presetManager, i, opts, *engine);
        {
            const juce::ScopedLock sl (lock);
            completed.push_back (std::move (result));
        }
        done.store (i + 1);
    }
}

PresetValidationResult PresetValidator::validateOne (PresetManager& presets, int index, const Options& options)
{
    auto engine = std::make_unique<SynthEngine>();
    return validateOne (presets, index, options, *engine);
}

PresetValidationResult PresetValidator::validateOne (PresetManager& presets, int index,
                                                     const Options& options, SynthEngine& engine)
{
    PresetValidationResult r;
    r.index = index;

    if (index < 0 || index >= presets.numFactoryPresets())
    {
        r.issues.add ("Preset index out of range");
        return r;
    }

    const auto& factory = presets.factoryPreset (index);
    r.name = factory.name;
    r.category = factory.category;

    const auto patch = presets.buildFactory (index);

    // ---- 1. parameter bounds
    r.parametersInRange = true;
    for (const auto& d : ParameterRegistry::all())
    {
        const float v = patch.params[(size_t) paramIndex (d.param)];
        if (! std::isfinite (v))
        {
            r.parametersInRange = false;
            r.issues.add (juce::String (d.id) + " is not finite");
            continue;
        }
        if (d.kind == ParamKind::Choice)
        {
            const int n = d.numChoices();
            if (v < -0.001f || (n > 0 && v > (float) (n - 1) + 0.001f))
            {
                r.parametersInRange = false;
                r.issues.add (juce::String (d.id) + " choice out of range (" + juce::String (v, 3) + ")");
            }
            continue;
        }
        if (v < d.min - 1.0e-4f || v > d.max + 1.0e-4f)
        {
            r.parametersInRange = false;
            r.issues.add (juce::String (d.id) + " out of range (" + juce::String (v, 4)
                          + " not in " + juce::String (d.min, 4) + ".." + juce::String (d.max, 4) + ")");
        }
    }

    // ---- 2. JSON round trip
    {
        const auto json = StateManager::toJson (patch, false);
        PatchState back;
        juce::String warnings;
        if (! StateManager::fromJson (json, back, &warnings))
        {
            r.issues.add ("JSON round trip failed to parse");
        }
        else
        {
            bool identical = true;
            for (int i = 0; i < kNumParams; ++i)
                if (std::abs (patch.params[(size_t) i] - back.params[(size_t) i]) > 1.0e-4f)
                {
                    identical = false;
                    r.issues.add (juce::String (ParameterRegistry::all()[(size_t) i].id) + " changed across JSON round trip");
                    break;
                }
            if (back.meta.name != patch.meta.name)
            {
                identical = false;
                r.issues.add ("Preset name changed across JSON round trip");
            }
            r.jsonRoundTrip = identical;
        }
    }

    // ---- 3. offline render through a private engine
    const double sr = options.sampleRate > 0.0 ? options.sampleRate : 48000.0;
    const int blockSize = juce::jlimit (16, kMaxBlockSize, options.blockSize);
    const int holdSamples = (int) (options.holdSeconds * sr);
    const int releaseSamples = (int) (options.releaseSeconds * sr);
    const int totalSamples = juce::jmax (blockSize, holdSamples + releaseSamples);

    if (! options.engineAlreadyPrepared) engine.prepare (sr, blockSize);
    engine.reset();
    engine.control().resetTo (patch.params);

    // The patch is more than its parameters: publish the sections the plugin
    // publishes so the validator judges the preset a player would hear.
    {
        auto table = patch.fracture.isVoid() ? FractureTable::makeDefault() : FractureTable::fromVar (patch.fracture);
        engine.fractureEngine().publishTable (std::make_unique<FractureTable> (table));

        auto routings = patch.mod.isVoid() ? ModRoutingTable() : ModRoutingTable::fromVar (patch.mod);
        engine.modulationEngine().publishRoutings (std::make_unique<ModRoutingTable> (routings));

        // Factory presets only ever reference built-in (generated) samples; a
        // user file is not loaded here because the validator must not touch disk.
        int builtIn = 0;
        if (auto* reference = patch.sample.getDynamicObject())
        {
            if (reference->hasProperty ("builtIn"))
                builtIn = (int) reference->getProperty ("builtIn");
            else
                builtIn = juce::jmax (0, BuiltInSamples::indexOf (reference->getProperty ("name").toString()));
        }
        engine.publishSample (BuiltInSamples::create (juce::jlimit (0, BuiltInSamples::count() - 1, builtIn)));
    }
    auto& diag = engine.diagnostics();
    diag.safety.reset();
    diag.profiler.reset();
    diag.dev.dryMode.store ((int) DryMode::FullSynth);
    diag.dev.bypassEvolve.store (false);
    diag.dev.evolveBypassMask.store (0u);
    diag.dev.bypassFracture.store (false);
    diag.dev.bypassSpace.store (false);
    diag.dev.profiling.store (true);
    diag.events.drain ([] (const EngineEvent&) {});

    OfflineRender render (blockSize, totalSamples);
    const TransportInfo transport;
    const auto startTime = juce::Time::getMillisecondCounterHiRes();

    int position = 0;
    bool noteSent = false, releaseSent = false;
    while (position < totalSamples)
    {
        const int n = std::min (blockSize, totalSamples - position);
        render.block.setSize (2, n, false, false, true);
        render.block.clear();

        juce::MidiBuffer midi;
        if (! noteSent)
        {
            midi.addEvent (juce::MidiMessage::noteOn (1, juce::jlimit (0, 127, options.midiNote), options.velocity), 0);
            noteSent = true;
        }
        else if (! releaseSent && position + n > holdSamples)
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, juce::jlimit (0, 127, options.midiNote)),
                           juce::jlimit (0, n - 1, holdSamples - position));
            releaseSent = true;
        }

        engine.process (render.block, midi, patch.params, transport);
        render.append (n);
        r.maxActiveVoices = juce::jmax (r.maxActiveVoices, engine.activeVoices());
        position += n;
    }

    r.renderSeconds = (juce::Time::getMillisecondCounterHiRes() - startTime) * 0.001;

    const auto metrics = SignalMetrics::measure (render.captureL.data(), render.captureR.data(), render.written);
    r.peak = metrics.peak;
    r.rms = metrics.rms;
    r.dc = metrics.dc;
    r.crestFactor = metrics.crestFactor;
    r.nonFinite = metrics.nonFinite;

    // Tail: the last 100 ms of the render (should have decayed after release).
    {
        const int tail = juce::jmin (render.written, (int) (0.1 * sr));
        if (tail > 0)
        {
            const auto tailMetrics = SignalMetrics::measure (render.captureL.data() + render.written - tail,
                                                             render.captureR.data() + render.written - tail, tail);
            r.tailRms = tailMetrics.rms;
        }
    }

    // Spectral centroid of a window inside the sustain.
    {
        SpectrumMeasurement spectrum (11);
        const int window = juce::jmin (render.written, spectrum.fftSize());
        const int start = juce::jlimit (0, juce::jmax (0, render.written - window), holdSamples / 2);
        std::vector<float> mono ((size_t) window, 0.0f);
        for (int i = 0; i < window; ++i)
            mono[(size_t) i] = 0.5f * (render.captureL[(size_t) (start + i)] + render.captureR[(size_t) (start + i)]);
        r.centroidHz = spectrum.spectralCentroid (mono.data(), window, sr);
    }

    const auto safety = diag.safety.snapshot();
    r.safetyTotal = safety.total;
    for (int i = 0; i < SafetyMonitor::kNumEvents; ++i)
        r.safetyCounts[i] = safety.counts[i];

    const auto perf = diag.profiler.snapshot();
    r.cpuAvgPercent = perf.totalAvgPercent;
    r.cpuPeakPercent = perf.totalPeakPercent;

    // ---- 4. verdict
    if (r.nonFinite > 0)
        r.issues.add (juce::String (r.nonFinite) + " non-finite samples");
    if (r.peak > options.peakLimit)
        r.issues.add ("Peak " + juce::String (r.peak, 3) + " exceeds " + juce::String (options.peakLimit, 2));
    if (r.rms < options.silenceRms)
        r.issues.add ("Preset is silent (rms " + juce::String (r.rms, 8) + ")");
    if (std::abs (r.dc) > options.dcLimit)
        r.issues.add ("DC offset " + juce::String (r.dc, 4));
    if (r.safetyTotal > 0)
    {
        juce::StringArray raised;
        for (int i = 0; i < SafetyMonitor::kNumEvents; ++i)
            if (r.safetyCounts[i] > 0)
                raised.add (juce::String (SafetyMonitor::eventName ((SafetyEvent) i)) + " x" + juce::String ((int) r.safetyCounts[i]));
        r.issues.add ("Safety: " + raised.joinIntoString (", "));
    }
    if (options.flagCpu && r.cpuAvgPercent > options.cpuLimit)
        r.issues.add ("CPU " + juce::String (r.cpuAvgPercent, 1) + "% above " + juce::String (options.cpuLimit, 0) + "%");

    return r;
}

} // namespace am::dev

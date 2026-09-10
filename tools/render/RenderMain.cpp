/*
    AntiMatrRender — offline renderer and analyser.

    Renders MIDI into a WAV file through the real SynthEngine and prints a JSON
    analysis (peak, RMS, DC, non-finite count, spectral centroid, decay time,
    safety counters, CPU profile). This is how engineers and agents "listen"
    without a DAW: numbers plus spectrograms from scripts/analyze.py.

    Usage:
      AntiMatrRender --out file.wav [--sr 48000] [--block 128] [--seconds 3]
                     [--note 60 --vel 100 --hold 1.5]            (single note)
                     [--seq "60:0:1.5,64:0.5:2.0"]               (note:start:end, seconds)
                     [--preset "Void Bloom"] [--set shape.form=0.7 ...]
                     [--mod "lfo1>shape.decay:0.5" ...]           (modulation routings)
                     [--bpm 120] [--dry full|source|matter|evolve] [--json report.json]

    --mod adds one modulation routing per occurrence:

        --mod "<source>><target>:<depth>[:uni|:bi][:<curve>]"

    <source> is a permanent modulation source id (lfo1…lfo4, env1…env4,
    chaos1…chaos4, macro1…macro8, velocity, keytrack, pressure, modwheel,
    pitchbend, timbre, noterandom, gate), <target> a modulatable parameter id
    and <depth> a signed fraction (-1…1) of the target's range. `uni` presents
    the source as 0…1, `bi` (the default) as -1…1; the optional curve bends
    the source (-1…1, 0 = linear).
                     [--dry full|source|matter|evolve] [--json report.json]
                     [--sample path.wav | --sample builtin:3]      (SAMPLE source data)
                     [--analyze]                                  (ANALYZE -> MATTER on that sample)
*/

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/SynthEngine.h"
#include "dsp/source/SampleData.h"
#include "dsp/source/SampleAnalyzer.h"
#include "dsp/fracture/Fragment.h"
#include "presets/PresetManager.h"
#include "state/StateManager.h"
#include "state/ModRouting.h"

using namespace am;

namespace
{
    /** Accepts both "--name value" and "--name=value". */
    juce::String optionValue (const juce::ArgumentList& args, const juce::String& name, const juce::String& fallback = {})
    {
        for (int i = 0; i < args.size(); ++i)
        {
            const auto& t = args[i].text;
            if (t == name) return i + 1 < args.size() ? args[i + 1].text : fallback;
            if (t.startsWith (name + "=")) return t.fromFirstOccurrenceOf ("=", false, false);
        }
        return fallback;
    }

    bool hasOption (const juce::ArgumentList& args, const juce::String& name)
    {
        for (int i = 0; i < args.size(); ++i)
            if (args[i].text == name || args[i].text.startsWith (name + "=")) return true;
        return false;
    }
}

namespace
{
    /** Parses one --mod "src>target:depth[:uni|:bi][:curve]" specification. */
    bool parseModRouting (const juce::String& spec, am::ModRouting& out)
    {
        const auto sourceId = spec.upToFirstOccurrenceOf (">", false, false).trim();
        const auto rest = spec.fromFirstOccurrenceOf (">", false, false);
        if (sourceId.isEmpty() || rest.isEmpty()) return false;

        juce::StringArray parts;
        parts.addTokens (rest, ":", "");
        if (parts.size() < 2) return false;

        out.source = am::modSourceFromId (sourceId.toRawUTF8());
        const auto target = am::ParameterRegistry::fromID (parts[0].trim().toStdString());
        if (out.source == am::ModSource::None || ! target.has_value()) return false;
        out.target = *target;
        out.depth = parts[1].getFloatValue();
        out.bipolar = true;
        out.curve = 0.0f;
        for (int i = 2; i < parts.size(); ++i)
        {
            const auto option = parts[i].trim().toLowerCase();
            if (option == "uni") out.bipolar = false;
            else if (option == "bi") out.bipolar = true;
            else out.curve = option.getFloatValue();
        }
        return am::ModRoutingTable::isValid (out);
    }

    struct NoteEvent { int note; double start; double end; float velocity; };

    struct Analysis
    {
        float peak = 0.0f, rms = 0.0f, dc = 0.0f, crest = 0.0f;
        int nonFinite = 0;
        double centroidHz = 0.0;
        double t60 = -1.0, t20 = -1.0;
        double sustainRms = 0.0;
        std::vector<double> centroidTrack;   // per 100 ms
        std::vector<double> rmsTrack;        // per 10 ms (dB)
    };

    Analysis analyse (const juce::AudioBuffer<float>& buf, double sr, double noteOffTime)
    {
        Analysis a;
        const int n = buf.getNumSamples();
        const float* l = buf.getReadPointer (0);
        const float* r = buf.getReadPointer (1);
        double sum = 0.0, dcSum = 0.0;
        for (int i = 0; i < n; ++i)
        {
            const float m = 0.5f * (l[i] + r[i]);
            if (! std::isfinite (l[i]) || ! std::isfinite (r[i])) { ++a.nonFinite; continue; }
            a.peak = juce::jmax (a.peak, std::abs (l[i]), std::abs (r[i]));
            sum += (double) m * m;
            dcSum += m;
        }
        a.rms = (float) std::sqrt (sum / juce::jmax (1, n));
        a.dc = (float) (dcSum / juce::jmax (1, n));
        a.crest = a.rms > 1.0e-9f ? a.peak / a.rms : 0.0f;

        // RMS track, 10 ms windows
        const int win = juce::jmax (1, (int) (sr * 0.01));
        for (int start = 0; start + win <= n; start += win)
        {
            double s = 0.0;
            for (int i = start; i < start + win; ++i) { const float m = 0.5f * (l[i] + r[i]); s += (double) m * m; }
            a.rmsTrack.push_back (juce::Decibels::gainToDecibels (std::sqrt (s / win), -120.0));
        }

        // Spectral centroid track (100 ms hop, 2048 FFT)
        constexpr int order = 11, size = 1 << order;
        juce::dsp::FFT fft (order);
        juce::dsp::WindowingFunction<float> window (size, juce::dsp::WindowingFunction<float>::hann);
        std::vector<float> data ((size_t) size * 2);
        const int hop = (int) (sr * 0.1);
        double centroidSum = 0.0; int centroidCount = 0;
        for (int start = 0; start + size <= n; start += hop)
        {
            for (int i = 0; i < size; ++i) data[(size_t) i] = 0.5f * (l[start + i] + r[start + i]);
            std::fill (data.begin() + size, data.end(), 0.0f);
            window.multiplyWithWindowingTable (data.data(), size);
            fft.performFrequencyOnlyForwardTransform (data.data(), true);
            double num = 0.0, den = 0.0;
            for (int k = 1; k < size / 2; ++k) { const double mag = data[(size_t) k]; num += mag * (k * sr / size); den += mag; }
            const double c = den > 1.0e-9 ? num / den : 0.0;
            a.centroidTrack.push_back (c);
            if (den > 1.0e-4) { centroidSum += c; ++centroidCount; }
        }
        a.centroidHz = centroidCount > 0 ? centroidSum / centroidCount : 0.0;

        // Decay after note off: level at note-off vs first time it drops 20/60 dB.
        if (noteOffTime > 0.0 && ! a.rmsTrack.empty())
        {
            const int offIndex = juce::jlimit (0, (int) a.rmsTrack.size() - 1, (int) (noteOffTime / 0.01));
            double ref = -120.0;
            for (int i = juce::jmax (0, offIndex - 5); i <= offIndex; ++i) ref = juce::jmax (ref, a.rmsTrack[(size_t) i]);
            a.sustainRms = ref;
            for (size_t i = (size_t) offIndex; i < a.rmsTrack.size(); ++i)
            {
                const double t = (double) (i - (size_t) offIndex) * 0.01;
                if (a.t20 < 0.0 && a.rmsTrack[i] <= ref - 20.0) a.t20 = t;
                if (a.rmsTrack[i] <= ref - 60.0) { a.t60 = t; break; }
            }
        }
        return a;
    }
}

int main (int argc, char* argv[])
{
    juce::ArgumentList args (argc, argv);
    juce::ScopedJuceInitialiser_GUI init;   // message manager for JSON/format helpers; no window is created

    const auto outFile = optionValue (args, "--out");
    const double sr = hasOption (args, "--sr") ? optionValue (args, "--sr").getDoubleValue() : 48000.0;
    const int block = hasOption (args, "--block") ? optionValue (args, "--block").getIntValue() : 128;
    double seconds = hasOption (args, "--seconds") ? optionValue (args, "--seconds").getDoubleValue() : 3.0;

    std::vector<NoteEvent> notes;
    if (hasOption (args, "--seq"))
    {
        juce::StringArray items; items.addTokens (optionValue (args, "--seq"), ",", "");
        for (auto& item : items)
        {
            juce::StringArray parts; parts.addTokens (item, ":", "");
            if (parts.size() >= 3)
                notes.push_back ({ parts[0].getIntValue(), parts[1].getDoubleValue(), parts[2].getDoubleValue(), parts.size() > 3 ? (float) parts[3].getDoubleValue() / 127.0f : 0.8f });
        }
    }
    else
    {
        const int note = hasOption (args, "--note") ? optionValue (args, "--note").getIntValue() : 60;
        const float vel = hasOption (args, "--vel") ? (float) optionValue (args, "--vel").getIntValue() / 127.0f : 0.8f;
        const double hold = hasOption (args, "--hold") ? optionValue (args, "--hold").getDoubleValue() : juce::jmin (1.5, seconds * 0.5);
        notes.push_back ({ note, 0.0, hold, vel });
    }

    // Patch
    PresetManager presets;
    PatchState patch = PresetManager::initPatch();
    if (hasOption (args, "--preset"))
    {
        const auto name = optionValue (args, "--preset");
        const int idx = presets.findFactory (name);
        if (idx >= 0) patch = presets.buildFactory (idx);
        else if (juce::File (name).existsAsFile()) presets.loadUserPreset (juce::File (name), patch);
        else { std::cerr << "Unknown preset: " << name << std::endl; return 2; }
    }
    for (int i = 0; i < args.size(); ++i)
    {
        if (args[i].text == "--set" && i + 1 < args.size())
        {
            const auto kv = args[i + 1].text;
            const auto id = kv.upToFirstOccurrenceOf ("=", false, false);
            const auto value = kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue();
            const auto p = ParameterRegistry::fromID (id.toStdString());
            if (! p.has_value()) { std::cerr << "Unknown parameter: " << id << std::endl; return 2; }
            patch.params[(size_t) paramIndex (*p)] = ParameterRegistry::get (*p).clampValue (value);
        }
    }

    // Modulation routings: the patch's own table unless --mod overrides it.
    ModRoutingTable routings = patch.mod.isVoid() ? ModRoutingTable() : ModRoutingTable::fromVar (patch.mod);
    bool modOverride = false;
    for (int i = 0; i < args.size(); ++i)
    {
        if (args[i].text != "--mod" || i + 1 >= args.size()) continue;
        if (! modOverride) { routings.clear(); modOverride = true; }
        ModRouting r;
        if (! parseModRouting (args[i + 1].text, r)) { std::cerr << "Bad --mod routing: " << args[i + 1].text << std::endl; return 2; }
        if (routings.add (r) < 0) { std::cerr << "Rejected --mod routing: " << args[i + 1].text << std::endl; return 2; }
    }
    patch.mod = routings.toVar();

    SynthEngine engine;
    engine.prepare (sr, block);
    engine.control().resetTo (patch.params);
    engine.modulationEngine().publishRoutings (std::make_unique<ModRoutingTable> (routings));

    // The patch's own Fracture table, exactly as the plugin publishes it.
    {
        auto table = patch.fracture.isVoid() ? FractureTable::makeDefault() : FractureTable::fromVar (patch.fracture);
        engine.fractureEngine().publishTable (std::make_unique<FractureTable> (table));
    }

    // SAMPLE source data: a built-in ("builtin:N") or any audio file. Without
    // --sample the patch's own reference decides (factory patches only ever
    // reference the generated built-ins).
    SampleRef sample;
    if (! hasOption (args, "--sample"))
    {
        if (auto* reference = patch.sample.getDynamicObject())
        {
            const int builtIn = reference->hasProperty ("builtIn")
                                  ? (int) reference->getProperty ("builtIn")
                                  : juce::jmax (0, BuiltInSamples::indexOf (reference->getProperty ("name").toString()));
            sample = BuiltInSamples::create (juce::jlimit (0, BuiltInSamples::count() - 1, builtIn));
            engine.publishSample (sample);
        }
    }
    else
    {
        const auto spec = optionValue (args, "--sample");
        if (spec.startsWithIgnoreCase ("builtin:"))
        {
            sample = BuiltInSamples::create (spec.fromFirstOccurrenceOf (":", false, false).getIntValue());
        }
        else
        {
            juce::String error;
            sample = loadSampleFile (juce::File::getCurrentWorkingDirectory().getChildFile (spec), &error);
            if (sample == nullptr) { std::cerr << error << std::endl; return 2; }
        }
        engine.publishSample (sample);
    }

    // ANALYZE -> MATTER: shape Matter from the sample's own partials.
    PartialTable analysis;
    if (hasOption (args, "--analyze"))
    {
        if (sample == nullptr) { std::cerr << "--analyze needs --sample" << std::endl; return 2; }
        analysis = SampleAnalyzer::analyse (*sample);
        if (! analysis.isValid()) { std::cerr << "Could not analyze the sample" << std::endl; return 2; }
        const auto fit = SampleAnalyzer::fitShape (analysis);
        patch.params[(size_t) paramIndex (Param::shapeForm)] = fit.form;
        patch.params[(size_t) paramIndex (Param::shapeTension)] = fit.tension;
        patch.params[(size_t) paramIndex (Param::shapeDecay)] = fit.decay;
        patch.params[(size_t) paramIndex (Param::shapeMass)] = fit.mass;
        patch.params[(size_t) paramIndex (Param::shapeDensity)] = fit.density;
        patch.params[(size_t) paramIndex (Param::shapeDistribution)] = fit.distribution;
        engine.control().resetTo (patch.params);
    }

    const auto dry = optionValue (args, "--dry");
    if (dry == "source") engine.diagnostics().dev.dryMode.store ((int) DryMode::SourceOnly);
    else if (dry == "matter") engine.diagnostics().dev.dryMode.store ((int) DryMode::MatterOnly);
    else if (dry == "evolve") engine.diagnostics().dev.dryMode.store ((int) DryMode::MatterAndEvolve);

    const int totalSamples = (int) (seconds * sr);
    juce::AudioBuffer<float> output (2, totalSamples);
    output.clear();

    // Build the MIDI timeline
    juce::MidiMessageSequence seq;
    double lastNoteOff = 0.0;
    for (auto& n : notes)
    {
        seq.addEvent (juce::MidiMessage::noteOn (1, n.note, n.velocity), n.start * sr);
        seq.addEvent (juce::MidiMessage::noteOff (1, n.note), n.end * sr);
        lastNoteOff = juce::jmax (lastNoteOff, n.end);
    }
    seq.sort();

    TransportInfo transport;
    transport.isPlaying = true;
    if (hasOption (args, "--bpm")) transport.bpm = juce::jlimit (20.0, 300.0, optionValue (args, "--bpm").getDoubleValue());
    int eventIndex = 0;
    const auto startTime = juce::Time::getMillisecondCounterHiRes();
    for (int pos = 0; pos < totalSamples; pos += block)
    {
        const int n = juce::jmin (block, totalSamples - pos);
        juce::MidiBuffer midi;
        while (eventIndex < seq.getNumEvents() && seq.getEventTime (eventIndex) < pos + n)
        {
            const auto* ev = seq.getEventPointer (eventIndex);
            midi.addEvent (ev->message, juce::jmax (0, (int) ev->message.getTimeStamp() - pos));
            ++eventIndex;
        }
        juce::AudioBuffer<float> chunk (output.getArrayOfWritePointers(), 2, pos, n);
        engine.process (chunk, midi, patch.params, transport);
        transport.ppqPosition += (double) n / sr * transport.bpm / 60.0;
    }
    const double renderMs = juce::Time::getMillisecondCounterHiRes() - startTime;

    // Write WAV
    if (outFile.isNotEmpty())
    {
        juce::File f (outFile);
        f.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0));
        if (writer == nullptr) { std::cerr << "Cannot write " << outFile << std::endl; return 3; }
        writer->writeFromAudioSampleBuffer (output, 0, totalSamples);
    }

    // Analysis
    const auto a = analyse (output, sr, lastNoteOff);
    const auto safety = engine.diagnostics().safety.snapshot();
    const auto perf = engine.diagnostics().profiler.snapshot();

    auto* root = new juce::DynamicObject();
    root->setProperty ("out", outFile);
    root->setProperty ("sampleRate", sr);
    root->setProperty ("block", block);
    root->setProperty ("seconds", seconds);
    root->setProperty ("peak", a.peak);
    root->setProperty ("peakDb", juce::Decibels::gainToDecibels (a.peak, -120.0f));
    root->setProperty ("rms", a.rms);
    root->setProperty ("rmsDb", juce::Decibels::gainToDecibels (a.rms, -120.0f));
    root->setProperty ("dc", a.dc);
    root->setProperty ("crest", a.crest);
    root->setProperty ("nonFinite", a.nonFinite);
    root->setProperty ("centroidHz", a.centroidHz);
    root->setProperty ("sustainDb", a.sustainRms);
    root->setProperty ("t20", a.t20);
    root->setProperty ("t60", a.t60);
    root->setProperty ("renderMs", renderMs);
    root->setProperty ("realtimeRatio", renderMs / (seconds * 1000.0));
    root->setProperty ("activeVoicesEnd", engine.activeVoices());
    root->setProperty ("modRoutings", routings.size());

    if (analysis.isValid())
    {
        auto* a = new juce::DynamicObject();
        a->setProperty ("fundamentalHz", analysis.fundamentalHz);
        a->setProperty ("partials", analysis.count);
        a->setProperty ("harmonicity", analysis.harmonicity);
        a->setProperty ("noiseFloorDb", analysis.noiseFloorDb);
        a->setProperty ("attackSeconds", analysis.attackSeconds);
        const auto fit = SampleAnalyzer::fitShape (analysis);
        a->setProperty ("form", fit.form);
        a->setProperty ("tension", fit.tension);
        a->setProperty ("decay", fit.decay);
        a->setProperty ("mass", fit.mass);
        a->setProperty ("density", fit.density);
        juce::Array<juce::var> ratios;
        for (int i = 0; i < juce::jmin (8, analysis.count); ++i) ratios.add (analysis.partials[(size_t) i].ratio);
        a->setProperty ("topRatios", ratios);
        root->setProperty ("analysis", juce::var (a));
    }

    juce::Array<juce::var> centroids; for (auto c : a.centroidTrack) centroids.add (c);
    root->setProperty ("centroidTrack", centroids);
    juce::Array<juce::var> rmsTrack; for (auto v : a.rmsTrack) rmsTrack.add (v);
    root->setProperty ("rmsTrackDb", rmsTrack);

    auto* safetyObj = new juce::DynamicObject();
    for (int i = 0; i < SafetyMonitor::kNumEvents; ++i) safetyObj->setProperty (SafetyMonitor::eventName ((SafetyEvent) i), (int) safety.counts[i]);
    root->setProperty ("safety", juce::var (safetyObj));

    auto* cpu = new juce::DynamicObject();
    for (int i = 0; i < PerformanceProfiler::kNumSubsystems; ++i) cpu->setProperty (subsystemName ((Subsystem) i), perf.avgPercent[i]);
    cpu->setProperty ("total", perf.totalAvgPercent);
    cpu->setProperty ("totalPeak", perf.totalPeakPercent);
    root->setProperty ("cpuPercent", juce::var (cpu));

    const auto json = juce::JSON::toString (juce::var (root));
    if (hasOption (args, "--json")) juce::File (optionValue (args, "--json")).replaceWithText (json);
    std::cout << json << std::endl;
    return a.nonFinite > 0 ? 1 : 0;
}

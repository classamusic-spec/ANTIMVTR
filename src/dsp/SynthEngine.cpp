#include "SynthEngine.h"

namespace am
{

SynthEngine::SynthEngine() = default;

void SynthEngine::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    maxBlock = std::min (maxBlockSize, kMaxBlockSize);

    controlGraph.prepare (sampleRate, maxBlock);
    voices.prepare (sampleRate, maxBlock, &diag);
    fracture.prepare (sampleRate, maxBlock);
    space.prepare (sampleRate, maxBlock);
    master.prepare (sampleRate, maxBlock);
    diag.profiler.prepare (sampleRate, maxBlock);
    diag.clearTaps();

    prepared = true;
    diag.events.push (EngineEventType::EnginePrepared, Subsystem::Unknown, -1, (uint32_t) maxBlock, (float) sampleRate, sampleTime);
}

void SynthEngine::reset()
{
    voices.reset();
    fracture.reset();
    space.reset();
    master.reset();
    diag.clearTaps();
    diag.events.push (EngineEventType::EngineReset, Subsystem::Unknown, -1, 0, 0.0f, sampleTime);
}

void SynthEngine::messageThreadMaintenance()
{
    fracture.messageThreadMaintenance();
    space.messageThreadMaintenance();
}

void SynthEngine::applyGlobalSettings (const ParamValues& params)
{
    static constexpr int voiceChoices[] = { 8, 16, 32, 64 };
    const int vc = juce::jlimit (0, 3, paramChoice (params, Param::masterVoices));
    if (voiceChoices[vc] != currentMaxVoices)
    {
        currentMaxVoices = voiceChoices[vc];
        voices.setMaxVoices (currentMaxVoices);
    }

    const auto q = (Quality) juce::jlimit (0, (int) Quality::Count - 1, paramChoice (params, Param::masterQuality));
    if (q != currentQuality)
    {
        currentQuality = q;
        diag.events.push (EngineEventType::QualityChanged, Subsystem::Matter, -1, (uint32_t) q, 0.0f, sampleTime);
    }
}

void SynthEngine::process (juce::AudioBuffer<float>& out, const juce::MidiBuffer& midi,
                           const ParamValues& hostParams, const TransportInfo& transport)
{
    jassert (prepared);
    juce::ScopedNoDenormals noDenormals;

    const int totalSamples = out.getNumSamples();
    float* outL = out.getWritePointer (0);
    float* outR = out.getNumChannels() > 1 ? out.getWritePointer (1) : nullptr;

    if (totalSamples <= 0 || outL == nullptr) return;

    diag.profiler.setEnabled (diag.dev.profiling.load (std::memory_order_relaxed));
    diag.profiler.beginBlock (totalSamples);
    diag.profiler.setActiveVoices (voices.activeVoiceCount());

    applyGlobalSettings (hostParams);

    RenderContext ctx;
    ctx.sampleRate  = sr;
    ctx.params      = &controlGraph.values();
    ctx.transport   = transport;
    ctx.dryMode     = (DryMode) juce::jlimit (0, (int) DryMode::Count - 1, diag.dev.dryMode.load (std::memory_order_relaxed));
    ctx.quality     = currentQuality;
    ctx.diagnostics = &diag;

    // The host block may exceed our internal maximum: process in chunks.
    int chunkStart = 0;
    auto midiIt = midi.begin();
    const auto midiEnd = midi.end();

    while (chunkStart < totalSamples)
    {
        const int chunkSize = std::min (maxBlock, totalSamples - chunkStart);
        const int chunkEnd  = chunkStart + chunkSize;

        {
            PerformanceProfiler::Scoped t (diag.profiler, Subsystem::Modulation);
            controlGraph.update (hostParams, chunkSize);
        }

        float* mL = mixL.data();
        float* mR = mixR.data();
        float* sL = srcL.data();
        float* sR = srcR.data();
        juce::FloatVectorOperations::clear (mL, chunkSize);
        juce::FloatVectorOperations::clear (mR, chunkSize);
        juce::FloatVectorOperations::clear (sL, chunkSize);
        juce::FloatVectorOperations::clear (sR, chunkSize);

        // --- Voices, split at MIDI events for sample-accurate note timing.
        {
            PerformanceProfiler::Scoped t (diag.profiler, Subsystem::VoiceMix);
            int pos = chunkStart;
            while (pos < chunkEnd)
            {
                int next = chunkEnd;
                while (midiIt != midiEnd)
                {
                    const auto meta = *midiIt;
                    if (meta.samplePosition > pos) { next = std::min (chunkEnd, meta.samplePosition); break; }
                    const auto message = meta.getMessage();
                    if (message.isNoteOn() && voices.activeVoiceCount() == 0)
                        fracture.noteStarted();     // first note of a phrase retriggers the fragment sequencer
                    voices.handleMidi (message, controlGraph.values(), currentQuality);
                    ++midiIt;
                }

                const int n = next - pos;
                if (n > 0)
                {
                    ctx.numSamples = n;
                    const int off = pos - chunkStart;
                    voices.render (mL + off, mR + off, sL + off, sR + off, n, ctx);
                    sampleTime += (uint64_t) n;
                    diag.sampleClock.store (sampleTime, std::memory_order_relaxed);
                }
                pos = next;
            }
            // Any events at the very end of the chunk (position == chunkEnd) are handled next chunk / next block.
        }

        ctx.numSamples = chunkSize;

        // --- Polyphony headroom: one voice sits at -3.7 dB, four at -8 dB, sixteen at -12 dB, sixty-four at -16 dB,
        //     eased over ~40 ms so notes joining a chord never step the level.
        {
            const int active = std::max (1, voices.activeVoiceCount());
            const float target = 0.65f / std::pow ((float) active, 0.3f);
            const float alpha = juce::jlimit (0.0f, 1.0f, (float) chunkSize / (float) (0.04 * sr));
            polyphonyGain += (target - polyphonyGain) * alpha;
            juce::FloatVectorOperations::multiply (mL, polyphonyGain, chunkSize);
            juce::FloatVectorOperations::multiply (mR, polyphonyGain, chunkSize);
            juce::FloatVectorOperations::multiply (sL, polyphonyGain, chunkSize);
            juce::FloatVectorOperations::multiply (sR, polyphonyGain, chunkSize);
        }

        // --- Diagnostics taps: source and post-matter
        diag.taps[(int) Stage::Source].push (sL, sR, chunkSize);
        diag.taps[(int) Stage::PostMatter].push (mL, mR, chunkSize);
        diag.taps[(int) Stage::PostEvolve].push (mL, mR, chunkSize);
        stageMeters[(int) Stage::Source].measureStereo (sL, sR, chunkSize);
        stageMeters[(int) Stage::PostMatter].measureStereo (mL, mR, chunkSize);
        stageMeters[(int) Stage::PostEvolve] = stageMeters[(int) Stage::PostMatter];

        // --- Fracture
        const bool dry = ctx.dryMode != DryMode::FullSynth;
        if (! dry && ! diag.dev.bypassFracture.load (std::memory_order_relaxed))
        {
            PerformanceProfiler::Scoped t (diag.profiler, Subsystem::Fracture);
            fracture.process (mL, mR, chunkSize, ctx);
        }
        diag.taps[(int) Stage::PostFracture].push (mL, mR, chunkSize);
        stageMeters[(int) Stage::PostFracture].measureStereo (mL, mR, chunkSize);

        // --- Space
        if (! dry && ! diag.dev.bypassSpace.load (std::memory_order_relaxed))
        {
            PerformanceProfiler::Scoped t (diag.profiler, Subsystem::Space);
            space.process (mL, mR, chunkSize, ctx);
        }
        diag.taps[(int) Stage::PostSpace].push (mL, mR, chunkSize);
        stageMeters[(int) Stage::PostSpace].measureStereo (mL, mR, chunkSize);

        // --- Master
        {
            PerformanceProfiler::Scoped t (diag.profiler, Subsystem::Master);
            master.process (mL, mR, chunkSize, ctx, &diag.safety);
        }

        juce::FloatVectorOperations::copy (outL + chunkStart, mL, chunkSize);
        if (outR != nullptr) juce::FloatVectorOperations::copy (outR + chunkStart, mR, chunkSize);
        diag.taps[(int) Stage::Master].push (mL, mR, chunkSize);
        stageMeters[(int) Stage::Master].measureStereo (mL, mR, chunkSize);

        publishSnapshots (mL, mR, chunkSize, ctx);

        chunkStart = chunkEnd;
    }

    // Any remaining events positioned at/after the end of the buffer (should not happen, but be safe).
    while (midiIt != midiEnd)
    {
        voices.handleMidi ((*midiIt).getMessage(), controlGraph.values(), currentQuality);
        ++midiIt;
    }

    for (int ch = 2; ch < out.getNumChannels(); ++ch)
        out.clear (ch, 0, totalSamples);

    diag.profiler.endBlock();
}

void SynthEngine::publishSnapshots (const float* outL, const float* outR, int numSamples, const RenderContext& ctx)
{
    PerformanceProfiler::Scoped t (diag.profiler, Subsystem::Visualization);
    juce::ignoreUnused (outL, outR, numSamples);

    const auto& p = controlGraph.values();
    const int active = voices.activeVoiceCount();

    // Focus voice: explicit from DSP LAB or the most recently started one.
    int focus = diag.dev.focusVoice.load (std::memory_order_relaxed);
    if (focus < 0 || focus >= voices.getMaxVoices() || ! voices.voice (focus).isActive())
        focus = voices.mostRecentVoice();
    const AntiMatrVoice* fv = (focus >= 0 && focus < kMaxVoices && voices.voice (focus).isActive()) ? &voices.voice (focus) : nullptr;

    // ---- Visual state
    {
        auto& v = diag.visualSnapshots.beginWrite();
        const auto& master = stageMeters[(int) Stage::Master];
        v.rmsL = master.rms; v.rmsR = master.rms; v.peak = master.peak;
        v.sourceRms = stageMeters[(int) Stage::Source].rms;
        v.matterRms = stageMeters[(int) Stage::PostMatter].rms;
        v.activeVoices = active;
        v.density = paramValue (p, Param::shapeDensity); v.form = paramValue (p, Param::shapeForm);
        v.mass = paramValue (p, Param::shapeMass); v.tension = paramValue (p, Param::shapeTension);
        v.decay = paramValue (p, Param::shapeDecay); v.surface = paramValue (p, Param::shapeSurface);
        v.bend = paramValue (p, Param::evolveBend); v.melt = paramValue (p, Param::evolveMelt);
        v.tear = paramValue (p, Param::evolveTear); v.magnet = paramValue (p, Param::evolveMagnet);
        v.gravity = paramValue (p, Param::evolveGravity); v.scatter = paramValue (p, Param::evolveScatter);
        v.crush = paramValue (p, Param::evolveCrush); v.freeze = paramBool (p, Param::evolveFreeze);
        v.fractureOn = paramBool (p, Param::fractureOn);
        v.fractureActivity = fracture.activity();
        v.spaceActivity = space.activity();
        v.spaceType = paramChoice (p, Param::spaceType);
        v.sampleTime = sampleTime;

        int activeNodes = 0, clusterCount = 0, visualNodes = 0;
        float pitch = 0.0f, energy = 0.0f;
        if (fv != nullptr)
        {
            const auto& m = fv->matter();
            activeNodes = m.activeNodes();
            clusterCount = m.clusterCount();
            pitch = (float) fv->noteState().frequency;
            energy = fv->envelopeLevel();
            const int count = std::min (m.numNodes(), VisualStateSnapshot::kVisualNodes);
            for (int i = 0; i < count; ++i)
            {
                const auto& node = m.node (i);
                v.nodeFrequency[i] = node.frequency;
                v.nodeEnergy[i] = node.energy;
                v.nodePan[i] = node.pan;
                v.nodeCluster[i] = node.cluster;
            }
            visualNodes = count;
        }
        v.activeNodes = activeNodes; v.clusterCount = clusterCount;
        v.pitchHz = pitch; v.noteEnergy = energy; v.numVisualNodes = visualNodes;
        diag.visualSnapshots.endWrite();
    }

    // ---- Diagnostic snapshot
    {
        auto& d = diag.diagnosticSnapshots.beginWrite();
        d.sampleRate = sr; d.blockSize = maxBlock;
        d.quality = (uint8_t) currentQuality; d.dryMode = (uint8_t) ctx.dryMode;
        d.activeVoices = active; d.maxVoices = voices.getMaxVoices();
        d.sampleTime = sampleTime; d.latencySamples = latencySamples();
        for (int i = 0; i < (int) Stage::Count; ++i)
        {
            d.stages[i].rms = stageMeters[i].rms;
            d.stages[i].peak = stageMeters[i].peak;
        }
        d.safety = diag.safety.snapshot();
        d.perf = diag.profiler.snapshot();
        d.focusVoice = fv != nullptr ? focus : -1;
        d.focusNote = fv != nullptr ? fv->midiNote() : -1;
        if (fv != nullptr)
        {
            const auto& m = fv->matter();
            d.numNodes = m.fillDiagnostics (d.nodes, kMaxMatterNodes);
            d.activeNodes = m.activeNodes();
            d.clusterCount = m.clusterCount();
            d.topologySeed = m.topologySeed();
            d.matterEnergy = m.energy();
            d.numEdges = m.fillEdgeDiagnostics (d.edges, kMaxMatterEdges);
            m.couplingStats (d.averageCoupling, d.maxCoupling);
            d.fundamentalHz = (float) fv->noteState().frequency;
        }
        else
        {
            d.numNodes = 0; d.numEdges = 0; d.activeNodes = 0; d.clusterCount = 0; d.matterEnergy = 0.0f;
            d.averageCoupling = 0.0f; d.maxCoupling = 0.0f; d.fundamentalHz = 0.0f;
        }
        d.materialA = (uint8_t) paramChoice (p, Param::shapeMaterialA);
        d.materialB = (uint8_t) paramChoice (p, Param::shapeMaterialB);
        d.materialBlend = paramValue (p, Param::shapeBlend);
        d.fractureFFTSize = fracture.fftSize();
        d.fractureHop = fracture.hopSize();
        d.fractureActivity = fracture.activity();
        d.eventsDropped = diag.events.droppedCount();
        diag.diagnosticSnapshots.endWrite();
    }
}

} // namespace am

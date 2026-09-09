#include "AntiMatrVoice.h"
#include "dev/diagnostics/Diagnostics.h"

namespace am
{

void AntiMatrVoice::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    sourceEngine.prepare (sampleRate, maxBlockSize);
    matterEngine.prepare (sampleRate, maxBlockSize);
    evolveEngine.prepare (sampleRate, maxBlockSize);
    ampEnv.prepare (sampleRate);
    reset();
}

void AntiMatrVoice::reset()
{
    sourceEngine.reset();
    matterEngine.reset();
    evolveEngine.reset();
    ampEnv.reset();
    note = NoteState();
    active = false;
    gliding = false;
    lastEnergy = 0.0f;
}

void AntiMatrVoice::start (const StartInfo& info, const ParamValues& params)
{
    const bool wasActive = active && info.legato;

    note.midiNote = info.midiNote;
    note.velocity = clamp01 (info.velocity);
    note.channel  = info.channel;
    note.noteId   = info.noteId;
    note.gate     = true;
    note.sustained = false;
    note.pitchBendSemis = info.pitchBendSemis;
    note.pressure = info.pressure;
    note.modWheel = info.modWheel;

    const double transpose = (double) paramValue (params, Param::masterTranspose)
                           + (double) paramValue (params, Param::masterFine) * 0.01;
    note.baseFrequency = midiNoteToHz ((double) info.midiNote + transpose);
    note.frequency = note.baseFrequency;

    // Velocity → gain: blend between constant and full velocity curve.
    const float velSens = paramValue (params, Param::ampVelocity);
    const float velCurve = note.velocity * note.velocity * 0.7f + note.velocity * 0.3f;
    velocityGain = lerp (1.0f, velCurve, velSens);

    // Glide
    glideTargetLog = std::log2 (note.baseFrequency);
    if (info.glideSeconds > 0.0005 && info.fromFrequency > 0.0)
    {
        glideLog = std::log2 (info.fromFrequency);
        glideCoeff = std::exp (-1.0 / (info.glideSeconds * sr * 0.25)); // reaches ~98% in glideSeconds
        gliding = true;
    }
    else
    {
        glideLog = glideTargetLog;
        gliding = false;
    }

    if (! wasActive)
    {
        sourceEngine.noteOn (note, params);
        matterEngine.noteOn (note, params, info.quality);
        evolveEngine.noteOn (note, params);
        ampEnv.setParameters (paramValue (params, Param::ampAttack), paramValue (params, Param::ampDecay),
                              paramValue (params, Param::ampSustain), paramValue (params, Param::ampRelease),
                              paramValue (params, Param::ampCurve));
        ampEnv.noteOn();
    }
    else
    {
        // Legato: keep everything running, only the pitch target changes.
        matterEngine.noteOn (note, params, info.quality);
    }

    active = true;
}

void AntiMatrVoice::release()
{
    note.gate = false;
    note.sustained = false;
    sourceEngine.noteOff();
    matterEngine.noteOff();
    ampEnv.noteOff();
}

void AntiMatrVoice::kill()
{
    note.gate = false;
    ampEnv.kill();
}

void AntiMatrVoice::updateFrequency (const RenderContext& ctx)
{
    if (gliding)
    {
        glideLog = glideTargetLog + std::pow (glideCoeff, (double) ctx.numSamples) * (glideLog - glideTargetLog);
        if (std::abs (glideLog - glideTargetLog) < 1.0e-4) { glideLog = glideTargetLog; gliding = false; }
    }
    const double bendSemis = (double) note.pitchBendSemis;
    note.frequency = std::exp2 (glideLog + bendSemis / 12.0);
    note.frequency = juce::jlimit ((double) kMinFrequencyHz, ctx.sampleRate * (double) kMaxFrequencyRatio, note.frequency);
}

void AntiMatrVoice::render (float* outL, float* outR, float* srcL, float* srcR, int n, const RenderContext& ctx)
{
    if (! active) return;

    updateFrequency (ctx);

    ampEnv.setParameters (ctx.param (Param::ampAttack), ctx.param (Param::ampDecay),
                          ctx.param (Param::ampSustain), ctx.param (Param::ampRelease),
                          ctx.param (Param::ampCurve));

    auto* diag = ctx.diagnostics;
    jassert (diag != nullptr); // SynthEngine always provides diagnostics
    float* sL = bufL.data();
    float* sR = bufR.data();
    float* mL = matL.data();
    float* mR = matR.data();

    // 1. Source
    {
        PerformanceProfiler::Scoped timer (diag->profiler, Subsystem::Source);
        sourceEngine.render (sL, sR, n, ctx, note);
    }

    if (srcL != nullptr) juce::FloatVectorOperations::add (srcL, sL, n);
    if (srcR != nullptr) juce::FloatVectorOperations::add (srcR, sR, n);

    // 2. Evolve (operates on Matter nodes before rendering)
    if (ctx.dryMode != DryMode::SourceOnly && ctx.dryMode != DryMode::MatterOnly
        && ! diag->dev.bypassEvolve.load (std::memory_order_relaxed))
    {
        PerformanceProfiler::Scoped timer (diag->profiler, Subsystem::Evolve);
        evolveEngine.apply (matterEngine, ctx, note);
    }

    // 3. Matter
    float mix = ctx.param (Param::shapeMix);
    if (ctx.dryMode == DryMode::SourceOnly) mix = 0.0f;

    if (mix > 0.0f)
    {
        PerformanceProfiler::Scoped timer (diag->profiler, Subsystem::Matter);
        matterEngine.process (sL, sR, mL, mR, n, ctx, note);
    }

    // 4. Mix source / matter, apply velocity + envelope, accumulate.
    const float dryGain = (1.0f - mix) * velocityGain;
    const float wetGain = mix * velocityGain;
    for (int i = 0; i < n; ++i)
    {
        const float env = ampEnv.next();
        const float a = (sL[i] * dryGain + (mix > 0.0f ? mL[i] * wetGain : 0.0f)) * env;
        const float b = (sR[i] * dryGain + (mix > 0.0f ? mR[i] * wetGain : 0.0f)) * env;
        outL[i] += a;
        outR[i] += b;
    }

    lastEnergy = ampEnv.getLevel() * std::max (matterEngine.energy(), sourceEngine.energy());

    // 5. Voice lifetime: ends when the amplitude envelope is finished.
    if (! ampEnv.isActive())
    {
        active = false;
        diag->events.push (EngineEventType::VoiceEnded, Subsystem::VoiceMix, -1, (uint32_t) note.midiNote, 0.0f,
                           diag->sampleClock.load (std::memory_order_relaxed));
    }
}

} // namespace am

#include "ModulationEngine.h"

namespace am
{

namespace
{
    inline Param macroParam (int slot) noexcept
    {
        static constexpr Param table[kNumMacros] =
        {
            Param::macro1, Param::macro2, Param::macro3, Param::macro4,
            Param::macro5, Param::macro6, Param::macro7, Param::macro8
        };
        return table[(size_t) juce::jlimit (0, kNumMacros - 1, slot)];
    }

    inline Param lfoRetrig (int slot) noexcept
    {
        switch (slot)
        {
            case 0:  return Param::lfo1Retrig;
            case 1:  return Param::lfo2Retrig;
            case 2:  return Param::lfo3Retrig;
            default: return Param::lfo4Retrig;
        }
    }

    /** Presents a compiled routing's source value and applies its curve. */
    inline float shapeContribution (const CompiledRouting& r, float sourceValue) noexcept
    {
        const float presented = presentModValue (sourceValue, r.sourceBipolar, r.wantBipolar);
        return applyModCurve (presented, r.curve) * r.scale;
    }
}

//==============================================================================
void VoiceModulator::prepare (double sampleRate) noexcept
{
    sr = sampleRate > 0.0 ? sampleRate : 48000.0;
    for (int i = 0; i < kNumLFOs; ++i) lfos[(size_t) i].prepare (sr, hashSeed (0x5EED0000u, (uint32_t) i));
    reset();
}

void VoiceModulator::reset() noexcept
{
    for (auto& l : lfos) l.reset();
    for (auto& e : envelopes) e.reset();
    notes = NoteSourceValues();
    deltas.fill (0.0f);
    numDeltas = 0;
}

void VoiceModulator::noteOn (const NoteState& note, uint32_t seed) noexcept
{
    for (auto& l : lfos) l.retrigger();
    for (auto& e : envelopes) e.noteOn();

    Rng rng (hashSeed (seed, 0x9E37u ^ (uint32_t) note.midiNote));
    notes.noteRandom = rng.nextBipolar();
    notes.update (note, 2.0f);
}

void VoiceModulator::noteOff() noexcept
{
    for (auto& e : envelopes) e.noteOff();
    notes.gate = 0.0f;
}

float VoiceModulator::value (ModSource s) const noexcept
{
    const int slot = modSourceSlot (s);
    switch (modSourceGroup (s))
    {
        case ModSourceGroup::LFO:      return slot >= 0 ? lfos[(size_t) slot].value() : 0.0f;
        case ModSourceGroup::Envelope: return slot >= 0 ? envelopes[(size_t) slot].value() : 0.0f;
        case ModSourceGroup::Note:     return notes.of (s);
        default:                       return 0.0f;
    }
}

const ParamValues* VoiceModulator::process (const ParamValues& global, const ModPlan* plan, int numSamples,
                                            double sampleRate, const NoteState& note, const TransportInfo& transport) noexcept
{
    numDeltas = 0;
    if (plan == nullptr || plan->numPoly == 0 || numSamples <= 0) return &global;

    sr = sampleRate > 0.0 ? sampleRate : sr;
    notes.update (note, paramValue (global, Param::masterBendRange));

    for (int i = 0; i < kNumLFOs; ++i)
        if (plan->usesSource ((ModSource) ((int) ModSource::LFO1 + i)))
            lfos[(size_t) i].advance (ModLFO::settingsFor (global, i), numSamples, sr, transport, false);

    for (int i = 0; i < kNumEnvelopes; ++i)
        if (plan->usesSource ((ModSource) ((int) ModSource::Env1 + i)))
            envelopes[(size_t) i].advance (ModEnvelope::settingsFor (global, i), numSamples, sr);

    numDeltas = plan->numPolyTargets;
    for (int i = 0; i < numDeltas; ++i) deltas[(size_t) i] = 0.0f;

    for (int i = 0; i < plan->numPoly; ++i)
    {
        const auto& r = plan->poly[(size_t) i];
        const float amount = shapeContribution (r, value ((ModSource) r.source));
        if (std::isfinite (amount)) deltas[(size_t) r.targetSlot] += amount;
    }

    voiceParams = global;
    const auto& table = ParameterRegistry::all();
    for (int i = 0; i < numDeltas; ++i)
    {
        const int target = plan->polyTargets[(size_t) i];
        voiceParams[(size_t) target] = table[(size_t) target].clampValue (global[(size_t) target] + deltas[(size_t) i]);
    }
    return &voiceParams;
}

//==============================================================================
ModulationEngine::ModulationEngine() = default;

void ModulationEngine::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate > 0.0 ? sampleRate : 48000.0;
    juce::ignoreUnused (maxBlockSize);
    for (int i = 0; i < kNumLFOs; ++i) lfos[(size_t) i].prepare (sr, hashSeed (0xA11CE000u, (uint32_t) i));
    for (int i = 0; i < kNumChaos; ++i) chaos[(size_t) i].prepare ((uint32_t) (i * 17));
    // The displayed modulation range relaxes back over roughly 1.5 s.
    envelopeDecay = (float) juce::jlimit (0.001, 0.5, (double) controlBlockForQuality (Quality::Normal) / (sr * 1.5));
    reset();
}

void ModulationEngine::reset()
{
    for (auto& l : lfos) l.reset();
    for (auto& c : chaos) c.reset();
    macros.fill (0.0f);
    monoMod.fill (0.0f);
    modMin.fill (0.0f);
    modMax.fill (0.0f);
    retriggerRequest.store (false, std::memory_order_relaxed);
}

void ModulationEngine::compile (const ParamValues& params) noexcept
{
    plan.numMono = plan.numPoly = plan.numPolyTargets = 0;
    plan.polySources = 0;
    targeted.fill (0);
    monoMod.fill (0.0f);
    modMin.fill (0.0f);
    modMax.fill (0.0f);

    const auto& descriptors = ParameterRegistry::all();
    for (int i = 0; i < routings.size(); ++i)
    {
        const auto& r = routings[i];
        if (! r.enabled || ! ModRoutingTable::isValid (r)) continue;

        const int targetIndex = paramIndex (r.target);
        const auto& d = descriptors[(size_t) targetIndex];

        CompiledRouting c;
        c.target        = (uint16_t) targetIndex;
        c.source        = (uint8_t) r.source;
        c.wantBipolar   = r.bipolar;
        c.sourceBipolar = modSourceIsBipolar (r.source);
        c.scale         = juce::jlimit (-1.0f, 1.0f, r.depth) * (d.max - d.min);
        c.curve         = r.curve;

        if (targeted[(size_t) targetIndex] < 255) ++targeted[(size_t) targetIndex];

        if (modSourceIsPerVoice (r.source, params))
        {
            int slot = -1;
            for (int t = 0; t < plan.numPolyTargets; ++t)
                if (plan.polyTargets[(size_t) t] == c.target) { slot = t; break; }
            if (slot < 0)
            {
                slot = plan.numPolyTargets;
                plan.polyTargets[(size_t) slot] = c.target;
                ++plan.numPolyTargets;
            }
            c.targetSlot = (uint8_t) slot;
            plan.polySources |= (1u << (uint32_t) r.source);
            plan.poly[(size_t) plan.numPoly++] = c;
        }
        else
        {
            plan.mono[(size_t) plan.numMono++] = c;
        }
    }
}

void ModulationEngine::beginBlock (const ParamValues& params) noexcept
{
    bool dirty = false;

    if (const auto* fresh = handoff.acquire())
    {
        if (! haveTable || routings != *fresh)
        {
            routings = *fresh;
            haveTable = true;
            dirty = true;
        }
    }

    uint8_t mask = 0;
    for (int i = 0; i < kNumLFOs; ++i)
        if (paramBool (params, lfoRetrig (i))) mask |= (uint8_t) (1u << i);
    if (mask != retrigMask) { retrigMask = mask; dirty = true; }

    if (dirty) compile (params);

    if (retriggerRequest.exchange (false, std::memory_order_acq_rel))
        for (auto& l : lfos) l.retrigger();
}

void ModulationEngine::process (ControlGraph& graph, int numSamples, const TransportInfo& transport) noexcept
{
    if (numSamples <= 0) return;
    const auto& p = graph.values();   // last slice's effective values (so sources can modulate sources)

    for (int i = 0; i < kNumMacros; ++i)
        macros[(size_t) i] = clamp01 (paramValue (p, macroParam (i)));

    for (int i = 0; i < kNumLFOs; ++i)
        lfos[(size_t) i].advance (ModLFO::settingsFor (p, i), numSamples, sr, transport, true);

    for (int i = 0; i < kNumChaos; ++i)
        chaos[(size_t) i].advance (ChaosGenerator::settingsFor (p, i), numSamples, sr);

    for (int i = 0; i < plan.numMono; ++i)
        monoMod[(size_t) plan.mono[(size_t) i].target] = 0.0f;

    for (int i = 0; i < plan.numMono; ++i)
    {
        const auto& r = plan.mono[(size_t) i];
        const float amount = shapeContribution (r, value ((ModSource) r.source));
        if (! std::isfinite (amount)) continue;
        graph.addModulation (paramFromIndex ((int) r.target), amount);
        monoMod[(size_t) r.target] += amount;
    }
}

float ModulationEngine::value (ModSource s) const noexcept
{
    const int slot = modSourceSlot (s);
    switch (modSourceGroup (s))
    {
        case ModSourceGroup::LFO:   return slot >= 0 ? lfos[(size_t) slot].value() : 0.0f;
        case ModSourceGroup::Chaos: return slot >= 0 ? chaos[(size_t) slot].value() : 0.0f;
        case ModSourceGroup::Macro: return slot >= 0 ? macros[(size_t) slot] : 0.0f;
        default:                    return 0.0f;
    }
}

void ModulationEngine::fillSnapshot (ModulationSnapshot& s, const VoiceModulator* focus, int focusVoice, uint64_t sampleTime) const noexcept
{
    auto& self = *const_cast<ModulationEngine*> (this);   // the min/max envelope is display-only state

    s.numRoutings     = routings.size();
    s.numEnabled      = routings.numEnabled();
    s.numPolyRoutings = plan.numPoly;
    s.controlBlock    = plan.isEmpty() ? 0 : controlBlockForQuality (quality);
    s.focusVoice      = focus != nullptr ? focusVoice : -1;
    s.sampleTime      = sampleTime;

    for (int i = 1; i < kNumModSources; ++i)
    {
        const auto src = (ModSource) i;
        const auto group = modSourceGroup (src);
        const bool perVoice = group == ModSourceGroup::Envelope || group == ModSourceGroup::Note
                            || (group == ModSourceGroup::LFO && plan.usesSource (src));
        s.sourceValue[i] = perVoice ? (focus != nullptr ? focus->value (src) : 0.0f) : value (src);
    }
    s.sourceValue[0] = 0.0f;

    std::memcpy (s.modulation, monoMod.data(), sizeof (s.modulation));
    if (focus != nullptr)
        for (int i = 0; i < plan.numPolyTargets && i < focus->numContributions(); ++i)
            s.modulation[plan.polyTargets[(size_t) i]] += focus->deltaAt (i);

    const auto track = [&self, &s] (int target) noexcept
    {
        const float current = s.modulation[target];
        float& lo = self.modMin[(size_t) target];
        float& hi = self.modMax[(size_t) target];
        lo += (current - lo) * self.envelopeDecay;
        hi += (current - hi) * self.envelopeDecay;
        if (current < lo) lo = current;
        if (current > hi) hi = current;
    };

    for (int i = 0; i < plan.numMono; ++i) track (plan.mono[(size_t) i].target);
    for (int i = 0; i < plan.numPolyTargets; ++i) track (plan.polyTargets[(size_t) i]);

    std::memcpy (s.modMin, modMin.data(), sizeof (s.modMin));
    std::memcpy (s.modMax, modMax.data(), sizeof (s.modMax));
    std::memcpy (s.targeted, targeted.data(), sizeof (s.targeted));
}

} // namespace am

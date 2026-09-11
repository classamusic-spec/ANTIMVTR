#include "MatterEngine.h"
#include "core/RealtimeUtils.h"
#include "core/FastMath.h"
#include "dev/diagnostics/Diagnostics.h"

namespace am
{

namespace
{
    constexpr float kSourceGain  = 0.24f;   ///< sustained excitation into the nodes (before damping normalisation)
    constexpr float kStrikeGain  = 1.5f;   ///< strike pulse into the nodes
    // A resonator answers a *sustained* drive at resonance with a build-up of roughly Q, but answers
    // an *impulse* with roughly the impulse itself. One input gain cannot serve both: the excitation
    // gain is normalised against Q so a bowed tone stays sane, and that same normalisation buries a
    // mallet roll 30 dB down — measured, not guessed.
    //
    // The object already has an input that is calibrated for impulses: the strike. So rather than
    // switching the excitation gain on some estimate of what the source is doing — which is a hidden
    // dynamics processor, and gets it wrong on the attack of every sustained note — the excitation is
    // *split*. Its steady part drives the Q-normalised excitation input as before, and its transient
    // part is added to the strike input, where an impulse already lands at the right level. A steady
    // tone contributes nothing to the strike input, so sustained sound is untouched.
    constexpr float kEnvFastMs      = 2.0f;   ///< envelope detector attack
    constexpr float kEnvFastRelMs   = 40.0f;  ///< ... and release: an envelope, not a rectified ripple
    constexpr float kEnvSlowAtkMs   = 8.0f;   ///< the envelope's own local mean: rises within a note attack
    constexpr float kEnvSlowRelMs   = 400.0f; ///< ... and falls slowly, so the gap in a roll stays a gap
    constexpr float kTransientGain  = 0.005f; ///< transient content into the strike input
    constexpr float kStrikeRef   = 0.90f;  ///< coherent strike sum of the default object (density .5, excite .6, note 60); the strike is normalised towards it
    constexpr float kStrikeNormPower = 0.75f; ///< 0 = purely physical strike level, 1 = strike peak independent of the node structure
    constexpr float kOutputGain  = 1.0f;
    constexpr float kCouplingMax = 0.0025f; ///< per-sample coupling strength at shape.coupling = 1 (before the stability bound)
    constexpr float kGlideMs     = 12.0f;
    constexpr float kRunawayLimit = 1.0e4f;
}

MatterEngine::ShapeValues MatterEngine::readShape (const ParamValues& p) noexcept
{
    ShapeValues v;
    v.morph.form         = paramValue (p, Param::shapeForm);
    v.morph.density      = paramValue (p, Param::shapeDensity);
    v.morph.mass         = paramValue (p, Param::shapeMass);
    v.morph.tension      = paramValue (p, Param::shapeTension);
    v.morph.decay        = paramValue (p, Param::shapeDecay);
    v.morph.surface      = paramValue (p, Param::shapeSurface);
    v.morph.blend        = paramValue (p, Param::shapeBlend);
    v.morph.distribution = paramValue (p, Param::shapeDistribution);
    v.morph.stereo       = paramValue (p, Param::shapeStereo);
    v.morph.excite       = paramValue (p, Param::shapeExcite);
    v.morph.keytrack     = paramValue (p, Param::shapeKeytrack);
    v.morph.pitchSemis   = paramValue (p, Param::shapePitch);
    v.coupling           = clamp01 (paramValue (p, Param::shapeCoupling));
    v.strike             = clamp01 (paramValue (p, Param::shapeStrike));
    v.surface            = clamp01 (v.morph.surface);
    v.mass               = clamp01 (v.morph.mass);
    v.materialA          = std::clamp (paramChoice (p, Param::shapeMaterialA), 0, (int) MaterialType::Count - 1);
    v.materialB          = std::clamp (paramChoice (p, Param::shapeMaterialB), 0, (int) MaterialType::Count - 1);
    v.topologyType       = std::clamp (paramChoice (p, Param::shapeTopology), 0, (int) TopologyType::Count - 1);
    v.seed               = (uint32_t) std::max (0, (int) std::lround (paramValue (p, Param::shapeSeed)));
    return v;
}

void MatterEngine::prepare (double sampleRate, int)
{
    sr = sampleRate;
    StructureLibrary::ensureBuilt();
    reset();
}

void MatterEngine::reset()
{
    for (auto& n : nodes) n = Node();
    bank.resetAll();
    nodeCount = 0;
    currentEnergy = 0.0f;
    gate = false;
    snapNext = true;
    everStarted = false;
    strikePos = strikeLen = 0;
    outRing.fill (0.0f);
    outRingPos = 0;
    lpState = 0.0f;
    strikeNorm = 1.0f;
    envFast = envSlow = 0.0f;
    transientActive = false;
    // The morpher and the topology cache their built structure and are asked to rebuild only when
    // the seed, materials or node count change. Reset wipes the nodes those caches correspond to,
    // so the caches have to go with them or the next note is built against a structure that no
    // longer exists.
    morpher = MaterialMorpher();
    topology = MatterTopology();
    topologyVersion = 0;
    edgeJitter.fill (0.0f);
    seed = 0;
    lastTopology = -1;
    lastQualityNodes = 0;
    lastEdgeScale = 0.0f;
    lastCoupScale = 0.0f; lastCoupRMax = 0.0f; lastCoupJitter = -1.0f; lastCoupTopology = -1;
}

void MatterEngine::rebuildIfNeeded (const ShapeValues& v, int count) noexcept
{
    const auto a = (MaterialType) v.materialA;
    const auto b = (MaterialType) v.materialB;
    if (morpher.needsRebuild (v.seed, a, b, count))
    {
        morpher.rebuild (v.seed, a, b, count);
        Rng rng (hashSeed (v.seed, 0xED6Eu));
        for (auto& j : edgeJitter) j = rng.nextBipolar();
        lastTopology = -1;   // topology depends on the seed and the materials too
    }
    if (lastTopology != v.topologyType || lastQualityNodes != count || seed != v.seed)
    {
        seed = v.seed;
        lastTopology = v.topologyType;
        lastQualityNodes = count;
        const int clusters = std::clamp (2 + (int) std::lround (v.morph.density * 6.0f), 2, MatterTopology::kMaxClusters);
        topology.build ((TopologyType) v.topologyType, v.seed, count, clusters, morpher.profile().bandBWeight);
        ++topologyVersion;
        for (int i = 0; i < count; ++i)
        {
            nodes[(size_t) i].cluster = topology.clusterOf (i);
            nodes[(size_t) i].couplingCount = topology.couplingCount (i);
        }
    }
}

void MatterEngine::computeNextTargets (const ShapeValues& v, const NoteState& note, float blockSeconds) noexcept
{
    auto in = v.morph;
    in.noteFrequency = note.frequency;
    in.sampleRate = sr;
    in.blockSeconds = blockSeconds;
    in.nodeCount = nodeCount;
    morpher.computeTargets (in, nodes.data());
}

void MatterEngine::buildStrike (const NoteState& note, const ShapeValues& v) noexcept
{
    strikeBuf.fill (0.0f);
    strikePos = 0;
    strikeLen = 0;
    if (v.strike <= 0.0005f) return;

    const auto& P = morpher.profile();
    const float vel = clamp01 (note.velocity);
    // STRIKE law: square root so the control feels even (0.35 → 0.36, 1.0 → 0.6); velocity shapes the rest.
    const float amp = 0.6f * std::sqrt (v.strike) * (0.25f + 0.75f * std::pow (vel, 1.3f));

    // Contact time: material, MASS (heavier = softer) and velocity (harder hits are shorter).
    const float contactMs = P.strikeContact * std::exp2 ((v.mass - 0.4f) * 1.5f) * (1.3f - 0.6f * vel);
    const int   L = std::clamp ((int) std::lround ((double) contactMs * 0.001 * sr), 4, 1024);
    const float pulseScale = amp * 2.0f / (float) L;              // pulse sums to `amp`
    const float noisePeak  = pulseScale * P.strikeNoise * 0.7f;
    const int   noiseLen   = std::min (kStrikeBuf, 4 * L);

    Rng rng (hashSeed (note.noteId ^ v.seed, 0x57121CEu));

    // Strike scatter: every note lands up to 1.5 ms "late" by a seeded amount. Inaudible as timing, but the
    // strikes of a chord no longer sum sample-aligned (sixteen coherent pulses would otherwise stack +24 dB).
    const int offset = std::min (kStrikeBuf / 4, (int) (rng.nextFloat() * 0.0015f * (float) sr));
    const int pulseEnd = std::min (kStrikeBuf, offset + L);
    const int noiseEnd = std::min (kStrikeBuf, offset + noiseLen);
    for (int t = offset; t < pulseEnd; ++t)
        strikeBuf[(size_t) t] = pulseScale * 0.5f * (1.0f - std::cos ((float) kTwoPi * (float) (t - offset) / (float) L));
    for (int t = offset; t < noiseEnd; ++t)
    {
        const float env = std::exp (-(float) (t - offset) / (2.0f * (float) L));
        strikeBuf[(size_t) t] += noisePeak * env * rng.nextBipolar();
    }
    strikeLen = std::max (pulseEnd, noiseEnd);
    strikePulseLen = L;
}

void MatterEngine::noteOn (const NoteState& note, const ParamValues& params, Quality quality)
{
    const bool legato = gate && everStarted;
    const auto v = readShape (params);

    if (! legato)
        nodeCount = matterNodesForQuality (quality);
    if (nodeCount < ModalBank::kLanes) nodeCount = ModalBank::kLanes;
    bank.setCount (nodeCount);

    rebuildIfNeeded (v, nodeCount);

    if (! legato)
    {
        for (int i = 0; i < nodeCount; ++i) nodes[(size_t) i].energy = 0.0f;
        bank.resetStates();
        buildStrike (note, v);
        snapNext = true;
        lpState = 0.0f;
        outRing.fill (0.0f);
        grainRng.reseed (hashSeed (note.noteId, 0x6A11u));
    }

    computeNextTargets (v, note, 0.0f);
    for (int i = 0; i < nodeCount; ++i)
    {
        if (! legato) renderFreq[(size_t) i] = nodes[(size_t) i].frequency;
        nodes[(size_t) i].cluster = topology.clusterOf (i);
        nodes[(size_t) i].couplingCount = topology.couplingCount (i);
    }
    gate = true;
    everStarted = true;
}

void MatterEngine::noteOff()
{
    gate = false;
}

void MatterEngine::reportSafety (const RenderContext& ctx, SafetyEvent e, int count) noexcept
{
    if (ctx.diagnostics != nullptr && count > 0)
        ctx.diagnostics->safety.note (e, Subsystem::Matter, -1, count);
}

void MatterEngine::conditionExcitation (const float* excL, const float* excR, int n, const ShapeValues& v) noexcept
{
    const auto& P = morpher.profile();
    const float surface = v.surface;

    // Material excitation response: a one-pole tilt (dark ← 0 → bright), heavier MASS is darker.
    const float tilt = std::clamp (P.exciteTilt + (0.4f - v.mass) * 0.6f, -1.0f, 1.0f);
    const float lpCoeff = 1.0f - std::exp (-(float) kTwoPi * 900.0f / (float) sr);
    const float darkMix = tilt < 0.0f ? -tilt : 0.0f;
    const float hpMix   = tilt > 0.0f ? 0.8f * tilt : 0.0f;

    // SURFACE: wavefold of the excitation and interaction with the object's own motion.
    const float foldMix   = surface * 0.35f;
    const float foldDrive = 0.25f * (1.0f + 1.2f * surface);   // phase units for fastSin01 (¼ = 90°): one fold at SURFACE 1
    const float interact  = surface * 0.8f;
    const float grain     = surface * P.grain * 0.06f;

    // Transient extraction. The coefficients come from the sample rate alone, so the split does not
    // depend on the host's block size: a per-block measurement of the same signal varies by 35 dB
    // between a 32- and a 1024-sample buffer, which would make the instrument sound different in
    // every host.
    const auto coef = [this] (float ms) { return 1.0f - std::exp (-1000.0f / (float) (sr * ms)); };
    const float fastAtk = coef (kEnvFastMs),    fastRel = coef (kEnvFastRelMs);
    const float slowAtk = coef (kEnvSlowAtkMs), slowRel = coef (kEnvSlowRelMs);
    float eF = envFast, eS = envSlow;
    float transPeak = 0.0f;

    float lp = lpState;
    for (int t = 0; t < n; ++t)
    {
        float u = 0.5f * (excL[t] + excR[t]);
        const float drive = std::abs (u);
        lp += (u - lp) * lpCoeff;
        u = u + (lp - u) * darkMix - hpMix * lp;

        if (surface > 0.0f)
        {
            const float folded = fastSin01 (u * foldDrive);
            u += (folded - u) * foldMix;
            const float delayed = outRing[(size_t) ((outRingPos + t) % kOutDelay)];
            u *= 1.0f + interact * fastTanh (3.0f * delayed);
            u += grain * grainRng.nextBipolar() * drive;   // excitation-referenced: never a feedback path
        }
        // Fast envelope of the excitation, and the local mean of that envelope. Where the envelope
        // stands above its own mean the signal is rising faster than the material can follow: that
        // excess is the transient.
        const float pw = u * u;
        eF += (pw - eF) * (pw > eF ? fastAtk : fastRel);
        eS += (eF - eS) * (eF > eS ? slowAtk : slowRel);
        const float g = eF > 1.0e-12f ? 1.0f - std::sqrt (std::min (1.0f, eS / eF)) : 0.0f;

        excBuf[(size_t) t] = u + antiDenormal.next();
        const float tr = u * g * kTransientGain;
        transBuf[(size_t) t] = tr;
        transPeak = std::max (transPeak, std::abs (tr));
    }
    lpState = lp;
    envFast = eF; envSlow = eS;
    transientActive = transPeak > 1.0e-6f;
}

void MatterEngine::applyCoupling (const ShapeValues& v, float rMax) noexcept
{
    const auto& P = morpher.profile();
    const float scale = v.coupling * v.coupling * P.couplingScale * kCouplingMax;
    if (scale <= 0.0f)
    {
        if (bank.isCouplingActive()) bank.clearCoupling();
        lastEdgeScale = 0.0f;
        return;
    }
    const float jit = v.surface * 0.6f;
    // The bank's coupling arrays are rebuilt only when something material changed.
    const bool changed = ! bank.isCouplingActive()
                      || std::abs (scale - lastCoupScale) > 0.003f * lastCoupScale
                      || std::abs (rMax - lastCoupRMax) > 2.0e-7f
                      || std::abs (jit - lastCoupJitter) > 0.002f
                      || topologyVersion != lastCoupTopology;
    if (! changed) return;
    lastCoupScale = scale; lastCoupRMax = rMax; lastCoupJitter = jit; lastCoupTopology = topologyVersion;
    const int N = nodeCount;
    for (int i = 0; i < N; ++i)
    {
        const float j = 1.0f + jit * edgeJitter[(size_t) i];
        coupA[(size_t) i]   = topology.bandA()[i] * scale * j;
        coupB[(size_t) i]   = topology.bandB()[i] * scale * (1.0f + jit * edgeJitter[(size_t) (N - 1 - i)]);
        coupHub[(size_t) i] = topology.hubStrength()[i] * scale * j;
    }
    bank.setBandA (topology.strideA(), coupA.data());
    bank.setBandB (topology.strideB(), coupB.data());
    bank.setHub (topology.hub(), coupHub.data());
    const int ne = topology.numExtraEdges();
    for (int e = 0; e < ne; ++e)
    {
        extraScaled[(size_t) e] = topology.extraEdges()[e];
        extraScaled[(size_t) e].k *= scale;
    }
    bank.setExtraEdges (extraScaled.data(), ne);
    lastEdgeScale = scale * bank.finalizeCoupling (rMax);
}

void MatterEngine::process (const float* excL, const float* excR, float* outL, float* outR, int n,
                            const RenderContext& ctx, const NoteState& note)
{
    if (n <= 0) return;
    if (nodeCount <= 0 || ctx.params == nullptr)
    {
        juce::FloatVectorOperations::clear (outL, n);
        juce::FloatVectorOperations::clear (outR, n);
        currentEnergy = 0.0f;
        return;
    }
    n = std::min (n, kMaxBlockSize);
    sr = ctx.sampleRate;
    const auto v = readShape (*ctx.params);
    const int N = nodeCount;

    // ---- 1. Excitation conditioning (material response, surface, strike).
    conditionExcitation (excL, excR, n, v);

    // ---- 2. Node records → resonator coefficients (glide, ramps, safety).
    const float fMin = kMinFrequencyHz;
    const float fMax = kMaxFrequencyRatio * (float) sr;
    const float glideAlpha = 1.0f - std::exp (-(float) n / ((float) sr * 0.001f * kGlideMs * std::exp2 (v.mass * 1.5f)));
    const float norm = morpher.outputNormalisation() * kOutputGain;
    const float strikeNodeGain = kStrikeGain;
    float* cT = bank.targetCos();  float* sT = bank.targetSin();
    float* gL = bank.targetGainL(); float* gR = bank.targetGainR();
    float* aIn = bank.inputGain();  float* bIn = bank.strikeGain();
    int invalidFreq = 0, invalidCoeff = 0;
    float rMax = 0.0f;
    // Coherent strike sum: every node starts in phase at the strike, so the peak of the response grows with the
    // excitation-weighted output sum (~sqrt N for N equal nodes) while the ring RMS does not. The pulse's contact
    // time low-passes what each node receives (H ≈ 1 / (1 + (f·L)²)).
    const bool  strikeActive = strikeLen > strikePos;
    const float pulseSeconds = (float) strikePulseLen / (float) sr;
    float coherentSum = 0.0f;

    for (int i = 0; i < N; ++i)
    {
        Node& nd = nodes[(size_t) i];
        float fT = nd.frequency;
        if (! std::isfinite (fT) || fT <= 0.0f) { fT = nd.targetFrequency; ++invalidFreq; }
        if (! std::isfinite (fT) || fT <= 0.0f) fT = fMin;
        fT = std::min (fMax, std::max (fMin, fT));

        float& fr = renderFreq[(size_t) i];
        if (snapNext || ! std::isfinite (fr)) fr = fT;
        else fr += (fT - fr) * glideAlpha;

        float damping = nd.damping;
        if (! std::isfinite (damping) || damping < 0.0f || damping > 1.0f) { damping = std::clamp (damping, 2.0e-6f, 1.0f); if (! std::isfinite (damping)) damping = 0.01f; ++invalidCoeff; }
        damping = std::max (2.0e-6f, damping);
        const float r = 1.0f - damping;
        const float w = (float) (kTwoPi / sr) * fr;
        float sn, cs;
        fastSinCos (std::min (w, 2.9f), sn, cs);
        cT[i] = r * cs;
        sT[i] = r * sn;

        float weight = nd.weight;
        if (! std::isfinite (weight)) { weight = 0.0f; ++invalidCoeff; }
        weight = std::clamp (weight, 0.0f, 1.5f);
        const bool active = nd.active && weight > 0.0f;
        const float g = active ? weight * norm : 0.0f;
        outGain[(size_t) i] = g;
        float pan = std::isfinite (nd.pan) ? std::clamp (nd.pan, -1.0f, 1.0f) : 0.0f;
        float ps, pc;
        fastSinCos (0.78539816f * (1.0f + pan), ps, pc);
        gL[i] = g * pc;
        gR[i] = g * ps;

        float exc = std::isfinite (nd.excitation) ? std::clamp (nd.excitation, 0.0f, 2.0f) : 0.0f;
        const float dampNorm = std::sqrt (damping) * std::sqrt (std::sqrt (damping));   // damping^0.75
        aIn[i] = active ? exc * kSourceGain * dampNorm : 0.0f;
        bIn[i] = active ? exc * strikeNodeGain : 0.0f;
        if (active) rMax = std::max (rMax, r);
        if (active && strikeActive)
        {
            const float fl = fr * pulseSeconds;
            coherentSum += exc * g / (1.0f + fl * fl);
        }
    }
    if (strikeActive)
    {
        const float target = std::pow (kStrikeRef / std::max (0.05f, coherentSum), kStrikeNormPower);
        strikeNorm = std::clamp (target, 0.2f, 1.5f);
    }

    // The impulse input carries the strike pulse and the extracted transient: both are impulses,
    // and only the pulse is coherence-normalised.
    for (int t = 0; t < n; ++t)
    {
        const int p = strikePos + t;
        const float pulse = p < strikeLen ? strikeBuf[(size_t) p] : 0.0f;
        strikeSig[(size_t) t] = pulse * strikeNorm + transBuf[(size_t) t];
    }
    reportSafety (ctx, SafetyEvent::InvalidFrequency, invalidFreq);
    reportSafety (ctx, SafetyEvent::InvalidCoefficient, invalidCoeff);

    // ---- 3. Coupling (scaled per block, provably contractive).
    applyCoupling (v, rMax);

    if (snapNext) { bank.snapToTargets(); snapNext = false; }

    // ---- 4. Render.
    bank.process (excBuf.data(), (strikeLen > strikePos || transientActive) ? strikeSig.data() : nullptr, outL, outR, n);
    strikePos = std::min (strikePos + n, strikeLen);

    // ---- 5. Safety and energies.
    const float probe = bank.stateEnergy();
    if (! std::isfinite (probe))
    {
        const int bad = bank.scrubNonFinite();
        reportSafety (ctx, SafetyEvent::ResonatorReset, std::max (1, bad));
        const int scrubbed = scrubBuffer (outL, n) + scrubBuffer (outR, n);
        reportSafety (ctx, SafetyEvent::NaN, scrubbed);
    }
    else if (probe > kRunawayLimit * kRunawayLimit)
    {
        reportSafety (ctx, SafetyEvent::FeedbackClamp, bank.clampRunaway (kRunawayLimit));
    }
    bank.flushQuiet (1.0e-12f);

    bank.amplitudes (amps.data());
    float e2 = 0.0f;
    for (int i = 0; i < N; ++i)
    {
        const float a = amps[(size_t) i] * outGain[(size_t) i];
        nodes[(size_t) i].energy = a;
        e2 += a * a;
    }
    currentEnergy = std::sqrt (e2);

    // Delayed mono output for the surface interaction on the next block.
    for (int t = 0; t < n; ++t)
    {
        outRing[(size_t) outRingPos] = 0.5f * (outL[t] + outR[t]);
        outRingPos = (outRingPos + 1) % kOutDelay;
    }

    // ---- 6. Material baseline for the next block (Evolve may modify it before then).
    rebuildIfNeeded (v, N);
    computeNextTargets (v, note, (float) n / (float) sr);
    for (int i = 0; i < N; ++i)
    {
        Node& nd = nodes[(size_t) i];
        nd.active = nd.active && nd.weight > 0.0f;
    }
}

int MatterEngine::activeNodes() const noexcept
{
    int a = 0;
    for (int i = 0; i < nodeCount; ++i)
        if (nodes[(size_t) i].active && (nodes[(size_t) i].energy > 1.0e-5f || gate)) ++a;
    return a;
}

int MatterEngine::fillDiagnostics (NodeDiag* dest, int maxNodes) const noexcept
{
    const int count = std::min (nodeCount, maxNodes);
    for (int i = 0; i < count; ++i)
    {
        const auto& s = nodes[(size_t) i];
        auto& d = dest[i];
        d.frequency = renderFreq[(size_t) i]; d.targetFrequency = s.targetFrequency;
        d.energy = s.energy; d.weight = s.weight; d.damping = s.damping;
        d.pan = s.pan; d.nonlinearity = s.nonlinearity; d.excitation = s.excitation;
        d.cluster = s.cluster; d.couplingCount = s.couplingCount; d.active = s.active ? 1 : 0;
    }
    return count;
}

int MatterEngine::fillEdgeDiagnostics (EdgeDiag* dest, int maxEdges) const noexcept
{
    const int count = std::min (topology.numEdges(), maxEdges);
    const float scale = lastEdgeScale;
    for (int i = 0; i < count; ++i)
    {
        const auto& e = topology.edge (i);
        dest[i].from = e.from; dest[i].to = e.to; dest[i].type = (uint8_t) e.type;
        dest[i].strength = e.base * scale;
    }
    return count;
}

void MatterEngine::couplingStats (float& average, float& maximum) const noexcept
{
    average = 0.0f; maximum = 0.0f;
    const int count = topology.numEdges();
    if (count == 0 || lastEdgeScale <= 0.0f) return;
    float sum = 0.0f;
    for (int i = 0; i < count; ++i)
    {
        const float s = topology.edge (i).base * lastEdgeScale;
        sum += s; maximum = std::max (maximum, s);
    }
    average = sum / (float) count;
}

} // namespace am

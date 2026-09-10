#include "EvolveEngine.h"
#include "core/FastMath.h"
#include "dev/diagnostics/Diagnostics.h"

namespace am
{

namespace
{
    // ---- bounds written into the nodes
    constexpr float kMinHz           = 20.0f;
    constexpr float kMinDamping      = 2.0e-6f;
    constexpr float kMaxDamping      = 1.0f;
    constexpr float kMaxWeight       = 1.5f;
    constexpr float kMaxExcitation   = 2.0f;
    constexpr float kMaxNonlinearity = 2.0f;

    // ---- operator laws
    constexpr float kHeightOctaves    = 4.0f;    ///< spectral height h = log2 (f / f0) / 4, saturating 4 octaves above the fundamental
    constexpr float kBendMaxOctaves   = 1.0f;    ///< ±1 octave at bendRange 1 for the node farthest from the pivot
    constexpr float kBendMotionDepth  = 0.3f;    ///< MOTION 1 breathes the bend by ±30 %
    constexpr float kMeltDropOctaves  = 0.5f;    ///< the top partial sags half an octave at MELT 1
    constexpr float kMeltRootOctaves  = 0.0f;    ///< the fundamental stays on pitch: a detuned root falls off the source's resonance
    constexpr float kMeltDampingGain  = 4.0f;    ///< extra damping factor at the top: d · (1 + a · (1 + 4h))
    constexpr float kMeltWeightFade   = 0.85f;   ///< the top partial keeps 15 % of its weight at MELT 1
    constexpr float kTearTwinOctaves  = 0.015f;  ///< twin detune (18 cents ≈ 2.7 Hz beating at C4) at TEAR 1
    constexpr float kTearPairFraction = 0.45f;   ///< fraction of the active nodes that become twins at TEAR 1
    constexpr float kTearClusterOct   = 0.04f;   ///< cluster pull-apart (±48 cents) at TEAR 1
    constexpr float kTearNonlinearity = 0.5f;
    constexpr float kGravitySinkBoost = 0.5f;    ///< the fundamental gains 3 dB when everything sinks
    constexpr float kGravitySinkCut   = 3.0f;    ///< the top loses 18 dB when everything sinks
    constexpr float kGravityLiftBoost = 1.33f;   ///< the top gains 8 dB when everything lifts
    constexpr float kGravityLiftCut   = 1.33f;   ///< the fundamental loses 8 dB when everything lifts
    constexpr float kGravityDampTilt  = 1.4f;    ///< the side gravity pushes away from decays up to 2.6x faster
    constexpr float kGravityDampFloor = 0.1f;    ///< … the favoured side still loses a little ring time
    constexpr float kGravityNormLimit = 8.0f;    ///< the level compensation never moves more than 18 dB
    constexpr float kScatterOctaves   = 0.25f;   ///< ±3 semitones for the partials at SCATTER 1
    constexpr float kScatterRootOct   = 0.025f;  ///< ±30 cents for the fundamental at SCATTER 1 (quadratic law)
    constexpr float kScatterJitterOct = 0.08f;   ///< animated jitter at MOTION 1 · SCATTER 1
    constexpr float kFreezeT60        = 120.0f;  ///< seconds asked for; the damping floor (2e-6) makes it ~72 s at 48 kHz
    constexpr float kFreezeExcitation = 0.35f;   ///< new energy entering a frozen object
    constexpr float kCrushGridOctaves = 0.25f;   ///< minor-third grid at CRUSH 1
    constexpr float kCrushDropFraction = 0.25f;  ///< nodes under this fraction of the loudest weight are silenced at CRUSH 1
    constexpr float kCrushLevels      = 4.0f;    ///< static weight levels and envelope levels (relative to the loudest) at CRUSH 1
    constexpr float kCrushHoldMin     = 0.010f;  ///< envelope hold time in seconds at CRUSH → 0 …
    constexpr float kCrushHoldRange   = 0.150f;  ///< … plus this at CRUSH 1
    constexpr float kCrushMaxBoost    = 4.0f;    ///< a held level never lifts a decaying node by more than 12 dB
    constexpr float kCrushHeadroom    = 0.4f;    ///< static weight reduction at CRUSH 1 so the hold can lift under Matter's 1.5 weight clamp
    constexpr float kCrushSmoothSec   = 0.008f;  ///< the hold gain moves over ~8 ms: level steps, not clicks
    constexpr float kCrushRefDbPerSec = 30.0f;   ///< the level reference falls from the strike peak toward the loudest ringing node
    constexpr float kCrushCutWeight   = 1.0e-3f; ///< a cut node keeps this fraction of its weight so it stays excitable
    constexpr float kCrushNonlinearity = 0.6f;
    constexpr float kMagnetLockCents  = 5.0f;

    constexpr float kLog2e = 1.44269504f;

    /** log2 for positive finite x (max error ~2e-6). Exponent from the bits, mantissa by an atanh series. */
    inline float fastLog2 (float x) noexcept
    {
        x = std::max (x, 1.0e-30f);
        uint32_t bits;
        std::memcpy (&bits, &x, sizeof (bits));
        const int e = (int) ((bits >> 23) & 0xFFu) - 127;
        bits = (bits & 0x007FFFFFu) | 0x3F800000u;
        float m;
        std::memcpy (&m, &bits, sizeof (m));                   // 1 <= m < 2
        const float t  = (m - 1.0f) / (m + 1.0f);              // 0 <= t < 1/3
        const float t2 = t * t;
        const float ln = t * (2.0f + t2 * (0.66666667f + t2 * (0.4f + t2 * (0.28571429f + t2 * 0.22222222f))));
        return (float) e + ln * kLog2e;
    }

    inline float pow2 (float x) noexcept { return MaterialMorpher::pow2 (x); }

    inline float smoothstep01 (float t) noexcept
    {
        t = clamp01 (t);
        return t * t * (3.0f - 2.0f * t);
    }

    inline float sane01 (float v) noexcept { return std::isfinite (v) ? clamp01 (v) : 0.0f; }

    /** Octave-repeating MAGNET grids as log2 fractions, ascending, first entry 0. */
    struct OctaveGrid { int count; float steps[12]; };

    constexpr float kSemi = 1.0f / 12.0f;
    constexpr float kFifthLog2 = 0.5849625f;   // log2 (3/2)

    const OctaveGrid kGrids[] =
    {
        { 1,  { 0.0f } },                                                                 // OCTAVE
        { 2,  { 0.0f, kFifthLog2 } },                                                     // FIFTH (pure)
        { 3,  { 0.0f, 4 * kSemi, 7 * kSemi } },                                           // MAJOR triad
        { 3,  { 0.0f, 3 * kSemi, 7 * kSemi } },                                           // MINOR triad
        { 12, { 0.0f, 1 * kSemi, 2 * kSemi, 3 * kSemi, 4 * kSemi, 5 * kSemi, 6 * kSemi,
                7 * kSemi, 8 * kSemi, 9 * kSemi, 10 * kSemi, 11 * kSemi } },              // CHROMATIC
        { 5,  { 0.0f, 2 * kSemi, 4 * kSemi, 7 * kSemi, 9 * kSemi } },                     // SCALE (major pentatonic)
    };
    constexpr int kNumOctaveGrids = 6;
}

//==============================================================================
float EvolveEngine::motionRateHz (float speed) noexcept
{
    return 0.02f * std::pow (400.0f, sane01 (speed));      // 0.02 Hz … 8 Hz
}

float EvolveEngine::magnetGridLog2 (int target, float lr) noexcept
{
    if (! std::isfinite (lr)) return 0.0f;
    if (target < 0 || target >= kNumOctaveGrids)
    {
        // CUSTOM: the material's own ratio rounded to a simple integer — a harmonic series
        // above the fundamental, sub-harmonics (1/n) below it.
        const float r = pow2 (lr);
        if (r >= 1.0f) return fastLog2 (std::max (1.0f, std::round (r)));
        return -fastLog2 (std::max (1.0f, std::round (1.0f / r)));
    }
    const auto& g = kGrids[target];
    const float base = std::floor (lr);
    const float frac = lr - base;
    float best = 0.0f, bestDist = std::abs (frac - 0.0f);
    for (int i = 1; i < g.count; ++i)
    {
        const float d = std::abs (frac - g.steps[i]);
        if (d < bestDist) { bestDist = d; best = g.steps[i]; }
    }
    if (std::abs (frac - 1.0f) < bestDist) best = 1.0f;     // the next octave's root
    return base + best;
}

float EvolveEngine::crushGridOctaves (float amount) noexcept
{
    return kCrushGridOctaves * sane01 (amount);
}

float EvolveEngine::freezeDamping (double sampleRate) noexcept
{
    const float d = 6.9077553f / (kFreezeT60 * (float) std::max (1.0, sampleRate));
    return std::clamp (d, kMinDamping, kMaxDamping);
}

//==============================================================================
void EvolveEngine::prepare (double sampleRate, int)
{
    sr = sampleRate > 1.0 ? sampleRate : 48000.0;
    reset();
}

void EvolveEngine::reset()
{
    phase = 0.0;
    rateHz = 0.0f;
    frozen = false;
    frozenCount = 0;
    scatterSeedUsed = 0xFFFFFFFFu;
    crushArmed = false;
    lastWrittenWeight.fill (0.0f);
    diag = EvolveDiag();
    buildMotionTables (noteId);
}

void EvolveEngine::noteOn (const NoteState& note, const ParamValues& params)
{
    noteId = note.noteId;
    frozen = false;
    frozenCount = 0;
    crushArmed = false;
    lastWrittenWeight.fill (0.0f);
    buildMotionTables (noteId);
    const int seed = (int) std::lround (std::isfinite (paramValue (params, Param::evolveScatterSeed)) ? paramValue (params, Param::evolveScatterSeed) : 0.0f);
    buildScatterTables ((uint32_t) std::max (0, seed));
}

void EvolveEngine::buildScatterTables (uint32_t seed) noexcept
{
    scatterSeedUsed = seed;
    Rng rng (hashSeed (seed + 0x5CA77Eu, noteId));
    for (int i = 0; i < kMaxMatterNodes; ++i)
    {
        scatterFreq[(size_t) i]   = rng.nextBipolar();
        scatterWeight[(size_t) i] = rng.nextBipolar();
        scatterPan[(size_t) i]    = rng.nextBipolar();
    }
}

void EvolveEngine::buildMotionTables (uint32_t seed) noexcept
{
    Rng rng (hashSeed (seed, 0x0FF5E7u));
    for (int i = 0; i < kMaxMatterNodes; ++i)
    {
        motionOffsetA[(size_t) i] = rng.nextFloat();
        motionOffsetB[(size_t) i] = rng.nextFloat();
    }
}

EvolveEngine::Amounts EvolveEngine::readAmounts (const RenderContext& ctx) noexcept
{
    Amounts a;
    if (ctx.params == nullptr) return a;
    a.mask = ctx.diagnostics != nullptr ? ctx.diagnostics->dev.evolveBypassMask.load (std::memory_order_relaxed) : 0u;
    const auto amount = [&] (Param p, EvolveOperator op) noexcept
    {
        return (a.mask & evolveBypassBit (op)) != 0u ? 0.0f : sane01 (ctx.param (p));
    };
    a.bend    = amount (Param::evolveBend,    EvolveOperator::Bend);
    a.melt    = amount (Param::evolveMelt,    EvolveOperator::Melt);
    a.tear    = amount (Param::evolveTear,    EvolveOperator::Tear);
    a.magnet  = amount (Param::evolveMagnet,  EvolveOperator::Magnet);
    a.scatter = amount (Param::evolveScatter, EvolveOperator::Scatter);
    a.crush   = amount (Param::evolveCrush,   EvolveOperator::Crush);
    const float g = (a.mask & evolveBypassBit (EvolveOperator::Gravity)) != 0u ? 0.5f : sane01 (ctx.param (Param::evolveGravity));
    a.gravity = (g - 0.5f) * 2.0f;
    if (std::abs (a.gravity) < 1.0e-4f) a.gravity = 0.0f;          // 0.5 is exactly neutral
    a.freeze  = (a.mask & evolveBypassBit (EvolveOperator::Freeze)) == 0u && ctx.flag (Param::evolveFreeze);
    a.speed   = sane01 (ctx.param (Param::evolveSpeed));
    a.motion  = sane01 (ctx.param (Param::evolveMotion));
    a.bendPivot = sane01 (ctx.param (Param::evolveBendPivot));
    a.bendRange = sane01 (ctx.param (Param::evolveBendRange));
    a.bendCurve = sane01 (ctx.param (Param::evolveBendCurve));
    a.magnetTarget = std::clamp (ctx.choice (Param::evolveMagnetTarget), 0, 6);
    const float seed = ctx.param (Param::evolveScatterSeed);
    a.scatterSeed = (uint32_t) std::max (0, (int) std::lround (std::isfinite (seed) ? seed : 0.0f));
    return a;
}

//==============================================================================
void EvolveEngine::apply (MatterEngine& matter, const RenderContext& ctx, const NoteState& note)
{
    if (ctx.sampleRate > 1.0) sr = ctx.sampleRate;
    const Amounts a = readAmounts (ctx);
    const int N = std::min (matter.numNodes(), kMaxMatterNodes);
    const int n = std::max (0, ctx.numSamples);

    // ---- motion clock (stops while frozen)
    rateHz = motionRateHz (a.speed);
    if (! a.freeze)
    {
        phase += (double) rateHz * (double) n / sr;
        phase -= std::floor (phase);
        if (! std::isfinite (phase)) phase = 0.0;
    }

    // ---- diagnostics header (the rest is filled below)
    diag = EvolveDiag();
    diag.amount[(int) EvolveOperator::Bend]    = a.bend;
    diag.amount[(int) EvolveOperator::Melt]    = a.melt;
    diag.amount[(int) EvolveOperator::Tear]    = a.tear;
    diag.amount[(int) EvolveOperator::Magnet]  = a.magnet;
    diag.amount[(int) EvolveOperator::Gravity] = a.gravity;
    diag.amount[(int) EvolveOperator::Scatter] = a.scatter;
    diag.amount[(int) EvolveOperator::Freeze]  = a.freeze ? 1.0f : 0.0f;
    diag.amount[(int) EvolveOperator::Crush]   = a.crush;
    diag.freeze = a.freeze ? 1 : 0;
    diag.bypassMask = (uint8_t) (a.mask & 0xFFu);
    diag.motionPhase = (float) phase;
    diag.motionRateHz = rateHz;

    const bool anyOperator = a.bend > 0.0f || a.melt > 0.0f || a.tear > 0.0f || a.magnet > 0.0f
                          || a.gravity != 0.0f || a.scatter > 0.0f || a.crush > 0.0f || a.freeze;
    if (! anyOperator || N <= 0)
    {
        frozen = false;
        crushArmed = false;
        for (int i = 0; i < N; ++i) lastWrittenWeight[(size_t) i] = matter.node (i).weight;
        return;                                                   // nothing written: nodes stay bit-identical
    }

    if (a.scatter > 0.0f && a.scatterSeed != scatterSeedUsed)
        buildScatterTables (a.scatterSeed);

    // ---- material fundamental and per-node log ratios / spectral heights
    const int modalCount = std::max (1, N - MaterialMorpher::kBodyNodes);
    float f0 = 0.0f;
    {
        const auto& root = matter.node (0);
        if (std::isfinite (root.targetFrequency) && std::isfinite (root.ratio) && root.ratio > 1.0e-4f && root.targetFrequency > 0.0f)
            f0 = root.targetFrequency / root.ratio;
        if (! std::isfinite (f0) || f0 < kMinHz)
            f0 = (float) std::max ((double) kMinHz, note.frequency);
    }
    diag.fundamentalHz = f0;

    const float invF0 = 1.0f / f0;
    int numActive = 0;
    float wMax = 0.0f;
    for (int i = 0; i < N; ++i)
    {
        const auto& nd = matter.node (i);
        const bool ok = nd.active && nd.weight > 0.0f && std::isfinite (nd.targetFrequency) && nd.targetFrequency > 0.0f
                     && std::isfinite (nd.weight) && std::isfinite (nd.damping) && std::isfinite (nd.pan)
                     && std::isfinite (nd.excitation) && std::isfinite (nd.nonlinearity);
        considered[(size_t) i] = ok;
        if (! ok) continue;
        const float lr = fastLog2 (nd.targetFrequency * invF0);
        logRatio[(size_t) i] = lr;
        height[(size_t) i] = clamp01 (lr * (1.0f / kHeightOctaves));
        weight[(size_t) i] = nd.weight;
        damping[(size_t) i] = nd.damping;
        pan[(size_t) i] = nd.pan;
        excitation[(size_t) i] = nd.excitation;
        nonlinearity[(size_t) i] = nd.nonlinearity;
        shiftOct[(size_t) i] = 0.0f;
        wMax = std::max (wMax, nd.weight);
        ++numActive;
    }
    diag.activeNodes = numActive;
    if (numActive == 0)
    {
        frozen = false;
        crushArmed = false;
        for (int i = 0; i < N; ++i) lastWrittenWeight[(size_t) i] = matter.node (i).weight;
        return;
    }


    // ---- per-node motion LFO (only when something uses it)
    const bool useMotion = a.motion > 0.0f && (a.bend > 0.0f || a.melt > 0.0f || a.tear > 0.0f || a.scatter > 0.0f);
    const float motion = useMotion ? a.motion : 0.0f;
    if (useMotion)
    {
        const float ph = (float) phase;
        for (int i = 0; i < N; ++i)
        {
            if (! considered[(size_t) i]) continue;
            lfo[(size_t) i] = 0.65f * fastSin01 (ph + motionOffsetA[(size_t) i]) + 0.35f * fastSin01 (2.0f * ph + motionOffsetB[(size_t) i]);
        }
    }
    else
    {
        for (int i = 0; i < N; ++i) lfo[(size_t) i] = 0.0f;
    }

    // Frozen objects keep the captured spectrum: nothing below is re-evaluated.
    const bool useCapture = a.freeze && frozen && frozenCount == N;

    if (! useCapture)
    {
        // ---- BEND: a lever around the pivot. Above it partials rise, below it they sink.
        if (a.bend > 0.0f)
        {
            const float pivot = a.bendPivot;
            const float xMax  = std::max (pivot, 1.0f - pivot);
            const float gamma = 1.0f + 3.0f * a.bendCurve;             // 1 = linear … 4 = exponential distribution
            const float full  = a.bend * a.bendRange * kBendMaxOctaves;
            for (int i = 0; i < N; ++i)
            {
                if (! considered[(size_t) i]) continue;
                const float x = (height[(size_t) i] - pivot) / xMax;
                const float ax = std::abs (x);
                const float g = ax > 1.0e-6f ? pow2 (gamma * fastLog2 (ax)) : 0.0f;
                shiftOct[(size_t) i] += full * (x < 0.0f ? -g : g) * (1.0f + kBendMotionDepth * motion * lfo[(size_t) i]);
            }
        }

        // ---- MELT: the object loses rigidity — partials sag with rank, damp faster, the top fades.
        if (a.melt > 0.0f)
        {
            const float m = a.melt;
            for (int i = 0; i < N; ++i)
            {
                if (! considered[(size_t) i]) continue;
                const float h = height[(size_t) i];
                const float sag = m * (kMeltDropOctaves * h + kMeltRootOctaves * m);
                shiftOct[(size_t) i] -= sag * (1.0f + 0.4f * motion * lfo[(size_t) i]);
                shiftOct[(size_t) i] += motion * m * 0.05f * lfo[(size_t) i] * (0.3f + 0.7f * h);
                damping[(size_t) i] *= 1.0f + m * (1.0f + kMeltDampingGain * h);
                weight[(size_t) i]  *= 1.0f - m * kMeltWeightFade * h;
                excitation[(size_t) i] *= 1.0f - 0.5f * m * h;
            }
        }

        // ---- TEAR: detuned twins (beating pairs) and clusters pulled apart.
        if (a.tear > 0.0f)
        {
            const float t = a.tear;
            int na = 0;
            for (int i = 0; i < modalCount; ++i)
                if (considered[(size_t) i]) activeIndex[(size_t) na++] = (uint8_t) i;

            const float pairsF = t * kTearPairFraction * (float) na;
            const int pairs = std::min (na / 2, (int) std::ceil (pairsF));
            int made = 0;
            for (int k = 0; k < pairs; ++k)
            {
                const int L = activeIndex[(size_t) k];
                const int T = activeIndex[(size_t) (na - 1 - k)];
                if (T <= L) break;
                const float frac = std::min (1.0f, pairsF - (float) k);   // the last pair fades in
                const float dir  = (k & 1) != 0 ? -1.0f : 1.0f;
                const float delta = t * kTearTwinOctaves * (1.0f + 0.5f * motion * lfo[(size_t) L]) * frac;
                const float split = 0.5f * t * frac;                       // weight handed to the twin

                // The original keeps its pitch (a sustained source still finds its resonance); the twin sits `delta` away.
                const float twinOct = logRatio[(size_t) L] + shiftOct[(size_t) L] + dir * delta;
                shiftOct[(size_t) T] += frac * (twinOct - (logRatio[(size_t) T] + shiftOct[(size_t) T]));

                const float wL = weight[(size_t) L];                       // power-conserving split: energy stays
                weight[(size_t) T] = weight[(size_t) T] * (1.0f - frac) + wL * std::sqrt (split);
                weight[(size_t) L] = wL * std::sqrt (1.0f - split);
                damping[(size_t) T] = damping[(size_t) T] + frac * (damping[(size_t) L] * pow2 (dir * 0.4f * t) - damping[(size_t) T]);
                excitation[(size_t) T] = excitation[(size_t) T] + frac * (excitation[(size_t) L] - excitation[(size_t) T]);
                const float pL = pan[(size_t) L];
                pan[(size_t) L] = std::clamp (pL + dir * 0.3f * t * frac, -1.0f, 1.0f);
                pan[(size_t) T] = std::clamp (pan[(size_t) T] + frac * (-pL - dir * 0.5f * t - pan[(size_t) T]), -1.0f, 1.0f);
                nonlinearity[(size_t) L] += kTearNonlinearity * t * frac;
                nonlinearity[(size_t) T] += kTearNonlinearity * t * frac;
                ++made;
            }
            diag.tearPairs = made;

            // The nodes in between: whole clusters drift apart in pitch, stereo and decay.
            const int clusters = std::max (1, matter.clusterCount());
            const float clusterScale = clusters > 1 ? 2.0f / (float) (clusters - 1) : 0.0f;
            for (int k = made; k < na - made; ++k)
            {
                const int i = activeIndex[(size_t) k];
                const float sc = clusters > 1 ? (float) matter.node (i).cluster * clusterScale - 1.0f : 0.0f;
                shiftOct[(size_t) i] += t * kTearClusterOct * sc * (1.0f + 0.5f * motion * lfo[(size_t) i]);
                pan[(size_t) i] = std::clamp (pan[(size_t) i] + t * 0.6f * sc, -1.0f, 1.0f);
                damping[(size_t) i] *= pow2 (t * 0.8f * sc);
                nonlinearity[(size_t) i] += kTearNonlinearity * 0.8f * t;
            }
        }

        // ---- SCATTER: seeded, deterministic per-node offsets; MOTION makes them jitter.
        if (a.scatter > 0.0f)
        {
            const float s = a.scatter;
            const float rootOct = kScatterRootOct * s * s;
            const float partOct = kScatterOctaves * s;
            for (int i = 0; i < N; ++i)
            {
                if (! considered[(size_t) i]) continue;
                const float h = height[(size_t) i];
                const float reach = rootOct + (partOct - rootOct) * std::min (1.0f, h * 4.0f);   // the fundamental scatters less
                shiftOct[(size_t) i] += reach * scatterFreq[(size_t) i]
                                      + motion * s * kScatterJitterOct * lfo[(size_t) i] * (0.3f + 0.7f * h);
                const float rw = scatterWeight[(size_t) i];
                weight[(size_t) i] *= pow2 (s * (rw > 0.0f ? 0.5f * rw : 2.0f * rw) + motion * s * 0.6f * lfo[(size_t) i]);
                pan[(size_t) i] = std::clamp (pan[(size_t) i] + s * 0.8f * scatterPan[(size_t) i], -1.0f, 1.0f);
            }
        }

        // ---- GRAVITY: spectral weight pull. Sink (> 0.5) or lift (< 0.5).
        //      The operator tilts the spectrum; it is not a volume control, so the drive of the
        //      node set is measured before and after and the weights are compensated back towards
        //      it. The compensation is bounded so it can never invert the pull on the fundamental
        //      (louder while lifting, quieter while sinking): gravity always reads as a direction.
        if (a.gravity != 0.0f)
        {
            const float g = a.gravity;
            const float u = -g;
            double before = 0.0, after = 0.0;
            float rootFactor = 1.0f;               ///< what the pull did to the fundamental's weight
            for (int i = 0; i < N; ++i)
            {
                if (! considered[(size_t) i]) continue;
                const float h = height[(size_t) i];
                const double d0 = (double) weight[(size_t) i] * (double) excitation[(size_t) i];
                const float wBefore = weight[(size_t) i];
                if (g > 0.0f)
                {
                    weight[(size_t) i]     *= pow2 (g * (kGravitySinkBoost * (1.0f - h) - kGravitySinkCut * h));
                    excitation[(size_t) i] *= pow2 (g * (0.3f * (1.0f - h) - 1.5f * h));
                    // Sinking shortens the top and leaves the bottom ringing.
                    damping[(size_t) i]    *= pow2 (g * (kGravityDampTilt * h + kGravityDampFloor * (1.0f - h)));
                }
                else
                {
                    weight[(size_t) i]     *= pow2 (u * (kGravityLiftBoost * h - kGravityLiftCut * (1.0f - h)));
                    excitation[(size_t) i] *= pow2 (u * (0.6f * h - 1.0f * (1.0f - h)));
                    // Lifting is the mirror image: the bottom dies away and the top keeps singing.
                    damping[(size_t) i]    *= pow2 (u * (kGravityDampTilt * (1.0f - h) + kGravityDampFloor * h));
                }
                const double d1 = (double) weight[(size_t) i] * (double) excitation[(size_t) i];
                before += d0 * d0;
                after  += d1 * d1;
                if (i == 0 && wBefore > 0.0f) rootFactor = weight[0] / wBefore;
            }

            float norm = 1.0f;
            if (after > 1.0e-20 && before > 1.0e-20)
                norm = std::clamp ((float) std::sqrt (before / after), 1.0f / kGravityNormLimit, kGravityNormLimit);
            // Never lift the fundamental above its baseline while the object sinks toward it, and
            // never push it below while everything is rising away from it.
            if (rootFactor > 1.0e-6f)
                norm = g > 0.0f ? std::max (norm, 1.0f / rootFactor)
                                : std::min (norm, 0.98f / rootFactor);
            if (std::isfinite (norm) && norm != 1.0f)
                for (int i = 0; i < N; ++i)
                    if (considered[(size_t) i]) weight[(size_t) i] *= norm;
        }

        // ---- MAGNET: attraction toward a musical grid relative to the fundamental.
        if (a.magnet > 0.0f)
        {
            int locked = 0;
            for (int i = 0; i < N; ++i)
            {
                if (! considered[(size_t) i]) continue;
                const float cur = logRatio[(size_t) i] + shiftOct[(size_t) i];
                const float target = magnetGridLog2 (a.magnetTarget, cur);
                shiftOct[(size_t) i] += a.magnet * (target - cur);
                if (std::abs (target - (logRatio[(size_t) i] + shiftOct[(size_t) i])) * 1200.0f < kMagnetLockCents) ++locked;
            }
            diag.magnetLocked = locked;
        }

        // ---- CRUSH: coarse frequency grid, few weight levels, the quietest nodes go silent.
        if (a.crush > 0.0f)
        {
            const float c = a.crush;
            const float step = crushGridOctaves (c);
            const float invStep = 1.0f / step;
            const float threshold = kCrushDropFraction * c * wMax;
            const float levelScale = wMax > 0.0f ? kCrushLevels / wMax : 0.0f;
            int dropped = 0;
            for (int i = 0; i < N; ++i)
            {
                if (! considered[(size_t) i]) continue;
                const float cur = logRatio[(size_t) i] + shiftOct[(size_t) i];
                shiftOct[(size_t) i] += std::round (cur * invStep) * step - cur;
                float w = weight[(size_t) i];
                const float keep = threshold > 0.0f ? smoothstep01 ((w - threshold) / threshold) : 1.0f;
                w *= keep;
                const float quantised = std::round (w * levelScale) / std::max (1.0e-9f, levelScale);
                w += c * (quantised - w);
                w *= 1.0f - kCrushHeadroom * c;
                weight[(size_t) i] = w;
                nonlinearity[(size_t) i] += kCrushNonlinearity * c;
            }

            // Digital decay: the level of every ringing node is sampled every hold period and held at one of a few
            // levels relative to the loudest node, so the object decays in steps and the quietest nodes are cut.
            // The node amplitude is recovered from the energy Matter measured with the weight written last block.
            if (! crushArmed)
            {
                crushArmed = true;
                crushPeak = 0.0f;
                crushClock = 1.0e9;
                crushHold.fill (0.0f);
                crushAmp.fill (0.0f);
                crushEnergySeen.fill (-1.0f);
                crushGain.fill (1.0f);
                crushState.fill (0);
            }
            const float holdSeconds = kCrushHoldMin + kCrushHoldRange * c;
            crushClock += (double) n / sr;
            const bool tick = crushClock >= (double) holdSeconds;
            if (tick) crushClock = 0.0;
            const float levels = std::ceil (kCrushLevels / c);
            const float smooth = 1.0f - pow2 (-(float) n / ((float) sr * kCrushSmoothSec * 1.4427f));
            const float peakCap = crushPeak * kCrushMaxBoost;          // the reference never jumps more than 12 dB in a block
            crushPeak *= pow2 (-(float) n / (float) sr * kCrushRefDbPerSec / 6.0206f);
            for (int i = 0; i < N; ++i)
            {
                if (! considered[(size_t) i]) continue;
                // The modal amplitude (times Matter's normalisation) is the energy Matter measured divided by the
                // weight it rendered with — the weight written last block. When the energy has not changed no block
                // was rendered in between (bypass, dry modes, tests), and the previous amplitude stays valid.
                const float energy = matter.node (i).energy;
                float amp = crushAmp[(size_t) i];
                if (energy != crushEnergySeen[(size_t) i])
                {
                    const float wPrev = lastWrittenWeight[(size_t) i];
                    amp = (wPrev > 1.0e-7f && std::isfinite (energy) && energy > 0.0f) ? energy / wPrev : 0.0f;
                    crushAmp[(size_t) i] = amp;
                    crushEnergySeen[(size_t) i] = energy;
                }
                crushPeak = peakCap > 0.0f ? std::min (peakCap, std::max (crushPeak, amp)) : std::max (crushPeak, amp);
                uint8_t& state = crushState[(size_t) i];
                if (tick && crushPeak > 0.0f)
                {
                    const float q = std::round (amp / crushPeak * levels) / levels;
                    if (q <= 0.0f) { state = 2; crushHold[(size_t) i] = 0.0f; }
                    else           { state = 1; crushHold[(size_t) i] = q * crushPeak; }
                }
                float target = 1.0f;
                if (state == 2)                       target = kCrushCutWeight;
                else if (state == 1 && amp > 1.0e-9f) target = std::min (kCrushMaxBoost, crushHold[(size_t) i] / amp);
                float& gain = crushGain[(size_t) i];
                gain += (target - gain) * smooth;
                weight[(size_t) i] *= gain;
                if (state == 2 || weight[(size_t) i] <= 0.0f) ++dropped;
            }
            diag.crushDropped = dropped;
        }
        else
        {
            crushArmed = false;
        }
    }

    // ---- FREEZE: capture once, then hold; ring forever; let little new energy in.
    if (a.freeze)
    {
        const float dFreeze = freezeDamping (sr);
        if (! useCapture)
        {
            for (int i = 0; i < N; ++i)
            {
                if (! considered[(size_t) i]) { frozenWeight[(size_t) i] = -1.0f; continue; }   // marks "not captured"
                frozenFreq[(size_t) i] = matter.node (i).targetFrequency * pow2 (shiftOct[(size_t) i]);
                frozenWeight[(size_t) i] = weight[(size_t) i];
                frozenPan[(size_t) i] = pan[(size_t) i];
                frozenNonlinearity[(size_t) i] = nonlinearity[(size_t) i];
            }
            frozen = true;
            frozenCount = N;
        }
        for (int i = 0; i < N; ++i)
        {
            if (! considered[(size_t) i]) continue;
            damping[(size_t) i] = std::min (damping[(size_t) i], dFreeze);
            excitation[(size_t) i] *= kFreezeExcitation;
        }
    }
    else
    {
        frozen = false;
    }

    // ---- write back with every bound enforced; statistics for DSP LAB
    const float fMax = kMaxFrequencyRatio * (float) sr;
    const float fadeStart = 0.40f * (float) sr;
    int moved = 0;
    float centsSum = 0.0f, centsMax = 0.0f;
    for (int i = 0; i < N; ++i)
    {
        if (! considered[(size_t) i]) continue;
        auto& nd = matter.node (i);

        float f, w, p, nl;
        if (useCapture && frozenWeight[(size_t) i] >= 0.0f)
        {
            f = frozenFreq[(size_t) i]; w = frozenWeight[(size_t) i]; p = frozenPan[(size_t) i]; nl = frozenNonlinearity[(size_t) i];
        }
        else
        {
            f = nd.targetFrequency * pow2 (shiftOct[(size_t) i]);
            w = weight[(size_t) i]; p = pan[(size_t) i]; nl = nonlinearity[(size_t) i];
        }
        if (! std::isfinite (f)) f = nd.targetFrequency;
        f = std::clamp (f, kMinHz, fMax);
        if (f > fadeStart) w *= clamp01 ((fMax - f) / (fMax - fadeStart));   // nothing piles up at the top of the band

        float d = damping[(size_t) i];
        float e = excitation[(size_t) i];
        if (! std::isfinite (w)) w = nd.weight;
        if (! std::isfinite (d)) d = nd.damping;
        if (! std::isfinite (p)) p = nd.pan;
        if (! std::isfinite (e)) e = nd.excitation;
        if (! std::isfinite (nl)) nl = nd.nonlinearity;
        w  = std::clamp (w, 0.0f, kMaxWeight);
        d  = std::clamp (d, kMinDamping, kMaxDamping);
        p  = std::clamp (p, -1.0f, 1.0f);
        e  = std::clamp (e, 0.0f, kMaxExcitation);
        nl = std::clamp (nl, 0.0f, kMaxNonlinearity);

        const float cents = std::abs (fastLog2 (f / nd.targetFrequency)) * 1200.0f;
        centsSum += cents;
        centsMax = std::max (centsMax, cents);
        if (f != nd.targetFrequency || w != nd.weight || d != nd.damping || p != nd.pan) ++moved;

        nd.frequency = f;
        nd.weight = w;
        nd.damping = d;
        nd.pan = p;
        nd.excitation = e;
        nd.nonlinearity = nl;
        lastWrittenWeight[(size_t) i] = w;
    }
    for (int i = 0; i < N; ++i)
        if (! considered[(size_t) i]) lastWrittenWeight[(size_t) i] = matter.node (i).weight;
    diag.nodesMoved = moved;
    diag.meanAbsCents = centsSum / (float) numActive;
    diag.maxAbsCents = centsMax;
}

void EvolveEngine::fillDiagnostics (EvolveDiag& dest) const noexcept
{
    dest = diag;
}

} // namespace am

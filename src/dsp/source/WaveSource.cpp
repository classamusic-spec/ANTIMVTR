#include "WaveSource.h"
#include "WavetableGenerator.h"
#include "core/Random.h"
#include "core/RealtimeUtils.h"
#include "dev/diagnostics/Diagnostics.h"

namespace am
{

namespace
{
    constexpr float kPiF = 3.14159265358979f;

    /** Folds a position into [0, 1] (reflecting), keeping scan sweeps continuous. */
    inline float foldUnit (float x) noexcept
    {
        if (x > 1.0f) x = 2.0f - x;
        if (x < 0.0f) x = -x;
        return x > 1.0f ? 1.0f : (x < 0.0f ? 0.0f : x);
    }

    /** Symmetric unison offset in [-1, 1]. */
    inline float unisonOffset (int index, int count) noexcept
    {
        return count < 2 ? 0.0f : 2.0f * (float) index / (float) (count - 1) - 1.0f;
    }

    /** Golden-ratio phase spread so unison voices never start perfectly coherent. */
    inline float unisonPhaseOffset (int index) noexcept
    {
        const float g = 0.61803398875f * (float) index;
        return g - std::floor (g);
    }

    // Musical scaling of the modulation amounts.
    inline float fmDepthOf   (float a) noexcept { return a * a * 4.0f; }       // increment multiplier swing
    inline float pmDepthOf   (float a) noexcept { return a * a * 0.35f; }      // cycles
    inline float syncCurveOf (float a) noexcept { return a * std::sqrt (a); }  // a^1.5
    inline float detuneCents (float a) noexcept { return a * a * 50.0f; }      // +/- cents at the edges
}

//==============================================================================
void WaveSource::Osc::clearRings() noexcept
{
    for (int i = 0; i < kWaveBlepLen; ++i) { rawRing[i] = 0.0f; corrRing[i] = 0.0f; }
    ringIndex = 0;
}

//==============================================================================
void WaveSource::prepare (double sampleRate, int)
{
    sr = sampleRate > 0.0 ? sampleRate : 48000.0;

    // Builds the shared, immutable table cache on first use. Message thread.
    Wavetables::prewarm();
    sineTab  = Wavetables::sineTable();
    blepTab  = Wavetables::blepTable();
    blampTab = Wavetables::blampTable();

    reset();
}

void WaveSource::reset()
{
    for (auto& o : oscs)
    {
        o.phase = o.master = o.modPhase = 0.0;
        o.ratio = 1.0f;
        o.gainL = o.gainR = 0.0f;
        o.clearRings();
    }
    scanPhase = 0.0;
    fadeCounter = 0;
    lastEnergy = 0.0f;
    primed = false;
}

void WaveSource::noteOn (const NoteState& note, const ParamValues& params)
{
    const float startPhase   = paramValue (params, Param::wavePhase);
    const float randomAmount = clamp01 (paramValue (params, Param::wavePhaseRandom));

    Rng rng (hashSeed (note.noteId, 0x57A7Eu));

    for (int i = 0; i < kMaxUnison; ++i)
    {
        auto& o = oscs[(size_t) i];
        double p = (double) startPhase + (double) unisonPhaseOffset (i)
                 + (double) (randomAmount * rng.nextFloat());
        p -= std::floor (p);
        o.phase = p;
        o.master = p;
        o.modPhase = 0.0;
        o.gainL = o.gainR = 0.0f;      // ramps up from silence inside the first block
        o.clearRings();
    }

    scanPhase = 0.0;
    fadeCounter = kFadeLength + kLatencySamples;
    primed = false;
    lastEnergy = 0.0f;
}

//==============================================================================
void WaveSource::render (float* l, float* r, int n, const RenderContext& ctx, const NoteState& note)
{
    juce::FloatVectorOperations::clear (l, n);
    juce::FloatVectorOperations::clear (r, n);
    if (n <= 0) return;

    // prepare() always builds the shared cache first. If it somehow has not
    // run, stay silent rather than allocating on the audio thread.
    if (sineTab == nullptr || blepTab == nullptr || blampTab == nullptr)
        return;

    //--------------------------------------------------------------- parameters
    const int   bankIndex = juce::jlimit (0, kWaveNumBanks - 1, ctx.choice (Param::waveTable));
    const auto& bankData  = Wavetables::bank (bankIndex);
    const int   numFrames = bankData.numFrames;
    const float frameScale = (float) (numFrames - 1);

    const float level    = juce::jmax (0.0f, ctx.param (Param::waveLevel));
    const float position = clamp01 (ctx.param (Param::wavePosition));
    const float morph    = clamp01 (ctx.param (Param::waveMorph));
    const float scan     = clamp01 (ctx.param (Param::waveScan));
    const int   unison   = juce::jlimit (1, kMaxUnison, (int) std::lround (ctx.param (Param::waveUnison)));
    const float detune   = clamp01 (ctx.param (Param::waveDetune));
    const float spread   = clamp01 (ctx.param (Param::waveSpread));
    const float fmAmt    = clamp01 (ctx.param (Param::waveFM));
    const float pmAmt    = clamp01 (ctx.param (Param::wavePM));
    const float amAmt    = clamp01 (ctx.param (Param::waveAM));
    const float ringAmt  = clamp01 (ctx.param (Param::waveRing));
    const float syncAmt  = clamp01 (ctx.param (Param::waveSync));
    const float modRatio = juce::jlimit (0.25f, 16.0f, ctx.param (Param::waveModRatio));

    const bool firstBlock = ! primed;
    if (firstBlock)
    {
        smPosition = position; smMorph = morph;
        smAm = amAmt; smRing = ringAmt; smFm = fmAmt; smPm = pmAmt;
        primed = true;
    }

    const float invN = 1.0f / (float) n;
    const float posStart = smPosition,  posStep  = (position - smPosition) * invN;
    const float mphStart = smMorph,     mphStep  = (morph - smMorph) * invN;
    const float amStart  = smAm,        amStep   = (amAmt - smAm) * invN;
    const float rngStart = smRing,      rngStep  = (ringAmt - smRing) * invN;
    const float fmStart  = smFm,        fmStep   = (fmAmt - smFm) * invN;
    const float pmStart  = smPm,        pmStep   = (pmAmt - smPm) * invN;

    //------------------------------------------------------------------- pitch
    const double pitchOffset = (double) std::round (ctx.param (Param::waveOctave)) * 12.0
                             + (double) std::round (ctx.param (Param::waveSemi))
                             + (double) ctx.param (Param::waveFine) * 0.01;
    const double baseFreq = note.frequency * std::exp2 (pitchOffset / 12.0);
    const double maxInc = (double) kMaxFrequencyRatio;

    //------------------------------------------------------------------ unison
    const float cents = detuneCents (detune);
    float weightSum = 0.0f;
    for (int i = 0; i < unison; ++i)
    {
        const float t = unisonOffset (i, unison);
        const float w = 1.0f - 0.25f * t * t;         // centre weighted
        weightSum += w * w;
    }
    const float unisonGain = weightSum > 1.0e-6f ? 1.0f / std::sqrt (weightSum) : 1.0f;

    float targetL[kMaxUnison] {}, targetR[kMaxUnison] {};
    for (int i = 0; i < unison; ++i)
    {
        const float t = unisonOffset (i, unison);
        const float w = 1.0f - 0.25f * t * t;
        const float pan = juce::jlimit (-1.0f, 1.0f, t * spread);
        const float ang = (pan + 1.0f) * kPiF * 0.25f;
        const float g = level * unisonGain * w * 1.41421356f;
        targetL[i] = g * std::cos (ang);
        targetR[i] = g * std::sin (ang);
        oscs[(size_t) i].ratio = std::exp2 (t * cents / 1200.0f);
    }
    for (int i = unison; i < kMaxUnison; ++i) { targetL[i] = 0.0f; targetR[i] = 0.0f; }

    // The first block after a note-on snaps to the target gains (the raised
    // cosine guard below removes the click) so the attack does not depend on
    // the host block size.
    if (firstBlock)
        for (int i = 0; i < kMaxUnison; ++i)
        {
            oscs[(size_t) i].gainL = targetL[i];
            oscs[(size_t) i].gainR = targetR[i];
        }

    //-------------------------------------------------------------- modulation
    const bool  syncOn = syncAmt > 0.002f;
    const float slaveRatio = 1.0f + syncCurveOf (syncAmt) * (modRatio - 1.0f);
    const float fmDepthMax = fmDepthOf (juce::jmax (fmAmt, smFm));
    const float pmDepthMax = pmDepthOf (juce::jmax (pmAmt, smPm));
    const bool  fmOn   = fmAmt > 1.0e-4f || smFm > 1.0e-4f;
    const bool  pmOn   = pmAmt > 1.0e-4f || smPm > 1.0e-4f;
    const bool  amOn   = amAmt > 1.0e-4f || smAm > 1.0e-4f;
    const bool  ringOn = ringAmt > 1.0e-4f || smRing > 1.0e-4f;
    const bool  modOn  = fmOn || pmOn || amOn || ringOn;

    // Conservative brightness allowance so modulation cannot push the table
    // content past Nyquist. Capped at 3 octaves so heavy FM stays usable.
    float modBright = 1.0f + fmDepthMax + pmDepthMax * 6.2831853f * modRatio;
    modBright = juce::jlimit (1.0f, 8.0f, modBright);

    // Hard sync: keeping the slave table an octave below its Nyquist limit
    // shrinks every higher-order term the BLEP/BLAMP pair cannot correct, and
    // costs almost no character because the sync buzz comes from the reset.
    if (syncOn) modBright *= 1.0f + 1.6f * syncCurveOf (syncAmt);
    const float warpSlope = waveMorphSlope (juce::jmax (morph, smMorph));

    //-------------------------------------------------------------------- scan
    const float scanRate = scan > 1.0e-3f ? 0.05f * std::pow (400.0f, scan) : 0.0f;
    const double scanInc = (double) scanRate / sr;
    const bool scanOn = scanRate > 0.0f;

    //------------------------------------------------------------------ render
    for (int u = 0; u < unison; ++u)
    {
        auto& o = oscs[(size_t) u];

        const double freq  = baseFreq * (double) o.ratio;
        const double incM  = juce::jlimit (0.0, maxInc, freq / sr);
        const double incS  = juce::jlimit (-maxInc, maxInc, incM * (double) slaveRatio);
        const double incMod = juce::jlimit (0.0, maxInc, incM * (double) modRatio);

        const auto mip = waveSelectMip ((float) std::abs (incS) * warpSlope * modBright);
        const auto& lvA = bankData.levels[(size_t) mip.levelA];
        const auto& lvB = bankData.levels[(size_t) mip.levelB];
        const bool mipCross = mip.blend > 1.0e-4f && mip.levelB != mip.levelA;

        float gl = o.gainL, gr = o.gainR;
        const float dgl = (targetL[u] - gl) * invN;
        const float dgr = (targetR[u] - gr) * invN;

        double phase = o.phase, master = o.master, modPhase = o.modPhase;
        double scanP = scanPhase;
        uint32_t ring = o.ringIndex;

        float pos = posStart, mph = mphStart, amV = amStart, rgV = rngStart;
        float fmV = fmStart, pmV = pmStart;

        for (int i = 0; i < n; ++i)
        {
            //---- frame selection (position + scan) --------------------------
            float p = pos;
            if (scanOn)
            {
                p += 1.0f - 2.0f * std::abs ((float) scanP - 0.5f);
                scanP += scanInc;
                if (scanP >= 1.0) scanP -= 1.0;
            }
            p = foldUnit (p);

            const float fp = p * frameScale;
            const int   fi = juce::jlimit (0, numFrames - 2, (int) fp);
            const float fb = juce::jlimit (0.0f, 1.0f, fp - (float) fi);
            const float warpAmount = waveMorphAmount (mph);

            const float* aA = lvA.frame (fi);
            const float* bA = aA + lvA.length;
            const float* aB = lvB.frame (fi);
            const float* bB = aB + lvB.length;

            //---- modulator ---------------------------------------------------
            float mod = 0.0f;
            if (modOn)
            {
                mod = waveSineLookup (sineTab, (float) modPhase);
                modPhase += incMod;
                if (modPhase >= 1.0) modPhase -= 1.0;
            }

            const float pmOff = pmOn ? pmV * mod : 0.0f;

            auto readAt = [&] (double ph) noexcept -> float
            {
                const float w = waveMorphWarp (wavePhaseWrap ((float) ph + pmOff), warpAmount, sineTab);
                float y = waveReadFrames (aA, bA, fb, lvA.mask, lvA.lengthF, w);
                if (mipCross)
                {
                    const float y2 = waveReadFrames (aB, bB, fb, lvB.mask, lvB.lengthF, w);
                    y += mip.blend * (y2 - y);
                }
                return y;
            };

            //---- advance carrier (through-zero FM) ---------------------------
            double step = incS;
            if (fmOn) step *= (1.0 + (double) (fmDepthOf (fmV) * mod));

            const double prevPhase = phase;
            phase += step;

            bool  didReset = false;
            float resetFrac = 0.0f, jump = 0.0f, slopeJump = 0.0f;

            if (syncOn && incM > 1.0e-9)
            {
                master += incM;
                if (master >= 1.0)
                {
                    const double d = juce::jlimit (0.0, 0.999999, (master - 1.0) / incM);
                    master -= 1.0;
                    const double atReset = prevPhase + step * (1.0 - d);

                    // Step and slope discontinuity of the reset, corrected below
                    // with a BLEP / BLAMP pair so hard sync stays band limited.
                    const float before = readAt (atReset);
                    const float after  = readAt (0.0);
                    jump = after - before;
                    const double h = step * 0.5;
                    slopeJump = (readAt (h) - readAt (-h))
                              - (readAt (atReset + h) - readAt (atReset - h));

                    phase = d * step;
                    resetFrac = (float) d;
                    didReset = true;
                }
            }

            phase -= std::floor (phase);
            const float raw = readAt (phase);

            //---- band-limited step correction --------------------------------
            o.rawRing[ring & kWaveBlepRingMask] = raw;
            if (didReset && (jump != 0.0f || slopeJump != 0.0f))
            {
                const float fpos = resetFrac * (float) kWaveBlepRes;
                const int   di = juce::jlimit (0, kWaveBlepRes - 1, (int) fpos);
                const float df = fpos - (float) di;
                const float* e0 = blepTab + (size_t) (di * kWaveBlepLen);
                const float* e1 = e0 + kWaveBlepLen;
                const float* a0 = blampTab + (size_t) (di * kWaveBlepLen);
                const float* a1 = a0 + kWaveBlepLen;
                for (int j = 0; j < kWaveBlepLen; ++j)
                {
                    const float ev = e0[j] + df * (e1[j] - e0[j]);
                    const float av = a0[j] + df * (a1[j] - a0[j]);
                    o.corrRing[(ring + (uint32_t) j - (uint32_t) kWaveBlepZ) & kWaveBlepRingMask]
                        += jump * ev + slopeJump * av;
                }
            }

            const uint32_t outSlot = (ring - (uint32_t) kWaveBlepZ) & kWaveBlepRingMask;
            float s = o.rawRing[outSlot] + o.corrRing[outSlot];
            o.corrRing[outSlot] = 0.0f;
            ++ring;

            //---- ring / AM ---------------------------------------------------
            if (ringOn) s *= 1.0f - rgV + rgV * mod;
            if (amOn)   s *= 1.0f - amV * 0.5f * (1.0f - mod);

            l[i] += s * gl;
            r[i] += s * gr;

            gl += dgl; gr += dgr;
            pos += posStep; mph += mphStep; amV += amStep; rgV += rngStep;
            fmV += fmStep; pmV += pmStep;
        }

        o.phase = phase; o.master = master; o.modPhase = modPhase;
        o.gainL = targetL[u]; o.gainR = targetR[u];
        o.ringIndex = ring;
    }

    // Unused unison slots decay their gain so changing the count never clicks.
    for (int u = unison; u < kMaxUnison; ++u) { oscs[(size_t) u].gainL = 0.0f; oscs[(size_t) u].gainR = 0.0f; }

    //---------------------------------------------------------- note-start fade
    if (fadeCounter > 0)
    {
        constexpr int total = kFadeLength + kLatencySamples;
        const int m = juce::jmin (n, fadeCounter);
        for (int i = 0; i < m; ++i)
        {
            const int c = total - fadeCounter + i - kLatencySamples;   // samples of real audio
            const float g = c <= 0 ? 0.0f
                                   : 0.5f - 0.5f * std::cos (kPiF * (float) c / (float) kFadeLength);
            l[i] *= g; r[i] *= g;
        }
        fadeCounter -= m;
    }

    //-------------------------------------------------------------- book-keeping
    smPosition = position; smMorph = morph;
    smAm = amAmt; smRing = ringAmt; smFm = fmAmt; smPm = pmAmt;

    if (scanOn)
    {
        scanPhase += scanInc * (double) n;
        scanPhase -= std::floor (scanPhase);
    }

    const int bad = scrubBuffer (l, n) + scrubBuffer (r, n);
    if (bad > 0)
    {
        if (ctx.diagnostics != nullptr)
            ctx.diagnostics->safety.note (SafetyEvent::NaN, Subsystem::Source, -1, bad);
        reset();
    }

    const auto range = juce::FloatVectorOperations::findMinAndMax (l, n);
    lastEnergy = clamp01 (juce::jmax (std::abs (range.getStart()), std::abs (range.getEnd())));
}

} // namespace am

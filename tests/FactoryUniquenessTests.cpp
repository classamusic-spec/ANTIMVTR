#include <juce_core/juce_core.h>

#include "FactoryBankSampling.h"

#include "dev/diagnostics/SignalMetrics.h"
#include "presets/FactoryContent.h"
#include "state/ModRouting.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>

using namespace am;
using namespace am::dev;
using namespace am::factorytest;

/*
    THE UNIQUENESS GATE

    Six authors are writing 264 patches in parallel and cannot hear each
    other's work. "Unique patches" is the whole point of the bank, so it is
    enforced here rather than hoped for.

    Two measurements, and a pair has to be close on BOTH to fail:

      1. WHAT IT SOUNDS LIKE. Each preset is rendered once (the short render
         the rest of the suite uses) and reduced to a fingerprint of five
         things a listener would name: the spectrum, how the level moves over
         the note, the attack, what the stereo image does, and the pitch
         content. Every part is normalised so that loudness on its own cannot
         make two patches look alike or unlike.

      2. WHAT IT IS MADE OF. A weighted distance over the parameters, where
         the choices that define character — source, material pair, topology,
         envelope times, the modulation routing set — count for far more than
         a trim like fine tune or output gain.

    Requiring both is what makes the gate trustworthy. Two patches that
    measure alike but are built differently are usually a real pair that the
    single-note render simply cannot separate (the same instrument an octave
    apart, say); two built alike that measure differently are a patch and its
    deliberate variation. A pair that is close on both is a duplicate.
*/
namespace
{
//==============================================================================
// FINGERPRINT
//==============================================================================

constexpr int kNumBands     = 8;    ///< log-spaced spectral bands, 40 Hz .. 20 kHz
constexpr int kNumSlices    = 12;   ///< envelope slices over the whole render
constexpr int kSpectrumFrames = 6;  ///< FFT frames averaged across the note

/** Everything the gate remembers about how one preset sounds. */
struct Fingerprint
{
    juce::String name, category;

    std::array<float, kNumBands>  bands {};      ///< share of the energy per band, on a dB scale, 0..1
    float centroid = 0.0f;                       ///< spectral centroid, log frequency 0..1
    float flatness = 0.0f;                       ///< 0 = tonal, 1 = noise

    std::array<float, kNumSlices> envelope {};   ///< RMS per slice / loudest slice, 0..1

    float attack = 0.0f;                         ///< time to 90 % of peak, log scaled 0..1
    float crest  = 0.0f;                         ///< peak / RMS, 0..1

    float correlation = 0.0f;                    ///< stereo correlation mapped to 0..1
    float width = 0.0f;                          ///< side / (mid + side)

    float dominant = 0.0f;                       ///< strongest partial, log frequency 0..1
    float harmonicity = 0.0f;                    ///< how much energy sits on multiples of it
};

float logFrequency (float hz) noexcept
{
    // 40 Hz .. 20 kHz onto 0..1: an octave is an octave wherever it happens.
    const float lo = std::log2 (40.0f), hi = std::log2 (20000.0f);
    return juce::jlimit (0.0f, 1.0f, (std::log2 (juce::jmax (20.0f, hz)) - lo) / (hi - lo));
}

/**
    A band's share of the energy, as an amplitude.

    Amplitude and not decibels on purpose. On a dB scale a band fifty decibels
    below the note swings as widely as the note itself, so two renders of the
    same patch with one knob moved look further apart than two different
    patches — the measure ends up describing the noise floor rather than the
    sound. As an amplitude share the bands that carry the patch are the bands
    that decide, which is what a listener means by "the same".
*/
float shareToLevel (float share) noexcept
{
    return juce::jlimit (0.0f, 1.0f, std::sqrt (juce::jmax (0.0f, share)));
}

Fingerprint fingerprintOf (const std::vector<float>& left, const std::vector<float>& right,
                           double sampleRate, SpectrumMeasurement& spectrum)
{
    Fingerprint f;
    const int n = (int) std::min (left.size(), right.size());
    if (n <= 0) return f;

    std::vector<float> mono ((size_t) n, 0.0f);
    double sum = 0.0, midSum = 0.0, sideSum = 0.0, lSum = 0.0, rSum = 0.0, lrSum = 0.0;
    float peak = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float l = std::isfinite (left[(size_t) i]) ? left[(size_t) i] : 0.0f;
        const float r = std::isfinite (right[(size_t) i]) ? right[(size_t) i] : 0.0f;
        const float m = 0.5f * (l + r), s = 0.5f * (l - r);
        mono[(size_t) i] = m;
        sum += (double) m * (double) m;
        midSum += (double) m * (double) m;
        sideSum += (double) s * (double) s;
        lSum += (double) l * (double) l;
        rSum += (double) r * (double) r;
        lrSum += (double) l * (double) r;
        peak = juce::jmax (peak, std::abs (l), std::abs (r));
    }
    const float rms = (float) std::sqrt (sum / juce::jmax (1, n));

    // ---- a short-window RMS envelope, which both the shape and the attack read
    const int hop = juce::jmax (1, (int) (0.005 * sampleRate));   // 5 ms
    std::vector<float> hops;
    float loudestHop = 0.0f;
    {
        hops.reserve ((size_t) (n / hop + 1));
        for (int from = 0; from < n; from += hop)
        {
            const int to = std::min (n, from + hop);
            double acc = 0.0;
            for (int i = from; i < to; ++i) acc += (double) mono[(size_t) i] * (double) mono[(size_t) i];
            hops.push_back ((float) std::sqrt (acc / juce::jmax (1, to - from)));
            loudestHop = juce::jmax (loudestHop, hops.back());
        }
    }

    // ---- level over time: the shape of the note, normalised by its own loudest slice
    {
        std::array<float, kNumSlices> slice {};
        float loudest = 0.0f;
        for (int s = 0; s < kNumSlices; ++s)
        {
            const int from = (int) ((int64_t) n * s / kNumSlices);
            const int to   = (int) ((int64_t) n * (s + 1) / kNumSlices);
            double acc = 0.0;
            for (int i = from; i < to; ++i) acc += (double) mono[(size_t) i] * (double) mono[(size_t) i];
            slice[(size_t) s] = (float) std::sqrt (acc / juce::jmax (1, to - from));
            loudest = juce::jmax (loudest, slice[(size_t) s]);
        }
        for (int s = 0; s < kNumSlices; ++s)
            f.envelope[(size_t) s] = loudest > 0.0f ? slice[(size_t) s] / loudest : 0.0f;
    }

    // ---- attack: how long the note takes to arrive. Measured on the smoothed
    // envelope rather than on raw samples, where a single loud cycle anywhere in
    // the note would decide the answer.
    {
        int at = (int) hops.size() - 1;
        for (size_t i = 0; i < hops.size(); ++i)
            if (hops[i] >= 0.5f * loudestHop) { at = (int) i; break; }   // half way up, not the very top:
                                                                        // the last few per cent of a rise
                                                                        // move around with the material
        const float seconds = (float) (at * hop) / (float) juce::jmax (1.0, sampleRate);
        f.attack = juce::jlimit (0.0f, 1.0f, std::log10 (1.0f + 1000.0f * seconds) / std::log10 (3001.0f));
        f.crest = juce::jlimit (0.0f, 1.0f, (rms > 0.0f ? peak / rms : 0.0f) / 20.0f);
    }

    // ---- stereo behaviour
    {
        const double denom = std::sqrt (lSum * rSum);
        const float correlation = denom > 1.0e-12 ? (float) (lrSum / denom) : 1.0f;
        f.correlation = juce::jlimit (0.0f, 1.0f, 0.5f * (correlation + 1.0f));
        const double energy = midSum + sideSum;
        f.width = energy > 1.0e-12 ? (float) (sideSum / energy) : 0.0f;
    }

    // ---- spectrum: magnitudes averaged over frames spread across the note
    {
        const int fftSize = spectrum.fftSize();
        std::vector<float> mags, average ((size_t) (fftSize / 2), 0.0f);
        int frames = 0;
        for (int k = 0; k < kSpectrumFrames; ++k)
        {
            const int start = (int) ((double) n * (0.05 + 0.15 * k));
            if (start + fftSize > n) break;
            spectrum.magnitudes (mono.data() + start, fftSize, mags);
            const size_t bins = std::min (average.size(), mags.size());
            for (size_t b = 0; b < bins; ++b) average[b] += mags[b];
            ++frames;
        }
        if (frames == 0)
        {
            spectrum.magnitudes (mono.data(), std::min (n, fftSize), mags);
            const size_t bins = std::min (average.size(), mags.size());
            for (size_t b = 0; b < bins; ++b) average[b] = mags[b];
            frames = 1;
        }
        for (auto& m : average) m /= (float) frames;

        static const float edges[kNumBands + 1] = { 40.0f, 90.0f, 200.0f, 450.0f, 1000.0f, 2200.0f, 5000.0f, 11000.0f, 20000.0f };
        std::array<double, kNumBands> band {};
        double total = 0.0;
        const double binHz = sampleRate / (double) fftSize;
        for (size_t b = 1; b < average.size(); ++b)
        {
            const double hz = (double) b * binHz;
            const double e = (double) average[b] * (double) average[b];
            total += e;
            for (int k = 0; k < kNumBands; ++k)
                if (hz >= edges[k] && hz < edges[k + 1]) { band[(size_t) k] += e; break; }
        }
        for (int k = 0; k < kNumBands; ++k)
            f.bands[(size_t) k] = shareToLevel (total > 0.0 ? (float) (band[(size_t) k] / total) : 0.0f);

        f.centroid = logFrequency (SpectrumMeasurement::centroidOf (average, sampleRate, fftSize));
        f.flatness = juce::jlimit (0.0f, 1.0f, SpectrumMeasurement::flatnessOf (average));

        // ---- pitch content: the strongest partial, and how much sits on its multiples
        size_t strongest = 1;
        for (size_t b = 2; b < average.size(); ++b)
            if (average[b] > average[strongest]) strongest = b;
        const float dominantHz = (float) ((double) strongest * binHz);
        f.dominant = logFrequency (dominantHz);

        double harmonic = 0.0, all = 0.0;
        for (size_t b = 1; b < average.size(); ++b) all += (double) average[b];
        for (int k = 1; k <= 8; ++k)
        {
            const size_t centre = strongest * (size_t) k;
            if (centre >= average.size()) break;
            for (size_t b = (centre > 1 ? centre - 1 : 1); b <= std::min (centre + 1, average.size() - 1); ++b)
                harmonic += (double) average[b];
        }
        f.harmonicity = all > 0.0 ? juce::jlimit (0.0f, 1.0f, (float) (harmonic / all)) : 0.0f;
    }

    return f;
}

//==============================================================================
/** RMS difference over a fixed-size block of the fingerprint. */
template <size_t N>
float blockDistance (const std::array<float, N>& a, const std::array<float, N>& b) noexcept
{
    double acc = 0.0;
    for (size_t i = 0; i < N; ++i) { const double d = (double) a[i] - (double) b[i]; acc += d * d; }
    return (float) std::sqrt (acc / (double) N);
}

float pairDistance (float a0, float b0, float a1, float b1) noexcept
{
    const double d0 = (double) a0 - b0, d1 = (double) a1 - b1;
    return (float) std::sqrt (0.5 * (d0 * d0 + d1 * d1));
}

/** How the five parts of the fingerprint are weighted against each other. */
struct AudioDistance
{
    float spectrum = 0.0f, envelope = 0.0f, attack = 0.0f, stereo = 0.0f, pitch = 0.0f, total = 0.0f;
};

AudioDistance audioDistance (const Fingerprint& a, const Fingerprint& b) noexcept
{
    AudioDistance d;
    d.spectrum = std::sqrt (0.75f * juce::square (blockDistance (a.bands, b.bands))
                            + 0.25f * juce::square (pairDistance (a.centroid, b.centroid, a.flatness, b.flatness)));
    d.envelope = blockDistance (a.envelope, b.envelope);
    d.attack   = pairDistance (a.attack, b.attack, a.crest, b.crest);
    d.stereo   = pairDistance (a.correlation, b.correlation, a.width, b.width);
    d.pitch    = pairDistance (a.dominant, b.dominant, a.harmonicity, b.harmonicity);

    d.total = std::sqrt (0.40f * juce::square (d.spectrum)
                         + 0.25f * juce::square (d.envelope)
                         + 0.15f * juce::square (d.attack)
                         + 0.08f * juce::square (d.stereo)
                         + 0.12f * juce::square (d.pitch));
    return d;
}

//==============================================================================
// PARAMETER DISTANCE
//==============================================================================

/** The parameter group that belongs to the selected source, so a WAVE patch is
    not judged on its (unused) DUST settings. */
ParamGroup groupForSource (int source) noexcept
{
    switch (source)
    {
        case 0:  return ParamGroup::Wave;
        case 1:  return ParamGroup::Dust;
        case 2:  return ParamGroup::Impact;
        case 3:  return ParamGroup::Sample;
        default: return ParamGroup::Gesture;
    }
}

/** The one choice that names a source's flavour — the thing on the front panel. */
bool isSourceFlavour (Param p) noexcept
{
    return p == Param::waveTable || p == Param::dustMode || p == Param::impactMode
        || p == Param::sampleMode || p == Param::gestureMode;
}

/** True for the trims: real controls, but not what makes a patch a patch. */
bool isTrim (Param p) noexcept
{
    switch (p)
    {
        case Param::masterGain: case Param::masterFine: case Param::masterBendRange:
        case Param::masterVoices: case Param::masterQuality:
        case Param::wavePhase: case Param::wavePhaseRandom: case Param::waveFine:
        case Param::ampVelocity:
            return true;
        default:
            return false;
    }
}

/**
    How much a parameter counts toward "these are the same patch".

    Weighted so the decisions the brief calls character — which source drives
    it, what it is made of, how it is connected, how the note is shaped —
    outweigh a trim by more than an order of magnitude. Anything belonging to
    a source that is not selected counts for nothing at all.
*/
float characterWeight (const ParamDesc& d, int sourceA, int sourceB, bool eitherFracture) noexcept
{
    const Param p = d.param;

    if (p == Param::sourceSelected || p == Param::sourceMode || p == Param::shapeTopology) return 6.0f;
    if (p == Param::shapeMaterialA || p == Param::shapeMaterialB) return 0.0f;   // scored as an unordered pair
    if (p == Param::ampAttack || p == Param::ampDecay || p == Param::ampSustain || p == Param::ampRelease) return 4.0f;
    if (p == Param::spaceType) return 3.0f;
    if (p == Param::masterTranspose || p == Param::masterMode) return 2.0f;
    if (isTrim (p)) return 0.25f;

    const auto group = d.group;
    const bool sourceGroup = group == ParamGroup::Wave || group == ParamGroup::Dust || group == ParamGroup::Impact
                          || group == ParamGroup::Sample || group == ParamGroup::Gesture;
    if (sourceGroup)
    {
        // Only the selected source is audible; the rest of the panel is dormant.
        if (group != groupForSource (sourceA) && group != groupForSource (sourceB)) return 0.0f;
        return isSourceFlavour (p) ? 4.0f : 1.0f;
    }

    if (d.mutation == MutationCategory::Chaos) return 0.5f;      // seeds and randomisation amounts

    switch (group)
    {
        case ParamGroup::Shape:    return 2.0f;
        case ParamGroup::Evolve:   return 2.0f;
        case ParamGroup::Fracture: return eitherFracture ? 2.0f : 0.0f;
        case ParamGroup::Space:    return 1.5f;
        case ParamGroup::Amp:      return 1.0f;
        case ParamGroup::Mod:      return 1.0f;
        case ParamGroup::Macro:    return 0.5f;
        default:                   return 0.25f;
    }
}

/** 0 (same pair) .. 1 (nothing in common), order independent. */
float materialPairDistance (const ParamValues& a, const ParamValues& b) noexcept
{
    const int a0 = paramChoice (a, Param::shapeMaterialA), a1 = paramChoice (a, Param::shapeMaterialB);
    const int b0 = paramChoice (b, Param::shapeMaterialA), b1 = paramChoice (b, Param::shapeMaterialB);
    int shared = 0;
    if (a0 == b0 || a0 == b1) ++shared;
    if (a1 == b0 || a1 == b1) ++shared;
    return 1.0f - 0.5f * (float) juce::jmin (2, shared);
}

/**
    Everything about one patch that the pair loop needs, computed once.

    At 300 presets there are 44,850 pairs; normalising two hundred parameters
    and rebuilding the routing key set inside that loop would cost more than
    rendering the bank. Everything that depends on ONE patch is done here.
*/
struct Signature
{
    ParamValues       normalised {};   ///< float params in the host curve, discrete params as-is
    std::vector<uint32_t> routings;    ///< sorted (source, target) keys
    int  source = 0;
    bool fracture = false;
};

Signature signatureOf (const PatchState& patch)
{
    Signature sig;
    for (const auto& d : ParameterRegistry::all())
    {
        const size_t i = (size_t) paramIndex (d.param);
        sig.normalised[i] = d.kind == ParamKind::Float ? d.toNormalised (patch.params[i]) : patch.params[i];
    }
    sig.source = paramChoice (patch.params, Param::sourceSelected);
    sig.fracture = paramBool (patch.params, Param::fractureOn);

    if (! patch.mod.isVoid())
        for (const auto& r : ModRoutingTable::fromVar (patch.mod))
            sig.routings.push_back (((uint32_t) r.source << 16) | (uint32_t) r.target);
    std::sort (sig.routings.begin(), sig.routings.end());
    sig.routings.erase (std::unique (sig.routings.begin(), sig.routings.end()), sig.routings.end());
    return sig;
}

/** Jaccard distance between the two modulation matrices, on (source -> target) pairs. */
float routingDistance (const Signature& a, const Signature& b) noexcept
{
    if (a.routings.empty() && b.routings.empty()) return 0.0f;
    std::vector<uint32_t> shared;
    std::set_intersection (a.routings.begin(), a.routings.end(),
                           b.routings.begin(), b.routings.end(), std::back_inserter (shared));
    const int total = (int) (a.routings.size() + b.routings.size() - shared.size());
    return total > 0 ? 1.0f - (float) shared.size() / (float) total : 0.0f;
}

/** Weighted mean parameter difference, 0 (identical) .. 1 (nothing alike). */
float parameterDistance (const PatchState& pa, const PatchState& pb,
                         const Signature& sa, const Signature& sb)
{
    const bool eitherFracture = sa.fracture || sb.fracture;

    double weighted = 0.0, weights = 0.0;
    for (const auto& d : ParameterRegistry::all())
    {
        const float w = characterWeight (d, sa.source, sb.source, eitherFracture);
        if (w <= 0.0f) continue;

        const size_t i = (size_t) paramIndex (d.param);
        // Normalised, so a 20 ms difference in a 5 ms attack counts and the same
        // 20 ms in a four second release does not; discrete choices are all or nothing.
        const float difference = d.kind == ParamKind::Float
                                   ? juce::jlimit (0.0f, 1.0f, std::abs (sa.normalised[i] - sb.normalised[i]))
                                   : (std::abs (sa.normalised[i] - sb.normalised[i]) > 0.5f ? 1.0f : 0.0f);

        weighted += (double) w * difference;
        weights += w;
    }

    // The material pair and the modulation matrix are not single parameters but
    // they are exactly what the brief means by "material and structure" and
    // "movement", so they are scored alongside them.
    weighted += 6.0 * materialPairDistance (pa.params, pb.params);   weights += 6.0;
    weighted += 4.0 * routingDistance (sa, sb);                      weights += 4.0;

    return weights > 0.0 ? (float) (weighted / weights) : 0.0f;
}

//==============================================================================
/** What two patches have in common, in the brief's own terms — so a failure is actionable. */
juce::String sharedGround (const PatchState& a, const PatchState& b, const Signature& sa, const Signature& sb,
                           const Fingerprint& fa, const Fingerprint& fb)
{
    juce::StringArray same;
    auto choiceName = [] (Param p, const ParamValues& v)
    {
        return ParameterRegistry::get (p).formatValue (paramValue (v, p));
    };

    if (paramChoice (a.params, Param::sourceSelected) == paramChoice (b.params, Param::sourceSelected))
        same.add ("source " + choiceName (Param::sourceSelected, a.params));
    if (materialPairDistance (a.params, b.params) <= 0.0f)
        same.add ("material pair " + choiceName (Param::shapeMaterialA, a.params) + "/" + choiceName (Param::shapeMaterialB, a.params));
    if (paramChoice (a.params, Param::shapeTopology) == paramChoice (b.params, Param::shapeTopology))
        same.add ("topology " + choiceName (Param::shapeTopology, a.params));
    if (paramChoice (a.params, Param::spaceType) == paramChoice (b.params, Param::spaceType))
        same.add ("Space " + choiceName (Param::spaceType, a.params));
    if (std::abs (fa.attack - fb.attack) < 0.05f)
        same.add ("the same attack");
    if (blockDistance (fa.envelope, fb.envelope) < 0.08f)
        same.add ("the same envelope shape");
    if (std::abs (fa.centroid - fb.centroid) < 0.04f)
        same.add ("the same brightness");
    if (std::abs (fa.dominant - fb.dominant) < 0.04f)
        same.add ("the same register");
    if (routingDistance (sa, sb) < 0.25f)
        same.add ("the same modulation matrix");

    return same.isEmpty() ? juce::String ("nothing obvious — check the render") : same.joinIntoString (", ");
}

//==============================================================================
// THE GATE
//
// Calibrated from both sides against the shipped bank, and the numbers behind
// the calibration are logged on every run so the headroom is never a mystery.
//
//   * PARAMETER is the sharp edge. The closest pair in the shipped 36 measures
//     0.0877 (Ash Keys / Bone Marimba); the padding the brief warns about —
//     one knob moved by 0.1 and a new name — measures 0.0007, two orders of
//     magnitude below it. The gate sits at 0.060, which is also close to what
//     the brief itself asks for: with these weights, a pair that differs in
//     two of the character axes and nothing else lands around 0.055, so the
//     gate is roughly "you have not changed two things".
//
//   * AUDIO is a veto, not a second sharp edge. Its job is to let through the
//     pairs that are built alike but genuinely do not sound alike — the same
//     instrument transposed, the same recipe with a different topology seed —
//     so it sits well above anything a duplicate measures (the worst padding
//     case renders at 0.106) rather than being tuned tight. A single-note
//     render cannot separate two hand-designed patches reliably enough to
//     carry the decision on its own, and pretending otherwise would produce a
//     gate that fails honest work.
//
// A pair has to be inside BOTH to fail.
//==============================================================================
constexpr float kAudioGate = 0.150f;
constexpr float kParamGate = 0.060f;

struct Closest
{
    int a = -1, b = -1;
    float audio = 1.0f, parameter = 1.0f, closeness = 1.0e9f;
};
}

//==============================================================================
class FactoryUniquenessTests : public juce::UnitTest
{
public:
    FactoryUniquenessTests() : juce::UnitTest ("Factory uniqueness", "factory") {}

    void runTest() override
    {
        const Stopwatch watch;
        PresetManager presets;
        const int count = presets.numFactoryPresets();

        beginTest ("no two factory presets are near-duplicates");

        // ---- one short render per preset (the shared fingerprint profile)
        const auto options = fingerprintOptions();

        std::vector<PatchState> bank ((size_t) count);
        std::vector<Fingerprint> prints ((size_t) count);
        std::vector<Signature> signatures ((size_t) count);

        parallelPresetSweep (count, options,
            [&] (SynthEngine& engine, int i, juce::StringArray&)
            {
                // One FFT workspace per worker: it owns scratch buffers, so it
                // cannot be shared, and rebuilding it per preset would cost more
                // than the render.
                thread_local SpectrumMeasurement spectrum (11);
                thread_local std::vector<float> left, right;

                auto patch = presets.buildFactory (i);
                renderPatch (engine, patch, options, 60, true, &left, &right);
                auto print = fingerprintOf (left, right, options.sampleRate, spectrum);
                print.name = patch.meta.name;
                print.category = patch.meta.category.toUpperCase();
                prints[(size_t) i] = std::move (print);
                signatures[(size_t) i] = signatureOf (patch);
                bank[(size_t) i] = std::move (patch);
            });
        logMessage ("uniqueness: " + juce::String (count) + " presets rendered and fingerprinted in " + watch.elapsed());

        // ---- every pair, on both measures (cheap: no rendering here)
        std::vector<Closest> nearest;
        juce::StringArray duplicates;
        Closest worst;

        for (int a = 0; a < count; ++a)
        {
            for (int b = a + 1; b < count; ++b)
            {
                // "Init" is the blank starting point, not a patch competing for shelf space.
                if (prints[(size_t) a].category == "INIT" || prints[(size_t) b].category == "INIT") continue;

                Closest c;
                c.a = a; c.b = b;
                c.audio = audioDistance (prints[(size_t) a], prints[(size_t) b]).total;
                c.parameter = parameterDistance (bank[(size_t) a], bank[(size_t) b],
                                                 signatures[(size_t) a], signatures[(size_t) b]);
                // How close the pair came to the gate: below 1 on both axes is a failure.
                c.closeness = juce::jmax (c.audio / kAudioGate, c.parameter / kParamGate);
                nearest.push_back (c);

                if (c.closeness < worst.closeness) worst = c;

                if (c.audio < kAudioGate && c.parameter < kParamGate)
                {
                    const auto& pa = bank[(size_t) a];
                    const auto& pb = bank[(size_t) b];
                    duplicates.add (pa.meta.name + "  <->  " + pb.meta.name
                                    + "\n      audio distance " + juce::String (c.audio, 4) + " (gate " + juce::String (kAudioGate, 3) + ")"
                                    + ",  parameter distance " + juce::String (c.parameter, 4) + " (gate " + juce::String (kParamGate, 3) + ")"
                                    + "\n      they share: " + sharedGround (pa, pb, signatures[(size_t) a], signatures[(size_t) b],
                                                                              prints[(size_t) a], prints[(size_t) b])
                                    + "\n      make them differ in at least two of: source, material and structure,"
                                      " envelope shape, register and pitch, movement, space.");
                }
            }
        }

        std::sort (nearest.begin(), nearest.end(),
                   [] (const Closest& x, const Closest& y) { return x.closeness < y.closeness; });

        logMessage ("uniqueness: " + juce::String ((int) nearest.size()) + " pairs compared, "
                    + watch.elapsed() + " total");
        logMessage ("the five closest pairs in the bank (1.00 = on the gate):");
        for (size_t i = 0; i < nearest.size() && i < 5; ++i)
        {
            const auto& c = nearest[i];
            const auto parts = audioDistance (prints[(size_t) c.a], prints[(size_t) c.b]);
            logMessage ("   " + juce::String (c.closeness, 2) + "x   "
                        + prints[(size_t) c.a].name.paddedRight (' ', 18) + prints[(size_t) c.b].name.paddedRight (' ', 18)
                        + "audio " + juce::String (c.audio, 4) + "   parameter " + juce::String (c.parameter, 4)
                        + "   [spectrum " + juce::String (parts.spectrum, 3)
                        + "  envelope " + juce::String (parts.envelope, 3)
                        + "  attack " + juce::String (parts.attack, 3)
                        + "  stereo " + juce::String (parts.stereo, 3)
                        + "  pitch " + juce::String (parts.pitch, 3) + "]");
        }
        if (worst.a >= 0)
        {
            logMessage ("headroom: the closest pair sits " + juce::String (worst.closeness, 2)
                        + "x outside the gate (audio " + juce::String (worst.audio / kAudioGate, 2)
                        + "x, parameter " + juce::String (worst.parameter / kParamGate, 2) + "x)");

            Closest minAudio, minParameter;
            minAudio.audio = minParameter.parameter = 1.0e9f;
            for (const auto& c : nearest)
            {
                if (c.audio < minAudio.audio) minAudio = c;
                if (c.parameter < minParameter.parameter) minParameter = c;
            }
            logMessage ("closest on audio:     " + prints[(size_t) minAudio.a].name + " / " + prints[(size_t) minAudio.b].name
                        + "  " + juce::String (minAudio.audio, 4) + " (gate " + juce::String (kAudioGate, 3) + ")");
            logMessage ("closest on parameter: " + prints[(size_t) minParameter.a].name + " / " + prints[(size_t) minParameter.b].name
                        + "  " + juce::String (minParameter.parameter, 4) + " (gate " + juce::String (kParamGate, 3) + ")");
        }

        expect (duplicates.isEmpty(),
                juce::String (duplicates.size()) + " near-duplicate pair(s) in the factory bank:\n   "
                + duplicates.joinIntoString ("\n   "));

        // The gate is only worth anything if it has room: a bank whose closest
        // legitimate pair sits on the threshold would fail at random.
        expect (worst.closeness > 1.35f,
                "the uniqueness gate has almost no headroom (closest legitimate pair is "
                + juce::String (worst.closeness, 2) + "x the gate) — recalibrate before trusting it");

        //----------------------------------------------------------------------
        // The other half of the calibration. A gate that never fires is not a
        // gate, so the padding the brief warns about — "changing one knob by 0.1
        // and renaming it" — is built here on purpose and has to be caught.
        //----------------------------------------------------------------------
        beginTest ("the gate catches the padding the brief warns about");
        {
            struct Nudge { const char* preset; Param param; float by; const char* what; };
            const Nudge nudges[] =
            {
                { "Void Bloom",   Param::shapeDensity, 0.10f, "Density +0.10" },
                { "Carbon Bass",  Param::shapeMass,    0.10f, "Mass +0.10" },
                { "Metal Bloom",  Param::shapeTension, 0.08f, "Tension +0.08" },
                { "Dust Piano",   Param::spaceMix,     0.10f, "Space Mix +0.10" },
                { "Nebula Pad",   Param::shapeSurface, 0.12f, "Surface +0.12" }
            };

            auto engine = std::make_unique<SynthEngine>();
            engine->prepare (options.sampleRate, options.blockSize);
            SpectrumMeasurement spectrum (11);
            std::vector<float> left, right;
            juce::StringArray missed;

            for (const auto& nudge : nudges)
            {
                const int index = presets.findFactory (nudge.preset);
                if (index < 0) continue;

                auto padded = bank[(size_t) index];
                const size_t at = (size_t) paramIndex (nudge.param);
                const auto& desc = ParameterRegistry::get (nudge.param);
                padded.params[at] = juce::jlimit (desc.min, desc.max, padded.params[at] + nudge.by);

                renderPatch (*engine, padded, options, 60, true, &left, &right);
                const auto print = fingerprintOf (left, right, options.sampleRate, spectrum);
                const auto signature = signatureOf (padded);

                const auto parts = audioDistance (prints[(size_t) index], print);
                const float audio = parts.total;
                const float parameter = parameterDistance (bank[(size_t) index], padded,
                                                           signatures[(size_t) index], signature);
                logMessage (juce::String (nudge.preset).paddedRight (' ', 18) + juce::String (nudge.what).paddedRight (' ', 18)
                            + "audio " + juce::String (audio, 4) + " (" + juce::String (audio / kAudioGate, 2) + "x gate)"
                            + "   parameter " + juce::String (parameter, 4) + " (" + juce::String (parameter / kParamGate, 2) + "x gate)"
                            + "   [spectrum " + juce::String (parts.spectrum, 3)
                            + "  envelope " + juce::String (parts.envelope, 3)
                            + "  attack " + juce::String (parts.attack, 3)
                            + "  stereo " + juce::String (parts.stereo, 3)
                            + "  pitch " + juce::String (parts.pitch, 3) + "]");

                if (audio >= kAudioGate || parameter >= kParamGate)
                    missed.add (juce::String (nudge.preset) + " with " + nudge.what + " escaped the gate: audio "
                                + juce::String (audio, 4) + ", parameter " + juce::String (parameter, 4));
            }

            expect (missed.isEmpty(), "the gate is too loose to catch padding:\n   " + missed.joinIntoString ("\n   "));
        }
    }
};

static FactoryUniquenessTests factoryUniquenessTests;

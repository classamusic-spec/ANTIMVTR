#include <juce_core/juce_core.h>

#include "FactoryBankSampling.h"

#include "dev/diagnostics/SignalMetrics.h"
#include "presets/FactoryContent.h"
#include "state/ModRouting.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
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
// DISTANCE IN PARAMETER SPACE, ON THE BRIEF'S OWN AXES
//
// The brief does not ask for a scalar. It says two patches differ meaningfully
// when they differ in AT LEAST TWO of: source, material and structure,
// envelope shape, register and pitch behaviour, movement, space. So that is
// what is measured — six distances, one per axis — and the rule is the brief's
// rule rather than a number nobody can argue with.
//
// Measuring it this way is also what makes the gate usable on a bank full of
// struck instruments. Twenty-five marimbas necessarily share a source and an
// attack; a single weighted average buries the material pair and the topology
// that actually tell them apart under the dozens of parameters they are bound
// to have in common. Per axis, "same source, same envelope, different material
// and different space" reads as two axes and passes, which is correct.
//==============================================================================

enum class Axis { Source, Structure, Envelope, Register, Movement, Space, Count };
constexpr int kNumAxes = (int) Axis::Count;

const char* axisName (Axis a) noexcept
{
    switch (a)
    {
        case Axis::Source:    return "source";
        case Axis::Structure: return "material and structure";
        case Axis::Envelope:  return "envelope shape";
        case Axis::Register:  return "register and pitch";
        case Axis::Movement:  return "movement";
        case Axis::Space:     return "space";
        default:              return "?";
    }
}

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

/** Where a parameter sits and how much it counts there. Weight 0 means it is a
    trim: a real control, but not one that makes a patch a different patch. */
struct AxisWeight
{
    Axis  axis = Axis::Source;
    float weight = 0.0f;
};

AxisWeight axisOf (const ParamDesc& d, int sourceA, int sourceB) noexcept
{
    const Param p = d.param;

    // ---- register and pitch behaviour: keytrack, Matter pitch, magnet target
    switch (p)
    {
        case Param::shapeKeytrack: case Param::shapePitch:
        case Param::masterTranspose: case Param::masterGlide: case Param::masterMode:
        case Param::evolveMagnetTarget:
        case Param::waveOctave: case Param::waveSemi:
        case Param::dustPitch: case Param::samplePitch: case Param::sampleRoot:
        case Param::sampleKeytrack: case Param::fracturePitch:
            return { Axis::Register, 3.0f };
        default: break;
    }

    // ---- envelope shape
    switch (p)
    {
        case Param::ampAttack: case Param::ampDecay:
        case Param::ampSustain: case Param::ampRelease:  return { Axis::Envelope, 4.0f };
        case Param::ampCurve:                            return { Axis::Envelope, 1.0f };
        default: break;
    }

    // ---- the trims: audible, but not identity
    switch (p)
    {
        case Param::masterGain: case Param::masterFine: case Param::masterBendRange:
        case Param::masterVoices: case Param::masterQuality:
        case Param::wavePhase: case Param::wavePhaseRandom: case Param::waveFine:
        case Param::ampVelocity:
            return { Axis::Source, 0.0f };
        default: break;
    }

    // ---- source: which energy drives it, and how that source is set up
    {
        const auto group = d.group;
        const bool sourceGroup = group == ParamGroup::Wave || group == ParamGroup::Dust || group == ParamGroup::Impact
                              || group == ParamGroup::Sample || group == ParamGroup::Gesture;
        if (p == Param::sourceSelected) return { Axis::Source, 14.0f };
        if (p == Param::sourceMode)     return { Axis::Source, 6.0f };
        if (sourceGroup)
        {
            // A source that is not selected is not audible: it counts for nothing.
            if (group != groupForSource (sourceA) && group != groupForSource (sourceB))
                return { Axis::Source, 0.0f };
            if (p == Param::gestureMotion) return { Axis::Movement, 2.0f };
            return { Axis::Source, isSourceFlavour (p) ? 10.0f : 1.0f };
        }
    }

    // ---- movement: what modulates what, and how fast
    switch (p)
    {
        case Param::evolveSpeed: case Param::evolveMotion:      return { Axis::Movement, 2.0f };
        case Param::fractureRate: case Param::fractureDivision:
        case Param::fractureSteps: case Param::fractureSwing:
        case Param::fractureDirection: case Param::fractureSequence:
        case Param::fractureProbability:                        return { Axis::Movement, 1.5f };
        default: break;
    }

    // ---- material and structure: what it is made of and how it is wired
    switch (p)
    {
        case Param::shapeMaterialA: case Param::shapeMaterialB: return { Axis::Structure, 0.0f };  // scored as an unordered pair
        case Param::shapeTopology:                              return { Axis::Structure, 18.0f };
        default: break;
    }

    switch (d.group)
    {
        case ParamGroup::Shape:    return { Axis::Structure, d.mutation == MutationCategory::Chaos ? 0.5f : 2.0f };
        case ParamGroup::Evolve:   return { Axis::Structure, d.mutation == MutationCategory::Chaos ? 0.5f : 2.0f };
        case ParamGroup::Fracture: return { Axis::Structure, d.mutation == MutationCategory::Chaos ? 0.5f : 1.5f };
        case ParamGroup::Space:    return { Axis::Space, p == Param::spaceType ? 14.0f : 1.5f };
        case ParamGroup::Mod:      return { Axis::Movement, 1.0f };
        case ParamGroup::Macro:    return { Axis::Movement, 0.5f };
        default:                   return { Axis::Source, 0.0f };
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

    At 300 presets there are 45,150 pairs; normalising two hundred parameters
    and rebuilding the routing key set inside that loop would cost more than
    rendering the bank. Everything that depends on ONE patch is done here.
*/
struct Signature
{
    ParamValues normalised {};         ///< float params in the host curve, discrete params as-is
    std::vector<uint32_t> routings;    ///< sorted (source, target) keys
    int  source = 0;
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

/** One distance per axis, 0 (identical on that axis) .. 1 (nothing alike). */
struct AxisDistances
{
    std::array<float, kNumAxes> d {};
    float total = 0.0f;   ///< weighted mean across the axes, for ranking only
};

AxisDistances axisDistances (const PatchState& pa, const PatchState& pb,
                             const Signature& sa, const Signature& sb)
{
    std::array<double, kNumAxes> weighted {}, weights {};

    auto add = [&] (Axis axis, double w, double difference)
    {
        weighted[(size_t) axis] += w * difference;
        weights[(size_t) axis] += w;
    };

    for (const auto& desc : ParameterRegistry::all())
    {
        const auto aw = axisOf (desc, sa.source, sb.source);
        if (aw.weight <= 0.0f) continue;

        const size_t i = (size_t) paramIndex (desc.param);
        // Normalised, so a 20 ms difference in a 5 ms attack counts and the same
        // 20 ms in a four second release does not; discrete choices are all or nothing.
        const float difference = desc.kind == ParamKind::Float
                                   ? juce::jlimit (0.0f, 1.0f, std::abs (sa.normalised[i] - sb.normalised[i]))
                                   : (std::abs (sa.normalised[i] - sb.normalised[i]) > 0.5f ? 1.0f : 0.0f);
        add (aw.axis, aw.weight, difference);
    }

    // The material pair and the modulation matrix are not single parameters but
    // they are exactly what the brief means by "material and structure" and
    // "movement", so they are scored alongside them.
    // Weighted to matter. The identity choices on an axis — which materials,
    // which topology, which source, which Space, which routings — carry about as
    // much as every continuous knob on that axis put together. Averaged flat, a
    // completely different instrument (new material pair, new topology) scored
    // 0.31 on the structure axis because twenty similar trim knobs dragged it
    // down, which is precisely what made a bank of struck instruments look like
    // one patch: the things that tell a marimba from a thumb piano were diluted
    // by the things every struck patch has in common.
    add (Axis::Structure, 18.0, materialPairDistance (pa.params, pb.params));
    add (Axis::Movement,  10.0, routingDistance (sa, sb));

    AxisDistances out;
    double sum = 0.0;
    for (int i = 0; i < kNumAxes; ++i)
    {
        out.d[(size_t) i] = weights[(size_t) i] > 0.0 ? (float) (weighted[(size_t) i] / weights[(size_t) i]) : 0.0f;
        sum += out.d[(size_t) i];
    }
    out.total = (float) (sum / kNumAxes);
    return out;
}

//==============================================================================
// THE GATE
//
// Two measurements, and a pair has to be close on BOTH to fail.
//
//   * AUDIO is a veto. Its job is to let through the pairs that are built
//     alike and genuinely do not sound alike. It sits well above anything a
//     duplicate measures rather than being tuned tight: a single held note
//     cannot separate two hand-designed patches reliably enough to carry the
//     decision on its own, and pretending otherwise produces a gate that fails
//     honest work. At 0.150 it lets through the closest fifth of the bank.
//
//   * THE AXES are the rule, and it is the brief's rule: a pair has to differ
//     in at least two of source, material and structure, envelope shape,
//     register and pitch, movement, space. What "differ" means is measured
//     against kAxisScale — the typical distance between two patches on that
//     axis — because the axes have wildly different natural ranges. Two
//     patches are a median 0.585 apart on source and 0.077 apart on register;
//     one absolute threshold across all six would mean register never counts
//     and source almost always does.
//
//     The number the gate judges is therefore the SECOND LARGEST of the six
//     relative distances: "on its two most different axes, this pair is only
//     this fraction as different as a typical pair of patches". Second largest
//     and not largest, because the brief asks for two.
//
// WITHIN A CATEGORY IS STRICTER. Twenty-five plucks are all plucks; if a
// player auditions one and the one before it and cannot say what is different,
// one of them should not exist — and the one before it is the one in the same
// list. A marimba in KEYS and a marimba in PERCUSSION are allowed to be
// cousins, so across categories the bar is lower.
//
// CALIBRATION, against the real 300-patch bank. THERE IS NO GAP in the
// distribution: the second-largest relative distance runs smoothly from 0.11
// up through 1.0 with nothing that looks like a boundary, because the bank
// genuinely contains a dense neighbourhood of struck instruments. So the
// threshold is a policy, not a discovery, and it is set where the claim is
// safe rather than where it catches the most:
//
//     p0.05  0.32     p0.5  0.52     p1  0.66     p10  0.99     p50  1.16
//
// Below 0.30 a pair is less than a third as different, on its two most
// different axes, as a typical pair — that is a duplicate, and the gate fails.
// Between there and 0.60 it is a close neighbourhood, which is worth a human
// look but is not a failure, so those pairs are printed as a watch list. Set
// ANTIMATR_UNIQUENESS_DUMP=<file> to write every pair's distances as CSV and
// check any of this for yourself.
//==============================================================================
constexpr float kAudioGate          = 0.150f;   ///< above this a pair audibly differs, whatever it is made of
constexpr float kSameCategoryGate   = 0.30f;    ///< relative second-largest axis, inside one category
constexpr float kCrossCategoryGate  = 0.20f;    ///< …and across two
constexpr float kSameCategoryWatch  = 0.60f;    ///< below this, worth a look but not a failure
constexpr float kCrossCategoryWatch = 0.40f;

/** Typical distance between two patches on each axis, measured on the 300-patch
    bank. The axes have very different natural ranges, so this is what makes
    them comparable. The test checks the bank has not drifted away from these. */
constexpr float kAxisScale[kNumAxes] = { 0.585f, 0.377f, 0.210f, 0.077f, 0.144f, 0.307f };

/** The second largest relative axis distance: how different the pair is on the
    two axes where it is most different, against a typical pair. */
float relativeSecondAxis (const AxisDistances& d) noexcept
{
    std::array<float, kNumAxes> rel {};
    for (int i = 0; i < kNumAxes; ++i)
        rel[(size_t) i] = d.d[(size_t) i] / juce::jmax (1.0e-4f, kAxisScale[i]);
    std::sort (rel.begin(), rel.end(), std::greater<float>());
    return rel[1];
}

/** Every axis, named, with how different this pair is on it against a typical
    pair — so a failure says exactly which axes were left alone. */
juce::String axisReport (const AxisDistances& d)
{
    juce::StringArray parts;
    std::vector<std::pair<float, int>> order;
    for (int i = 0; i < kNumAxes; ++i)
        order.push_back ({ d.d[(size_t) i] / juce::jmax (1.0e-4f, kAxisScale[i]), i });
    std::sort (order.begin(), order.end(), std::greater<std::pair<float, int>>());

    for (const auto& [relative, axis] : order)
        parts.add (juce::String (axisName ((Axis) axis)) + " " + juce::String (relative, 2) + "x");
    return parts.joinIntoString (", ");
}

/** The concrete things two patches have in common, for the failure message. */
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
/** One compared pair, kept for the report. */
struct Pair
{
    int a = -1, b = -1;
    float audio = 1.0f;
    AxisDistances axes;
    float second = 1.0f;         ///< second largest relative axis distance
    bool sameCategory = false;

    float gate() const noexcept  { return sameCategory ? kSameCategoryGate : kCrossCategoryGate; }
    float watch() const noexcept { return sameCategory ? kSameCategoryWatch : kCrossCategoryWatch; }

    bool duplicate() const noexcept { return audio < kAudioGate && second < gate(); }
    bool worthALook() const noexcept { return ! duplicate() && audio < kAudioGate && second < watch(); }

    /** How far outside the gate the pair sits; below 1 is a failure. Reported so
        the headroom of the whole bank is visible rather than guessed at. */
    float margin() const noexcept { return juce::jmax (audio / kAudioGate, second / gate()); }
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
        std::vector<Pair> pairs;
        pairs.reserve ((size_t) count * (size_t) count / 2);
        juce::StringArray duplicates;

        for (int a = 0; a < count; ++a)
        {
            for (int b = a + 1; b < count; ++b)
            {
                // "Init" is the blank starting point, not a patch competing for shelf space.
                if (prints[(size_t) a].category == "INIT" || prints[(size_t) b].category == "INIT") continue;

                Pair p;
                p.a = a; p.b = b;
                p.audio = audioDistance (prints[(size_t) a], prints[(size_t) b]).total;
                p.axes = axisDistances (bank[(size_t) a], bank[(size_t) b],
                                        signatures[(size_t) a], signatures[(size_t) b]);
                p.second = relativeSecondAxis (p.axes);
                p.sameCategory = prints[(size_t) a].category == prints[(size_t) b].category;
                pairs.push_back (p);

                if (p.duplicate())
                {
                    const auto& pa = bank[(size_t) a];
                    const auto& pb = bank[(size_t) b];
                    duplicates.add (pa.meta.name + " (" + pa.meta.category + ")  <->  " + pb.meta.name + " (" + pb.meta.category + ")"
                                    + "\n      on the two axes where they differ most they are only "
                                    + juce::String (p.second, 2) + "x as far apart as a typical pair"
                                    + (p.sameCategory ? " — and they are in the same category, where the bar is "
                                                      : " — across categories the bar is ")
                                    + juce::String (p.gate(), 2) + "x"
                                    + "\n      axis by axis, against a typical pair: " + axisReport (p.axes)
                                    + "\n      audio distance " + juce::String (p.audio, 4) + " (not audibly distinct; the veto lets go at "
                                    + juce::String (kAudioGate, 3) + ")"
                                    + "\n      they share: " + sharedGround (pa, pb, signatures[(size_t) a], signatures[(size_t) b],
                                                                             prints[(size_t) a], prints[(size_t) b])
                                    + "\n      the brief asks for a real difference in at least two of: source, material and"
                                      " structure, envelope shape, register and pitch, movement, space. The axes at the"
                                      " right-hand end of that list are the ones you left alone — change two of them.");
                }
            }
        }

        // ---- the distance distribution, so the calibration can be checked
        {
            std::vector<Pair> sorted = pairs;
            std::sort (sorted.begin(), sorted.end(),
                       [] (const Pair& x, const Pair& y) { return x.margin() < y.margin(); });

            auto percentile = [&] (double q, auto pick)
            {
                std::vector<float> values;
                values.reserve (pairs.size());
                for (const auto& p : pairs) values.push_back (pick (p));
                std::sort (values.begin(), values.end());
                const size_t at = (size_t) juce::jlimit (0.0, (double) values.size() - 1, q * (double) values.size());
                return values.empty() ? 0.0f : values[at];
            };

            int sameCategoryPairs = 0;
            for (const auto& p : pairs) if (p.sameCategory) ++sameCategoryPairs;

            logMessage ("uniqueness: " + juce::String ((int) pairs.size()) + " pairs compared ("
                        + juce::String (sameCategoryPairs) + " inside a category), " + watch.elapsed() + " total");
            logMessage ("audio distance:  p0 " + juce::String (percentile (0.0, [] (const Pair& p) { return p.audio; }), 4)
                        + "  p1 " + juce::String (percentile (0.01, [] (const Pair& p) { return p.audio; }), 4)
                        + "  p50 " + juce::String (percentile (0.5, [] (const Pair& p) { return p.audio; }), 4)
                        + "   (gate " + juce::String (kAudioGate, 3) + ")");
            for (int axis = 0; axis < kNumAxes; ++axis)
                logMessage (juce::String ("axis ") + juce::String (axisName ((Axis) axis)).paddedRight (' ', 24)
                            + "p0 " + juce::String (percentile (0.0, [axis] (const Pair& p) { return p.axes.d[(size_t) axis]; }), 3)
                            + "  p1 " + juce::String (percentile (0.01, [axis] (const Pair& p) { return p.axes.d[(size_t) axis]; }), 3)
                            + "  p50 " + juce::String (percentile (0.5, [axis] (const Pair& p) { return p.axes.d[(size_t) axis]; }), 3)
                            + "  p99 " + juce::String (percentile (0.99, [axis] (const Pair& p) { return p.axes.d[(size_t) axis]; }), 3));
            logMessage ("second-largest relative axis (the number the gate judges):"
                        "  p0.05 " + juce::String (percentile (0.0005, [] (const Pair& p) { return p.second; }), 3)
                        + "  p0.5 " + juce::String (percentile (0.005, [] (const Pair& p) { return p.second; }), 3)
                        + "  p1 " + juce::String (percentile (0.01, [] (const Pair& p) { return p.second; }), 3)
                        + "  p10 " + juce::String (percentile (0.10, [] (const Pair& p) { return p.second; }), 3)
                        + "  p50 " + juce::String (percentile (0.50, [] (const Pair& p) { return p.second; }), 3)
                        + "   (gate " + juce::String (kSameCategoryGate, 2) + " inside a category, "
                        + juce::String (kCrossCategoryGate, 2) + " across)");

            // The axis scales are constants measured on the bank. If the bank
            // drifts away from them the gate quietly changes meaning, so say so.
            juce::StringArray drifted;
            for (int axis = 0; axis < kNumAxes; ++axis)
            {
                const float median = percentile (0.50, [axis] (const Pair& p) { return p.axes.d[(size_t) axis]; });
                const float ratio = median / juce::jmax (1.0e-4f, kAxisScale[axis]);
                if (ratio < 0.6f || ratio > 1.6f)
                    drifted.add (juce::String (axisName ((Axis) axis)) + " (typical distance " + juce::String (median, 3)
                                 + ", calibrated at " + juce::String (kAxisScale[axis], 3) + ")");
            }

            logMessage ("the ten closest pairs in the bank (1.00 = on the gate):");
            for (size_t i = 0; i < sorted.size() && i < 10; ++i)
            {
                const auto& p = sorted[i];
                logMessage ("   " + juce::String (p.margin(), 2) + "x   "
                            + prints[(size_t) p.a].name.paddedRight (' ', 20) + prints[(size_t) p.b].name.paddedRight (' ', 20)
                            + (p.sameCategory ? "same cat  " : "cross cat ")
                            + "audio " + juce::String (p.audio, 4)
                            + "   second axis " + juce::String (p.second, 2) + "x"
                            + "   [" + axisReport (p.axes) + "]");
            }

            // Not failures: the neighbourhood just below the line, so a dense
            // corner of the bank is visible before it becomes a problem.
            int watched = 0;
            juce::StringArray watch;
            for (const auto& p : sorted)
            {
                if (! p.worthALook()) continue;
                ++watched;
                if (watch.size() < 12)
                    watch.add (prints[(size_t) p.a].name + " / " + prints[(size_t) p.b].name
                               + " " + juce::String (p.second, 2) + "x"
                               + (p.sameCategory ? "" : " (across categories)"));
            }
            logMessage ("close neighbours, not failures — " + juce::String (watched) + " pairs, closest first:");
            for (const auto& line : watch) logMessage ("   " + line);
            if (watched > watch.size()) logMessage ("   … and " + juce::String (watched - watch.size()) + " more");

            // Which patches the failures cluster on. Ten failing pairs are rarely
            // ten problems — more often four patches that need pulling apart.
            std::map<juce::String, int> appearances;
            int failing = 0;
            for (const auto& p : pairs)
                if (p.duplicate())
                {
                    ++failing;
                    ++appearances[prints[(size_t) p.a].name];
                    ++appearances[prints[(size_t) p.b].name];
                }
            if (failing > 0)
            {
                std::vector<std::pair<int, juce::String>> ranked;
                for (const auto& [name, n] : appearances) ranked.push_back ({ n, name });
                std::sort (ranked.begin(), ranked.end(), std::greater<std::pair<int, juce::String>>());
                juce::StringArray worst;
                for (const auto& [n, name] : ranked)
                    worst.add (name + " (" + juce::String (n) + ")");
                logMessage ("the failures involve " + juce::String ((int) ranked.size()) + " patches, most often: "
                            + worst.joinIntoString (", "));
            }
            for (const auto& p : sorted)
                if (! p.duplicate())
                {
                    logMessage ("headroom: the closest pair the gate passes sits " + juce::String (p.margin(), 2)
                                + "x outside it (" + prints[(size_t) p.a].name + " / " + prints[(size_t) p.b].name + ")");
                    break;
                }
            logMessage (juce::String (failing) + " pair(s) fail the gate, " + juce::String (watched)
                        + " more are close enough to be worth a listen");

            expect (drifted.isEmpty(), "the bank has drifted away from the axis scales the gate is calibrated on — "
                                       "recalibrate kAxisScale before trusting it:\n   " + drifted.joinIntoString ("\n   "));

            // The whole distribution, for offline work on the weighting.
            const auto dump = juce::SystemStats::getEnvironmentVariable ("ANTIMATR_UNIQUENESS_DUMP", juce::String()).trim();
            if (dump.isNotEmpty())
            {
                juce::String csv ("a,aCategory,b,bCategory,sameCategory,audio,source,structure,envelope,register,movement,space,second\n");
                for (const auto& p : pairs)
                {
                    csv << prints[(size_t) p.a].name.replace (",", " ") << "," << prints[(size_t) p.a].category << ","
                        << prints[(size_t) p.b].name.replace (",", " ") << "," << prints[(size_t) p.b].category << ","
                        << (p.sameCategory ? 1 : 0) << "," << juce::String (p.audio, 5);
                    for (int axis = 0; axis < kNumAxes; ++axis) csv << "," << juce::String (p.axes.d[(size_t) axis], 5);
                    csv << "," << juce::String (p.second, 5) << "\n";
                }
                juce::File (dump).replaceWithText (csv);
                logMessage ("wrote the whole distribution to " + dump);
            }
        }

        expect (duplicates.isEmpty(),
                juce::String (duplicates.size()) + " near-duplicate pair(s) in the factory bank:\n   "
                + duplicates.joinIntoString ("\n   "));

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

                const float audio = audioDistance (prints[(size_t) index], print).total;
                const auto axes = axisDistances (bank[(size_t) index], padded,
                                                 signatures[(size_t) index], signature);
                const float second = relativeSecondAxis (axes);

                logMessage (juce::String (nudge.preset).paddedRight (' ', 18) + juce::String (nudge.what).paddedRight (' ', 18)
                            + "audio " + juce::String (audio, 4) + " (" + juce::String (audio / kAudioGate, 2) + "x the veto)"
                            + "   second axis " + juce::String (second, 3) + "x (gate "
                            + juce::String (kSameCategoryGate, 2) + "x)");

                if (audio >= kAudioGate || second >= kSameCategoryGate)
                    missed.add (juce::String (nudge.preset) + " with " + nudge.what + " escaped the gate: audio "
                                + juce::String (audio, 4) + ", second axis " + juce::String (second, 3) + "x");
            }

            expect (missed.isEmpty(), "the gate is too loose to catch padding:\n   " + missed.joinIntoString ("\n   "));
        }
    }
};

static FactoryUniquenessTests factoryUniquenessTests;

#include "SampleAnalyzer.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <numeric>

namespace am
{

namespace
{
    /** Parabolic interpolation of three log-magnitudes around a peak.
        Returns the offset in bins (-0.5..0.5) and the interpolated magnitude. */
    void refinePeak (float ym1, float y0, float yp1, float& offsetBins, float& magnitude) noexcept
    {
        const float a = std::log (juce::jmax (1.0e-12f, ym1));
        const float b = std::log (juce::jmax (1.0e-12f, y0));
        const float c = std::log (juce::jmax (1.0e-12f, yp1));
        const float den = a - 2.0f * b + c;
        offsetBins = std::abs (den) > 1.0e-12f ? 0.5f * (a - c) / den : 0.0f;
        offsetBins = juce::jlimit (-0.5f, 0.5f, offsetBins);
        magnitude = std::exp (b - 0.25f * (a - c) * offsetBins);
    }

    struct Peak
    {
        float hz = 0.0f;
        float magnitude = 0.0f;
        int   bin = 0;
        float t60 = 1.0f;
    };
}

//==============================================================================
juce::var PartialTable::toVar() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("source", source);
    obj->setProperty ("fundamentalHz", fundamentalHz);
    obj->setProperty ("attackSeconds", attackSeconds);
    obj->setProperty ("noiseFloorDb", noiseFloorDb);
    obj->setProperty ("harmonicity", harmonicity);

    juce::Array<juce::var> list;
    for (int i = 0; i < count; ++i)
    {
        auto* p = new juce::DynamicObject();
        p->setProperty ("ratio", partials[(size_t) i].ratio);
        p->setProperty ("weight", partials[(size_t) i].weight);
        p->setProperty ("t60", partials[(size_t) i].t60);
        list.add (juce::var (p));
    }
    obj->setProperty ("partials", list);
    return juce::var (obj);
}

PartialTable PartialTable::fromVar (const juce::var& v)
{
    PartialTable t;
    if (auto* obj = v.getDynamicObject())
    {
        t.source        = obj->getProperty ("source").toString();
        t.fundamentalHz = (float) (double) obj->getProperty ("fundamentalHz");
        t.attackSeconds = (float) (double) obj->getProperty ("attackSeconds");
        t.noiseFloorDb  = (float) (double) obj->getProperty ("noiseFloorDb");
        t.harmonicity   = (float) (double) obj->getProperty ("harmonicity");

        if (auto* list = obj->getProperty ("partials").getArray())
        {
            for (const auto& item : *list)
            {
                if (t.count >= kMaxPartials) break;
                if (auto* p = item.getDynamicObject())
                {
                    Partial partial;
                    partial.ratio  = juce::jlimit (0.01f, 200.0f, (float) (double) p->getProperty ("ratio"));
                    partial.weight = juce::jlimit (0.0f, 1.0f, (float) (double) p->getProperty ("weight"));
                    partial.t60    = juce::jlimit (0.005f, 60.0f, (float) (double) p->getProperty ("t60"));
                    t.partials[(size_t) t.count++] = partial;
                }
            }
        }
    }
    return t;
}

//==============================================================================
PartialTable SampleAnalyzer::analyse (const SampleData& sample)
{
    return analyse (sample, Options {});
}

PartialTable SampleAnalyzer::analyse (const SampleData& sample, const Options& options)
{
    PartialTable table;
    table.source = sample.name;
    if (sample.isEmpty()) return table;

    const double sr = sample.sampleRate > 0.0 ? sample.sampleRate : 48000.0;
    const int order = juce::jlimit (10, 15, options.fftOrder);
    const int fftSize = 1 << order;
    if (sample.numFrames < fftSize / 2) return table;

    // ---- mono mixdown
    std::vector<float> mono ((size_t) sample.numFrames, 0.0f);
    const float* a = sample.channel (0);
    const float* b = sample.numChannels > 1 ? sample.channel (1) : nullptr;
    for (int i = 0; i < sample.numFrames; ++i)
        mono[(size_t) i] = b != nullptr ? 0.5f * (a[i] + b[i]) : a[i];

    // ---- onset: first sample above a tenth of the peak, backed off 5 ms
    float peak = 0.0f;
    for (float v : mono) peak = juce::jmax (peak, std::abs (v));
    if (peak <= 1.0e-6f) return table;

    int onset = 0;
    for (int i = 0; i < sample.numFrames; ++i)
        if (std::abs (mono[(size_t) i]) > 0.1f * peak) { onset = i; break; }
    onset = juce::jmax (0, onset - (int) (0.005 * sr));

    // ---- attack time: onset to the loudest 1 ms window
    {
        const int win = juce::jmax (8, (int) (0.001 * sr));
        float best = 0.0f;
        int bestAt = onset;
        for (int i = onset; i + win < sample.numFrames; i += win)
        {
            float m = 0.0f;
            for (int k = 0; k < win; ++k) m = juce::jmax (m, std::abs (mono[(size_t) (i + k)]));
            if (m > best) { best = m; bestAt = i; }
            if (m < best * 0.5f && i > bestAt + 20 * win) break;
        }
        table.attackSeconds = (float) ((double) (bestAt - onset) / sr);
    }

    // ---- STFT of the window after the onset
    juce::dsp::FFT fft (order);
    juce::dsp::WindowingFunction<float> window ((size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, false);
    const int hop = fftSize / 4;
    const int available = sample.numFrames - onset;
    const int wanted = juce::jmin (available, (int) (options.maxSeconds * sr));
    const int numFrames = juce::jmax (1, (wanted - fftSize) / hop + 1);
    if (wanted < fftSize) return table;

    const int numBins = fftSize / 2;
    std::vector<std::vector<float>> frames ((size_t) numFrames);
    std::vector<float> scratch ((size_t) fftSize * 2, 0.0f);

    for (int f = 0; f < numFrames; ++f)
    {
        const int start = onset + f * hop;
        std::fill (scratch.begin(), scratch.end(), 0.0f);
        for (int i = 0; i < fftSize; ++i)
            scratch[(size_t) i] = mono[(size_t) juce::jmin (sample.numFrames - 1, start + i)];
        window.multiplyWithWindowingTable (scratch.data(), (size_t) fftSize);
        fft.performFrequencyOnlyForwardTransform (scratch.data(), true);
        frames[(size_t) f].assign (scratch.begin(), scratch.begin() + numBins);
    }

    // ---- attack spectrum: the loudest of the first frames (a strike settles fast)
    std::vector<float> attack ((size_t) numBins, 0.0f);
    const int attackFrames = juce::jmin (numFrames, 3);
    for (int f = 0; f < attackFrames; ++f)
        for (int k = 0; k < numBins; ++k)
            attack[(size_t) k] = juce::jmax (attack[(size_t) k], frames[(size_t) f][(size_t) k]);

    float maxMag = 0.0f;
    for (float v : attack) maxMag = juce::jmax (maxMag, v);
    if (maxMag <= 1.0e-9f) return table;

    // ---- noise floor: the median bin of the attack spectrum
    {
        std::vector<float> sorted (attack.begin() + 1, attack.end());
        std::nth_element (sorted.begin(), sorted.begin() + (long) sorted.size() / 2, sorted.end());
        const float median = sorted[sorted.size() / 2];
        table.noiseFloorDb = juce::Decibels::gainToDecibels (median / maxMag, -120.0f);
    }

    // ---- peak picking
    const double binHz = sr / (double) fftSize;
    const float threshold = maxMag * std::pow (10.0f, juce::jlimit (-90.0f, -6.0f, options.floorDb) / 20.0f);
    const int kMin = juce::jmax (1, (int) (options.minHz / binHz));
    const int kMax = juce::jmin (numBins - 2, (int) (options.maxHz / binHz));

    std::vector<Peak> peaks;
    peaks.reserve (256);
    for (int k = kMin; k <= kMax; ++k)
    {
        const float y = attack[(size_t) k];
        if (y < threshold) continue;
        if (y < attack[(size_t) (k - 1)] || y < attack[(size_t) (k + 1)]) continue;

        float offset = 0.0f, magnitude = y;
        refinePeak (attack[(size_t) (k - 1)], y, attack[(size_t) (k + 1)], offset, magnitude);
        Peak p;
        p.bin = k;
        p.hz = (float) (((double) k + (double) offset) * binHz);
        p.magnitude = magnitude;
        if (p.hz >= options.minHz && p.hz <= options.maxHz) peaks.push_back (p);
        k += 1;   // a Hann main lobe is 4 bins wide: never pick its shoulder
    }
    if (peaks.empty()) return table;

    // ---- fundamental: the lowest peak that carries real weight, checked for a
    //      stronger sub-octave (a struck body often hides its fundamental).
    float strongest = 0.0f;
    for (const auto& p : peaks) strongest = juce::jmax (strongest, p.magnitude);
    float fundamental = 0.0f;
    for (const auto& p : peaks)
        if (p.magnitude >= 0.18f * strongest) { fundamental = p.hz; break; }
    if (fundamental <= 0.0f) fundamental = peaks.front().hz;

    for (const auto& p : peaks)
    {
        if (p.hz > fundamental * 0.62f) break;
        if (p.magnitude >= 0.06f * strongest && p.hz > options.minHz) { fundamental = p.hz; break; }
    }
    table.fundamentalHz = fundamental;

    // ---- per-partial T60 from the decay of its bin across the frames
    const double frameSeconds = (double) hop / sr;
    for (auto& p : peaks)
    {
        int bestFrame = 0;
        float bestMag = 0.0f;
        std::vector<float> track ((size_t) numFrames, 0.0f);
        for (int f = 0; f < numFrames; ++f)
        {
            float m = 0.0f;
            for (int d = -1; d <= 1; ++d)
            {
                const int k = juce::jlimit (0, numBins - 1, p.bin + d);
                m = juce::jmax (m, frames[(size_t) f][(size_t) k]);
            }
            track[(size_t) f] = m;
            if (m > bestMag) { bestMag = m; bestFrame = f; }
        }

        // Least squares fit of dB against time from the peak frame down to -40 dB.
        double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
        int n = 0;
        const float peakDb = juce::Decibels::gainToDecibels (bestMag, -160.0f);
        for (int f = bestFrame; f < numFrames; ++f)
        {
            const float db = juce::Decibels::gainToDecibels (track[(size_t) f], -160.0f);
            if (db < peakDb - 40.0f || db < peakDb + juce::jmin (-3.0f, table.noiseFloorDb)) break;
            const double x = (double) (f - bestFrame) * frameSeconds;
            sx += x; sy += db; sxx += x * x; sxy += x * db;
            ++n;
        }
        if (n >= 3)
        {
            const double den = (double) n * sxx - sx * sx;
            const double slope = std::abs (den) > 1.0e-12 ? ((double) n * sxy - sx * sy) / den : 0.0;   // dB per second
            p.t60 = slope < -0.5 ? (float) (-60.0 / slope) : 20.0f;
        }
        else
        {
            p.t60 = (float) juce::jmax (0.02, (double) (numFrames - bestFrame) * frameSeconds);
        }
        p.t60 = juce::jlimit (0.01f, 30.0f, p.t60);
    }

    // ---- strongest partials first, fundamental always at index 0
    std::stable_sort (peaks.begin(), peaks.end(), [] (const Peak& x, const Peak& y) { return x.magnitude > y.magnitude; });
    const int wantedPartials = juce::jlimit (1, PartialTable::kMaxPartials, options.maxPartials);
    if ((int) peaks.size() > wantedPartials) peaks.resize ((size_t) wantedPartials);

    auto fundamentalIt = std::find_if (peaks.begin(), peaks.end(),
                                       [fundamental] (const Peak& p) { return std::abs (p.hz - fundamental) < 0.01f * fundamental; });
    if (fundamentalIt != peaks.end() && fundamentalIt != peaks.begin())
        std::rotate (peaks.begin(), fundamentalIt, fundamentalIt + 1);

    const float norm = 1.0f / juce::jmax (1.0e-9f, strongest);
    double harmonicDeviation = 0.0, weightSum = 0.0;
    for (const auto& p : peaks)
    {
        if (table.count >= PartialTable::kMaxPartials) break;
        PartialTable::Partial partial;
        partial.ratio  = juce::jlimit (0.01f, 200.0f, p.hz / juce::jmax (1.0f, fundamental));
        partial.weight = juce::jlimit (0.0f, 1.0f, p.magnitude * norm);
        partial.t60    = p.t60;
        table.partials[(size_t) table.count++] = partial;

        if (partial.ratio <= 16.0f)
        {
            const float nearest = juce::jmax (1.0f, std::round (partial.ratio));
            harmonicDeviation += (double) partial.weight * (double) std::abs (partial.ratio - nearest) / (double) nearest;
            weightSum += (double) partial.weight;
        }
    }
    table.harmonicity = weightSum > 0.0 ? juce::jlimit (0.0f, 1.0f, (float) (harmonicDeviation / weightSum) * 8.0f) : 0.0f;

    return table;
}

//==============================================================================
SampleAnalyzer::MatterFit SampleAnalyzer::fitShape (const PartialTable& table)
{
    MatterFit fit;
    if (! table.isValid()) return fit;

    // FORM walks harmonic → stretched → membrane → metallic → inharmonic → …
    fit.form = juce::jlimit (0.0f, 0.85f, table.harmonicity * 0.9f);

    // TENSION: how much the partials are stretched away from an integer series.
    double expansion = 0.0, weight = 0.0;
    for (int i = 0; i < table.count; ++i)
    {
        const auto& p = table.partials[(size_t) i];
        if (p.ratio < 1.5f || p.ratio > 24.0f) continue;
        const float nearest = juce::jmax (2.0f, std::round (p.ratio));
        expansion += (double) p.weight * (double) (p.ratio / nearest);
        weight += (double) p.weight;
    }
    const float meanExpansion = weight > 0.0 ? (float) (expansion / weight) : 1.0f;
    fit.tension = juce::jlimit (0.0f, 1.0f, 0.5f + (meanExpansion - 1.0f) * 4.0f);

    // DECAY: Matter's ring time is t60 = 0.05 * 2^(decay * 8.6) seconds.
    double t60Sum = 0.0, t60Weight = 0.0;
    for (int i = 0; i < table.count; ++i)
    {
        const auto& p = table.partials[(size_t) i];
        t60Sum += (double) p.weight * (double) p.t60;
        t60Weight += (double) p.weight;
    }
    const float meanT60 = t60Weight > 0.0 ? (float) (t60Sum / t60Weight) : 1.0f;
    fit.decay = juce::jlimit (0.0f, 1.0f, std::log2 (juce::jmax (0.05f, meanT60) / 0.05f) / 8.6f);

    // MASS from the spectral tilt: weight ~ ratio^(-tilt). Regression in log-log.
    double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    int n = 0;
    for (int i = 0; i < table.count; ++i)
    {
        const auto& p = table.partials[(size_t) i];
        if (p.ratio <= 1.02f || p.weight <= 1.0e-4f) continue;
        const double x = std::log ((double) p.ratio);
        const double y = std::log ((double) p.weight);
        sx += x; sy += y; sxx += x * x; sxy += x * y;
        ++n;
    }
    float tilt = 1.0f;
    if (n >= 3)
    {
        const double den = (double) n * sxx - sx * sx;
        if (std::abs (den) > 1.0e-12) tilt = (float) -(((double) n * sxy - sx * sy) / den);
    }
    fit.mass = juce::jlimit (0.0f, 1.0f, 0.10f + 0.30f * juce::jlimit (0.0f, 3.0f, tilt));

    // DENSITY from how many partials carry the sound.
    int significant = 0;
    for (int i = 0; i < table.count; ++i)
        if (table.partials[(size_t) i].weight > 0.05f) ++significant;
    fit.density = juce::jlimit (0.05f, 1.0f, (float) significant / 40.0f);
    fit.distribution = juce::jlimit (0.0f, 1.0f, 0.35f + 0.4f * table.harmonicity);
    return fit;
}

} // namespace am

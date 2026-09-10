#include "WavetableGenerator.h"
#include "core/Random.h"

#include <juce_dsp/juce_dsp.h>

namespace am
{

namespace
{
    constexpr float kPiF    = 3.14159265358979f;
    constexpr float kTwoPiF = 6.28318530717959f;

    using Cplx = std::complex<float>;

    /** Complex harmonic bins: x(t) = sum_k 2 * Re (bin[k] * e^(i 2 pi k t)). */
    struct FrameSpectrum
    {
        std::array<Cplx, kWaveMaxHarmonics + 1> bin {};

        void clear() noexcept { bin.fill (Cplx (0.0f, 0.0f)); }

        /** Adds amplitude `a` at harmonic k with phase `ph` (radians, cosine reference). */
        void add (int k, float a, float ph) noexcept
        {
            if (k < 1 || k > kWaveMaxHarmonics) return;
            bin[(size_t) k] += Cplx (0.5f * a * std::cos (ph), 0.5f * a * std::sin (ph));
        }

        /** Sine-phase harmonic (the usual convention for saw / square / pulse). */
        void addSine (int k, float a) noexcept { add (k, a, -kPiF * 0.5f); }
    };

    //==========================================================================
    // Time-domain helpers

    /** Triangular wavefolder: identity on [-1, 1], reflecting beyond. */
    inline float triFold (float x) noexcept
    {
        const float u = (x + 1.0f) * 0.25f;
        return 4.0f * std::abs (u - std::floor (u + 0.5f)) - 1.0f;
    }

    inline float quantise (float x, float levels) noexcept
    {
        return levels < 1.5f ? (x < 0.0f ? -1.0f : 1.0f) : std::round (x * levels) / levels;
    }

    //==========================================================================
    // BANK 0 — BASIC: sine -> triangle -> saw -> square -> narrowing pulses.

    void basicBins (int frame, FrameSpectrum& s)
    {
        auto sineWave = [&s] (float g)
        {
            s.addSine (1, g);
        };

        auto triangleWave = [&s] (float g)
        {
            for (int k = 1; k <= kWaveMaxHarmonics; k += 2)
            {
                const float sign = (((k - 1) / 2) & 1) ? -1.0f : 1.0f;
                s.addSine (k, g * sign * 8.0f / (kPiF * kPiF) / (float) (k * k));
            }
        };

        auto sawWave = [&s] (float g)
        {
            for (int k = 1; k <= kWaveMaxHarmonics; ++k)
                s.addSine (k, g / (float) k);
        };

        // Pulse as the difference of two saws: smooth in duty and identical to
        // a square at duty 0.5, so blending across frames stays continuous.
        auto pulseWave = [&s] (float g, float duty)
        {
            for (int k = 1; k <= kWaveMaxHarmonics; ++k)
            {
                const float ang = kTwoPiF * (float) k * duty;
                const Cplx d = Cplx (1.0f, 0.0f) - Cplx (std::cos (-ang), std::sin (-ang));
                const float mag = std::abs (d);
                if (mag < 1.0e-7f) continue;
                s.add (k, g * mag / (float) k, std::arg (d) - kPiF * 0.5f);
            }
        };

        switch (frame)
        {
            case 0:  sineWave (1.0f); break;
            case 1:  sineWave (0.5f); triangleWave (0.5f); break;
            case 2:  triangleWave (1.0f); break;
            case 3:  triangleWave (0.67f); sawWave (0.33f); break;
            case 4:  triangleWave (0.33f); sawWave (0.67f); break;
            case 5:  sawWave (1.0f); break;
            case 6:  sawWave (0.67f); pulseWave (0.33f, 0.5f); break;
            case 7:  sawWave (0.33f); pulseWave (0.67f, 0.5f); break;
            case 8:  pulseWave (1.0f, 0.5f);  break;
            case 9:  pulseWave (1.0f, 0.42f); break;
            case 10: pulseWave (1.0f, 0.34f); break;
            case 11: pulseWave (1.0f, 0.27f); break;
            case 12: pulseWave (1.0f, 0.20f); break;
            case 13: pulseWave (1.0f, 0.14f); break;
            case 14: pulseWave (1.0f, 0.09f); break;
            default: pulseWave (1.0f, 0.05f); break;
        }
    }

    //==========================================================================
    // BANK 1 — HARMONIC: odd series (frames 0-7), even+fundamental (8-15),
    // each with a rolloff sweeping from steep (hollow) to shallow (bright).

    void harmonicBins (int frame, FrameSpectrum& s)
    {
        const bool odd = frame < 8;
        const int  i = odd ? frame : frame - 8;
        const float e = 2.2f - 0.1857f * (float) i;      // 2.2 .. 0.9

        for (int k = 1; k <= kWaveMaxHarmonics; ++k)
        {
            const bool keep = odd ? ((k & 1) == 1) : (k == 1 || (k & 1) == 0);
            if (! keep) continue;
            // The gentle exponential tail keeps the brightest frames musical
            // instead of turning them into impulse trains.
            s.addSine (k, std::pow ((float) k, -e) * std::exp (-(float) k / 300.0f));
        }
    }

    //==========================================================================
    // BANK 2 — FORMANT: saw-like source shaped by three log-Gaussian formants
    // sweeping A -> E -> I -> O -> U (and back) across the frames.

    void formantBins (int frame, FrameSpectrum& s)
    {
        static const float vowels[5][3] =
        {
            {  7.0f, 18.0f, 30.0f },   // A
            {  4.0f, 32.0f, 45.0f },   // E
            {  3.0f, 44.0f, 62.0f },   // I
            {  5.0f, 11.0f, 28.0f },   // O
            {  4.0f,  8.0f, 24.0f }    // U
        };

        const float t = (float) frame / (float) (kWaveFramesPerBank - 1) * 5.0f;   // 0..5
        const int   a = juce::jlimit (0, 4, (int) t);
        const int   b = (a + 1) % 5;
        const float m = t - (float) a;

        float f[3];
        for (int j = 0; j < 3; ++j)
            f[j] = std::exp2 (lerp (std::log2 (vowels[a][j]), std::log2 (vowels[b][j]), m));

        const float sigma[3] = { 0.34f, 0.42f, 0.58f };
        const float gain[3]  = { 1.0f, 0.72f, 0.38f };

        for (int k = 1; k <= kWaveMaxHarmonics; ++k)
        {
            const float lk = std::log2 ((float) k);
            float res = 0.03f;                                  // breathy floor
            for (int j = 0; j < 3; ++j)
            {
                const float d = (lk - std::log2 (f[j])) / sigma[j];
                res += gain[j] * std::exp (-0.5f * d * d);
            }
            const float src = 1.0f / (float) k;                 // glottal-ish source
            const float amp = src * res * std::exp (-(float) k / 320.0f);
            if (amp > 1.0e-5f) s.addSine (k, amp);
        }
    }

    //==========================================================================
    // BANK 3 — FOLDED (time domain): a sine driven into a triangular
    // wavefolder with growing depth and asymmetry.

    void foldedWave (int frame, float* buf, int n)
    {
        const float u = (float) frame / (float) (kWaveFramesPerBank - 1);
        const float drive = std::pow (9.0f, u);            // 1 .. 9
        const float bias  = 0.35f * u;

        for (int i = 0; i < n; ++i)
        {
            const float t = (float) i / (float) n;
            buf[i] = triFold (drive * std::sin (kTwoPiF * t) + bias);
        }
    }

    //==========================================================================
    // BANK 4 — METALLIC: sparse, bar/bell-like partial sets. The ideal free
    // bar ratios are stretched per frame and rounded onto the harmonic grid,
    // which is what makes them read as inharmonic against the fundamental.

    void metallicBins (int frame, FrameSpectrum& s)
    {
        static const float barModes[16] =
        {
            1.0f, 2.756f, 5.404f, 8.933f, 13.34f, 18.64f, 24.82f, 31.87f,
            39.80f, 48.60f, 58.28f, 68.83f, 80.26f, 92.56f, 105.7f, 119.8f
        };

        const float u = (float) frame / (float) (kWaveFramesPerBank - 1);
        const float stretch = 1.0f + 1.2f * u;
        const int   count = 6 + (int) std::round (10.0f * u);
        Rng rng (hashSeed (0x4D45u, (uint32_t) frame * 977u + 13u));

        for (int j = 0; j < count && j < 16; ++j)
        {
            const float ratio = std::pow (barModes[j], j == 0 ? 1.0f : stretch * 0.62f + 0.38f);
            const int   k = juce::jlimit (1, kWaveMaxHarmonics, (int) std::round (ratio));
            const float amp = j == 0 ? 1.0f
                                     : std::pow ((float) (j + 1), -0.85f) * (0.6f + 0.4f * rng.nextFloat());
            s.add (k, amp, rng.nextFloat() * kTwoPiF * u);
        }

        // A whisper of ring-modulated shimmer above the modes.
        const int shimmer = (int) std::round (12.0f * u);
        for (int j = 0; j < shimmer; ++j)
        {
            const int k = juce::jlimit (1, kWaveMaxHarmonics, 40 + rng.nextInt (300));
            s.add (k, 0.05f * u * (0.5f + 0.5f * rng.nextFloat()), rng.nextFloat() * kTwoPiF);
        }
    }

    //==========================================================================
    // BANK 5 — SPECTRAL: designed spectra. One continuous family combining a
    // tilt (rolloff), a comb (periodic notches) and a spectral stretch, so the
    // whole bank scans smoothly from soft saw to hollow comb to bright tilt.

    void spectralBins (int frame, FrameSpectrum& s)
    {
        const float u = (float) frame / (float) (kWaveFramesPerBank - 1);
        const float tilt    = 1.5f - 0.8f * u;                  // 1.5 .. 0.7
        const float stretch = 1.0f + 0.16f * u;                 // spectral envelope stretch
        const float combDepth  = 0.95f * std::sin (kPiF * u);   // 0 at both ends
        const float combPeriod = 2.0f + 9.0f * u;
        const float dispersion = 12.0f * u;   // quadratic phase: spreads the impulse into a chirp

        for (int k = 1; k <= kWaveMaxHarmonics; ++k)
        {
            const float warped = std::pow ((float) k, 1.0f / stretch);
            float amp = std::pow (warped, -tilt);
            const float comb = 0.5f + 0.5f * std::cos (kTwoPiF * (float) k / combPeriod);
            amp *= 1.0f - combDepth * (1.0f - comb);
            amp *= std::exp (-(float) k / 300.0f);
            if (amp < 1.0e-5f) continue;
            const float ph = -kPiF * 0.5f + dispersion * (float) (k * k) / 4096.0f;
            s.add (k, amp, ph);
        }
    }

    //==========================================================================
    // BANK 6 — FRACTURED (time domain): the cycle is cut into segments with
    // jumped phase, windowed fragments and bit-reduced sections. Frame 0 is a
    // clean sine; the damage grows across the bank.

    void fracturedWave (int frame, float* buf, int n)
    {
        const float u = (float) frame / (float) (kWaveFramesPerBank - 1);
        const int   segments = 2 + (int) std::round (6.0f * u);

        // One fixed random basis for the whole bank keeps neighbouring frames
        // related, so scanning position sounds like progressive damage.
        Rng rng (0x0FAC5EEDu);
        float phaseOff[9] {}, gap[9] {}, mult[9] {}, crush[9] {};
        for (int i = 0; i < 9; ++i)
        {
            phaseOff[i] = rng.nextFloat();
            gap[i]      = rng.nextFloat();
            mult[i]     = (float) (1 + rng.nextInt (4));
            crush[i]    = rng.nextFloat();
        }

        for (int i = 0; i < n; ++i)
        {
            const float t = (float) i / (float) n;
            const float sf = t * (float) segments;
            const int   seg = juce::jlimit (0, segments - 1, (int) sf);
            const float local = sf - (float) seg;

            const float m = 1.0f + (mult[seg] - 1.0f) * u;
            float y = std::sin (kTwoPiF * (t * m + u * phaseOff[seg]));

            // Windowed fragment: part of the segment is silenced as u grows.
            const float width = 1.0f - 0.75f * u * gap[seg];
            if (local > width)
            {
                y = 0.0f;
            }
            else
            {
                const float edge = 0.12f;
                const float a = juce::jmin (local, width - local) / (edge * width + 1.0e-6f);
                y *= juce::jlimit (0.0f, 1.0f, a);
            }

            // Bit-reduced sections.
            if (crush[seg] < u * 0.7f)
                y = quantise (y, std::round (juce::jmax (1.0f, 32.0f * (1.0f - u))));

            buf[i] = y;
        }
    }

    //==========================================================================
    // BANK 7 — NOISE: periodic frames built from a seeded random spectrum.
    // All frames share one random basis so the bank scans as a colour sweep
    // from filtered hum to bright hiss instead of jumping between textures.

    void noiseBins (int frame, FrameSpectrum& s)
    {
        const float u = (float) frame / (float) (kWaveFramesPerBank - 1);
        const float rolloff = 1.6f - 1.3f * u;
        const float density = 0.35f + 0.65f * u;

        Rng rng (0x0157ABCDu);
        for (int k = 1; k <= kWaveMaxHarmonics; ++k)
        {
            const float ra = rng.nextFloat();
            const float rp = rng.nextFloat();
            const float rd = rng.nextFloat();
            if (k > 1 && rd > density) continue;      // the fundamental always survives
            const float amp = (0.35f + 0.65f * ra) * std::pow ((float) k, -rolloff)
                            * std::exp (-(float) k / 500.0f);
            if (amp < 1.0e-6f) continue;
            s.add (k, amp, rp * kTwoPiF);
        }
    }

    //==========================================================================

    const char* const kBankNames[kWaveNumBanks] =
    {
        "BASIC", "HARMONIC", "FORMANT", "FOLDED", "METALLIC", "SPECTRAL", "FRACTURED", "NOISE"
    };

    /** Analyses a time-domain cycle into harmonic bins (8x oversampled). */
    void analyseCycle (const float* cycle, int n, juce::dsp::FFT& fft, FrameSpectrum& out)
    {
        std::vector<Cplx> in ((size_t) n), spec ((size_t) n);
        for (int i = 0; i < n; ++i) in[(size_t) i] = Cplx (cycle[i], 0.0f);
        fft.perform (in.data(), spec.data(), false);

        const float norm = 2.0f / (float) n;
        for (int k = 1; k <= kWaveMaxHarmonics && k < n / 2; ++k)
            out.bin[(size_t) k] = spec[(size_t) k] * (norm * 0.5f);
    }

    /** Synthesises one mip level from the spectrum with an inverse FFT. */
    void synthLevel (const FrameSpectrum& s, int length, int maxHarmonic, float* dest)
    {
        std::vector<Cplx> spec ((size_t) length, Cplx (0.0f, 0.0f));
        const int limit = juce::jmin (maxHarmonic, length / 2 - 1);
        for (int k = 1; k <= limit; ++k)
        {
            const Cplx v = s.bin[(size_t) k] * (float) length;
            spec[(size_t) k] = v;
            spec[(size_t) (length - k)] = std::conj (v);
        }

        std::vector<Cplx> time ((size_t) length);
        juce::dsp::FFT fft ((int) std::round (std::log2 ((double) length)));
        fft.perform (spec.data(), time.data(), true);
        for (int i = 0; i < length; ++i) dest[i] = time[(size_t) i].real();
    }
}

//==============================================================================
const char* WavetableGenerator::bankName (int index) noexcept
{
    return kBankNames[juce::jlimit (0, kWaveNumBanks - 1, index)];
}

void WavetableGenerator::build (WavetableCache& cache)
{
    // ---- geometry -----------------------------------------------------------
    int samplesPerFrame = 0;
    for (int l = 0; l < kWaveNumLevels; ++l) samplesPerFrame += waveLevelLength (l);
    const size_t total = (size_t) samplesPerFrame * (size_t) kWaveFramesPerBank * (size_t) kWaveNumBanks;

    cache.storage.assign (total, 0.0f);

    size_t offset = 0;
    for (int b = 0; b < kWaveNumBanks; ++b)
    {
        auto& bank = cache.banks[(size_t) b];
        bank.numFrames = kWaveFramesPerBank;
        bank.name = kBankNames[b];
        for (int l = 0; l < kWaveNumLevels; ++l)
        {
            auto& lv = bank.levels[(size_t) l];
            lv.length = waveLevelLength (l);
            lv.mask = (uint32_t) lv.length - 1u;
            lv.lengthF = (float) lv.length;
            lv.maxHarmonics = waveLevelHarmonics (l);
            lv.data = cache.storage.data() + offset;
            offset += (size_t) lv.length * (size_t) kWaveFramesPerBank;
        }
    }
    jassert (offset == total);

    // ---- frames -------------------------------------------------------------
    constexpr int kAnalysisSize = 8192;
    juce::dsp::FFT analysisFFT (13);                 // 8192
    std::vector<float> cycle ((size_t) kAnalysisSize);
    auto spectrum = std::make_unique<FrameSpectrum>();

    for (int b = 0; b < kWaveNumBanks; ++b)
    {
        auto& bank = cache.banks[(size_t) b];

        for (int f = 0; f < kWaveFramesPerBank; ++f)
        {
            spectrum->clear();

            switch (b)
            {
                case 0: basicBins (f, *spectrum); break;
                case 1: harmonicBins (f, *spectrum); break;
                case 2: formantBins (f, *spectrum); break;
                case 3: foldedWave (f, cycle.data(), kAnalysisSize);
                        analyseCycle (cycle.data(), kAnalysisSize, analysisFFT, *spectrum); break;
                case 4: metallicBins (f, *spectrum); break;
                case 5: spectralBins (f, *spectrum); break;
                case 6: fracturedWave (f, cycle.data(), kAnalysisSize);
                        analyseCycle (cycle.data(), kAnalysisSize, analysisFFT, *spectrum); break;
                default: noiseBins (f, *spectrum); break;
            }

            spectrum->bin[0] = Cplx (0.0f, 0.0f);        // no DC, ever

            for (int l = 0; l < kWaveNumLevels; ++l)
            {
                auto& lv = bank.levels[(size_t) l];
                synthLevel (*spectrum, lv.length, lv.maxHarmonics,
                            const_cast<float*> (lv.frame (f)));
            }

            // ---- RMS matching (peak capped) ---------------------------------
            const auto& l0 = bank.levels[0];
            const float* base = l0.frame (f);
            double sum = 0.0; float peak = 0.0f;
            for (int i = 0; i < l0.length; ++i)
            {
                sum += (double) base[i] * (double) base[i];
                peak = juce::jmax (peak, std::abs (base[i]));
            }
            const float rms = (float) std::sqrt (sum / (double) l0.length);
            float scale = 1.0f;
            if (rms > 1.0e-9f) scale = kTargetRms / rms;
            if (peak > 1.0e-9f) scale = juce::jmin (scale, kPeakCap / peak);

            for (int l = 0; l < kWaveNumLevels; ++l)
            {
                auto& lv = bank.levels[(size_t) l];
                float* d = const_cast<float*> (lv.frame (f));
                for (int i = 0; i < lv.length; ++i) d[i] *= scale;
            }
        }
    }

    // ---- modulator sine -----------------------------------------------------
    for (int i = 0; i < kWaveSineSize; ++i)
        cache.sine[(size_t) i] = std::sin (kTwoPiF * (float) i / (float) kWaveSineSize);

    // ---- BLEP residual ------------------------------------------------------
    // Integrate a Blackman-windowed sinc into a band-limited step, subtract the
    // ideal step and sample the residual at kWaveBlepRes fractional delays.
    {
        constexpr int kOversample = 512;
        const int half = kWaveBlepZ * kOversample;
        std::vector<float> step ((size_t) (2 * half + 1), 0.0f);

        // Blackman-Harris (-92 dB sidelobes) and a cutoff just below Nyquist:
        // the transition band then fits under Nyquist instead of folding back,
        // which is what sets the noise floor of a BLEP-corrected sync reset.
        constexpr double kCutoff = 0.92;
        double acc = 0.0;
        for (int i = -half; i <= half; ++i)
        {
            const double t = (double) i / (double) kOversample;
            const double x = kPi * kCutoff * t;
            const double sinc = std::abs (x) < 1.0e-12 ? 1.0 : std::sin (x) / x;
            const double u = (double) (i + half) / (double) (2 * half);
            const double w = 0.35875 - 0.48829 * std::cos (kTwoPi * u)
                                     + 0.14128 * std::cos (2.0 * kTwoPi * u)
                                     - 0.01168 * std::cos (3.0 * kTwoPi * u);
            acc += kCutoff * sinc * w / (double) kOversample;
            step[(size_t) (i + half)] = (float) acc;
        }
        const float endValue = step.back();
        if (endValue > 1.0e-9f)
            for (auto& v : step) v /= endValue;

        // The step residual, and its running integral: the ramp (BLAMP)
        // residual that band-limits the slope change at a sync reset.
        std::vector<float> residual ((size_t) (2 * half + 1), 0.0f);
        std::vector<float> rampResidual ((size_t) (2 * half + 1), 0.0f);
        double integral = 0.0;
        for (int i = 0; i <= 2 * half; ++i)
        {
            const double t = (double) (i - half) / (double) kOversample;
            residual[(size_t) i] = step[(size_t) i] - (t >= 0.0 ? 1.0f : 0.0f);
            integral += (double) residual[(size_t) i] / (double) kOversample;
            rampResidual[(size_t) i] = (float) integral;
        }

        cache.blep.assign ((size_t) ((kWaveBlepRes + 1) * kWaveBlepLen), 0.0f);
        cache.blamp.assign ((size_t) ((kWaveBlepRes + 1) * kWaveBlepLen), 0.0f);
        for (int d = 0; d <= kWaveBlepRes; ++d)
        {
            const float frac = (float) d / (float) kWaveBlepRes;
            for (int j = 0; j < kWaveBlepLen; ++j)
            {
                const float t = (float) (j - kWaveBlepZ) + frac;
                const int idx = juce::jlimit (0, 2 * half, (int) std::round ((t + (float) kWaveBlepZ) * (float) kOversample));
                cache.blep[(size_t) (d * kWaveBlepLen + j)]  = residual[(size_t) idx];
                cache.blamp[(size_t) (d * kWaveBlepLen + j)] = rampResidual[(size_t) idx];
            }
        }
    }

    cache.bytes = cache.storage.size() * sizeof (float)
                + cache.sine.size() * sizeof (float)
                + cache.blep.size() * sizeof (float)
                + cache.blamp.size() * sizeof (float);
}

} // namespace am

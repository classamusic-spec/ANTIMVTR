#include "SampleData.h"
#include "NoiseGenerators.h"

namespace am
{

namespace
{
    using namespace excitation;

    constexpr const char* kBuiltInNames[] =
    {
        "Glass Strike", "Metal Ping", "Wood Knock", "Stone Drop", "Breath", "Vinyl Dust", "Noise Burst"
    };

    static_assert ((int) BuiltInSamples::Kind::Count == (int) (sizeof (kBuiltInNames) / sizeof (kBuiltInNames[0])),
                   "built-in sample names must match the Kind enum");

    /** One decaying mode of a struck body. */
    struct Mode
    {
        float ratio = 1.0f;     ///< multiple of the fundamental
        float amp   = 1.0f;
        float t60   = 1.0f;     ///< seconds to -60 dB
        float phase = 0.0f;     ///< cycles
    };

    /** Adds a bank of exponentially decaying sinusoids. Skips anything above Nyquist. */
    void addModes (float* dest, int frames, double sr, double f0, const Mode* modes, int count, float gain)
    {
        const double nyquist = sr * 0.47;
        for (int m = 0; m < count; ++m)
        {
            const double f = f0 * (double) modes[m].ratio;
            if (f <= 1.0 || f >= nyquist) continue;

            const double w = kTwoPi * f / sr;
            const double decay = std::exp (-6.907755 / (juce::jmax (0.002f, modes[m].t60) * sr));
            // Quadrature rotator: cheap, stable and exact in frequency.
            double c = std::cos (w), s = std::sin (w);
            double x = std::cos (kTwoPi * (double) modes[m].phase);
            double y = std::sin (kTwoPi * (double) modes[m].phase);
            double env = (double) modes[m].amp * (double) gain;
            for (int i = 0; i < frames; ++i)
            {
                dest[i] += (float) (env * y);
                const double nx = x * c - y * s;
                y = x * s + y * c;
                x = nx;
                env *= decay;
                if (env < 1.0e-7) break;
            }
        }
    }

    /** Adds band-passed noise with an exponential envelope and an optional cutoff sweep. */
    void addNoiseBand (float* dest, int frames, double sr, Rng& rng, double startHz, double endHz,
                       float q, float t60, float gain, int startFrame = 0)
    {
        Svf filter;
        filter.prepare (sr);
        const double decay = std::exp (-6.907755 / (juce::jmax (0.002f, t60) * sr));
        double env = 1.0;
        const int last = juce::jmin (frames, startFrame + (int) (juce::jmax (0.002f, t60) * 2.2 * sr) + 32);
        for (int i = juce::jmax (0, startFrame); i < last; ++i)
        {
            const float t = (float) (i - startFrame) / (float) juce::jmax (1, last - startFrame);
            const float hz = (float) (startHz * std::pow (endHz / juce::jmax (1.0, startHz), (double) t));
            if ((i & 15) == 0) filter.set (hz, q);
            const float band = filter.bandpass (rng.nextBipolar()) * filter.bandpassNoiseGain();
            dest[i] += (float) (env * (double) band * (double) gain);
            env *= decay;
        }
    }

    /** Short bright transient: a band-limited spike through a two pole low pass. */
    void addClick (float* dest, int frames, double sr, double cutoffHz, float gain, int startFrame = 0)
    {
        ExciterLowpass lp;
        lp.prepare (sr);
        lp.setCutoff ((float) cutoffHz);
        const int last = juce::jmin (frames, startFrame + (int) (sr * 0.05));
        for (int i = juce::jmax (0, startFrame); i < last; ++i)
            dest[i] += lp.process (i == startFrame ? gain * lp.impulsePeakGain() : 0.0f);
    }

    /** Raised-cosine fade in / out so no built-in can ever click at its edges. */
    void applyEdgeFades (float* d, int frames, double sr, double fadeInSeconds, double fadeOutSeconds)
    {
        const int fin = juce::jlimit (1, juce::jmax (1, frames / 2), (int) (fadeInSeconds * sr));
        const int fout = juce::jlimit (1, juce::jmax (1, frames / 2), (int) (fadeOutSeconds * sr));
        for (int i = 0; i < fin; ++i)
            d[i] *= 0.5f - 0.5f * std::cos ((float) kPi * (float) i / (float) fin);
        for (int i = 0; i < fout; ++i)
        {
            const float w = 0.5f - 0.5f * std::cos ((float) kPi * (float) i / (float) fout);
            d[frames - 1 - i] *= w;
        }
    }

    void removeDc (float* d, int frames, double sr)
    {
        DcBlocker dc;
        dc.prepare (sr, 18.0f);
        for (int i = 0; i < frames; ++i) d[i] = dc.process (d[i]);
    }

    void normalise (SampleData& s, float target)
    {
        s.updatePeak();
        if (s.peak <= 1.0e-6f) return;
        const float g = target / s.peak;
        for (int c = 0; c < s.numChannels; ++c)
            if (float* d = s.write (c))
                juce::FloatVectorOperations::multiply (d, g, s.numFrames);
        s.updatePeak();
    }

    //==========================================================================
    void makeGlassStrike (SampleData& s, double sr)
    {
        const int n = s.numFrames;
        float* d = s.write (0);
        Rng rng (0x61A55u);

        // A thin glass plate: high, sharply inharmonic, long shimmering tail.
        const Mode modes[] = {
            { 1.00f, 1.00f, 1.30f, 0.00f }, { 2.76f, 0.72f, 0.95f, 0.13f },
            { 5.40f, 0.50f, 0.66f, 0.41f }, { 8.93f, 0.34f, 0.48f, 0.77f },
            { 13.34f, 0.22f, 0.32f, 0.22f }, { 18.44f, 0.15f, 0.24f, 0.61f },
            { 24.20f, 0.10f, 0.17f, 0.05f }, { 1.98f, 0.30f, 1.05f, 0.50f },
            { 3.42f, 0.26f, 0.80f, 0.31f }, { 6.85f, 0.18f, 0.55f, 0.90f }
        };
        addModes (d, n, sr, 1180.0, modes, (int) (sizeof (modes) / sizeof (Mode)), 0.55f);
        addNoiseBand (d, n, sr, rng, 6000.0, 2600.0, 0.9f, 0.020f, 0.55f);
        addClick (d, n, sr, 9000.0, 0.5f);
    }

    void makeMetalPing (SampleData& s, double sr)
    {
        const int n = s.numFrames;
        float* d = s.write (0);
        Rng rng (0x4E7A1u);

        // Struck bar / bell: stretched partials with a seeded irregularity.
        Mode modes[16];
        for (int i = 0; i < 16; ++i)
        {
            const float k = (float) (i + 1);
            modes[i].ratio = std::pow (k, 1.32f) * (1.0f + 0.035f * rng.nextBipolar());
            modes[i].amp   = std::pow (k, -0.75f) * (0.7f + 0.4f * rng.nextFloat());
            modes[i].t60   = 2.4f * std::pow (k, -0.55f);
            modes[i].phase = rng.nextFloat();
        }
        addModes (d, n, sr, 660.0, modes, 16, 0.50f);
        addNoiseBand (d, n, sr, rng, 9000.0, 4000.0, 0.8f, 0.012f, 0.40f);
        addClick (d, n, sr, 12000.0, 0.45f);
    }

    void makeWoodKnock (SampleData& s, double sr)
    {
        const int n = s.numFrames;
        float* d = s.write (0);
        Rng rng (0x0D00Du);

        const Mode modes[] = {
            { 1.00f, 1.00f, 0.130f, 0.00f }, { 2.31f, 0.62f, 0.095f, 0.27f },
            { 4.11f, 0.40f, 0.070f, 0.63f }, { 6.79f, 0.24f, 0.050f, 0.11f },
            { 9.62f, 0.15f, 0.038f, 0.84f }, { 13.9f, 0.09f, 0.026f, 0.35f }
        };
        addModes (d, n, sr, 196.0, modes, (int) (sizeof (modes) / sizeof (Mode)), 0.62f);
        addNoiseBand (d, n, sr, rng, 2400.0, 900.0, 0.7f, 0.022f, 0.75f);
        addClick (d, n, sr, 5200.0, 0.55f);
    }

    void makeStoneDrop (SampleData& s, double sr)
    {
        const int n = s.numFrames;
        float* d = s.write (0);
        Rng rng (0x57012u);

        // Three bounces, each a dense granite cluster over a short body thump.
        const double bounce[] = { 0.0, 0.115, 0.205, 0.268 };
        const float  level[]  = { 1.0f, 0.55f, 0.3f, 0.18f };
        for (int b = 0; b < 4; ++b)
        {
            const int start = (int) (bounce[b] * sr);
            const Mode body[] = {
                { 1.00f, 1.00f, 0.085f, 0.0f }, { 2.62f, 0.45f, 0.055f, 0.3f }, { 4.83f, 0.22f, 0.035f, 0.7f }
            };
            if (start < n)
                addModes (d + start, n - start, sr, 92.0 * (1.0 + 0.05 * b), body, 3, 0.55f * level[b]);
            addNoiseBand (d, n, sr, rng, 3200.0, 700.0, 1.1f, 0.030f, 0.55f * level[b], start);
            addNoiseBand (d, n, sr, rng, 480.0, 320.0, 2.2f, 0.055f, 0.45f * level[b], start);
        }
    }

    void makeBreath (SampleData& s, double sr)
    {
        const int n = s.numFrames;
        Rng rng (0xB1EA7u);

        for (int c = 0; c < 2; ++c)
        {
            float* d = s.write (c);
            Svf lowBand, formant1, formant2;
            lowBand.prepare (sr);  lowBand.set (520.0f, 0.85f);
            formant1.prepare (sr); formant1.set (760.0f, 3.0f);
            formant2.prepare (sr); formant2.set (1620.0f, 4.5f);
            PinkFilter pink;

            // Slow turbulence: a random walk smoothed twice, decorrelated per channel.
            float turb = 0.0f, turbLp = 0.0f, target = 0.0f;
            int   hold = 0;
            for (int i = 0; i < n; ++i)
            {
                if (--hold <= 0) { target = rng.nextFloat(); hold = (int) (sr * (0.045 + 0.06 * rng.nextFloat())); }
                turb += 0.0016f * (target - turb);
                turbLp += 0.004f * (turb - turbLp);

                const float w = pink.process (rng.nextBipolar());
                const float body = lowBand.lowpass (w) * lowBand.lowpassNoiseGain();
                const float f1 = formant1.bandpass (w) * formant1.bandpassNoiseGain();
                const float f2 = formant2.bandpass (w) * formant2.bandpassNoiseGain();
                const float mix = body * 0.85f + f1 * 0.30f + f2 * 0.16f;
                const float amp = 0.45f + 0.85f * turbLp;
                d[i] = mix * amp;
            }
            // A gentle overall swell so the sample reads as one breath.
            for (int i = 0; i < n; ++i)
            {
                const float t = (float) i / (float) juce::jmax (1, n - 1);
                d[i] *= 0.35f + 0.9f * std::sin ((float) kPi * std::pow (t, 0.8f));
            }
        }
    }

    void makeVinylDust (SampleData& s, double sr)
    {
        const int n = s.numFrames;
        Rng rng (0x71D057u);

        for (int c = 0; c < 2; ++c)
        {
            float* d = s.write (c);
            Svf hiss;   hiss.prepare (sr);   hiss.set (3800.0f, 0.7f);
            Svf rumble; rumble.prepare (sr); rumble.set (48.0f, 0.9f);
            ExciterLowpass crackLp;
            crackLp.prepare (sr);

            // Surface noise floor plus a slow rumble.
            for (int i = 0; i < n; ++i)
            {
                const float w = rng.nextBipolar();
                d[i] = hiss.bandpass (w) * hiss.bandpassNoiseGain() * 0.055f
                     + rumble.bandpass (rng.nextBipolar()) * rumble.bandpassNoiseGain() * 0.10f;
            }

            // Sparse crackle: Poisson events with a random brightness.
            double next = rng.nextFloat() * 0.05 * sr;
            while (next < (double) n)
            {
                const int start = (int) next;
                const float cut = 1400.0f + 5200.0f * rng.nextFloat();
                crackLp.setCutoff (cut);
                crackLp.reset();
                const float amp = (0.25f + 0.75f * rng.nextFloat()) * (rng.chance (0.5f) ? 1.0f : -1.0f);
                const int len = juce::jmin (n - start, (int) (sr * 0.01));
                for (int i = 0; i < len; ++i)
                    d[start + i] += crackLp.process (i == 0 ? amp * crackLp.impulsePeakGain() * 0.75f : 0.0f);
                next += sr * (0.006 + 0.11 * rng.nextFloat() * rng.nextFloat());
            }
        }
    }

    void makeNoiseBurst (SampleData& s, double sr)
    {
        const int n = s.numFrames;
        float* d = s.write (0);
        Rng rng (0x0B0057u);

        addNoiseBand (d, n, sr, rng, 9000.0, 420.0, 0.75f, 0.16f, 0.9f);
        addNoiseBand (d, n, sr, rng, 1600.0, 240.0, 1.6f, 0.28f, 0.45f);
        addClick (d, n, sr, 11000.0, 0.35f);
    }

    double builtInSeconds (int index) noexcept
    {
        switch ((BuiltInSamples::Kind) index)
        {
            case BuiltInSamples::Kind::GlassStrike: return 1.60;
            case BuiltInSamples::Kind::MetalPing:   return 2.00;
            case BuiltInSamples::Kind::WoodKnock:   return 0.50;
            case BuiltInSamples::Kind::StoneDrop:   return 0.90;
            case BuiltInSamples::Kind::Breath:      return 2.00;
            case BuiltInSamples::Kind::VinylDust:   return 2.00;
            default:                                return 0.60;
        }
    }
}

//==============================================================================
namespace BuiltInSamples
{

int count() noexcept { return (int) Kind::Count; }

const char* name (int index) noexcept
{
    return kBuiltInNames[(size_t) juce::jlimit (0, (int) Kind::Count - 1, index)];
}

int indexOf (const juce::String& n) noexcept
{
    for (int i = 0; i < (int) Kind::Count; ++i)
        if (n.equalsIgnoreCase (kBuiltInNames[(size_t) i])) return i;
    return -1;
}

std::shared_ptr<const SampleData> create (int index, double sampleRate)
{
    index = juce::jlimit (0, (int) Kind::Count - 1, index);
    const double sr = juce::jlimit (8000.0, 384000.0, sampleRate);

    auto s = std::make_shared<SampleData>();
    const bool stereo = index == (int) Kind::Breath || index == (int) Kind::VinylDust;
    s->allocate (stereo ? 2 : 1, (int) (builtInSeconds (index) * sr));
    s->sampleRate = sr;
    s->name = kBuiltInNames[(size_t) index];
    s->builtInIndex = index;

    switch ((Kind) index)
    {
        case Kind::GlassStrike: makeGlassStrike (*s, sr); break;
        case Kind::MetalPing:   makeMetalPing (*s, sr);   break;
        case Kind::WoodKnock:   makeWoodKnock (*s, sr);   break;
        case Kind::StoneDrop:   makeStoneDrop (*s, sr);   break;
        case Kind::Breath:      makeBreath (*s, sr);      break;
        case Kind::VinylDust:   makeVinylDust (*s, sr);   break;
        default:                makeNoiseBurst (*s, sr);  break;
    }

    const bool sustained = index == (int) Kind::Breath || index == (int) Kind::VinylDust;
    for (int c = 0; c < s->numChannels; ++c)
    {
        float* d = s->write (c);
        removeDc (d, s->numFrames, sr);
        applyEdgeFades (d, s->numFrames, sr, sustained ? 0.020 : 0.0006, sustained ? 0.040 : 0.010);
        for (int i = 0; i < s->numFrames; ++i)
            if (! std::isfinite (d[i])) d[i] = 0.0f;
    }
    normalise (*s, 0.94f);
    return s;
}

} // namespace BuiltInSamples

} // namespace am

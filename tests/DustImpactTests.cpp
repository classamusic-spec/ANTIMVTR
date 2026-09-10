#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/source/DustSource.h"
#include "dsp/source/ImpactSource.h"
#include "dsp/source/SourceEngine.h"

#include <vector>

using namespace am;

namespace
{
    //==========================================================================
    /** Parameter block plus a note, everything a source needs to be rendered standalone. */
    struct Harness
    {
        ParamValues params = ParameterRegistry::defaults();
        NoteState   note;
        double      sr = 48000.0;
        int         block = 128;

        Harness()
        {
            note.midiNote = 60;
            note.velocity = 0.8f;
            note.noteId = 1;
            note.gate = true;
            note.baseFrequency = midiNoteToHz (60.0);
            note.frequency = note.baseFrequency;
        }

        void set (Param p, float v) noexcept { params[(size_t) paramIndex (p)] = v; }

        void setNote (int midiNote) noexcept
        {
            note.midiNote = midiNote;
            note.baseFrequency = midiNoteToHz ((double) midiNote);
            note.frequency = note.baseFrequency;
        }

        RenderContext context (int n) const noexcept
        {
            RenderContext c;
            c.sampleRate = sr;
            c.numSamples = n;
            c.params = &params;
            c.dryMode = DryMode::SourceOnly;
            return c;
        }
    };

    /** Renders a source from note-on; releases the key after `hold` seconds. */
    juce::AudioBuffer<float> renderSource (SourceBase& src, Harness& h, double seconds, double hold = 1.0e9)
    {
        const int total = juce::jmax (1, (int) (seconds * h.sr));
        juce::AudioBuffer<float> buf (2, total);
        buf.clear();

        src.prepare (h.sr, h.block);
        src.reset();
        h.note.gate = true;
        src.noteOn (h.note, h.params);

        const int offSample = (int) (hold * h.sr);
        bool released = false;
        for (int pos = 0; pos < total; pos += h.block)
        {
            const int n = juce::jmin (h.block, total - pos);
            if (! released && pos >= offSample) { src.noteOff(); h.note.gate = false; released = true; }
            const auto ctx = h.context (n);
            src.render (buf.getWritePointer (0) + pos, buf.getWritePointer (1) + pos, n, ctx, h.note);
        }
        return buf;
    }

    //==========================================================================
    bool identical (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumSamples() != b.getNumSamples()) return false;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < a.getNumSamples(); ++i)
                if (a.getSample (ch, i) != b.getSample (ch, i)) return false;
        return true;
    }

    float peakOf (const juce::AudioBuffer<float>& b)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            for (int i = 0; i < b.getNumSamples(); ++i)
                peak = juce::jmax (peak, std::abs (b.getSample (ch, i)));
        return peak;
    }

    bool allFinite (const juce::AudioBuffer<float>& b)
    {
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            for (int i = 0; i < b.getNumSamples(); ++i)
                if (! std::isfinite (b.getSample (ch, i))) return false;
        return true;
    }

    double meanOf (const juce::AudioBuffer<float>& b, int channel = 0)
    {
        double s = 0.0;
        for (int i = 0; i < b.getNumSamples(); ++i) s += (double) b.getSample (channel, i);
        return s / juce::jmax (1, b.getNumSamples());
    }

    double rmsOf (const juce::AudioBuffer<float>& b, int channel = 0)
    {
        double s = 0.0;
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const double v = (double) b.getSample (channel, i);
            s += v * v;
        }
        return std::sqrt (s / juce::jmax (1, b.getNumSamples()));
    }

    double correlation (const juce::AudioBuffer<float>& b)
    {
        double sxy = 0.0, sxx = 0.0, syy = 0.0;
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const double x = b.getSample (0, i), y = b.getSample (1, i);
            sxy += x * y; sxx += x * x; syy += y * y;
        }
        const double den = std::sqrt (sxx * syy);
        return den > 1.0e-18 ? sxy / den : 1.0;
    }

    //==========================================================================
    /** Summed power in [lo, hi) Hz, measured with overlapping 4096 point Hann frames. */
    double bandPower (const juce::AudioBuffer<float>& b, double sr, double lo, double hi)
    {
        constexpr int order = 12, size = 1 << order;
        if (b.getNumSamples() < size) return 0.0;

        juce::dsp::FFT fft (order);
        juce::dsp::WindowingFunction<float> window (size, juce::dsp::WindowingFunction<float>::hann);
        std::vector<float> data ((size_t) size * 2);
        double total = 0.0;

        for (int start = 0; start + size <= b.getNumSamples(); start += size / 2)
        {
            for (int i = 0; i < size; ++i) data[(size_t) i] = b.getSample (0, start + i);
            std::fill (data.begin() + size, data.end(), 0.0f);
            window.multiplyWithWindowingTable (data.data(), size);
            fft.performFrequencyOnlyForwardTransform (data.data(), true);
            for (int k = 1; k < size / 2; ++k)
            {
                const double f = k * sr / size;
                if (f >= lo && f < hi) total += (double) data[(size_t) k] * data[(size_t) k];
            }
        }
        return total;
    }

    /** High-band to low-band power ratio: the practical measure of spectral slope. */
    double tiltRatio (const juce::AudioBuffer<float>& b, double sr)
    {
        const double low = bandPower (b, sr, 60.0, 400.0);
        const double high = bandPower (b, sr, 4000.0, 16000.0);
        return high / juce::jmax (1.0e-24, low);
    }

    /** Counts transient onsets above a fraction of the peak, with a refractory gap. */
    int countOnsets (const juce::AudioBuffer<float>& b, double sr, float relThreshold, double refractorySeconds)
    {
        const int n = b.getNumSamples();
        const float* x = b.getReadPointer (0);
        float peak = 0.0f;
        for (int i = 0; i < n; ++i) peak = juce::jmax (peak, std::abs (x[i]));
        if (peak < 1.0e-6f) return 0;

        const float threshold = peak * relThreshold;
        const int refractory = juce::jmax (1, (int) (refractorySeconds * sr));
        int count = 0, wait = 0;
        for (int i = 0; i < n; ++i)
        {
            if (wait > 0) { --wait; continue; }
            if (std::abs (x[i]) >= threshold) { ++count; wait = refractory; }
        }
        return count;
    }

    /** Short-term RMS envelope, one value per `windowSeconds`. */
    std::vector<double> envelopeOf (const juce::AudioBuffer<float>& b, double sr, double windowSeconds)
    {
        const int w = juce::jmax (1, (int) (windowSeconds * sr));
        std::vector<double> env;
        for (int start = 0; start + w <= b.getNumSamples(); start += w)
        {
            double s = 0.0;
            for (int i = start; i < start + w; ++i) { const double v = b.getSample (0, i); s += v * v; }
            env.push_back (std::sqrt (s / w));
        }
        return env;
    }

    /**
        Normalised autocorrelation of the envelope at a fixed lag. It rises with
        grain size: an envelope built from long grains still resembles itself
        20 ms later, one built from short grains does not.
    */
    double envelopeAutocorrelation (const juce::AudioBuffer<float>& b, double sr,
                                    double windowSeconds, double lagSeconds)
    {
        const auto env = envelopeOf (b, sr, windowSeconds);
        const int lag = juce::jmax (1, (int) std::lround (lagSeconds / windowSeconds));
        if ((int) env.size() < lag + 8) return 0.0;

        double mean = 0.0;
        for (auto v : env) mean += v;
        mean /= (double) env.size();

        double num = 0.0, den = 0.0;
        for (size_t i = 0; i < env.size(); ++i)
        {
            const double a = env[i] - mean;
            den += a * a;
            if (i + (size_t) lag < env.size()) num += a * (env[i + (size_t) lag] - mean);
        }
        return den > 1.0e-18 ? num / den : 0.0;
    }

    /** Fraction of the envelope that sits below 5 % of its peak (the gaps between grains). */
    double silenceFraction (const juce::AudioBuffer<float>& b, double sr, double windowSeconds)
    {
        const auto env = envelopeOf (b, sr, windowSeconds);
        if (env.empty()) return 0.0;
        double peak = 0.0;
        for (auto v : env) peak = juce::jmax (peak, v);
        if (peak < 1.0e-9) return 1.0;
        int quiet = 0;
        for (auto v : env) if (v < peak * 0.05) ++quiet;
        return (double) quiet / (double) env.size();
    }

    /** Time from the loudest window until the level has fallen by 40 dB. */
    double decayTime40 (const juce::AudioBuffer<float>& b, double sr, double windowSeconds = 0.005)
    {
        const int w = juce::jmax (1, (int) (windowSeconds * sr));
        std::vector<double> track;
        for (int start = 0; start + w <= b.getNumSamples(); start += w)
        {
            double s = 0.0;
            for (int i = start; i < start + w; ++i) { const double v = b.getSample (0, i); s += v * v; }
            track.push_back (std::sqrt (s / w));
        }
        if (track.empty()) return 0.0;

        size_t peakIndex = 0;
        for (size_t i = 1; i < track.size(); ++i) if (track[i] > track[peakIndex]) peakIndex = i;
        const double target = track[peakIndex] * 0.01;   // -40 dB
        for (size_t i = peakIndex; i < track.size(); ++i)
            if (track[i] <= target) return (double) (i - peakIndex) * windowSeconds;
        return (double) (track.size() - peakIndex) * windowSeconds;
    }

    const char* dustModeName (int m)
    {
        static const char* names[] = { "WHITE", "PINK", "BROWN", "BLUE", "FILTERED",
                                       "CRACKLE", "IMPULSE", "CLOUD", "FROZEN" };
        return names[juce::jlimit (0, 8, m)];
    }

    const char* impactModeName (int m)
    {
        static const char* names[] = { "IMPULSE", "CLICK", "PLUCK", "NOISE STRIKE",
                                       "METAL STRIKE", "DAMPED SINE", "MEMBRANE HIT" };
        return names[juce::jlimit (0, 6, m)];
    }

    constexpr int kNumDustModes = 9;
    constexpr int kNumImpactModes = 7;
}

//==============================================================================
class DustImpactTests : public juce::UnitTest
{
public:
    DustImpactTests() : juce::UnitTest ("Dust and Impact sources", "source") {}

    void runTest() override
    {
        testDustDeterminism();
        testDustSpectralSlopes();
        testDustStereo();
        testDustDensity();
        testCloudGrainSize();
        testDustDc();
        testDustSafety();
        testImpactDeterminism();
        testImpactLength();
        testImpactRepeatRate();
        testImpactCleanliness();
        testImpactSafety();
        testSourceEngineIntegration();
    }

private:
    //--------------------------------------------------------------------------
    void testDustDeterminism()
    {
        beginTest ("DUST: a fixed seed reproduces the texture bit for bit, a new seed changes it");
        for (int mode = 0; mode < kNumDustModes; ++mode)
        {
            Harness h;
            h.set (Param::dustMode, (float) mode);
            h.set (Param::dustSeed, 17.0f);
            h.set (Param::dustDensity, 0.6f);

            DustSource a, b;
            const auto bufA = renderSource (a, h, 0.4);
            const auto bufB = renderSource (b, h, 0.4);
            expect (identical (bufA, bufB), juce::String ("seed 17 not reproducible in ") + dustModeName (mode));

            Harness h2 = h;
            h2.set (Param::dustSeed, 18.0f);
            DustSource c;
            const auto bufC = renderSource (c, h2, 0.4);
            expect (! identical (bufA, bufC), juce::String ("seed 18 matched seed 17 in ") + dustModeName (mode));
            expect (rmsOf (bufC) > 1.0e-5, juce::String ("silent output in ") + dustModeName (mode));
        }

        beginTest ("DUST: simultaneous voices decorrelate through the note id, and stay reproducible");
        {
            Harness h;
            h.set (Param::dustMode, 0.0f);
            h.set (Param::dustSeed, 3.0f);

            DustSource a;
            h.note.noteId = 1;
            const auto v1 = renderSource (a, h, 0.2);
            DustSource b;
            h.note.noteId = 2;
            const auto v2 = renderSource (b, h, 0.2);
            expect (! identical (v1, v2), "two voices with the same seed produced identical noise");

            DustSource c;
            const auto v2again = renderSource (c, h, 0.2);
            expect (identical (v2, v2again), "note id 2 was not reproducible");
        }
    }

    //--------------------------------------------------------------------------
    void testDustSpectralSlopes()
    {
        beginTest ("DUST: BLUE > WHITE > PINK > BROWN in high band energy");
        double ratio[4] = {};
        for (int mode = 0; mode < 4; ++mode)
        {
            Harness h;
            h.set (Param::dustMode, (float) mode);
            h.set (Param::dustDensity, 1.0f);      // no occupancy gating
            h.set (Param::dustColor, 0.5f);        // neutral tilt
            h.set (Param::dustStereo, 0.0f);
            h.set (Param::dustSeed, 5.0f);
            DustSource src;
            const auto buf = renderSource (src, h, 1.5);
            ratio[mode] = tiltRatio (buf, h.sr);
            expect (allFinite (buf), juce::String (dustModeName (mode)) + " produced non-finite samples");
            expect (std::abs (meanOf (buf)) < 1.0e-3, juce::String (dustModeName (mode)) + " has DC offset");
        }

        expect (ratio[3] > ratio[2], "BLUE is not brighter than WHITE: "
                                     + juce::String (ratio[3]) + " vs " + juce::String (ratio[2]));
        expect (ratio[0] > ratio[1], "WHITE is not brighter than PINK: "
                                     + juce::String (ratio[0]) + " vs " + juce::String (ratio[1]));
        expect (ratio[1] > ratio[2], "PINK is not brighter than BROWN: "
                                     + juce::String (ratio[1]) + " vs " + juce::String (ratio[2]));

        beginTest ("DUST: colour tilts the spectrum of every continuous mode");
        for (int mode = 0; mode < 4; ++mode)
        {
            Harness h;
            h.set (Param::dustMode, (float) mode);
            h.set (Param::dustDensity, 1.0f);
            h.set (Param::dustStereo, 0.0f);

            Harness dark = h, bright = h;
            dark.set (Param::dustColor, 0.0f);
            bright.set (Param::dustColor, 1.0f);

            DustSource a, b;
            const double darkRatio = tiltRatio (renderSource (a, dark, 1.0), h.sr);
            const double brightRatio = tiltRatio (renderSource (b, bright, 1.0), h.sr);
            expect (brightRatio > darkRatio * 2.0,
                    juce::String ("colour did not tilt ") + dustModeName (mode)
                    + ": " + juce::String (darkRatio) + " -> " + juce::String (brightRatio));
        }
    }

    //--------------------------------------------------------------------------
    void testDustStereo()
    {
        beginTest ("DUST: stereo 0 is mono, stereo 1 decorrelates the channels");
        for (int mode = 0; mode < kNumDustModes; ++mode)
        {
            Harness mono;
            mono.set (Param::dustMode, (float) mode);
            mono.set (Param::dustStereo, 0.0f);
            mono.set (Param::dustSpread, 0.0f);
            mono.set (Param::dustDensity, 0.8f);
            DustSource a;
            const auto m = renderSource (a, mono, 0.5);
            bool sameChannels = true;
            for (int i = 0; i < m.getNumSamples() && sameChannels; ++i)
                if (m.getSample (0, i) != m.getSample (1, i)) sameChannels = false;
            expect (sameChannels, juce::String (dustModeName (mode)) + " is not mono at stereo 0");

            if (mode <= 4)   // the continuous textures must decorrelate fully
            {
                Harness wide = mono;
                wide.set (Param::dustStereo, 1.0f);
                DustSource b;
                const auto w = renderSource (b, wide, 0.5);
                expect (std::abs (correlation (w)) < 0.35,
                        juce::String (dustModeName (mode)) + " stayed correlated at stereo 1: "
                        + juce::String (correlation (w)));
            }
        }
    }

    //--------------------------------------------------------------------------
    void testDustDensity()
    {
        beginTest ("DUST: density raises the CRACKLE event rate");
        {
            Harness low;
            low.setNote (48);
            low.set (Param::dustMode, 5.0f);
            low.set (Param::dustGrain, 0.1f);
            low.set (Param::dustJitter, 0.0f);
            low.set (Param::dustSpread, 0.0f);
            low.set (Param::dustDensity, 0.25f);
            Harness high = low;
            high.set (Param::dustDensity, 0.75f);

            DustSource a, b;
            const int nLow = countOnsets (renderSource (a, low, 1.0), low.sr, 0.15f, 0.0015);
            const int nHigh = countOnsets (renderSource (b, high, 1.0), high.sr, 0.15f, 0.0015);
            expect (nHigh > nLow * 3, "CRACKLE density did not raise the event count: "
                                      + juce::String (nLow) + " -> " + juce::String (nHigh));
        }

        beginTest ("DUST: density raises the IMPULSE train event rate");
        {
            Harness low;
            low.setNote (48);
            low.set (Param::dustMode, 6.0f);
            low.set (Param::dustGrain, 0.1f);
            low.set (Param::dustJitter, 0.0f);
            low.set (Param::dustSpread, 0.0f);
            low.set (Param::dustStereo, 0.0f);
            low.set (Param::dustDensity, 0.0f);
            Harness high = low;
            high.set (Param::dustDensity, 1.0f);

            DustSource a, b;
            const int nLow = countOnsets (renderSource (a, low, 1.0), low.sr, 0.2f, 0.0004);
            const int nHigh = countOnsets (renderSource (b, high, 1.0), high.sr, 0.2f, 0.0004);
            expect (nHigh > nLow * 2, "IMPULSE density did not raise the event count: "
                                      + juce::String (nLow) + " -> " + juce::String (nHigh));
            // One impulse per cycle at density 0 ~ the note frequency (MIDI 48 = 130.8 Hz).
            expect (nLow > 100 && nLow < 170, "IMPULSE base rate is not the note pitch: " + juce::String (nLow));
        }
    }

    //--------------------------------------------------------------------------
    void testCloudGrainSize()
    {
        beginTest ("DUST: CLOUD grain size changes the envelope statistics");
        Harness small;
        small.set (Param::dustMode, 7.0f);
        small.set (Param::dustDensity, 0.5f);
        small.set (Param::dustJitter, 0.3f);
        small.set (Param::dustGrain, 0.02f);       // ~9 ms grains
        Harness large = small;
        large.set (Param::dustGrain, 0.95f);       // ~190 ms grains

        DustSource a, b;
        const auto bufSmall = renderSource (a, small, 3.0);
        const auto bufLarge = renderSource (b, large, 3.0);

        // Long grains keep the envelope correlated with itself 20 ms later; short ones do not.
        const double acSmall = envelopeAutocorrelation (bufSmall, small.sr, 0.002, 0.02);
        const double acLarge = envelopeAutocorrelation (bufLarge, large.sr, 0.002, 0.02);
        expect (acLarge > acSmall + 0.25, "CLOUD grain size did not change the envelope correlation: "
                                          + juce::String (acSmall) + " -> " + juce::String (acLarge));

        // At a fixed grain rate, short grains leave audible gaps between them.
        const double gapSmall = silenceFraction (bufSmall, small.sr, 0.002);
        const double gapLarge = silenceFraction (bufLarge, large.sr, 0.002);
        expect (gapSmall > gapLarge + 0.2, "CLOUD grain size did not change the gaps: "
                                           + juce::String (gapSmall) + " vs " + juce::String (gapLarge));
        expect (allFinite (bufSmall) && allFinite (bufLarge), "CLOUD produced non-finite samples");
    }

    //--------------------------------------------------------------------------
    void testDustDc()
    {
        beginTest ("DUST: no DC offset in any mode over a long window");
        for (int mode = 0; mode < kNumDustModes; ++mode)
        {
            Harness h;
            h.set (Param::dustMode, (float) mode);
            h.set (Param::dustDensity, 0.8f);
            h.set (Param::dustGrain, 0.4f);
            DustSource src;
            const auto buf = renderSource (src, h, 2.0);
            expect (std::abs (meanOf (buf)) < 2.0e-3,
                    juce::String (dustModeName (mode)) + " DC = " + juce::String (meanOf (buf)));
        }
    }

    //--------------------------------------------------------------------------
    void testDustSafety()
    {
        beginTest ("DUST: every mode is finite, bounded and DC free at extreme settings and sample rates");
        struct Extreme { float density, color, grain, jitter, pitch, position, spread, stereo; };
        const Extreme sets[] =
        {
            { 0.0f, 0.0f, 0.0f, 0.0f, -24.0f, 0.0f, 0.0f, 0.0f },
            { 1.0f, 1.0f, 1.0f, 1.0f,  24.0f, 1.0f, 1.0f, 1.0f },
            { 1.0f, 0.0f, 0.0f, 1.0f,  24.0f, 1.0f, 0.0f, 1.0f },
            { 1.0f, 1.0f, 0.0f, 0.0f, -24.0f, 0.5f, 1.0f, 0.5f },
        };

        for (double sr : { 44100.0, 48000.0, 96000.0 })
        {
            for (int block : { 32, 512 })
            {
                for (int mode = 0; mode < kNumDustModes; ++mode)
                {
                    for (const auto& e : sets)
                    {
                        for (int midi : { 12, 108 })
                        {
                            Harness h;
                            h.sr = sr;
                            h.block = block;
                            h.setNote (midi);
                            h.set (Param::dustMode, (float) mode);
                            h.set (Param::dustLevel, 1.0f);
                            h.set (Param::dustDensity, e.density);
                            h.set (Param::dustColor, e.color);
                            h.set (Param::dustGrain, e.grain);
                            h.set (Param::dustJitter, e.jitter);
                            h.set (Param::dustPitch, e.pitch);
                            h.set (Param::dustPosition, e.position);
                            h.set (Param::dustSpread, e.spread);
                            h.set (Param::dustStereo, e.stereo);

                            DustSource src;
                            const auto buf = renderSource (src, h, 0.2);
                            const juce::String tag = juce::String (dustModeName (mode)) + " @" + juce::String (sr)
                                                   + "/" + juce::String (block) + " note " + juce::String (midi);
                            expect (allFinite (buf), "non-finite output: " + tag);
                            expect (peakOf (buf) <= 1.0f, "peak above ceiling: " + tag
                                                          + " peak=" + juce::String (peakOf (buf)));
                            // At MIDI 12 the window holds only a few cycles, so compare the
                            // mean with the signal's own level rather than with an absolute floor.
                            expect (std::abs (meanOf (buf)) < 0.25 * rmsOf (buf) + 2.0e-3,
                                    "DC offset: " + tag + " dc=" + juce::String (meanOf (buf))
                                    + " rms=" + juce::String (rmsOf (buf)));
                        }
                    }
                }
            }
        }
    }

    //--------------------------------------------------------------------------
    void testImpactDeterminism()
    {
        beginTest ("IMPACT: a strike is reproducible and per-strike randomness follows the note id");
        for (int mode = 0; mode < kNumImpactModes; ++mode)
        {
            Harness h;
            h.set (Param::impactMode, (float) mode);
            h.set (Param::impactRandom, 0.6f);
            h.set (Param::impactRate, 0.5f);

            ImpactSource a, b;
            h.note.noteId = 4;
            const auto bufA = renderSource (a, h, 0.6, 0.5);
            const auto bufB = renderSource (b, h, 0.6, 0.5);
            expect (identical (bufA, bufB), juce::String (impactModeName (mode)) + " is not reproducible");

            ImpactSource c;
            h.note.noteId = 5;
            const auto bufC = renderSource (c, h, 0.6, 0.5);
            expect (! identical (bufA, bufC),
                    juce::String (impactModeName (mode)) + " ignored the note id with random 0.6");
            expect (rmsOf (bufA) > 1.0e-5, juce::String (impactModeName (mode)) + " was silent");
        }

        beginTest ("IMPACT: velocity sensitivity scales the strike");
        {
            Harness soft;
            soft.set (Param::impactMode, 3.0f);
            soft.set (Param::impactVelocity, 1.0f);
            soft.note.velocity = 0.2f;
            Harness loud = soft;
            loud.note.velocity = 1.0f;

            ImpactSource a, b;
            const float softPeak = peakOf (renderSource (a, soft, 0.5, 0.05));
            const float loudPeak = peakOf (renderSource (b, loud, 0.5, 0.05));
            expect (loudPeak > softPeak * 2.5f, "velocity sensitivity is too weak: "
                                                + juce::String (softPeak) + " -> " + juce::String (loudPeak));

            Harness fixed = soft;
            fixed.set (Param::impactVelocity, 0.0f);
            Harness fixedLoud = loud;
            fixedLoud.set (Param::impactVelocity, 0.0f);
            ImpactSource c, d;
            const float p1 = peakOf (renderSource (c, fixed, 0.5, 0.05));
            const float p2 = peakOf (renderSource (d, fixedLoud, 0.5, 0.05));
            expectWithinAbsoluteError (p1, p2, 1.0e-6f);
        }
    }

    //--------------------------------------------------------------------------
    void testImpactLength()
    {
        beginTest ("IMPACT: length stretches the -40 dB decay time of every mode");
        for (int mode = 0; mode < kNumImpactModes; ++mode)
        {
            Harness shortH;
            shortH.set (Param::impactMode, (float) mode);
            shortH.set (Param::impactRandom, 0.0f);
            shortH.set (Param::impactCurve, 0.5f);
            shortH.set (Param::impactLength, 0.05f);
            Harness longH = shortH;
            longH.set (Param::impactLength, 0.8f);

            ImpactSource a, b;
            const double dShort = decayTime40 (renderSource (a, shortH, 3.0, 0.02), shortH.sr, 0.001);
            const double dLong = decayTime40 (renderSource (b, longH, 3.0, 0.02), longH.sr, 0.001);
            expect (dLong > dShort * 1.5,
                    juce::String (impactModeName (mode)) + " length did not stretch the decay: "
                    + juce::String (dShort) + " s -> " + juce::String (dLong) + " s");
        }

        beginTest ("IMPACT: curve reshapes the envelope without changing the mode");
        {
            Harness punchy;
            punchy.set (Param::impactMode, 3.0f);
            punchy.set (Param::impactLength, 0.5f);
            punchy.set (Param::impactRandom, 0.0f);
            punchy.set (Param::impactCurve, 0.0f);
            Harness rounded = punchy;
            rounded.set (Param::impactCurve, 1.0f);

            ImpactSource a, b;
            const double dPunchy = decayTime40 (renderSource (a, punchy, 3.0, 0.02), punchy.sr);
            const double dRounded = decayTime40 (renderSource (b, rounded, 3.0, 0.02), rounded.sr);
            expect (dRounded > dPunchy, "curve did not round the envelope: "
                                        + juce::String (dPunchy) + " s vs " + juce::String (dRounded) + " s");
        }
    }

    //--------------------------------------------------------------------------
    void testImpactRepeatRate()
    {
        beginTest ("IMPACT: the repeat rate delivers the expected number of strikes per second");
        // rate maps exponentially over 0.5 .. 40 Hz; solve for a few musical values.
        const double targets[] = { 4.0, 8.0, 20.0 };
        for (double hz : targets)
        {
            const float rate = (float) (std::log (hz / 0.5) / std::log (80.0));
            Harness h;
            h.set (Param::impactMode, 1.0f);            // CLICK: short and easy to count
            h.set (Param::impactLength, 0.05f);
            h.set (Param::impactRandom, 0.0f);          // exact timing
            h.set (Param::impactRate, rate);
            h.set (Param::impactHardness, 0.7f);

            ImpactSource src;
            const auto buf = renderSource (src, h, 1.0, 10.0);
            const int strikes = countOnsets (buf, h.sr, 0.2f, 0.5 / hz);
            const int expected = (int) std::lround (hz) + 1;   // the note-on strike plus the repeats
            expect (std::abs (strikes - expected) <= 1,
                    "repeat rate " + juce::String (hz) + " Hz produced " + juce::String (strikes)
                    + " strikes, expected " + juce::String (expected));
        }

        beginTest ("IMPACT: repeats stop at note-off but the tail keeps ringing");
        {
            Harness h;
            h.set (Param::impactMode, 3.0f);
            h.set (Param::impactLength, 0.25f);
            h.set (Param::impactRandom, 0.0f);
            h.set (Param::impactRate, 0.7f);

            ImpactSource src;
            const auto buf = renderSource (src, h, 2.0, 0.5);
            const int before = countOnsets (buf, h.sr, 0.25f, 0.02);

            juce::AudioBuffer<float> after (2, (int) (0.9 * h.sr));
            for (int i = 0; i < after.getNumSamples(); ++i)
            {
                after.setSample (0, i, buf.getSample (0, (int) (1.0 * h.sr) + i));
                after.setSample (1, i, buf.getSample (1, (int) (1.0 * h.sr) + i));
            }
            expect (before > 3, "no repeats while the key was held: " + juce::String (before));
            expect (rmsOf (after) < 1.0e-4, "strikes kept firing after note-off: " + juce::String (rmsOf (after)));
        }

        beginTest ("IMPACT: a roll with long tails keeps firing instead of filling the pool");
        {
            // 40 Hz repeats with 1.5 s tails need far more strikes than the pool
            // holds. Stealing the quietest slot (and carrying its last sample into
            // a short decaying residue) must keep the roll going at a steady rate.
            Harness h;
            h.set (Param::impactMode, 5.0f);        // DAMPED SINE: the longest tails
            h.set (Param::impactLength, 1.0f);
            h.set (Param::impactRandom, 0.0f);
            h.set (Param::impactRate, 1.0f);        // 40 Hz

            ImpactSource src;
            const auto buf = renderSource (src, h, 2.0, 3.0);

            auto onsetsIn = [&buf, &h] (double from, double to)
            {
                const int a = (int) (from * h.sr), b = (int) (to * h.sr);
                juce::AudioBuffer<float> slice (2, b - a);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = a; i < b; ++i) slice.setSample (ch, i - a, buf.getSample (ch, i));
                return countOnsets (slice, h.sr, 0.3f, 0.012);
            };

            const int early = onsetsIn (0.05, 0.55);
            const int late = onsetsIn (1.45, 1.95);
            expect (early >= 15, "the roll never got going: " + juce::String (early));
            expect (late >= early / 2, "the roll died once the strike pool filled: "
                                       + juce::String (early) + " -> " + juce::String (late));

            // Stealing a slot must not tear the waveform.
            float maxJump = 0.0f;
            for (int i = 1; i < buf.getNumSamples(); ++i)
                maxJump = juce::jmax (maxJump, std::abs (buf.getSample (0, i) - buf.getSample (0, i - 1)));
            expect (maxJump < 0.25f, "strike stealing produced a step of " + juce::String (maxJump));
        }

        beginTest ("IMPACT: rate 0 fires exactly one strike and then reports inactive");
        {
            Harness h;
            h.set (Param::impactMode, 1.0f);
            h.set (Param::impactLength, 0.1f);
            h.set (Param::impactRandom, 0.0f);
            h.set (Param::impactRate, 0.0f);

            ImpactSource src;
            const auto buf = renderSource (src, h, 1.0, 10.0);
            expectEquals (countOnsets (buf, h.sr, 0.2f, 0.01), 1);
            expect (! src.isActive(), "the source stayed active after its only strike decayed");
        }
    }

    //--------------------------------------------------------------------------
    void testImpactCleanliness()
    {
        beginTest ("IMPACT: strikes start and end without a click and carry no DC");
        for (int mode = 0; mode < kNumImpactModes; ++mode)
        {
            Harness h;
            h.set (Param::impactMode, (float) mode);
            h.set (Param::impactLength, 0.3f);
            h.set (Param::impactRandom, 0.0f);
            h.set (Param::impactRate, 0.0f);

            ImpactSource src;
            const auto buf = renderSource (src, h, 3.0, 0.02);
            const float peak = peakOf (buf);
            const juce::String tag (impactModeName (mode));

            expect (allFinite (buf), tag + " produced non-finite samples");
            expect (peak <= 1.0f, tag + " exceeded the ceiling: " + juce::String (peak));
            expect (std::abs (meanOf (buf)) < 1.0e-4, tag + " has DC: " + juce::String (meanOf (buf)));
            expect (std::abs (buf.getSample (0, buf.getNumSamples() - 1)) < 1.0e-4,
                    tag + " was truncated instead of decaying");

            if (mode != 0)   // IMPULSE is a transient by definition; the rest ramp in
                expect (std::abs (buf.getSample (0, 0)) < 0.25f * peak,
                        tag + " starts with a step: " + juce::String (buf.getSample (0, 0)));
        }
    }

    //--------------------------------------------------------------------------
    void testImpactSafety()
    {
        beginTest ("IMPACT: every mode stays finite and bounded at extreme settings and sample rates");
        struct Extreme { float hardness, brightness, length, curve, random, rate; };
        const Extreme sets[] =
        {
            { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
            { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
            { 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f },
            { 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f },
        };

        for (double sr : { 44100.0, 48000.0, 96000.0 })
        {
            for (int block : { 32, 1024 })
            {
                for (int mode = 0; mode < kNumImpactModes; ++mode)
                {
                    for (const auto& e : sets)
                    {
                        for (int midi : { 12, 108 })
                        {
                            Harness h;
                            h.sr = sr;
                            h.block = block;
                            h.setNote (midi);
                            h.note.velocity = 1.0f;
                            h.set (Param::impactMode, (float) mode);
                            h.set (Param::impactLevel, 1.0f);
                            h.set (Param::impactHardness, e.hardness);
                            h.set (Param::impactBrightness, e.brightness);
                            h.set (Param::impactLength, e.length);
                            h.set (Param::impactCurve, e.curve);
                            h.set (Param::impactRandom, e.random);
                            h.set (Param::impactRate, e.rate);

                            ImpactSource src;
                            const auto buf = renderSource (src, h, 0.4, 0.35);
                            const juce::String tag = juce::String (impactModeName (mode)) + " @" + juce::String (sr)
                                                   + "/" + juce::String (block) + " note " + juce::String (midi);
                            expect (allFinite (buf), "non-finite output: " + tag);
                            expect (peakOf (buf) <= 1.0f, "peak above ceiling: " + tag
                                                          + " peak=" + juce::String (peakOf (buf)));
                            expect (std::abs (meanOf (buf)) < 0.25 * rmsOf (buf) + 2.0e-3,
                                    "DC offset: " + tag + " dc=" + juce::String (meanOf (buf))
                                    + " rms=" + juce::String (rmsOf (buf)));
                        }
                    }
                }
            }
        }
    }

    //--------------------------------------------------------------------------
    void testSourceEngineIntegration()
    {
        beginTest ("SourceEngine: selecting DUST or IMPACT renders that source");
        Harness h;
        h.set (Param::dustMode, 0.0f);
        h.set (Param::impactMode, 3.0f);
        h.set (Param::impactLength, 0.6f);

        auto renderEngine = [&h] (int selected, bool layer)
        {
            Harness local = h;
            local.set (Param::sourceSelected, (float) selected);
            local.set (Param::sourceMode, layer ? 1.0f : 0.0f);

            const int total = (int) (0.5 * local.sr);
            juce::AudioBuffer<float> buf (2, total);
            buf.clear();

            SourceEngine engine;
            engine.prepare (local.sr, local.block);
            engine.reset();
            engine.noteOn (local.note, local.params);
            for (int pos = 0; pos < total; pos += local.block)
            {
                const int n = juce::jmin (local.block, total - pos);
                const auto ctx = local.context (n);
                engine.render (buf.getWritePointer (0) + pos, buf.getWritePointer (1) + pos, n, ctx, local.note);
            }
            return buf;
        };

        const auto wave = renderEngine (0, false);
        const auto dust = renderEngine (1, false);
        const auto impact = renderEngine (2, false);
        const auto sample = renderEngine (3, false);     // SAMPLE and GESTURE joined the engine in Phase 15
        const auto gesture = renderEngine (4, false);

        expect (rmsOf (wave) > 0.01, "WAVE is silent through SourceEngine");
        expect (rmsOf (dust) > 0.01, "DUST is silent through SourceEngine: " + juce::String (rmsOf (dust)));
        expect (rmsOf (impact) > 0.001, "IMPACT is silent through SourceEngine: " + juce::String (rmsOf (impact)));
        expect (! identical (wave, dust), "DUST rendered the WAVE source");
        expect (! identical (dust, impact), "IMPACT rendered the DUST source");

        beginTest ("SourceEngine: LAYER mode sums the sources");
        {
            const auto layered = renderEngine (0, true);
            double maxError = 0.0;
            for (int i = 0; i < layered.getNumSamples(); ++i)
            {
                const double sum = (double) wave.getSample (0, i) + dust.getSample (0, i) + impact.getSample (0, i)
                                 + sample.getSample (0, i) + gesture.getSample (0, i);
                maxError = juce::jmax (maxError, std::abs (sum - (double) layered.getSample (0, i)));
            }
            expect (maxError < 1.0e-5, "LAYER is not the sum of the sources, max error " + juce::String (maxError));
            expect (rmsOf (layered) > rmsOf (wave), "LAYER is quieter than WAVE alone");
        }
    }
};

static DustImpactTests dustImpactTests;

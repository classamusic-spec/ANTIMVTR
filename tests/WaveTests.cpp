#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/source/WaveSource.h"
#include "dsp/source/WavetableGenerator.h"
#include "dsp/SynthEngine.h"

#include <vector>

using namespace am;

namespace
{
    constexpr double kPiD = 3.14159265358979323846;

    /** Drives a bare WaveSource so a test can dial in an exact frequency. */
    struct WaveHarness
    {
        WaveSource   src;
        ParamValues  params = ParameterRegistry::defaults();
        std::vector<float> left, right;

        WaveHarness()
        {
            set (Param::wavePhaseRandom, 0.0f);
            set (Param::waveDetune, 0.0f);
            set (Param::waveSpread, 0.0f);
        }

        void set (Param p, float v) noexcept { params[(size_t) paramIndex (p)] = v; }

        void render (double sr, double freq, int total, int block = 256, uint32_t noteId = 7)
        {
            NoteState note;
            note.noteId = noteId;
            note.midiNote = 60;
            note.gate = true;
            note.baseFrequency = freq;
            note.frequency = freq;

            src.prepare (sr, block);
            src.noteOn (note, params);

            RenderContext ctx;
            ctx.sampleRate = sr;
            ctx.params = &params;

            left.assign ((size_t) total, 0.0f);
            right.assign ((size_t) total, 0.0f);
            for (int pos = 0; pos < total; pos += block)
            {
                const int n = juce::jmin (block, total - pos);
                ctx.numSamples = n;
                src.render (left.data() + pos, right.data() + pos, n, ctx, note);
            }
        }
    };

    enum class Win { rectangular, hann, blackmanHarris };

    /** Magnitude spectrum of `1 << order` samples starting at `offset`. */
    std::vector<float> spectrum (const std::vector<float>& x, int offset, int order, Win w = Win::hann)
    {
        const int size = 1 << order;
        juce::dsp::FFT fft (order);
        std::vector<float> buf ((size_t) size * 2, 0.0f);
        for (int i = 0; i < size; ++i)
        {
            const size_t j = (size_t) (offset + i);
            buf[(size_t) i] = j < x.size() ? x[j] : 0.0f;
        }
        if (w != Win::rectangular)
        {
            juce::dsp::WindowingFunction<float> window ((size_t) size,
                w == Win::hann ? juce::dsp::WindowingFunction<float>::hann
                               : juce::dsp::WindowingFunction<float>::blackmanHarris);
            window.multiplyWithWindowingTable (buf.data(), (size_t) size);
        }
        fft.performFrequencyOnlyForwardTransform (buf.data(), true);
        buf.resize ((size_t) (size / 2));
        return buf;
    }

    float rmsOf (const std::vector<float>& x, int from, int to)
    {
        double s = 0.0; int n = 0;
        for (int i = juce::jmax (0, from); i < juce::jmin (to, (int) x.size()); ++i) { s += (double) x[(size_t) i] * x[(size_t) i]; ++n; }
        return n > 0 ? (float) std::sqrt (s / n) : 0.0f;
    }

    float peakOf (const std::vector<float>& x, int from, int to)
    {
        float p = 0.0f;
        for (int i = juce::jmax (0, from); i < juce::jmin (to, (int) x.size()); ++i) p = juce::jmax (p, std::abs (x[(size_t) i]));
        return p;
    }

    bool allFinite (const std::vector<float>& x)
    {
        for (auto v : x) if (! std::isfinite (v)) return false;
        return true;
    }

    /**
        Splits a spectrum into energy that sits on the harmonic grid of f0 and
        energy that does not (= aliasing / inharmonic junk), ignoring the DC
        region. Returns the ratio junk/fundamental in dB.
    */
    struct GridSplit
    {
        double harmonicPower = 0.0, junkPower = 0.0, fundamental = 0.0, loudestHarmonic = 0.0;
        double loudestJunkHz = 0.0, loudestJunk = 0.0;
        /** Loudest off-grid component relative to the loudest real partial. */
        double junkPeakDb() const { return 20.0 * std::log10 ((loudestJunk + 1.0e-20) / (loudestHarmonic + 1.0e-20)); }
        double junkPowerDb() const { return 10.0 * std::log10 ((junkPower + 1.0e-20) / (harmonicPower + 1.0e-20)); }
    };

    /**
        Splits a spectrum into energy that sits on the harmonic grid of f0 and
        energy that does not (= aliasing / inharmonic junk). `tolBins` must
        cover the analysis window's main lobe; bins below `skipHz` are ignored.
    */
    GridSplit splitOnHarmonicGrid (const std::vector<float>& mag, double sr, int fftSize,
                                   double f0, int tolBins, double skipHz)
    {
        GridSplit g;
        const double binHz = sr / (double) fftSize;
        const double tolHz = (double) tolBins * binHz;
        const int firstBin = juce::jmax (1, (int) std::ceil (skipHz / binHz));
        for (int k = firstBin; k < (int) mag.size(); ++k)
        {
            const double hz = (double) k * binHz;
            const double m = (double) mag[(size_t) k];
            const double nearest = std::round (hz / f0);
            const bool onGrid = nearest >= 1.0 && std::abs (hz - nearest * f0) <= tolHz;
            if (onGrid) { g.harmonicPower += m * m; g.loudestHarmonic = juce::jmax (g.loudestHarmonic, m); }
            else
            {
                g.junkPower += m * m;
                if (m > g.loudestJunk) { g.loudestJunk = m; g.loudestJunkHz = hz; }
            }
            if (std::abs (hz - f0) <= tolHz) g.fundamental = juce::jmax (g.fundamental, m);
        }
        return g;
    }
}

//==============================================================================
class WaveTests : public juce::UnitTest
{
public:
    WaveTests() : juce::UnitTest ("WAVE source", "wave") {}

    void runTest() override
    {
        tableGeometry();
        frequencyAccuracy();
        aliasing();
        unisonBehaviour();
        determinism();
        extremes();
        scanBehaviour();
        noteStartTransient();
        displayHelper();
        rateAndBlockIndependence();
        engineIntegration();
    }

private:
    static void selectBank (WaveHarness& h, int bank, float position)
    {
        h.set (Param::waveTable, (float) bank);
        h.set (Param::wavePosition, position);
    }

    //==========================================================================
    void tableGeometry()
    {
        beginTest ("Banks are built once, RMS matched and free of DC");

        Wavetables::prewarm();
        logMessage ("wavetable memory: " + juce::String ((double) Wavetables::memoryBytes() / (1024.0 * 1024.0), 2) + " MiB");

        for (int b = 0; b < kWaveNumBanks; ++b)
        {
            const auto& bank = Wavetables::bank (b);
            expectEquals (bank.numFrames, kWaveFramesPerBank);

            float minRms = 1.0e9f, maxRms = 0.0f, maxCrest = 0.0f, maxPeak = 0.0f;
            int crestFrame = 0;
            for (int f = 0; f < bank.numFrames; ++f)
            {
                for (int l = 0; l < kWaveNumLevels; ++l)
                {
                    const auto& lv = bank.levels[(size_t) l];
                    expect (lv.data != nullptr);
                    expectEquals (lv.length, waveLevelLength (l));
                    const float* t = lv.frame (f);
                    double sum = 0.0, dc = 0.0;
                    for (int i = 0; i < lv.length; ++i)
                    {
                        expect (std::isfinite (t[i]), juce::String ("non-finite sample in ") + bank.name);
                        expect (std::abs (t[i]) <= 2.5f);
                        sum += (double) t[i] * t[i];
                        dc += (double) t[i];
                    }
                    expect (std::abs (dc / lv.length) < 1.0e-3, juce::String ("DC in ") + bank.name);
                    if (l == 0)
                    {
                        const float rms = (float) std::sqrt (sum / lv.length);
                        minRms = juce::jmin (minRms, rms);
                        maxRms = juce::jmax (maxRms, rms);
                        float pk = 0.0f;
                        for (int i = 0; i < lv.length; ++i) pk = juce::jmax (pk, std::abs (t[i]));
                        maxPeak = juce::jmax (maxPeak, pk);
                        const float crest = pk / juce::jmax (1.0e-9f, rms);
                        if (crest > maxCrest) { maxCrest = crest; crestFrame = f; }
                    }
                }
            }
            // RMS matching: no more than 3 dB spread between the frames of a bank.
            const float spreadDb = juce::Decibels::gainToDecibels (maxRms / juce::jmax (1.0e-6f, minRms));
            logMessage (juce::String (bank.name) + ": frame RMS spread " + juce::String (spreadDb, 2)
                        + " dB, max peak " + juce::String (maxPeak, 3)
                        + ", max crest " + juce::String (maxCrest, 2) + " (frame " + juce::String (crestFrame) + ")");
            expect (spreadDb < 0.5f, juce::String ("level jump across frames of ") + bank.name);
            expect (maxPeak < 1.1f, juce::String ("frame peak too hot in ") + bank.name);
        }
    }

    //==========================================================================
    void frequencyAccuracy()
    {
        beginTest ("Every bank plays the requested frequency at 44.1 / 48 / 96 kHz");

        const double rates[] = { 44100.0, 48000.0, 96000.0 };
        for (double sr : rates)
        {
            const double freq = sr / 1024.0;              // exactly 1024 samples per period
            for (int b = 0; b < kWaveNumBanks; ++b)
            {
                WaveHarness h;
                selectBank (h, b, 0.0f);
                h.render (sr, freq, 8192, 512);
                expect (allFinite (h.left));

                // The waveform repeats exactly one period later.
                float worst = 0.0f;
                for (int i = 2048; i < 6144; ++i)
                    worst = juce::jmax (worst, std::abs (h.left[(size_t) i] - h.left[(size_t) (i + 1024)]));
                expect (worst < 1.0e-5f,
                        juce::String (Wavetables::bankName (b)) + " period mismatch " + juce::String (worst)
                            + " at " + juce::String (sr));

                // 4096 samples = exactly four periods, so a rectangular window
                // leaks nothing: every partial must land on an exact bin.
                const auto mag = spectrum (h.left, 2048, 12, Win::rectangular);
                const auto g = splitOnHarmonicGrid (mag, sr, 4096, freq, 0, 5.0);
                expect (g.junkPowerDb() < -50.0,
                        juce::String (Wavetables::bankName (b)) + " off-grid energy "
                            + juce::String (g.junkPowerDb(), 1) + " dB");
            }
        }
    }

    //==========================================================================
    void aliasing()
    {
        beginTest ("A saw at C8 is band limited (and far cleaner than a naive saw)");

        const double sr = 48000.0;
        const double f0 = midiNoteToHz (108.0);
        constexpr int order = 15, fftSize = 1 << order;
        const double binHz = sr / (double) fftSize;

        WaveHarness h;
        selectBank (h, 0, 5.0f / 15.0f);           // BASIC frame 5 = saw
        h.render (sr, f0, fftSize + 8192, 256);
        expect (allFinite (h.left));

        // Blackman-Harris keeps window leakage below -90 dB so what is left is
        // genuinely aliasing rather than analysis skirt.
        const auto mag = spectrum (h.left, 4096, order, Win::blackmanHarris);
        const auto g = splitOnHarmonicGrid (mag, sr, fftSize, f0, 10, 40.0);

        // 1. Nothing spurious below the fundamental.
        double worstBelow = 0.0, worstBelowHz = 0.0;
        for (int k = (int) std::ceil (40.0 / binHz); k < (int) ((f0 - 15.0 * binHz) / binHz); ++k)
            if ((double) mag[(size_t) k] > worstBelow) { worstBelow = mag[(size_t) k]; worstBelowHz = k * binHz; }
        const double belowDb = 20.0 * std::log10 ((worstBelow + 1.0e-20) / (g.fundamental + 1.0e-20));
        logMessage ("sub-fundamental worst: " + juce::String (belowDb, 1) + " dB at " + juce::String (worstBelowHz, 1) + " Hz");
        expect (belowDb < -60.0, "spurious energy below the fundamental: " + juce::String (belowDb, 1) + " dB");

        // 2. No component off the harmonic grid anywhere (that is aliasing).
        logMessage ("band-limited saw: loudest alias " + juce::String (g.junkPeakDb(), 1)
                    + " dB at " + juce::String (g.loudestJunkHz, 1) + " Hz");
        expect (g.junkPeakDb() < -60.0, "aliasing at C8: " + juce::String (g.junkPeakDb(), 1) + " dB");

        // 3. Nothing left above the highest legal harmonic.
        const int topHarmonic = (int) std::floor (sr * 0.5 / f0);
        double aboveTop = 0.0;
        for (int k = (int) (((double) topHarmonic + 0.5) * f0 / binHz); k < (int) mag.size(); ++k)
            aboveTop += (double) mag[(size_t) k] * mag[(size_t) k];
        const double aboveDb = 10.0 * std::log10 ((aboveTop + 1.0e-20) / (g.fundamental * g.fundamental + 1.0e-20));
        logMessage ("energy above harmonic " + juce::String (topHarmonic) + ": " + juce::String (aboveDb, 1) + " dB");
        expect (aboveDb < -60.0);

        // 4. The same measurement on a naive saw must be dramatically worse.
        std::vector<float> naive ((size_t) (fftSize + 8192), 0.0f);
        {
            double phase = 0.0;
            const double inc = f0 / sr;
            for (size_t i = 0; i < naive.size(); ++i)
            {
                naive[i] = (float) (2.0 * phase - 1.0) * 0.4f;
                phase += inc; if (phase >= 1.0) phase -= 1.0;
            }
        }
        const auto naiveMag = spectrum (naive, 4096, order, Win::blackmanHarris);
        const auto ng = splitOnHarmonicGrid (naiveMag, sr, fftSize, f0, 10, 40.0);
        logMessage ("naive saw: loudest alias " + juce::String (ng.junkPeakDb(), 1)
                    + " dB at " + juce::String (ng.loudestJunkHz, 1) + " Hz");
        expect (ng.junkPeakDb() - g.junkPeakDb() > 40.0,
                "band-limited saw is only " + juce::String (ng.junkPeakDb() - g.junkPeakDb(), 1) + " dB cleaner than naive");

        // 5. Morph and sync must not reintroduce audible aliasing at C6.
        const double c6 = midiNoteToHz (84.0);
        auto measure = [&] (const char* label, float morph, float sync, float ratio)
        {
            WaveHarness hard;
            selectBank (hard, 0, 5.0f / 15.0f);
            hard.set (Param::waveMorph, morph);
            hard.set (Param::waveSync, sync);
            hard.set (Param::waveModRatio, ratio);
            hard.render (sr, c6, fftSize + 8192, 256);
            expect (allFinite (hard.left));
            const auto hm = spectrum (hard.left, 4096, order, Win::blackmanHarris);
            const auto hg = splitOnHarmonicGrid (hm, sr, fftSize, c6, 10, 40.0);
            logMessage (juce::String (label) + " at C6: loudest off-grid " + juce::String (hg.junkPeakDb(), 1)
                        + " dB, total " + juce::String (hg.junkPowerDb(), 1) + " dB");
            return hg.junkPeakDb();
        };

        const double morphOnly = measure ("morph 1.0", 1.0f, 0.0f, 2.0f);
        const double syncOnly  = measure ("sync 0.7",  0.0f, 0.7f, 4.0f);
        const double syncFull  = measure ("sync 1.0",  0.0f, 1.0f, 8.0f);
        const double both      = measure ("sync+morph", 0.8f, 0.7f, 4.0f);

        expect (morphOnly < -60.0, "morph aliasing: " + juce::String (morphOnly, 1) + " dB");
        expect (syncOnly  < -45.0, "sync aliasing: " + juce::String (syncOnly, 1) + " dB");
        expect (syncFull  < -45.0, "sync aliasing (full): " + juce::String (syncFull, 1) + " dB");
        expect (both      < -45.0, "sync + morph aliasing: " + juce::String (both, 1) + " dB");
    }

    //==========================================================================
    void unisonBehaviour()
    {
        beginTest ("Unison detune is symmetric and gain compensated");

        const double sr = 48000.0;
        const double f0 = 1000.0;
        constexpr int order = 17, fftSize = 1 << order;   // 2.7 s: several beat cycles
        const int total = fftSize + 8192;

        WaveHarness mono;
        selectBank (mono, 0, 5.0f / 15.0f);
        mono.set (Param::waveUnison, 1.0f);
        mono.render (sr, f0, total, 256);

        WaveHarness uni;
        selectBank (uni, 0, 5.0f / 15.0f);
        uni.set (Param::waveUnison, 8.0f);
        uni.set (Param::waveDetune, 1.0f);
        uni.set (Param::waveSpread, 0.8f);
        uni.render (sr, f0, total, 256);
        expect (allFinite (uni.left) && allFinite (uni.right));

        const float monoRms = rmsOf (mono.left, 4096, total);
        const float uniRms  = rmsOf (uni.left, 4096, total);
        const float monoPeak = peakOf (mono.left, 4096, total);
        const float uniPeak  = peakOf (uni.left, 4096, total);
        const float rmsDb = juce::Decibels::gainToDecibels (uniRms / juce::jmax (1.0e-6f, monoRms));
        logMessage ("unison 8 vs 1: RMS " + juce::String (rmsDb, 2) + " dB, peak "
                    + juce::String (uniPeak, 3) + " vs " + juce::String (monoPeak, 3));
        expect (std::abs (rmsDb) < 2.0f, "unison changes loudness by " + juce::String (rmsDb, 2) + " dB");
        expect (uniPeak < monoPeak * 2.6f, "unison peak runs away");
        expect (uniPeak < 1.8f, "unison peak exceeds headroom: " + juce::String (uniPeak, 3));
        expect (uniPeak > 0.05f);

        // Detune must be symmetric: the centroid of the fundamental cluster,
        // measured in cents (not Hz, which would bias upwards) on the mid
        // signal (a single channel is biased by the stereo spread), stays on f0.
        std::vector<float> mid (uni.left.size(), 0.0f);
        for (size_t i = 0; i < mid.size(); ++i) mid[i] = 0.5f * (uni.left[i] + uni.right[i]);
        const auto mag = spectrum (mid, 4096, order, Win::blackmanHarris);
        const double binHz = sr / (double) fftSize;
        double num = 0.0, den = 0.0;
        for (int k = (int) (f0 * 0.92 / binHz); k <= (int) (f0 * 1.08 / binHz); ++k)
        {
            const double m = (double) mag[(size_t) k] * mag[(size_t) k];
            num += m * (1200.0 * std::log2 ((double) k * binHz / f0));
            den += m;
        }
        const double cents = den > 0.0 ? num / den : 0.0;
        logMessage ("detuned cluster centroid: " + juce::String (cents, 2) + " cents from f0");
        expect (std::abs (cents) < 2.0, "detune is not symmetric: " + juce::String (cents, 2) + " cents");

        // Detune scaling stays subtle at low settings.
        WaveHarness subtle;
        selectBank (subtle, 0, 5.0f / 15.0f);
        subtle.set (Param::waveUnison, 8.0f);
        subtle.set (Param::waveDetune, 0.15f);          // the parameter default
        subtle.render (sr, f0, 65536, 256);
        std::vector<float> subtleMid (subtle.left.size(), 0.0f);
        for (size_t i = 0; i < subtleMid.size(); ++i) subtleMid[i] = 0.5f * (subtle.left[i] + subtle.right[i]);
        const auto sm = spectrum (subtleMid, 4096, 16, Win::blackmanHarris);
        const double sBin = sr / 65536.0;
        double width = 0.0, sden = 0.0;
        for (int k = (int) (f0 * 0.96 / sBin); k <= (int) (f0 * 1.04 / sBin); ++k)
        {
            const double m = (double) sm[(size_t) k] * sm[(size_t) k];
            width += m * std::abs (1200.0 * std::log2 ((double) k * sBin / f0));
            sden += m;
        }
        width = sden > 0.0 ? width / sden : 0.0;
        logMessage ("detune 0.15 mean spread: " + juce::String (width, 2) + " cents");
        expect (width < 4.0, "detune is not subtle at low settings");

        // Stereo spread gives a real image without collapsing or losing balance.
        double corr = 0.0, powL = 0.0, powR = 0.0;
        for (int i = 4096; i < total; ++i)
        {
            corr += (double) uni.left[(size_t) i] * uni.right[(size_t) i];
            powL += (double) uni.left[(size_t) i] * uni.left[(size_t) i];
            powR += (double) uni.right[(size_t) i] * uni.right[(size_t) i];
        }
        const double correlation = corr / std::sqrt (juce::jmax (1.0e-12, powL * powR));
        logMessage ("unison stereo correlation: " + juce::String (correlation, 3));
        expect (correlation < 0.98 && correlation > -0.2);
        expectWithinAbsoluteError (powL, powR, powL * 0.25);

        // Every unison count is bounded and roughly equal in level.
        for (int count = 1; count <= 8; ++count)
        {
            WaveHarness h;
            selectBank (h, 0, 5.0f / 15.0f);
            h.set (Param::waveUnison, (float) count);
            h.set (Param::waveDetune, 0.5f);
            h.set (Param::waveSpread, 0.5f);
            h.render (sr, 220.0, 131072, 256);
            const float db = juce::Decibels::gainToDecibels (rmsOf (h.left, 4096, 131072) / juce::jmax (1.0e-6f, monoRms));
            expect (std::abs (db) < 3.0f, "unison " + juce::String (count) + " level " + juce::String (db, 2) + " dB");
            expect (peakOf (h.left, 0, 131072) < 1.8f, "unison " + juce::String (count) + " peak too hot");
        }
    }

    //==========================================================================
    void determinism()
    {
        beginTest ("Same note id renders identical audio; different ids differ");

        for (int b = 0; b < kWaveNumBanks; ++b)
        {
            WaveHarness a, c, d;
            for (auto* h : { &a, &c, &d })
            {
                selectBank (*h, b, 0.42f);
                h->set (Param::wavePhaseRandom, 1.0f);
                h->set (Param::waveUnison, 5.0f);
                h->set (Param::waveDetune, 0.4f);
                h->set (Param::waveScan, 0.3f);
                h->set (Param::waveMorph, 0.35f);
            }
            a.render (48000.0, 261.63, 6000, 128, 11u);
            c.render (48000.0, 261.63, 6000, 128, 11u);
            d.render (48000.0, 261.63, 6000, 128, 12u);

            bool identical = true, differs = false;
            for (size_t i = 0; i < a.left.size(); ++i)
            {
                identical = identical && a.left[i] == c.left[i] && a.right[i] == c.right[i];
                differs = differs || a.left[i] != d.left[i];
            }
            expect (identical, juce::String (Wavetables::bankName (b)) + " is not deterministic");
            expect (differs, juce::String (Wavetables::bankName (b)) + " ignores phase randomisation");
        }
    }

    //==========================================================================
    void extremes()
    {
        beginTest ("Extreme parameters stay finite and bounded");

        const double rates[] = { 44100.0, 96000.0 };
        for (double sr : rates)
        {
            for (int b = 0; b < kWaveNumBanks; ++b)
            {
                for (float pos : { 0.0f, 0.5f, 1.0f })
                {
                    WaveHarness h;
                    selectBank (h, b, pos);
                    h.set (Param::waveMorph, 1.0f);
                    h.set (Param::waveScan, 1.0f);
                    h.set (Param::waveUnison, 8.0f);
                    h.set (Param::waveDetune, 1.0f);
                    h.set (Param::waveSpread, 1.0f);
                    h.set (Param::waveFM, 1.0f);
                    h.set (Param::wavePM, 1.0f);
                    h.set (Param::waveAM, 1.0f);
                    h.set (Param::waveRing, 1.0f);
                    h.set (Param::waveSync, 1.0f);
                    h.set (Param::waveModRatio, 16.0f);
                    h.set (Param::wavePhaseRandom, 1.0f);
                    h.set (Param::waveOctave, 3.0f);
                    h.set (Param::waveSemi, 12.0f);

                    h.render (sr, 55.0, 8192, 64);
                    expect (allFinite (h.left) && allFinite (h.right),
                            juce::String (Wavetables::bankName (b)) + " produced non-finite output");
                    expect (peakOf (h.left, 0, 8192) < 8.0f, juce::String (Wavetables::bankName (b)) + " ran away");
                }
            }
        }

        // Sub-audio and beyond-Nyquist requests must not explode either.
        for (double freq : { 0.05, 1.0, 12000.0, 40000.0 })
        {
            WaveHarness h;
            selectBank (h, 0, 0.5f);
            h.set (Param::waveSync, 1.0f);
            h.set (Param::waveModRatio, 16.0f);
            h.render (48000.0, freq, 4096, 128);
            expect (allFinite (h.left), "freq " + juce::String (freq) + " broke the oscillator");
            expect (peakOf (h.left, 0, 4096) < 4.0f);
        }
    }

    //==========================================================================
    void scanBehaviour()
    {
        beginTest ("Scan advances deterministically and actually moves the frame");

        WaveHarness still, moving, again;
        for (auto* h : { &still, &moving, &again }) selectBank (*h, 5, 0.0f);
        moving.set (Param::waveScan, 0.5f);
        again.set (Param::waveScan, 0.5f);

        const int total = 96000;
        still.render (48000.0, 220.0, total, 256);
        moving.render (48000.0, 220.0, total, 256);
        again.render (48000.0, 220.0, total, 256);

        bool identical = true;
        for (size_t i = 0; i < moving.left.size(); ++i) identical = identical && moving.left[i] == again.left[i];
        expect (identical, "scan is not deterministic");

        // The timbre changes over time when scanning and does not when it is off.
        auto centroid = [] (const std::vector<float>& x, int offset)
        {
            const auto m = spectrum (x, offset, 12);
            double num = 0.0, den = 0.0;
            for (size_t k = 1; k < m.size(); ++k) { num += (double) m[k] * (double) k; den += (double) m[k]; }
            return den > 0.0 ? num / den : 0.0;
        };
        const double movedA = centroid (moving.left, 8192);
        const double movedB = centroid (moving.left, 60000);
        const double stillA = centroid (still.left, 8192);
        const double stillB = centroid (still.left, 60000);
        logMessage ("scan centroid drift: " + juce::String (movedA, 1) + " -> " + juce::String (movedB, 1)
                    + " (static: " + juce::String (stillA, 1) + " -> " + juce::String (stillB, 1) + ")");
        expect (std::abs (movedB - movedA) > 0.5, "scan did not move the frame");
        expectWithinAbsoluteError (stillB, stillA, juce::jmax (0.5, stillA * 0.02));
    }

    //==========================================================================
    void noteStartTransient()
    {
        beginTest ("Note starts are click free at every random phase");

        // A sine frame isolates the note-on transient from the waveform's own
        // slew: with the raised-cosine guard the first sample is silent and the
        // opening slope stays two orders of magnitude below a raw phase jump.
        float worstFirst = 0.0f, worstStep = 0.0f;
        for (uint32_t id = 1; id <= 96u; ++id)
        {
            WaveHarness h;
            selectBank (h, 0, 0.0f);
            h.set (Param::wavePhaseRandom, 1.0f);
            h.render (48000.0, 220.0, 4096, 128, id);
            worstFirst = juce::jmax (worstFirst, std::abs (h.left[0]));
            for (int i = 1; i < 64; ++i)
                worstStep = juce::jmax (worstStep, std::abs (h.left[(size_t) i] - h.left[(size_t) (i - 1)]));
        }
        logMessage ("note start: worst first sample " + juce::String (worstFirst, 5)
                    + ", worst opening step " + juce::String (worstStep, 5));
        expect (worstFirst < 0.005f);
        expect (worstStep < 0.06f);

        // For every bank the opening slope never exceeds the steady-state slope
        // of the waveform itself by more than the guard ramp.
        for (int b = 0; b < kWaveNumBanks; ++b)
        {
            WaveHarness h;
            selectBank (h, b, 0.5f);
            h.set (Param::wavePhaseRandom, 1.0f);
            h.set (Param::waveUnison, 4.0f);
            h.render (48000.0, 220.0, 8192, 128, 5u);
            float start = 0.0f, steady = 0.0f;
            for (int i = 1; i < 64; ++i) start = juce::jmax (start, std::abs (h.left[(size_t) i] - h.left[(size_t) (i - 1)]));
            for (int i = 4096; i < 8192; ++i) steady = juce::jmax (steady, std::abs (h.left[(size_t) i] - h.left[(size_t) (i - 1)]));
            expect (start <= juce::jmax (steady, 0.06f) + 1.0e-4f,
                    juce::String (Wavetables::bankName (b)) + " clicks on note start ("
                        + juce::String (start, 4) + " vs " + juce::String (steady, 4) + ")");
        }
    }

    //==========================================================================
    void displayHelper()
    {
        beginTest ("Wavetables::renderDisplayFrame matches the audio table");

        const double sr = 48000.0;
        // A long period keeps the oscillator on mip level 0 for every morph
        // setting (the warp slope is folded into the level selection), so the
        // audio path and the display helper read exactly the same table.
        const int    period = 8192;
        const double freq = sr / (double) period;

        for (int b = 0; b < kWaveNumBanks; ++b)
        {
            for (float pos : { 0.0f, 0.37f, 1.0f })
            {
                for (float morph : { 0.0f, 0.55f })
                {
                    WaveHarness h;
                    selectBank (h, b, pos);
                    h.set (Param::waveMorph, morph);
                    h.render (sr, freq, period * 3, 512);

                    std::vector<float> display ((size_t) period, 0.0f);
                    Wavetables::renderDisplayFrame (b, pos, morph, display.data(), period);

                    // Find the alignment (the sync corrector adds a constant delay).
                    double best = 1.0e30; int bestOffset = -1;
                    for (int off = 0; off < period; ++off)
                    {
                        double s = 0.0;
                        for (int i = 0; i < period; i += 8)
                        {
                            const double d = (double) h.left[(size_t) (period + i)] - display[(size_t) ((i + off) % period)];
                            s += d * d;
                        }
                        if (s < best) { best = s; bestOffset = off; }
                    }
                    double err = 0.0;
                    for (int i = 0; i < period; ++i)
                    {
                        const double d = (double) h.left[(size_t) (period + i)] - display[(size_t) ((i + bestOffset) % period)];
                        err += d * d;
                    }
                    err = std::sqrt (err / period);
                    expect (err < 1.0e-4,
                            juce::String (Wavetables::bankName (b)) + " display mismatch rms " + juce::String (err));
                    expectEquals (bestOffset, period - (WaveSource::kLatencySamples - 1));
                }
            }
        }
    }

    //==========================================================================
    void rateAndBlockIndependence()
    {
        beginTest ("Output is independent of the host block size, at every sample rate");

        const double rates[] = { 44100.0, 48000.0, 88200.0, 96000.0 };
        for (double sr : rates)
        {
            std::vector<float> reference;
            for (int block : { 32, 64, 128, 512, 1024 })
            {
                WaveHarness h;
                selectBank (h, 1, 0.6f);
                h.set (Param::waveUnison, 4.0f);
                h.set (Param::waveDetune, 0.3f);
                h.set (Param::waveMorph, 0.4f);
                h.render (sr, 330.0, 32768, block);
                expect (allFinite (h.left), "block " + juce::String (block) + " produced non-finite output");

                if (reference.empty()) reference = h.left;
                else
                {
                    float worst = 0.0f;
                    for (int i = 2048; i < 32768; ++i)
                        worst = juce::jmax (worst, std::abs (reference[(size_t) i] - h.left[(size_t) i]));
                    expect (worst < 1.0e-5f,
                            "block size " + juce::String (block) + " changed the output by " + juce::String (worst));
                }
            }
        }
    }

    //==========================================================================
    void engineIntegration()
    {
        beginTest ("WAVE through the full engine: level, no DC, no clicks");

        const double sr = 48000.0;
        const int block = 128;
        SynthEngine engine;
        engine.prepare (sr, block);

        auto params = ParameterRegistry::defaults();
        params[(size_t) paramIndex (Param::shapeMix)] = 0.0f;      // dry source
        params[(size_t) paramIndex (Param::waveTable)] = 0.0f;
        params[(size_t) paramIndex (Param::wavePosition)] = 5.0f / 15.0f;
        params[(size_t) paramIndex (Param::waveUnison)] = 6.0f;
        params[(size_t) paramIndex (Param::waveDetune)] = 0.35f;
        params[(size_t) paramIndex (Param::waveLevel)] = 0.7f;
        engine.control().resetTo (params);

        const int total = (int) (sr * 1.5);
        juce::AudioBuffer<float> out (2, total);
        out.clear();
        TransportInfo transport;
        for (int pos = 0; pos < total; pos += block)
        {
            const int n = juce::jmin (block, total - pos);
            juce::MidiBuffer midi;
            if (pos == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 48, 0.9f), 0);
            if (pos <= (int) (sr * 1.0) && pos + n > (int) (sr * 1.0)) midi.addEvent (juce::MidiMessage::noteOff (1, 48), 0);
            juce::AudioBuffer<float> chunk (out.getArrayOfWritePointers(), 2, pos, n);
            engine.process (chunk, midi, params, transport);
        }

        double dc = 0.0; float peak = 0.0f; bool finite = true; float onsetStep = 0.0f;
        for (int i = 0; i < total; ++i)
        {
            const float v = out.getSample (0, i);
            if (! std::isfinite (v)) finite = false;
            dc += (double) v;
            peak = juce::jmax (peak, std::abs (v));
            if (i > 0 && i < 64) onsetStep = juce::jmax (onsetStep, std::abs (v - out.getSample (0, i - 1)));
        }
        dc /= total;
        logMessage ("engine dry WAVE: peak " + juce::String (peak, 3) + ", dc " + juce::String (dc, 6)
                    + ", onset step " + juce::String (onsetStep, 5));
        expect (finite);
        expect (peak > 0.05f && peak < 1.0f, "peak " + juce::String (peak, 3));
        expect (std::abs (dc) < 1.0e-3);
        expect (onsetStep < 0.05f, "the note onset clicks");

        const auto safety = engine.diagnostics().safety.snapshot();
        expectEquals ((int) safety.counts[(int) SafetyEvent::NaN], 0);
    }
};

static WaveTests waveTests;

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/fracture/FractureEngine.h"
#include "dev/diagnostics/Diagnostics.h"

using namespace am;

namespace
{
    /** Drives a FractureEngine directly with a chosen parameter set. */
    struct Harness
    {
        Harness (double sampleRate = 48000.0, int blockSize = 256)
            : sr (sampleRate), block (blockSize)
        {
            params = ParameterRegistry::defaults();
            engine.prepare (sr, block);
            set (Param::fractureOn, 1.0f);
            set (Param::fractureAmount, 1.0f);
            set (Param::fractureMix, 1.0f);
            set (Param::fractureSpread, 0.0f);
            set (Param::fractureSequence, 0.0f);
            set (Param::fractureRandom, 0.0f);
            set (Param::fractureFeedback, 0.0f);
            set (Param::fractureDelay, 0.0f);
            set (Param::fractureEvolve, 0.0f);
            set (Param::fractureTone, 0.5f);
            set (Param::fracturePitch, 0.0f);
            set (Param::fractureProbability, 1.0f);
        }

        void set (Param p, float v) noexcept { params[(size_t) paramIndex (p)] = v; }

        /** Neutral table: every fragment passes its band through untouched. */
        static std::unique_ptr<FractureTable> unityTable()
        {
            auto t = std::make_unique<FractureTable>();
            for (auto& f : t->fragments)
            {
                f = Fragment();
                f.spread = 0.0f;
                f.decay = 0.0f;
            }
            for (auto& s : t->steps) s = SequencerStep();
            return t;
        }

        void process (juce::AudioBuffer<float>& buffer)
        {
            RenderContext ctx;
            ctx.sampleRate  = sr;
            ctx.params      = &params;
            ctx.transport   = transport;
            ctx.diagnostics = &diag;

            const int total = buffer.getNumSamples();
            for (int pos = 0; pos < total; pos += block)
            {
                const int n = juce::jmin (block, total - pos);
                ctx.numSamples = n;
                engine.process (buffer.getWritePointer (0) + pos, buffer.getWritePointer (1) + pos, n, ctx);
            }
        }

        FractureEngine engine;
        ParamValues params;
        TransportInfo transport;
        Diagnostics diag;
        double sr;
        int block;
    };

    /** Deterministic broadband test signal: three sines plus low-level noise. */
    void fillTestSignal (juce::AudioBuffer<float>& b, double sr, uint32_t seed = 7)
    {
        Rng r (seed);
        const int n = b.getNumSamples();
        for (int i = 0; i < n; ++i)
        {
            const double t = (double) i / sr;
            const float v = 0.30f * (float) std::sin (kTwoPi * 220.0 * t)
                          + 0.20f * (float) std::sin (kTwoPi * 1310.0 * t)
                          + 0.12f * (float) std::sin (kTwoPi * 5300.0 * t)
                          + 0.02f * r.nextBipolar();
            b.setSample (0, i, v);
            b.setSample (1, i, v * 0.9f);
        }
    }

    void fillSine (juce::AudioBuffer<float>& b, double sr, double hz, float amp = 0.4f)
    {
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float v = amp * (float) std::sin (kTwoPi * hz * (double) i / sr);
            b.setSample (0, i, v);
            b.setSample (1, i, v);
        }
    }

    /** Error between `out` (shifted by `delay`) and `ref`, in dB relative to `ref`. */
    double errorDb (const juce::AudioBuffer<float>& ref, const juce::AudioBuffer<float>& out,
                    int delay, int from, int to)
    {
        double e = 0.0, s = 0.0;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = from; i < to; ++i)
            {
                const double a = ref.getSample (ch, i);
                const double b = out.getSample (ch, i + delay);
                e += (a - b) * (a - b);
                s += a * a;
            }
        return 10.0 * std::log10 ((e + 1.0e-30) / (s + 1.0e-30));
    }

    /** Magnitude spectrum peak (in Hz) of a mono window of `buffer`. */
    double spectralPeakHz (const juce::AudioBuffer<float>& buffer, int start, int fftSize, double sr,
                           double* energyAtHz = nullptr, double probeHz = 0.0)
    {
        const int order = (int) std::round (std::log2 ((double) fftSize));
        juce::dsp::FFT fft (order);
        std::vector<float> data ((size_t) (2 * fftSize), 0.0f);
        for (int i = 0; i < fftSize; ++i)
        {
            const float w = 0.5f - 0.5f * (float) std::cos (kTwoPi * i / fftSize);
            data[(size_t) i] = buffer.getSample (0, start + i) * w;
        }
        fft.performRealOnlyForwardTransform (data.data(), true);

        int peak = 1;
        double best = 0.0;
        for (int k = 1; k < fftSize / 2; ++k)
        {
            const double m = (double) data[(size_t) (2 * k)] * data[(size_t) (2 * k)]
                           + (double) data[(size_t) (2 * k + 1)] * data[(size_t) (2 * k + 1)];
            if (m > best) { best = m; peak = k; }
        }

        if (energyAtHz != nullptr)
        {
            const int k = juce::jlimit (1, fftSize / 2 - 1, (int) std::lround (probeHz * fftSize / sr));
            double sum = 0.0;
            for (int j = k - 1; j <= k + 1; ++j)
                sum += (double) data[(size_t) (2 * j)] * data[(size_t) (2 * j)]
                     + (double) data[(size_t) (2 * j + 1)] * data[(size_t) (2 * j + 1)];
            *energyAtHz = sum;
        }
        return (double) peak * sr / (double) fftSize;
    }

    float peakOf (const juce::AudioBuffer<float>& b, int from = 0, int to = -1)
    {
        const int last = to < 0 ? b.getNumSamples() : to;
        float p = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = from; i < last; ++i) p = juce::jmax (p, std::abs (b.getSample (ch, i)));
        return p;
    }

    bool allFinite (const juce::AudioBuffer<float>& b)
    {
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            for (int i = 0; i < b.getNumSamples(); ++i)
                if (! std::isfinite (b.getSample (ch, i))) return false;
        return true;
    }
}

//==============================================================================
class FractureTests : public juce::UnitTest
{
public:
    FractureTests() : juce::UnitTest ("Fracture", "fracture") {}

    void runTest() override
    {
        testTableJson();
        testBypassTransparency();
        testLatencyReporting();
        testStftIdentity();
        testFragmentPitchShift();
        testFragmentDelay();
        testFeedbackStability();
        testSequencerTiming();
        testDeterminism();
        testModesAndRates();
        testActivity();
    }

private:
    //==========================================================================
    void testTableJson()
    {
        beginTest ("Fragment table JSON round trip");

        auto original = FractureTable::makeDefault();
        original.numFragments = 32;
        original.numSteps = 13;
        original.fragments[3].pitch = -7.5f;
        original.fragments[3].delay = 0.42f;
        original.fragments[3].pan = -0.8f;
        original.fragments[7].feedback = 0.77f;
        original.steps[5].mask = 0xA5A5A5A5u;
        original.steps[5].gate = 0.31f;
        original.steps[5].evolve = 0.66f;
        original.steps[5].shape = 0.25f;

        const auto json = juce::JSON::toString (original.toVar());
        const auto parsed = juce::JSON::parse (json);
        const auto back = FractureTable::fromVar (parsed);

        expectEquals (back.numFragments, 32);
        expectEquals (back.numSteps, 13);
        for (int f = 0; f < kMaxFractureFragments; ++f)
        {
            const auto& a = original.fragments[(size_t) f];
            const auto& b = back.fragments[(size_t) f];
            expectWithinAbsoluteError (b.pitch, a.pitch, 1.0e-4f);
            expectWithinAbsoluteError (b.delay, a.delay, 1.0e-4f);
            expectWithinAbsoluteError (b.pan, a.pan, 1.0e-4f);
            expectWithinAbsoluteError (b.decay, a.decay, 1.0e-4f);
            expectWithinAbsoluteError (b.feedback, a.feedback, 1.0e-4f);
            expectWithinAbsoluteError (b.probability, a.probability, 1.0e-4f);
            expectWithinAbsoluteError (b.spread, a.spread, 1.0e-4f);
            expectWithinAbsoluteError (b.gain, a.gain, 1.0e-4f);
        }
        for (int s = 0; s < kMaxSequencerSteps; ++s)
        {
            const auto& a = original.steps[(size_t) s];
            const auto& b = back.steps[(size_t) s];
            expect (a.mask == b.mask, "step mask mismatch at " + juce::String (s));
            expectWithinAbsoluteError (b.gate, a.gate, 1.0e-4f);
            expectWithinAbsoluteError (b.pitch, a.pitch, 1.0e-4f);
            expectWithinAbsoluteError (b.pan, a.pan, 1.0e-4f);
            expectWithinAbsoluteError (b.gain, a.gain, 1.0e-4f);
            expectWithinAbsoluteError (b.evolve, a.evolve, 1.0e-4f);
            expectWithinAbsoluteError (b.shape, a.shape, 1.0e-4f);
        }

        // Tolerant reader: garbage in, defaults out.
        const auto fallback = FractureTable::fromVar (juce::var());
        expectEquals (fallback.numFragments, FractureTable::makeDefault().numFragments);

        auto* partial = new juce::DynamicObject();
        partial->setProperty ("numSteps", 999);
        const auto clamped = FractureTable::fromVar (juce::var (partial));
        expectEquals (clamped.numSteps, kMaxSequencerSteps, "out-of-range step count should clamp");
    }

    //==========================================================================
    void testBypassTransparency()
    {
        beginTest ("Bypass is bit-exact with zero latency");

        for (int block : { 32, 128, 1024 })
        {
            Harness h (48000.0, block);
            h.set (Param::fractureOn, 0.0f);

            juce::AudioBuffer<float> ref (2, 24000);
            fillTestSignal (ref, 48000.0);
            juce::AudioBuffer<float> out (ref);
            h.process (out);

            expectEquals (h.engine.latencySamples(), 0, "bypass should add no latency");

            int mismatches = 0;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < ref.getNumSamples(); ++i)
                    if (ref.getSample (ch, i) != out.getSample (ch, i)) ++mismatches;
            expectEquals (mismatches, 0, "bypass is not bit-exact at block " + juce::String (block));
        }

        // amount = 0 with the effect switched on is equally transparent.
        Harness h (48000.0, 256);
        h.set (Param::fractureAmount, 0.0f);
        juce::AudioBuffer<float> ref (2, 24000);
        fillTestSignal (ref, 48000.0);
        juce::AudioBuffer<float> out (ref);
        h.process (out);
        expectEquals (h.engine.latencySamples(), 0, "amount 0 should add no latency");
        expectWithinAbsoluteError (peakOf (out), peakOf (ref), 1.0e-7f);
        expect (errorDb (ref, out, 0, 0, ref.getNumSamples()) < -100.0, "amount 0 is not transparent");
    }

    //==========================================================================
    void testLatencyReporting()
    {
        beginTest ("Reported latency matches the measured impulse delay");

        for (double sr : { 44100.0, 48000.0, 96000.0 })
        {
            Harness h (sr, 256);
            h.engine.publishTable (Harness::unityTable());

            const int expectedLatency = h.engine.fftSize();
            const int impulseAt = (int) (0.4 * sr);
            const int total = impulseAt + expectedLatency * 4;

            juce::AudioBuffer<float> buf (2, total);
            buf.clear();
            buf.setSample (0, impulseAt, 1.0f);
            buf.setSample (1, impulseAt, 1.0f);
            h.process (buf);

            expectEquals (h.engine.latencySamples(), expectedLatency,
                          "engaged latency should equal the FFT size at " + juce::String (sr));
            expectEquals (h.engine.maxLatencySamples(), expectedLatency);
            expectEquals (h.engine.hopSize(), expectedLatency / 4);
            expect (h.engine.fftSize() == (sr > 64000.0 ? 2048 : 1024),
                    "unexpected FFT size at " + juce::String (sr));

            int measured = -1;
            float best = 0.0f;
            for (int i = impulseAt; i < total; ++i)
                if (std::abs (buf.getSample (0, i)) > best)
                {
                    best = std::abs (buf.getSample (0, i));
                    measured = i - impulseAt;
                }

            expect (best > 0.5f, "impulse did not survive the transform at " + juce::String (sr));
            expect (std::abs (measured - expectedLatency) <= 1,
                    "impulse delay " + juce::String (measured) + " != reported "
                    + juce::String (expectedLatency) + " at " + juce::String (sr));
        }
    }

    //==========================================================================
    void testStftIdentity()
    {
        beginTest ("STFT reconstruction is transparent with a neutral fragment table");

        for (double sr : { 44100.0, 48000.0, 96000.0 })
        {
            for (int block : { 32, 256, 1024 })
            {
                Harness h (sr, block);
                h.engine.publishTable (Harness::unityTable());

                const int latency = h.engine.fftSize();
                const int total = (int) (1.0 * sr);
                juce::AudioBuffer<float> ref (2, total);
                fillTestSignal (ref, sr);
                juce::AudioBuffer<float> out (ref);
                h.process (out);

                const int from = (int) (0.3 * sr);
                const int to = total - latency - 16;
                const double err = errorDb (ref, out, latency, from, to);
                expect (err < -60.0, "reconstruction error " + juce::String (err, 1)
                                      + " dB at " + juce::String (sr) + "/" + juce::String (block));
                expect (allFinite (out));
            }
        }
    }

    //==========================================================================
    void testFragmentPitchShift()
    {
        beginTest ("Per-fragment pitch shift moves a sine to the octave");

        const double sr = 48000.0;
        Harness h (sr, 256);
        h.set (Param::fractureFragments, 0.0f);   // 8 bands: fragment 0 covers DC … ~380 Hz

        auto t = Harness::unityTable();
        t->fragments[0].pitch = 12.0f;
        h.engine.publishTable (std::move (t));

        const int total = (int) (2.0 * sr);
        juce::AudioBuffer<float> buf (2, total);
        fillSine (buf, sr, 220.0, 0.5f);
        h.process (buf);

        expect (allFinite (buf));

        double at220 = 0.0, at440 = 0.0;
        const int start = (int) (1.0 * sr);
        const double peakHz = spectralPeakHz (buf, start, 8192, sr, &at440, 440.0);
        spectralPeakHz (buf, start, 8192, sr, &at220, 220.0);

        expectWithinAbsoluteError (peakHz, 440.0, 8.0);
        expect (at440 > at220 * 40.0, "fundamental not suppressed: 440 = " + juce::String (at440)
                                        + " 220 = " + juce::String (at220));

        // The global pitch offset does the same thing across every fragment.
        Harness g (sr, 256);
        g.engine.publishTable (Harness::unityTable());
        g.set (Param::fracturePitch, -12.0f);
        juce::AudioBuffer<float> down (2, total);
        fillSine (down, sr, 880.0, 0.5f);
        g.process (down);
        expect (allFinite (down));
        expectWithinAbsoluteError (spectralPeakHz (down, start, 8192, sr), 440.0, 8.0);
    }

    //==========================================================================
    void testFragmentDelay()
    {
        beginTest ("Per-fragment delay produces a delayed copy at the expected time");

        const double sr = 48000.0;
        Harness h (sr, 256);
        h.set (Param::fractureDelay, 1.0f);       // global scale: fragment delay 1.0 == 1 s

        const int hop = 256;
        const int wantFrames = 20;
        const float normalised = (float) (wantFrames * hop) / (float) sr;

        auto t = Harness::unityTable();
        for (auto& f : t->fragments) f.delay = normalised;
        h.engine.publishTable (std::move (t));

        const int burstAt = (int) (0.6 * sr);      // after the delay slew has settled
        const int total = burstAt + (int) (0.5 * sr);
        juce::AudioBuffer<float> buf (2, total);
        buf.clear();
        for (int i = 0; i < 240; ++i)              // 5 ms burst
        {
            const float v = 0.6f * (float) std::sin (kTwoPi * 1000.0 * i / sr)
                          * (float) std::sin (kPi * i / 240.0);
            buf.setSample (0, burstAt + i, v);
            buf.setSample (1, burstAt + i, v);
        }
        h.process (buf);

        expect (allFinite (buf));

        // Envelope peak of the output burst.
        int measured = -1;
        float best = 0.0f;
        for (int i = burstAt; i < total; ++i)
            if (std::abs (buf.getSample (0, i)) > best) { best = std::abs (buf.getSample (0, i)); measured = i; }

        const int expectedPos = burstAt + h.engine.fftSize() + wantFrames * hop + 120;
        expect (best > 0.2f, "delayed burst is missing (peak " + juce::String (best) + ")");
        expect (std::abs (measured - expectedPos) <= hop,
                "burst at " + juce::String (measured) + ", expected " + juce::String (expectedPos));

        // Nothing should arrive before the delay has elapsed.
        float early = 0.0f;
        for (int i = burstAt; i < burstAt + h.engine.fftSize() + wantFrames * hop - hop; ++i)
            early = juce::jmax (early, std::abs (buf.getSample (0, i)));
        expect (early < best * 0.2f, "signal leaked ahead of the delay time");
    }

    //==========================================================================
    void testFeedbackStability()
    {
        beginTest ("Maximum feedback stays bounded for 10 seconds");

        const double sr = 48000.0;
        Harness h (sr, 512);
        h.set (Param::fractureFeedback, 1.0f);
        h.set (Param::fractureDecay, 1.0f);
        h.set (Param::fractureDelay, 0.25f);
        h.set (Param::fractureSpread, 1.0f);
        h.set (Param::fractureFragments, 2.0f);   // 32 fragments
        h.set (Param::fractureMode, 0.0f);

        auto t = Harness::unityTable();
        for (auto& f : t->fragments) { f.feedback = 1.0f; f.decay = 1.0f; f.delay = 0.08f; }
        h.engine.publishTable (std::move (t));

        const int total = (int) (10.0 * sr);
        juce::AudioBuffer<float> buf (2, total);
        buf.clear();
        juce::AudioBuffer<float> excite (2, (int) (2.0 * sr));
        fillTestSignal (excite, sr);
        for (int ch = 0; ch < 2; ++ch)
            buf.copyFrom (ch, 0, excite, ch, 0, excite.getNumSamples());

        h.process (buf);

        expect (allFinite (buf), "feedback produced non-finite samples");
        const float peak = peakOf (buf);
        expect (peak < 12.0f, "feedback grew without bound (peak " + juce::String (peak) + ")");
        expectEquals ((int) h.diag.safety.count (SafetyEvent::NaN), 0, "NaN during feedback");

        // Energy in the last second must not exceed the excited region.
        double lateEnergy = 0.0, earlyEnergy = 0.0;
        for (int i = (int) (9.0 * sr); i < total; ++i) lateEnergy += (double) buf.getSample (0, i) * buf.getSample (0, i);
        for (int i = (int) (1.0 * sr); i < (int) (2.0 * sr); ++i) earlyEnergy += (double) buf.getSample (0, i) * buf.getSample (0, i);
        expect (lateEnergy <= earlyEnergy * 4.0 + 1.0e-6,
                "tail energy grew: late " + juce::String (lateEnergy) + " early " + juce::String (earlyEnergy));

        logMessage ("  feedback peak " + juce::String (peak, 3)
                    + ", FeedbackClamp events " + juce::String ((int) h.diag.safety.count (SafetyEvent::FeedbackClamp)));

        // Drive the delay line past its ceiling on purpose: the clamp must engage,
        // report itself and still leave a finite, bounded output.
        Harness hot (sr, 512);
        hot.set (Param::fractureFeedback, 1.0f);
        hot.set (Param::fractureDecay, 1.0f);
        hot.set (Param::fractureDelay, 0.05f);
        hot.set (Param::fractureFragments, 2.0f);

        auto loud = Harness::unityTable();
        for (auto& f : loud->fragments) { f.feedback = 1.0f; f.decay = 1.0f; f.delay = 0.02f; f.gain = 2.0f; }
        hot.engine.publishTable (std::move (loud));

        juce::AudioBuffer<float> big (2, (int) (4.0 * sr));
        fillTestSignal (big, sr);
        big.applyGain (6.0f);
        hot.process (big);

        expect (allFinite (big), "clamped feedback produced non-finite samples");
        expect (hot.diag.safety.count (SafetyEvent::FeedbackClamp) > 0,
                "the delay-line ceiling never engaged under deliberate overdrive");
        expectEquals ((int) hot.diag.safety.count (SafetyEvent::NaN), 0);
        logMessage ("  overdriven: peak " + juce::String (peakOf (big), 2)
                    + ", FeedbackClamp events "
                    + juce::String ((int) hot.diag.safety.count (SafetyEvent::FeedbackClamp)));
    }

    //==========================================================================
    void testSequencerTiming()
    {
        beginTest ("Sequencer steps land within one hop at 120 bpm 1/8");

        const double sr = 48000.0;
        const int hop = 256;
        FractureSequencer::Settings s;
        s.numSteps = 8;
        s.sync = true;
        s.division = 3;          // 1/8
        s.bpm = 120.0;
        s.swing = 0.0f;
        s.probability = 1.0f;
        s.randomAmount = 0.0f;
        s.seed = 11;

        const double expectedStep = 0.25 * sr;   // 1/8 at 120 bpm = 250 ms
        expectWithinAbsoluteError (FractureSequencer::stepSamples (s, sr), expectedStep, 1.0e-6);

        const auto table = FractureTable::makeDefault();
        FractureSequencer seq;
        seq.prepare (sr);
        seq.reset (s.seed);

        int lastIndex = -1;
        int stepsSeen = 0;
        const int totalHops = (int) (8.0 * sr) / hop;
        for (int f = 0; f < totalHops; ++f)
        {
            const int samplePos = f * hop;
            if (seq.advance (hop, s, table))
            {
                if (lastIndex >= 0)
                {
                    const double ideal = (double) stepsSeen * expectedStep;
                    expect (std::abs ((double) samplePos - ideal) <= (double) hop,
                            "step " + juce::String (stepsSeen) + " at " + juce::String (samplePos)
                            + " ideal " + juce::String (ideal));
                }
                ++stepsSeen;
                lastIndex = seq.stepIndex();
            }
        }
        expect (stepsSeen >= 30, "expected ~32 steps in 8 s, saw " + juce::String (stepsSeen));

        // Directions visit the expected indices.
        auto collect = [&] (FractureDirection dir, int count)
        {
            FractureSequencer q;
            q.prepare (sr);
            q.reset (5);
            auto settings = s;
            settings.direction = dir;
            settings.numSteps = 4;
            juce::Array<int> seen;
            for (int f = 0; seen.size() < count && f < 100000; ++f)
                if (q.advance (hop, settings, table)) seen.add (q.stepIndex());
            return seen;
        };

        const auto forward = collect (FractureDirection::Forward, 6);
        expectEquals (forward[0], 0); expectEquals (forward[1], 1);
        expectEquals (forward[2], 2); expectEquals (forward[3], 3); expectEquals (forward[4], 0);

        const auto backward = collect (FractureDirection::Backward, 3);
        expectEquals (backward[0], 3); expectEquals (backward[1], 2); expectEquals (backward[2], 1);

        const auto pingpong = collect (FractureDirection::PingPong, 7);
        expectEquals (pingpong[0], 0); expectEquals (pingpong[1], 1); expectEquals (pingpong[2], 2);
        expectEquals (pingpong[3], 3); expectEquals (pingpong[4], 2); expectEquals (pingpong[5], 1);

        // Swing lengthens even steps and shortens odd ones without changing the pair total.
        auto swung = s;
        swung.swing = 1.0f;
        FractureSequencer sw;
        sw.prepare (sr);
        sw.reset (11);
        sw.advance (hop, swung, table);
        const double even = sw.stepLengthSamples();
        int guard = 0;
        while (! sw.advance (hop, swung, table) && guard++ < 10000) {}
        const double odd = sw.stepLengthSamples();
        expect (even > odd, "swing did not lengthen the downbeat");
        expectWithinAbsoluteError (even + odd, 2.0 * expectedStep, expectedStep * 0.05);
    }

    //==========================================================================
    void testDeterminism()
    {
        beginTest ("A fixed seed reproduces the output exactly");

        const double sr = 48000.0;
        auto render = [sr] (int seed, int block)
        {
            Harness h (sr, block);
            h.set (Param::fractureMode, 3.0f);        // EVOLVE
            h.set (Param::fractureEvolve, 0.8f);
            h.set (Param::fractureRandom, 0.7f);
            h.set (Param::fractureProbability, 0.6f);
            h.set (Param::fractureSpread, 0.9f);
            h.set (Param::fractureFeedback, 0.5f);
            h.set (Param::fractureDelay, 0.4f);
            h.set (Param::fractureSeed, (float) seed);
            juce::AudioBuffer<float> b (2, (int) (1.5 * sr));
            fillTestSignal (b, sr);
            h.process (b);
            return b;
        };

        const auto a = render (11, 256);
        const auto b = render (11, 256);
        expect (allFinite (a));

        int mismatches = 0;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < a.getNumSamples(); ++i)
                if (a.getSample (ch, i) != b.getSample (ch, i)) ++mismatches;
        expectEquals (mismatches, 0, "same seed produced different audio");

        const auto c = render (4242, 256);
        double diff = 0.0;
        for (int i = 0; i < a.getNumSamples(); ++i)
            diff += std::abs ((double) a.getSample (0, i) - c.getSample (0, i));
        expect (diff > 1.0, "a different seed produced identical audio");
    }

    //==========================================================================
    void testModesAndRates()
    {
        beginTest ("Every mode stays finite and bounded at all rates and block sizes");

        for (double sr : { 44100.0, 48000.0, 96000.0 })
        {
            for (int block : { 32, 1024 })
            {
                for (int mode = 0; mode < (int) FractureMode::Count; ++mode)
                {
                    for (int frag = 0; frag < 3; ++frag)
                    {
                        Harness h (sr, block);
                        h.set (Param::fractureMode, (float) mode);
                        h.set (Param::fractureFragments, (float) frag);
                        h.set (Param::fractureSpread, 1.0f);
                        h.set (Param::fractureSequence, 1.0f);
                        h.set (Param::fractureRandom, 1.0f);
                        h.set (Param::fractureFeedback, 0.8f);
                        h.set (Param::fractureDelay, 0.6f);
                        h.set (Param::fractureDecay, 0.9f);
                        h.set (Param::fractureEvolve, 1.0f);
                        h.set (Param::fracturePitch, 7.0f);
                        h.set (Param::fractureTone, frag == 0 ? 0.0f : 1.0f);
                        h.set (Param::fractureSwing, 0.6f);
                        h.set (Param::fractureSteps, 32.0f);
                        h.set (Param::fractureDirection, (float) (mode % 4));
                        h.transport.isPlaying = true;
                        h.transport.bpm = 128.0;

                        juce::AudioBuffer<float> b (2, (int) (1.5 * sr));
                        fillTestSignal (b, sr);
                        h.process (b);

                        const juce::String tag = juce::String (sr) + "/" + juce::String (block)
                                               + " mode " + juce::String (mode) + " frag " + juce::String (frag);
                        expect (allFinite (b), "non-finite at " + tag);
                        expect (peakOf (b) < 16.0f, "runaway level at " + tag
                                                    + " peak " + juce::String (peakOf (b)));
                        expectEquals ((int) h.diag.safety.count (SafetyEvent::NaN), 0, "NaN at " + tag);
                    }
                }
            }
        }
    }

    //==========================================================================
    void testActivity()
    {
        beginTest ("Activity and fragment diagnostics are meaningful");

        const double sr = 48000.0;
        Harness h (sr, 256);
        h.set (Param::fractureFragments, 1.0f);   // 16
        h.engine.publishTable (Harness::unityTable());

        juce::AudioBuffer<float> b (2, (int) (1.0 * sr));
        fillTestSignal (b, sr);
        h.process (b);

        expect (h.engine.activity() > 0.8f, "wet-only activity should be near 1, got "
                                             + juce::String (h.engine.activity()));

        FractureEngine::FragmentActivity fa;
        h.engine.fillFragmentActivity (fa);
        expectEquals (fa.numFragments, 16);
        expect (fa.overall > 0.5f);

        int active = 0;
        for (int f = 0; f < fa.numFragments; ++f)
        {
            expect (fa.gain[(size_t) f] >= 0.0f && fa.gain[(size_t) f] <= 1.0f);
            expect (fa.energy[(size_t) f] >= 0.0f && fa.energy[(size_t) f] <= 1.0f);
            if (fa.energy[(size_t) f] > 0.0f) ++active;
        }
        expect (active > 0, "no fragment reported any energy");

        // Bypassed: activity falls back to zero.
        Harness off (sr, 256);
        off.set (Param::fractureOn, 0.0f);
        juce::AudioBuffer<float> c (2, (int) (0.5 * sr));
        fillTestSignal (c, sr);
        off.process (c);
        expectWithinAbsoluteError (off.engine.activity(), 0.0f, 1.0e-6f);

        // noteStarted() restarts the sequencer without upsetting the audio.
        h.engine.noteStarted();
        juce::AudioBuffer<float> d (2, 4096);
        fillTestSignal (d, sr);
        h.process (d);
        expect (allFinite (d));
    }
};

static FractureTests fractureTests;

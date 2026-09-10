#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/fx/SpaceEngine.h"
#include "dsp/fx/SpacePresets.h"
#include "dev/diagnostics/Diagnostics.h"

using namespace am;

namespace
{

//==============================================================================
/** Drives SpaceEngine directly: parameters in, stereo buffer out. */
struct Harness
{
    SpaceEngine engine;
    ParamValues params = ParameterRegistry::defaults();
    std::unique_ptr<Diagnostics> diag { std::make_unique<Diagnostics>() };
    TransportInfo transport;
    double sr = 48000.0;
    int block = 128;

    void prepare (double sampleRate, int blockSize)
    {
        sr = sampleRate;
        block = blockSize;
        engine.prepare (sampleRate, blockSize);
    }

    void set (Param p, float v) noexcept { params[(size_t) paramIndex (p)] = v; }
    float get (Param p) const noexcept { return params[(size_t) paramIndex (p)]; }

    /** Rack silent: every module off, EQ flat, TONE centred. */
    void allModulesOff() noexcept
    {
        for (auto p : { Param::spaceDistOn, Param::spaceChorusOn, Param::spaceDelayOn, Param::spaceGrainOn,
                        Param::spaceShiftOn, Param::spaceDiffuseOn, Param::spaceReverbOn, Param::spaceCompOn,
                        Param::spaceLimiterOn })
            set (p, 0.0f);

        set (Param::spaceEqLow, 0.0f);
        set (Param::spaceEqMid, 0.0f);
        set (Param::spaceEqHigh, 0.0f);
        set (Param::spaceTone, 0.5f);
        set (Param::spaceSize, 0.5f);
    }

    /** Isolates one module: everything off, full wet, macros neutral. */
    void solo (Param moduleOn) noexcept
    {
        allModulesOff();
        set (moduleOn, 1.0f);
        set (Param::spaceMix, 1.0f);
        set (Param::spaceFeedback, 0.0f);
    }

    RenderContext context (int n) noexcept
    {
        RenderContext ctx;
        ctx.sampleRate = sr;
        ctx.numSamples = n;
        ctx.params = &params;
        ctx.transport = transport;
        ctx.diagnostics = diag.get();
        return ctx;
    }

    /** Renders in place; `atBlock` (optional) can change parameters between blocks. */
    void render (juce::AudioBuffer<float>& buf, const std::function<void (Harness&, int)>& atBlock = {})
    {
        const int total = buf.getNumSamples();
        for (int pos = 0; pos < total; pos += block)
        {
            if (atBlock) atBlock (*this, pos);
            const int n = juce::jmin (block, total - pos);
            auto ctx = context (n);
            engine.process (buf.getWritePointer (0) + pos, buf.getWritePointer (1) + pos, n, ctx);
        }
    }
};

//==============================================================================
juce::AudioBuffer<float> makeBuffer (double sr, double seconds)
{
    juce::AudioBuffer<float> b (2, (int) (sr * seconds));
    b.clear();
    return b;
}

void addSine (juce::AudioBuffer<float>& b, double sr, double hz, float amplitude, int start = 0, int length = -1)
{
    const int end = length < 0 ? b.getNumSamples() : juce::jmin (b.getNumSamples(), start + length);
    for (int i = start; i < end; ++i)
    {
        const float v = amplitude * (float) std::sin (kTwoPi * hz * (double) (i - start) / sr);
        b.setSample (0, i, b.getSample (0, i) + v);
        b.setSample (1, i, b.getSample (1, i) + v);
    }
}

void addNoiseBurst (juce::AudioBuffer<float>& b, int start, int length, float amplitude, uint32_t seed = 7)
{
    Rng rng (seed);
    for (int i = start; i < juce::jmin (b.getNumSamples(), start + length); ++i)
    {
        const float v = amplitude * rng.nextBipolar();
        b.setSample (0, i, b.getSample (0, i) + v);
        b.setSample (1, i, b.getSample (1, i) + v * 0.85f);
    }
}

bool identical (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b) noexcept
{
    if (a.getNumSamples() != b.getNumSamples()) return false;
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < a.getNumSamples(); ++i)
            if (a.getSample (c, i) != b.getSample (c, i)) return false;
    return true;
}

bool allFinite (const juce::AudioBuffer<float>& b) noexcept
{
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (! std::isfinite (b.getSample (c, i))) return false;
    return true;
}

float peakOf (const juce::AudioBuffer<float>& b, int start = 0, int end = -1) noexcept
{
    const int last = end < 0 ? b.getNumSamples() : juce::jmin (end, b.getNumSamples());
    float p = 0.0f;
    for (int c = 0; c < 2; ++c)
        for (int i = juce::jmax (0, start); i < last; ++i)
            p = juce::jmax (p, std::abs (b.getSample (c, i)));
    return p;
}

float rmsOf (const juce::AudioBuffer<float>& b, int start, int end) noexcept
{
    double sum = 0.0;
    int n = 0;
    for (int i = juce::jmax (0, start); i < juce::jmin (end, b.getNumSamples()); ++i)
    {
        const double m = 0.5 * ((double) b.getSample (0, i) + (double) b.getSample (1, i));
        sum += m * m;
        ++n;
    }
    return n > 0 ? (float) std::sqrt (sum / n) : 0.0f;
}

/** Index of the loudest sample at or after `from` (mono sum). */
int peakIndex (const juce::AudioBuffer<float>& b, int from) noexcept
{
    int best = from;
    float bestValue = -1.0f;
    for (int i = juce::jmax (0, from); i < b.getNumSamples(); ++i)
    {
        const float v = std::abs (b.getSample (0, i)) + std::abs (b.getSample (1, i));
        if (v > bestValue) { bestValue = v; best = i; }
    }
    return best;
}

/** Time (seconds) for the level to fall 20 dB below its peak after `fromSample`. */
double decayT20 (const juce::AudioBuffer<float>& b, double sr, int fromSample)
{
    const int window = juce::jmax (16, (int) (sr * 0.02));
    std::vector<float> env;
    for (int start = fromSample; start + window <= b.getNumSamples(); start += window)
        env.push_back (rmsOf (b, start, start + window));
    if (env.empty()) return 0.0;

    const float peak = *std::max_element (env.begin(), env.end());
    if (peak <= 1.0e-9f) return 0.0;
    const size_t peakIdx = (size_t) std::distance (env.begin(), std::max_element (env.begin(), env.end()));
    const float threshold = peak * 0.1f;                       // -20 dB
    for (size_t i = peakIdx; i < env.size(); ++i)
        if (env[i] <= threshold)
            return (double) (i - peakIdx) * (double) window / sr;
    return (double) (env.size() - peakIdx) * (double) window / sr;
}

//==============================================================================
struct Spectrum
{
    std::vector<float> magnitude;
    double binHz = 1.0;

    float at (double hz) const noexcept
    {
        const int bin = (int) std::lround (hz / binHz);
        return bin >= 0 && bin < (int) magnitude.size() ? magnitude[(size_t) bin] : 0.0f;
    }

    double peakHz (double minHz = 20.0) const noexcept
    {
        int best = 0;
        float bestValue = -1.0f;
        for (int i = (int) (minHz / binHz); i < (int) magnitude.size(); ++i)
            if (magnitude[(size_t) i] > bestValue) { bestValue = magnitude[(size_t) i]; best = i; }
        return best * binHz;
    }

    double centroidHz() const noexcept
    {
        double num = 0.0, den = 0.0;
        for (size_t i = 1; i < magnitude.size(); ++i)
        {
            num += (double) magnitude[i] * ((double) i * binHz);
            den += (double) magnitude[i];
        }
        return den > 1.0e-9 ? num / den : 0.0;
    }
};

Spectrum analyse (const juce::AudioBuffer<float>& b, double sr, int start, int order = 14)
{
    const int size = 1 << order;
    Spectrum s;
    s.binHz = sr / size;
    if (start + size > b.getNumSamples()) return s;

    juce::dsp::FFT fft (order);
    juce::dsp::WindowingFunction<float> window (size, juce::dsp::WindowingFunction<float>::hann);
    std::vector<float> data ((size_t) size * 2, 0.0f);
    for (int i = 0; i < size; ++i)
        data[(size_t) i] = 0.5f * (b.getSample (0, start + i) + b.getSample (1, start + i));
    window.multiplyWithWindowingTable (data.data(), (size_t) size);
    fft.performFrequencyOnlyForwardTransform (data.data(), true);

    s.magnitude.assign (data.begin(), data.begin() + size / 2);
    return s;
}

/** Largest sample-to-sample step in the buffer (the click detector). */
float maxSlope (const juce::AudioBuffer<float>& b, int from = 1) noexcept
{
    float worst = 0.0f;
    for (int c = 0; c < 2; ++c)
        for (int i = juce::jmax (1, from); i < b.getNumSamples(); ++i)
            worst = juce::jmax (worst, std::abs (b.getSample (c, i) - b.getSample (c, i - 1)));
    return worst;
}

} // namespace

//==============================================================================
class SpaceTests : public juce::UnitTest
{
public:
    SpaceTests() : juce::UnitTest ("Space FX rack", "space") {}

    void runTest() override
    {
        testTransparency();
        testDelay();
        testReverb();
        testShifter();
        testDistortion();
        testPresets();
        testSpaceSignatures();
        testClickFreeToggling();
        testExtremes();
    }

private:
    //==========================================================================
    void testTransparency()
    {
        beginTest ("MIX = 0 is bit-exact bypass at every sample rate and block size");
        {
            for (double sr : { 44100.0, 48000.0, 96000.0 })
                for (int block : { 32, 128, 1024 })
                {
                    Harness h;
                    h.prepare (sr, block);
                    h.set (Param::spaceMix, 0.0f);

                    auto buf = makeBuffer (sr, 0.4);
                    addSine (buf, sr, 220.0, 0.4f);
                    auto reference = buf;

                    h.render (buf);
                    expect (identical (buf, reference),
                            "mix 0 altered the signal at " + juce::String (sr) + "/" + juce::String (block));
                }
        }

        beginTest ("The wet path is unity when every module is off");
        {
            for (double sr : { 44100.0, 48000.0, 96000.0 })
                for (int block : { 32, 1024 })
                {
                    Harness h;
                    h.prepare (sr, block);
                    h.allModulesOff();
                    h.set (Param::spaceMix, 1.0f);      // 100 % wet: the rack chain must be transparent

                    auto buf = makeBuffer (sr, 0.4);
                    addSine (buf, sr, 330.0, 0.35f);
                    auto reference = buf;

                    h.render (buf);
                    expect (identical (buf, reference),
                            "the empty rack coloured the signal at " + juce::String (sr) + "/" + juce::String (block));
                }
        }

        beginTest ("Equal-power mix keeps the level steady with an empty rack");
        {
            Harness h;
            h.prepare (48000.0, 128);
            h.allModulesOff();
            h.set (Param::spaceMix, 0.5f);

            auto buf = makeBuffer (48000.0, 0.4);
            addSine (buf, 48000.0, 440.0, 0.3f);
            auto reference = buf;
            h.render (buf);

            const float gain = rmsOf (buf, 4800, 19200) / juce::jmax (1.0e-9f, rmsOf (reference, 4800, 19200));
            expectWithinAbsoluteError (gain, std::cos (0.25f * (float) kPi) + std::sin (0.25f * (float) kPi), 0.01f);
        }
    }

    //==========================================================================
    void testDelay()
    {
        beginTest ("Delay time accuracy — free and tempo synced");
        {
            struct Case { float time01; bool sync; double bpm; };
            const Case cases[]
            {
                { 0.5f,        false, 120.0 },
                { 0.203f,      false, 120.0 },
                { 8.0f / 12.0f, true, 120.0 },   // 1/4 = 500 ms
                { 5.0f / 12.0f, true, 120.0 },   // 1/8 = 250 ms
                { 8.0f / 12.0f, true,  90.0 }    // 1/4 = 666.7 ms
            };

            for (double sr : { 44100.0, 48000.0, 96000.0 })
                for (const auto& c : cases)
                {
                    Harness h;
                    h.prepare (sr, 128);
                    h.transport.bpm = c.bpm;
                    h.solo (Param::spaceDelayOn);
                    h.set (Param::spaceDelayTime, c.time01);
                    h.set (Param::spaceDelaySync, c.sync ? 1.0f : 0.0f);
                    h.set (Param::spaceDelayFeedback, 0.0f);
                    h.set (Param::spaceDelayMix, 1.0f);

                    const float expectedSeconds = fx::Delay::timeSeconds (c.time01, c.sync, c.bpm, 1.0f);
                    auto buf = makeBuffer (sr, expectedSeconds + 0.3);
                    const int impulse = 2048;
                    buf.setSample (0, impulse, 1.0f);
                    buf.setSample (1, impulse, 1.0f);

                    h.render (buf);

                    const int expected = impulse + (int) std::lround (expectedSeconds * sr);
                    const int found = peakIndex (buf, impulse + 64);
                    expect (std::abs (found - expected) <= 2,
                            "echo at " + juce::String (found) + ", expected " + juce::String (expected)
                                + " (sync " + juce::String ((int) c.sync) + ", " + juce::String (sr) + " Hz)");
                }
        }

        beginTest ("SIZE scales the delay time musically");
        {
            const float base = fx::Delay::timeSeconds (0.5f, false, 120.0, 1.0f);
            const float small = fx::Delay::timeSeconds (0.5f, false, 120.0, FXRack::sizeScale (0.0f));
            const float big = fx::Delay::timeSeconds (0.5f, false, 120.0, FXRack::sizeScale (1.0f));
            expect (small < base * 0.75f, "SIZE 0 did not shorten the delay");
            expect (big > base * 1.3f, "SIZE 1 did not lengthen the delay");
            expectWithinAbsoluteError (FXRack::sizeScale (0.5f), 1.0f, 1.0e-5f);
        }

        beginTest ("Delay feedback is clamped and reported");
        {
            Harness h;
            h.prepare (48000.0, 128);
            h.solo (Param::spaceDelayOn);
            h.set (Param::spaceDelayTime, 0.3f);
            h.set (Param::spaceDelaySync, 0.0f);
            h.set (Param::spaceDelayFeedback, 1.0f);
            h.set (Param::spaceDelayMix, 1.0f);
            h.set (Param::spaceFeedback, 1.0f);

            auto buf = makeBuffer (48000.0, 6.0);
            addNoiseBurst (buf, 0, 4800, 0.5f);
            h.render (buf);

            expect (allFinite (buf), "delay produced non-finite output at maximum feedback");
            expect (peakOf (buf) <= 1.05f, "delay ran away: peak " + juce::String (peakOf (buf)));
            expect (h.diag->safety.count (SafetyEvent::FeedbackClamp) > 0, "no FeedbackClamp was reported");
        }
    }

    //==========================================================================
    void testReverb()
    {
        beginTest ("Reverb decay time grows with the DECAY control");
        {
            double previous = 0.0;
            for (float decay : { 0.15f, 0.4f, 0.7f, 0.95f })
            {
                Harness h;
                h.prepare (48000.0, 128);
                h.solo (Param::spaceReverbOn);
                h.set (Param::spaceReverbDecay, decay);
                h.set (Param::spaceReverbMix, 1.0f);
                h.set (Param::spaceReverbDamp, 0.3f);
                h.set (Param::spaceFeedback, 0.5f);

                auto buf = makeBuffer (48000.0, 14.0);
                addNoiseBurst (buf, 0, 2400, 0.5f);
                h.render (buf);

                const double t20 = decayT20 (buf, 48000.0, 2400);
                logMessage ("  decay " + juce::String (decay) + " → t20 " + juce::String (t20, 2)
                            + " s (RT60 target " + juce::String (fx::Reverb::rt60Seconds (decay, 0.5f), 2) + " s)");
                expect (allFinite (buf), "non-finite reverb output");
                expect (t20 > previous * 1.15, "decay " + juce::String (decay) + " did not lengthen the tail");
                previous = t20;
            }
        }

        beginTest ("FEEDBACK lengthens the tail");
        {
            double previous = 0.0;
            for (float fb : { 0.0f, 0.5f, 1.0f })
            {
                Harness h;
                h.prepare (48000.0, 128);
                h.solo (Param::spaceReverbOn);
                h.set (Param::spaceReverbDecay, 0.5f);
                h.set (Param::spaceReverbMix, 1.0f);
                h.set (Param::spaceFeedback, fb);

                auto buf = makeBuffer (48000.0, 20.0);
                addNoiseBurst (buf, 0, 2400, 0.5f);
                h.render (buf);

                const double t20 = decayT20 (buf, 48000.0, 2400);
                expect (t20 > previous, "feedback " + juce::String (fb) + " did not lengthen the tail");
                previous = t20;
            }
        }

        beginTest ("Maximum decay and feedback stay bounded for 20 seconds");
        {
            for (double sr : { 44100.0, 48000.0, 96000.0 })
            {
                Harness h;
                h.prepare (sr, 512);
                h.solo (Param::spaceReverbOn);
                h.set (Param::spaceReverbDecay, 1.0f);
                h.set (Param::spaceReverbSize, 1.0f);
                h.set (Param::spaceReverbDamp, 0.0f);
                h.set (Param::spaceReverbMod, 1.0f);
                h.set (Param::spaceReverbMix, 1.0f);
                h.set (Param::spaceFeedback, 1.0f);
                h.set (Param::spaceSize, 1.0f);

                auto buf = makeBuffer (sr, 20.0);
                addNoiseBurst (buf, 0, (int) (sr * 0.5), 0.7f);
                h.render (buf);

                expect (allFinite (buf), "non-finite output at maximum reverb settings, " + juce::String (sr) + " Hz");
                const float late = rmsOf (buf, (int) (sr * 18.0), (int) (sr * 20.0));
                const float early = rmsOf (buf, (int) (sr * 0.6), (int) (sr * 2.0));
                expect (peakOf (buf) < 2.0f, "reverb grew: peak " + juce::String (peakOf (buf)));
                expect (late <= early, "energy grew over 20 s: " + juce::String (early) + " → " + juce::String (late));
                expectEquals ((int) h.diag->safety.count (SafetyEvent::NaN), 0);
            }
        }

        beginTest ("Reverb pre-delay holds the tail back");
        {
            Harness h;
            h.prepare (48000.0, 128);
            h.solo (Param::spaceReverbOn);
            h.set (Param::spaceReverbMix, 1.0f);
            h.set (Param::spaceReverbPredelay, 120.0f);
            h.set (Param::spaceReverbSize, 0.5f);

            auto buf = makeBuffer (48000.0, 2.0);
            buf.setSample (0, 4800, 1.0f);
            buf.setSample (1, 4800, 1.0f);
            h.render (buf);

            const float beforePredelay = rmsOf (buf, 4800, 4800 + (int) (0.10 * 48000));
            const float afterPredelay = rmsOf (buf, 4800 + (int) (0.13 * 48000), 4800 + (int) (0.30 * 48000));
            expect (afterPredelay > beforePredelay * 4.0f,
                    "pre-delay did not delay the tail: " + juce::String (beforePredelay) + " → " + juce::String (afterPredelay));
        }
    }

    //==========================================================================
    void testShifter()
    {
        beginTest ("Frequency shifter moves a sine by the expected number of Hz");
        {
            expectWithinAbsoluteError (fx::FrequencyShifter::shiftHz (0.0f), 0.0f, 1.0e-6f);
            expectWithinAbsoluteError (fx::FrequencyShifter::shiftHz (1.0f), 500.0f, 1.0e-3f);
            expectWithinAbsoluteError (fx::FrequencyShifter::shiftHz (-1.0f), -500.0f, 1.0e-3f);

            for (double sr : { 44100.0, 48000.0, 96000.0 })
                for (float amount : { 1.0f, 0.5f, -0.7f })
                {
                    Harness h;
                    h.prepare (sr, 256);
                    h.solo (Param::spaceShiftOn);
                    h.set (Param::spaceShiftAmount, amount);
                    h.set (Param::spaceShiftMix, 1.0f);

                    auto buf = makeBuffer (sr, 1.2);
                    addSine (buf, sr, 1000.0, 0.5f);
                    h.render (buf);

                    const auto spectrum = analyse (buf, sr, (int) (sr * 0.4));
                    const double expected = 1000.0 + (double) fx::FrequencyShifter::shiftHz (amount);
                    const double found = spectrum.peakHz (100.0);
                    expect (std::abs (found - expected) < 12.0,
                            "shift " + juce::String (amount) + " at " + juce::String (sr) + " Hz gave "
                                + juce::String (found) + " Hz, expected " + juce::String (expected));
                }
        }
    }

    //==========================================================================
    void testDistortion()
    {
        beginTest ("Distortion adds harmonics and stays bounded at full drive");
        {
            for (int mode = 0; mode < 5; ++mode)
            {
                Harness h;
                h.prepare (48000.0, 128);
                h.solo (Param::spaceDistOn);
                h.set (Param::spaceDistMode, (float) mode);
                h.set (Param::spaceDistDrive, 1.0f);
                h.set (Param::spaceDistMix, 1.0f);

                auto buf = makeBuffer (48000.0, 1.0);
                addSine (buf, 48000.0, 220.0, 0.4f);
                h.render (buf);

                expect (allFinite (buf), "non-finite distortion output, mode " + juce::String (mode));
                expect (peakOf (buf) < 1.5f, "distortion mode " + juce::String (mode)
                            + " exploded: peak " + juce::String (peakOf (buf)));

                const auto spectrum = analyse (buf, 48000.0, 12000);
                const float fundamental = spectrum.at (220.0);
                float harmonics = 0.0f;
                for (int k = 2; k <= 8; ++k) harmonics += spectrum.at (220.0 * k);
                const float thd = harmonics / juce::jmax (1.0e-9f, fundamental);
                logMessage ("  mode " + juce::String (mode) + " THD ratio " + juce::String (thd, 3)
                            + ", peak " + juce::String (peakOf (buf), 3));
                expect (thd > 0.02f, "mode " + juce::String (mode) + " added no harmonics (" + juce::String (thd) + ")");
            }
        }

        beginTest ("Distortion at drive 0 is close to clean");
        {
            Harness h;
            h.prepare (48000.0, 128);
            h.solo (Param::spaceDistOn);
            h.set (Param::spaceDistMode, 0.0f);
            h.set (Param::spaceDistDrive, 0.0f);
            h.set (Param::spaceDistMix, 1.0f);

            auto buf = makeBuffer (48000.0, 0.5);
            addSine (buf, 48000.0, 220.0, 0.3f);
            auto reference = buf;
            h.render (buf);

            const auto spectrum = analyse (buf, 48000.0, 4096);
            float harmonics = 0.0f;
            for (int k = 2; k <= 8; ++k) harmonics += spectrum.at (220.0 * k);
            expect (harmonics / juce::jmax (1.0e-9f, spectrum.at (220.0)) < 0.05f, "clean setting is not clean");
            expectWithinAbsoluteError (rmsOf (buf, 8000, 20000) / juce::jmax (1.0e-9f, rmsOf (reference, 8000, 20000)),
                                       1.0f, 0.15f);
        }
    }

    //==========================================================================
    void testPresets()
    {
        beginTest ("SpacePresets::apply stays inside the parameter ranges and leaves the macros alone");
        {
            expectEquals (SpacePresets::count(), (int) SpacePresets::NumTypes);
            expectEquals (SpacePresets::count(), ParameterRegistry::get (Param::spaceType).numChoices());

            for (int type = 0; type < SpacePresets::count(); ++type)
            {
                auto values = ParameterRegistry::defaults();
                values[(size_t) paramIndex (Param::spaceMix)] = 0.42f;
                values[(size_t) paramIndex (Param::spaceSize)] = 0.7f;
                values[(size_t) paramIndex (Param::spaceTone)] = 0.3f;
                values[(size_t) paramIndex (Param::spaceFeedback)] = 0.8f;
                values[(size_t) paramIndex (Param::shapeDensity)] = 0.11f;

                SpacePresets::apply (type, values);

                const juce::String tag (SpacePresets::name (type));
                expect (juce::String (SpacePresets::name (type)).isNotEmpty(), "empty name");
                expect (juce::String (SpacePresets::description (type)).length() > 12, "missing description for " + tag);

                for (int i = 0; i < kNumParams; ++i)
                {
                    const auto& desc = ParameterRegistry::get (paramFromIndex (i));
                    const float v = values[(size_t) i];
                    expect (std::isfinite (v), juce::String (desc.id) + " is not finite in " + tag);
                    expectWithinAbsoluteError (desc.clampValue (v), v, 1.0e-6f);
                }

                // Performance macros and everything outside SPACE are untouched.
                expectWithinAbsoluteError (values[(size_t) paramIndex (Param::spaceMix)], 0.42f, 1.0e-6f);
                expectWithinAbsoluteError (values[(size_t) paramIndex (Param::spaceSize)], 0.7f, 1.0e-6f);
                expectWithinAbsoluteError (values[(size_t) paramIndex (Param::spaceTone)], 0.3f, 1.0e-6f);
                expectWithinAbsoluteError (values[(size_t) paramIndex (Param::spaceFeedback)], 0.8f, 1.0e-6f);
                expectWithinAbsoluteError (values[(size_t) paramIndex (Param::shapeDensity)], 0.11f, 1.0e-6f);

                const auto r = SpacePresets::routing (type);
                std::array<bool, SpacePresets::kNumSlots> seen {};
                for (auto slot : r.order)
                {
                    expect (slot < SpacePresets::kNumSlots, "slot out of range in " + tag);
                    expect (! seen[slot], "duplicate slot in " + tag);
                    seen[slot] = true;
                }
            }
        }

        beginTest ("Out of range Space types fall back to a valid configuration");
        {
            auto values = ParameterRegistry::defaults();
            SpacePresets::apply (-3, values);
            SpacePresets::apply (999, values);
            expect (juce::String (SpacePresets::name (-3)).isNotEmpty());
            expect (juce::String (SpacePresets::name (999)).isNotEmpty());
        }
    }

    //==========================================================================
    /** Renders a struck note through one Space and reports its fingerprint. */
    struct Signature
    {
        double t20 = 0.0, centroid = 0.0;
        float wetRatio = 0.0f, peak = 0.0f;
    };

    Signature renderSpace (int type, double sr, int block)
    {
        Harness h;
        h.prepare (sr, block);
        SpacePresets::apply (type, h.params);
        h.set (Param::spaceMix, 0.5f);

        auto buf = makeBuffer (sr, 8.0);
        // A struck, decaying tone: broadband attack plus a sustained partial.
        addNoiseBurst (buf, 0, (int) (sr * 0.01), 0.6f);
        for (int i = 0; i < (int) (sr * 1.0); ++i)
        {
            const float env = std::exp (-3.0f * (float) i / (float) sr);
            const float v = 0.45f * env * (float) std::sin (kTwoPi * 220.0 * i / sr);
            buf.setSample (0, i, buf.getSample (0, i) + v);
            buf.setSample (1, i, buf.getSample (1, i) + v);
        }
        auto dry = buf;
        h.render (buf);

        Signature s;
        s.t20 = decayT20 (buf, sr, (int) (sr * 1.0));
        s.centroid = analyse (buf, sr, (int) (sr * 1.2)).centroidHz();
        s.peak = peakOf (buf);
        s.wetRatio = rmsOf (buf, (int) (sr * 1.5), (int) (sr * 3.0))
                   / juce::jmax (1.0e-9f, rmsOf (dry, 0, (int) (sr * 1.0)));
        expect (allFinite (buf), juce::String (SpacePresets::name (type)) + " produced non-finite output");
        expectEquals ((int) h.diag->safety.count (SafetyEvent::NaN), 0,
                      juce::String (SpacePresets::name (type)) + " raised NaN events");
        return s;
    }

    void testSpaceSignatures()
    {
        beginTest ("Every Space renders cleanly and has its own signature");
        {
            std::array<Signature, SpacePresets::NumTypes> signatures;
            for (int type = 0; type < SpacePresets::count(); ++type)
            {
                signatures[(size_t) type] = renderSpace (type, 48000.0, 128);
                const auto& s = signatures[(size_t) type];
                logMessage ("  " + juce::String (SpacePresets::name (type)).paddedRight (' ', 9)
                            + " t20 " + juce::String (s.t20, 2) + " s"
                            + "  centroid " + juce::String (s.centroid, 0) + " Hz"
                            + "  tail/dry " + juce::String (s.wetRatio, 3)
                            + "  peak " + juce::String (s.peak, 2));
                expect (s.peak > 0.02f, juce::String (SpacePresets::name (type)) + " is silent");
                expect (s.peak <= 1.0f, juce::String (SpacePresets::name (type)) + " is too loud");
            }

            // Normalised fingerprints must be measurably different from each other.
            auto distance = [] (const Signature& a, const Signature& b)
            {
                const double dt = std::log ((a.t20 + 0.05) / (b.t20 + 0.05));
                const double dc = std::log ((a.centroid + 20.0) / (b.centroid + 20.0));
                const double dw = (double) a.wetRatio - (double) b.wetRatio;
                return std::sqrt (dt * dt + dc * dc + dw * dw * 4.0);
            };

            for (int a = 0; a < SpacePresets::count(); ++a)
                for (int b = a + 1; b < SpacePresets::count(); ++b)
                    expect (distance (signatures[(size_t) a], signatures[(size_t) b]) > 0.10,
                            juce::String (SpacePresets::name (a)) + " and " + juce::String (SpacePresets::name (b))
                                + " sound too similar (" + juce::String (distance (signatures[(size_t) a], signatures[(size_t) b]), 3) + ")");
        }

        beginTest ("Every Space is stable at 44.1 / 96 kHz and tiny / large blocks");
        {
            for (int type = 0; type < SpacePresets::count(); ++type)
            {
                for (double sr : { 44100.0, 96000.0 })
                    for (int block : { 32, 1024 })
                    {
                        Harness h;
                        h.prepare (sr, block);
                        SpacePresets::apply (type, h.params);
                        h.set (Param::spaceMix, 0.6f);

                        auto buf = makeBuffer (sr, 2.0);
                        addNoiseBurst (buf, 0, (int) (sr * 0.2), 0.5f);
                        h.render (buf);

                        expect (allFinite (buf), juce::String (SpacePresets::name (type)) + " non-finite at "
                                    + juce::String (sr) + "/" + juce::String (block));
                        expect (peakOf (buf) <= 1.0f, juce::String (SpacePresets::name (type)) + " above ceiling at "
                                    + juce::String (sr) + "/" + juce::String (block));
                    }
            }
        }

        beginTest ("Switching Space type mid-note stays clean");
        {
            Harness h;
            h.prepare (48000.0, 128);
            SpacePresets::apply (SpacePresets::Nebula, h.params);
            h.set (Param::spaceMix, 0.5f);

            auto buf = makeBuffer (48000.0, 4.0);
            addSine (buf, 48000.0, 330.0, 0.3f);

            h.render (buf, [] (Harness& harness, int pos)
            {
                if (pos == 48000)      { SpacePresets::apply (SpacePresets::Machine, harness.params); harness.set (Param::spaceType, (float) SpacePresets::Machine); }
                else if (pos == 96000) { SpacePresets::apply (SpacePresets::Dust, harness.params);    harness.set (Param::spaceType, (float) SpacePresets::Dust); }
                else if (pos == 144000){ SpacePresets::apply (SpacePresets::Shimmer, harness.params); harness.set (Param::spaceType, (float) SpacePresets::Shimmer); }
            });

            expect (allFinite (buf), "non-finite output while switching Spaces");
            expect (peakOf (buf) <= 1.0f, "level jumped while switching Spaces");
            expect (maxSlope (buf) < 0.2f, "click while switching Spaces: " + juce::String (maxSlope (buf)));
        }
    }

    //==========================================================================
    void testClickFreeToggling()
    {
        beginTest ("Toggling modules mid-note does not click");
        {
            const Param toggles[]
            {
                Param::spaceReverbOn, Param::spaceDelayOn, Param::spaceDistOn, Param::spaceChorusOn,
                Param::spaceGrainOn, Param::spaceDiffuseOn, Param::spaceShiftOn, Param::spaceCompOn,
                Param::spaceLimiterOn
            };

            for (auto toggle : toggles)
            {
                auto run = [&] (bool doToggle)
                {
                    Harness h;
                    h.prepare (48000.0, 64);
                    h.allModulesOff();
                    h.set (Param::spaceMix, 0.5f);
                    h.set (Param::spaceDelayTime, 0.35f);
                    h.set (Param::spaceDelaySync, 0.0f);
                    h.set (Param::spaceDistDrive, 0.6f);
                    h.set (Param::spaceShiftAmount, 0.4f);
                    h.set (Param::spaceGrainMix, 0.5f);

                    auto buf = makeBuffer (48000.0, 3.0);
                    addSine (buf, 48000.0, 220.0, 0.35f);

                    h.render (buf, [&] (Harness& harness, int pos)
                    {
                        if (! doToggle) return;
                        if (pos == 24000) harness.set (toggle, 1.0f);
                        if (pos == 72000) harness.set (toggle, 0.0f);
                        if (pos == 120000) harness.set (toggle, 1.0f);
                    });
                    return buf;
                };

                auto reference = run (false);
                auto toggled = run (true);

                const float referenceSlope = maxSlope (reference, 480);
                const float toggledSlope = maxSlope (toggled, 480);
                const juce::String tag (ParameterRegistry::get (toggle).id);
                expect (allFinite (toggled), "non-finite output while toggling " + tag);
                expect (toggledSlope < juce::jmax (0.05f, referenceSlope * 1.6f),
                        "toggling " + tag + " clicked: slope " + juce::String (toggledSlope)
                            + " vs " + juce::String (referenceSlope));
            }
        }

        beginTest ("Jumping MIX from fully dry to fully wet does not click");
        {
            Harness h;
            h.prepare (48000.0, 64);
            SpacePresets::apply (SpacePresets::Nebula, h.params);
            h.set (Param::spaceMix, 0.0f);

            auto buf = makeBuffer (48000.0, 3.0);
            addSine (buf, 48000.0, 220.0, 0.35f);

            // A long fully dry stretch (the rack is skipped and released), then a hard jump.
            h.render (buf, [] (Harness& harness, int pos)
            {
                if (pos == 96000) harness.set (Param::spaceMix, 1.0f);
                if (pos == 120000) harness.set (Param::spaceMix, 0.0f);
            });

            expect (allFinite (buf), "non-finite output while jumping MIX");
            expect (maxSlope (buf, 480) < 0.12f, "MIX jump clicked: " + juce::String (maxSlope (buf, 480)));
        }

        beginTest ("Sweeping MIX from dry to wet and back is smooth");
        {
            Harness h;
            h.prepare (48000.0, 64);
            SpacePresets::apply (SpacePresets::Nebula, h.params);
            h.set (Param::spaceMix, 0.0f);

            auto buf = makeBuffer (48000.0, 4.0);
            addSine (buf, 48000.0, 220.0, 0.35f);

            h.render (buf, [] (Harness& harness, int pos)
            {
                const float t = (float) pos / 192000.0f;
                harness.set (Param::spaceMix, t < 0.5f ? t * 2.0f : (1.0f - t) * 2.0f);
            });

            expect (allFinite (buf), "non-finite output while sweeping MIX");
            expect (maxSlope (buf, 480) < 0.12f, "MIX sweep clicked: " + juce::String (maxSlope (buf, 480)));
        }
    }

    //==========================================================================
    void testExtremes()
    {
        beginTest ("Extreme macro settings stay finite and bounded");
        {
            for (double sr : { 44100.0, 48000.0, 96000.0 })
                for (int block : { 32, 1024 })
                    for (float size : { 0.0f, 1.0f })
                        for (float tone : { 0.0f, 1.0f })
                        {
                            Harness h;
                            h.prepare (sr, block);
                            for (auto p : { Param::spaceDistOn, Param::spaceChorusOn, Param::spaceDelayOn,
                                            Param::spaceGrainOn, Param::spaceShiftOn, Param::spaceDiffuseOn,
                                            Param::spaceReverbOn, Param::spaceCompOn, Param::spaceLimiterOn })
                                h.set (p, 1.0f);

                            h.set (Param::spaceMix, 1.0f);
                            h.set (Param::spaceSize, size);
                            h.set (Param::spaceTone, tone);
                            h.set (Param::spaceFeedback, 1.0f);
                            h.set (Param::spaceDistDrive, 1.0f);
                            h.set (Param::spaceDelayFeedback, 1.0f);
                            h.set (Param::spaceGrainMix, 1.0f);
                            h.set (Param::spaceShiftAmount, -1.0f);
                            h.set (Param::spaceDiffuseAmount, 1.0f);
                            h.set (Param::spaceReverbDecay, 1.0f);
                            h.set (Param::spaceReverbSize, 1.0f);
                            h.set (Param::spaceCompAmount, 1.0f);

                            auto buf = makeBuffer (sr, 3.0);
                            addNoiseBurst (buf, 0, (int) (sr * 1.0), 0.8f);
                            h.render (buf);

                            const juce::String tag = juce::String (sr) + "/" + juce::String (block)
                                                   + " size " + juce::String (size) + " tone " + juce::String (tone);
                            expect (allFinite (buf), "non-finite output at " + tag);
                            expect (peakOf (buf) <= 1.0f, "above ceiling at " + tag + ": " + juce::String (peakOf (buf)));
                            expectEquals ((int) h.diag->safety.count (SafetyEvent::NaN), 0, "NaN events at " + tag);
                        }
        }

        beginTest ("A silent input produces a silent output");
        {
            Harness h;
            h.prepare (48000.0, 128);
            SpacePresets::apply (SpacePresets::Void, h.params);
            h.set (Param::spaceMix, 1.0f);

            auto buf = makeBuffer (48000.0, 1.0);
            h.render (buf);
            expectWithinAbsoluteError (peakOf (buf), 0.0f, 1.0e-9f);
        }

        beginTest ("SPACE reports no latency and stays idle when fully dry");
        {
            Harness h;
            h.prepare (48000.0, 128);
            expectEquals (h.engine.latencySamples(), 0);

            SpacePresets::apply (SpacePresets::Nebula, h.params);
            h.set (Param::spaceMix, 0.0f);
            auto buf = makeBuffer (48000.0, 1.0);
            addSine (buf, 48000.0, 220.0, 0.4f);
            auto reference = buf;
            h.render (buf);
            expect (identical (buf, reference), "a curated Space leaked signal at MIX 0");
            expectWithinAbsoluteError (h.engine.activity(), 0.0f, 1.0e-6f);
        }

        beginTest ("Activity tracks the wet content");
        {
            {
                Harness h;
                h.prepare (48000.0, 128);
                SpacePresets::apply (SpacePresets::Nebula, h.params);
                h.set (Param::spaceMix, 1.0f);

                auto buf = makeBuffer (48000.0, 1.0);
                addNoiseBurst (buf, 0, buf.getNumSamples(), 0.4f);
                h.render (buf);
                expect (h.engine.activity() > 0.9f, "fully wet Space did not report activity: "
                            + juce::String (h.engine.activity()));
                expect (h.engine.activity() <= 1.0f, "activity out of range");
            }
            {
                // Empty rack at MIX 0.5: half the output energy is "wet" by definition.
                Harness h;
                h.prepare (48000.0, 128);
                h.allModulesOff();
                h.set (Param::spaceMix, 0.5f);

                auto buf = makeBuffer (48000.0, 1.0);
                addNoiseBurst (buf, 0, buf.getNumSamples(), 0.4f);
                h.render (buf);
                expectWithinAbsoluteError (h.engine.activity(), 0.5f, 0.02f);
            }
        }
    }
};

static SpaceTests spaceTests;

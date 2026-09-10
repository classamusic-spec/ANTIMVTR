#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/SynthEngine.h"
#include "dsp/source/SampleSource.h"
#include "dsp/source/GestureSource.h"
#include "dsp/source/SampleAnalyzer.h"
#include "dsp/source/SourceEngine.h"
#include "dsp/source/WaveSource.h"

#include <thread>
#include <vector>

using namespace am;

namespace
{
    //==========================================================================
    /** Parameter block, note and the sample the engine would have published. */
    struct Harness
    {
        ParamValues params = ParameterRegistry::defaults();
        NoteState   note;
        double      sr = 48000.0;
        int         block = 128;
        const SampleData* sample = nullptr;

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
            c.sample = sample;
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

        const double offSample = hold * h.sr;
        bool released = false;
        for (int pos = 0; pos < total; pos += h.block)
        {
            const int n = juce::jmin (h.block, total - pos);
            if (! released && (double) pos >= offSample) { src.noteOff(); h.note.gate = false; released = true; }
            const auto ctx = h.context (n);
            src.render (buf.getWritePointer (0) + pos, buf.getWritePointer (1) + pos, n, ctx, h.note);
        }
        return buf;
    }

    //==========================================================================
    bool allFinite (const juce::AudioBuffer<float>& b)
    {
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            for (int i = 0; i < b.getNumSamples(); ++i)
                if (! std::isfinite (b.getSample (ch, i))) return false;
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

    double rmsOf (const juce::AudioBuffer<float>& b, int start = 0, int end = -1, int channel = 0)
    {
        if (end < 0) end = b.getNumSamples();
        start = juce::jlimit (0, b.getNumSamples(), start);
        end = juce::jlimit (start, b.getNumSamples(), end);
        double s = 0.0;
        for (int i = start; i < end; ++i)
        {
            const double v = (double) b.getSample (channel, i);
            s += v * v;
        }
        return std::sqrt (s / juce::jmax (1, end - start));
    }

    /** Spectral centroid of the middle of a buffer (one 8192 point Hann frame). */
    double centroidOf (const juce::AudioBuffer<float>& b, double sr, int channel = 0)
    {
        constexpr int order = 13, size = 1 << order;
        if (b.getNumSamples() < size) return 0.0;
        const int start = juce::jlimit (0, b.getNumSamples() - size, (b.getNumSamples() - size) / 2);
        std::vector<float> data ((size_t) size * 2, 0.0f);
        for (int i = 0; i < size; ++i) data[(size_t) i] = b.getSample (channel, start + i);
        juce::dsp::WindowingFunction<float> window (size, juce::dsp::WindowingFunction<float>::hann);
        window.multiplyWithWindowingTable (data.data(), size);
        juce::dsp::FFT fft (order);
        fft.performFrequencyOnlyForwardTransform (data.data(), true);
        double num = 0.0, den = 0.0;
        for (int k = 1; k < size / 2; ++k)
        {
            const double mag = data[(size_t) k];
            num += mag * ((double) k * sr / size);
            den += mag;
        }
        return den > 1.0e-12 ? num / den : 0.0;
    }

    double meanOf (const juce::AudioBuffer<float>& b, int channel = 0)
    {
        double s = 0.0;
        for (int i = 0; i < b.getNumSamples(); ++i) s += (double) b.getSample (channel, i);
        return s / juce::jmax (1, b.getNumSamples());
    }

    /** Largest sample-to-sample step in a range (click detector). */
    float maxStep (const juce::AudioBuffer<float>& b, int start = 1, int end = -1, int channel = 0)
    {
        if (end < 0) end = b.getNumSamples();
        start = juce::jlimit (1, b.getNumSamples(), start);
        end = juce::jlimit (start, b.getNumSamples(), end);
        float m = 0.0f;
        for (int i = start; i < end; ++i)
            m = juce::jmax (m, std::abs (b.getSample (channel, i) - b.getSample (channel, i - 1)));
        return m;
    }

    /** Frequency from zero crossings over a window (accurate for clean tones). */
    double frequencyOf (const juce::AudioBuffer<float>& b, double sr, int start, int end, int channel = 0)
    {
        start = juce::jlimit (0, b.getNumSamples() - 1, start);
        end = juce::jlimit (start + 2, b.getNumSamples(), end);

        // First and last upward zero crossing, with linear interpolation.
        double first = -1.0, last = -1.0;
        int crossings = 0;
        for (int i = start + 1; i < end; ++i)
        {
            const float a = b.getSample (channel, i - 1), c = b.getSample (channel, i);
            if (a < 0.0f && c >= 0.0f)
            {
                const double frac = (double) (-a) / juce::jmax (1.0e-12, (double) (c - a));
                const double t = (double) (i - 1) + frac;
                if (first < 0.0) first = t;
                else { last = t; ++crossings; }
            }
        }
        if (crossings < 1 || last <= first) return 0.0;
        return (double) crossings * sr / (last - first);
    }

    //==========================================================================
    /** A sine sample with an optional amplitude ramp, in its own sample rate. */
    std::shared_ptr<SampleData> makeSine (double freq, double sampleRate, double seconds,
                                          int channels = 1, float ampStart = 1.0f, float ampEnd = 1.0f)
    {
        auto s = std::make_shared<SampleData>();
        const int frames = juce::jmax (4, (int) (seconds * sampleRate));
        s->allocate (channels, frames);
        s->sampleRate = sampleRate;
        s->name = "Test Sine";
        for (int c = 0; c < channels; ++c)
        {
            float* d = s->write (c);
            for (int i = 0; i < frames; ++i)
            {
                const double t = (double) i / sampleRate;
                const float amp = ampStart + (ampEnd - ampStart) * (float) i / (float) juce::jmax (1, frames - 1);
                d[i] = amp * (float) std::sin (kTwoPi * freq * t + (c == 1 ? 0.5 : 0.0));
            }
        }
        s->updatePeak();
        return s;
    }

    /** Two halves: silence then a tone (start/end tests). */
    std::shared_ptr<SampleData> makeHalfTone (double sampleRate, double seconds)
    {
        auto s = std::make_shared<SampleData>();
        const int frames = juce::jmax (8, (int) (seconds * sampleRate));
        s->allocate (1, frames);
        s->sampleRate = sampleRate;
        s->name = "Half Tone";
        float* d = s->write (0);
        for (int i = frames / 2; i < frames; ++i)
            d[i] = 0.8f * (float) std::sin (kTwoPi * 400.0 * (double) i / sampleRate);
        s->updatePeak();
        return s;
    }

    /** A modal test tone with known ratios, weights and ring times. */
    std::shared_ptr<SampleData> makeModalTone (double f0, double sampleRate, double seconds,
                                               const std::vector<std::array<float, 3>>& modes)
    {
        auto s = std::make_shared<SampleData>();
        const int frames = juce::jmax (16, (int) (seconds * sampleRate));
        s->allocate (1, frames);
        s->sampleRate = sampleRate;
        s->name = "Modal Tone";
        float* d = s->write (0);
        for (const auto& m : modes)
        {
            const double f = f0 * (double) m[0];
            if (f >= sampleRate * 0.45) continue;
            const double decay = std::exp (-6.907755 / ((double) m[2] * sampleRate));
            double env = (double) m[1];
            for (int i = 0; i < frames; ++i)
            {
                d[i] += (float) (env * std::sin (kTwoPi * f * (double) i / sampleRate));
                env *= decay;
            }
        }
        s->updatePeak();
        return s;
    }

    ParamValues engineParams()
    {
        auto p = ParameterRegistry::defaults();
        p[(size_t) paramIndex (Param::ampAttack)] = 0.002f;
        p[(size_t) paramIndex (Param::ampRelease)] = 0.05f;
        p[(size_t) paramIndex (Param::spaceMix)] = 0.0f;
        return p;
    }
}

//==============================================================================
class SampleGestureTests : public juce::UnitTest
{
public:
    SampleGestureTests() : juce::UnitTest ("Sample & Gesture sources", "sources") {}

    void runTest() override
    {
        testBuiltIns();
        testPitchAccuracy();
        testLoopContinuity();
        testReverse();
        testGranular();
        testStartEnd();
        testExtremeReads();
        testSampleHandoff();
        testHandoffThreadSafety();
        testAnalyzer();
        testGestureSafety();
        testGesturePressure();
        testGestureBandwidthLevel();
        testGestureHeadroom();
        testSampleLevel();
        testGestureDc();
        testEnergyReporting();
        testFullEngine();
    }

private:
    //==========================================================================
    void testBuiltIns()
    {
        beginTest ("Built-in samples are generated, finite, normalised and DC free");
        {
            expect (BuiltInSamples::count() == (int) BuiltInSamples::Kind::Count);
            for (int i = 0; i < BuiltInSamples::count(); ++i)
            {
                auto s = BuiltInSamples::create (i);
                expect (s != nullptr);
                expect (! s->isEmpty(), juce::String (BuiltInSamples::name (i)) + " is empty");
                expectWithinAbsoluteError (s->sampleRate, 48000.0, 1.0);
                expect (s->lengthSeconds() >= 0.45 && s->lengthSeconds() <= 2.05,
                        juce::String (BuiltInSamples::name (i)) + " length " + juce::String (s->lengthSeconds()));
                expectWithinAbsoluteError (s->peak, 0.94f, 0.02f);
                expect (s->builtInIndex == i);
                expect (BuiltInSamples::indexOf (s->name) == i);

                double sum = 0.0, energy = 0.0;
                for (int c = 0; c < s->numChannels; ++c)
                {
                    const float* d = s->channel (c);
                    for (int k = 0; k < s->numFrames; ++k)
                    {
                        expect (std::isfinite (d[k]));
                        sum += (double) d[k];
                        energy += (double) d[k] * (double) d[k];
                    }
                }
                const double n = (double) (s->numFrames * s->numChannels);
                const double dc = std::abs (sum / n);
                const double rms = std::sqrt (energy / n);
                expect (dc < rms * 0.02 + 1.0e-4, juce::String (BuiltInSamples::name (i)) + " DC " + juce::String (dc));

                // The edges must be silent: a built-in can never click when it starts or ends.
                expect (std::abs (s->channel (0)[0]) < 0.02f);
                expect (std::abs (s->channel (0)[s->numFrames - 1]) < 0.02f);
            }
        }
    }

    //==========================================================================
    void testPitchAccuracy()
    {
        beginTest ("Sample playback pitch follows root, keytrack and pitch at every sample rate and block size");
        {
            const double sampleHz = 220.0;
            auto sine = makeSine (sampleHz, 48000.0, 2.0);

            struct Case { int note; int root; bool keytrack; float pitch; };
            const Case cases[] = {
                { 60, 60, true,   0.0f },
                { 72, 60, true,   0.0f },
                { 48, 60, true,   0.0f },
                { 60, 60, true,   7.0f },
                { 60, 60, true, -12.0f },
                { 84, 72, true,   0.0f },
                { 67, 60, false,  0.0f },     // keytrack off: the note must not change the pitch
                { 67, 60, false,  5.0f }
            };

            for (double sr : { 44100.0, 48000.0, 88200.0, 96000.0 })
            {
                for (int block : { 32, 128, 512, 1024 })
                {
                    for (const auto& c : cases)
                    {
                        Harness h;
                        h.sr = sr;
                        h.block = block;
                        h.sample = sine.get();
                        h.set (Param::sampleMode, 1.0f);        // LOOP: keeps sounding for the whole window
                        h.set (Param::sampleRoot, (float) c.root);
                        h.set (Param::sampleKeytrack, c.keytrack ? 1.0f : 0.0f);
                        h.set (Param::samplePitch, c.pitch);
                        h.setNote (c.note);

                        SampleSource src;
                        auto buf = renderSource (src, h, 0.5);

                        const double expected = sampleHz
                                              * (c.keytrack ? std::pow (2.0, (c.note - c.root) / 12.0) : 1.0)
                                              * std::pow (2.0, (double) c.pitch / 12.0);
                        const double measured = frequencyOf (buf, sr, (int) (0.05 * sr), (int) (0.45 * sr));
                        const double error = std::abs (measured - expected) / expected;
                        expect (error < 0.005,
                                "sr " + juce::String (sr) + " block " + juce::String (block)
                                    + " note " + juce::String (c.note) + " expected " + juce::String (expected)
                                    + " Hz, measured " + juce::String (measured) + " Hz");
                        expect (allFinite (buf));
                    }
                }
            }
        }
    }

    //==========================================================================
    void testLoopContinuity()
    {
        beginTest ("LOOP is continuous at the loop point");
        {
            // A sine whose loop region holds a non-integer number of cycles: without
            // a crossfade the seam would be a hard step.
            auto sine = makeSine (317.0, 48000.0, 1.0);

            Harness h;
            h.sample = sine.get();
            h.set (Param::sampleMode, 1.0f);
            h.set (Param::sampleStart, 0.10f);
            h.set (Param::sampleEnd, 0.35f);
            h.set (Param::sampleKeytrack, 0.0f);

            SampleSource src;
            auto buf = renderSource (src, h, 2.0);
            expect (allFinite (buf));

            // The loop wraps several times inside two seconds; the biggest step
            // anywhere must stay close to the step of the underlying tone.
            const double cycleStep = 2.0 * kPi * 317.0 / 48000.0;   // peak slope of the source per sample
            const float observed = maxStep (buf, (int) (0.02 * h.sr));
            expect (observed < (float) cycleStep * 1.6f,
                    "max step " + juce::String (observed) + " vs source step " + juce::String (cycleStep));

            // ...and the loop keeps sounding at a steady level.
            const double early = rmsOf (buf, (int) (0.2 * h.sr), (int) (0.4 * h.sr));
            const double late  = rmsOf (buf, (int) (1.6 * h.sr), (int) (1.8 * h.sr));
            expect (late > early * 0.7 && late < early * 1.4,
                    "loop level drifted: " + juce::String (early) + " -> " + juce::String (late));
        }
    }

    //==========================================================================
    void testReverse()
    {
        beginTest ("REVERSE plays the sample backwards");
        {
            // A tone that fades in: played backwards it must fade out.
            auto ramp = makeSine (300.0, 48000.0, 1.0, 1, 0.05f, 1.0f);

            Harness h;
            h.sample = ramp.get();
            h.set (Param::sampleKeytrack, 0.0f);
            h.set (Param::sampleMode, 0.0f);   // ONE SHOT
            SampleSource forward;
            auto fwd = renderSource (forward, h, 1.2);

            h.set (Param::sampleMode, 2.0f);   // REVERSE
            SampleSource backward;
            auto rev = renderSource (backward, h, 1.2);

            expect (allFinite (fwd) && allFinite (rev));

            const double fwdEarly = rmsOf (fwd, (int) (0.05 * h.sr), (int) (0.20 * h.sr));
            const double fwdLate  = rmsOf (fwd, (int) (0.75 * h.sr), (int) (0.90 * h.sr));
            const double revEarly = rmsOf (rev, (int) (0.05 * h.sr), (int) (0.20 * h.sr));
            const double revLate  = rmsOf (rev, (int) (0.75 * h.sr), (int) (0.90 * h.sr));

            expect (fwdLate > fwdEarly * 2.0, "forward should grow");
            expect (revEarly > revLate * 2.0, "reverse should shrink");
            expectWithinAbsoluteError (revEarly, fwdLate, fwdLate * 0.25);
        }
    }

    //==========================================================================
    void testGranular()
    {
        beginTest ("GRANULAR is deterministic, bounded and smooth");
        {
            auto sine = makeSine (200.0, 48000.0, 1.0);
            Harness h;
            h.sample = sine.get();
            h.set (Param::sampleMode, 3.0f);
            h.set (Param::sampleGrain, 0.3f);
            h.set (Param::sampleSpread, 0.6f);

            SampleSource a, b;
            auto first = renderSource (a, h, 0.75);
            auto second = renderSource (b, h, 0.75);

            expect (allFinite (first));
            for (int i = 0; i < first.getNumSamples(); ++i)
                if (first.getSample (0, i) != second.getSample (0, i))
                {
                    expect (false, "granular output differs for the same note id at sample " + juce::String (i));
                    break;
                }

            // A different note id decorrelates the cloud.
            h.note.noteId = 77;
            SampleSource c;
            auto third = renderSource (c, h, 0.75);
            bool identical = true;
            for (int i = 0; i < first.getNumSamples() && identical; ++i)
                identical = first.getSample (0, i) == third.getSample (0, i);
            expect (! identical, "a different note id should draw a different cloud");

            expect (peakOf (first) <= 1.0f);
            expect (rmsOf (first, (int) (0.1 * h.sr)) > 1.0e-3, "granular should keep sounding");

            // Grain size and spread extremes stay bounded and finite.
            for (float grain : { 0.0f, 0.5f, 1.0f })
            {
                for (float spread : { 0.0f, 1.0f })
                {
                    h.set (Param::sampleGrain, grain);
                    h.set (Param::sampleSpread, spread);
                    SampleSource s;
                    auto buf = renderSource (s, h, 0.4);
                    expect (allFinite (buf), "grain " + juce::String (grain) + " spread " + juce::String (spread));
                    expect (peakOf (buf) <= 1.0f);
                }
            }
        }
    }

    //==========================================================================
    void testStartEnd()
    {
        beginTest ("Start and end bound the playback");
        {
            auto half = makeHalfTone (48000.0, 1.0);

            Harness h;
            h.sample = half.get();
            h.set (Param::sampleKeytrack, 0.0f);
            h.set (Param::sampleMode, 0.0f);

            // First half only: silence.
            h.set (Param::sampleStart, 0.0f);
            h.set (Param::sampleEnd, 0.5f);
            SampleSource a;
            auto quiet = renderSource (a, h, 0.45);
            expect (rmsOf (quiet) < 1.0e-4, "start..end selected the silent half but produced " + juce::String (rmsOf (quiet)));

            // Second half only: the tone starts immediately.
            h.set (Param::sampleStart, 0.5f);
            h.set (Param::sampleEnd, 1.0f);
            SampleSource b;
            auto loud = renderSource (b, h, 0.45);
            expect (rmsOf (loud, (int) (0.01 * h.sr), (int) (0.1 * h.sr)) > 0.05, "the selected half should sound immediately");

            // One shot ends when the selection ends.
            expect (rmsOf (loud, (int) (0.52 * h.sr)) < 1.0e-4 || loud.getNumSamples() < (int) (0.52 * h.sr));
        }
    }

    //==========================================================================
    void testExtremeReads()
    {
        beginTest ("Extreme positions, tiny samples and extreme pitch never read outside the sample");
        {
            // Direct reads at impossible positions.
            auto tiny = makeSine (1000.0, 48000.0, 0.0002);   // ~10 frames
            for (double position : { -1.0e9, -5.0, -1.5, 0.0, 0.5, 1.0e9, (double) tiny->numFrames + 100.0 })
                expect (std::isfinite (tiny->read (0, position)), "read at " + juce::String (position));
            expect (std::isfinite (tiny->read (0, std::numeric_limits<double>::quiet_NaN())));
            expect (std::isfinite (tiny->read (1, 3.25)));

            auto sine = makeSine (300.0, 48000.0, 0.5);
            const std::shared_ptr<SampleData> samples[] = { tiny, sine };

            for (const auto& s : samples)
            {
                for (int mode = 0; mode < (int) SampleSource::Mode::Count; ++mode)
                {
                    for (float pitch : { -24.0f, 0.0f, 24.0f })
                    {
                        for (auto bounds : { std::pair<float, float> { 0.0f, 1.0f },
                                             std::pair<float, float> { 0.5f, 0.5f },     // start == end
                                             std::pair<float, float> { 0.99f, 1.0f },
                                             std::pair<float, float> { 0.0f, 0.001f },
                                             std::pair<float, float> { 0.8f, 0.2f } })   // inverted
                        {
                            Harness h;
                            h.sample = s.get();
                            h.set (Param::sampleMode, (float) mode);
                            h.set (Param::samplePitch, pitch);
                            h.set (Param::sampleStart, bounds.first);
                            h.set (Param::sampleEnd, bounds.second);
                            h.setNote (mode == 1 ? 108 : 21);      // extreme keytrack ratios too
                            SampleSource src;
                            auto buf = renderSource (src, h, 0.15);
                            expect (allFinite (buf), "mode " + juce::String (mode) + " pitch " + juce::String (pitch));
                            expect (peakOf (buf) <= 1.0f);
                        }
                    }
                }
            }

            // No sample at all: silence, not a crash.
            Harness h;
            h.sample = nullptr;
            SampleSource src;
            auto buf = renderSource (src, h, 0.1);
            expect (allFinite (buf) && peakOf (buf) == 0.0f);
        }
    }

    //==========================================================================
    void testSampleHandoff()
    {
        beginTest ("Swapping the sample mid-note is click free and NaN free");
        {
            auto a = BuiltInSamples::create ((int) BuiltInSamples::Kind::Breath);
            auto b = BuiltInSamples::create ((int) BuiltInSamples::Kind::VinylDust);

            Harness h;
            h.sample = a.get();
            h.set (Param::sampleMode, 1.0f);     // LOOP, so both samples keep sounding
            h.set (Param::sampleKeytrack, 0.0f);

            SampleSource src;
            const int total = (int) (1.0 * h.sr);
            juce::AudioBuffer<float> buf (2, total);
            buf.clear();
            src.prepare (h.sr, h.block);
            src.reset();
            src.noteOn (h.note, h.params);

            const int swapAt = total / 2;
            for (int pos = 0; pos < total; pos += h.block)
            {
                const int n = juce::jmin (h.block, total - pos);
                if (pos >= swapAt && h.sample == a.get()) h.sample = b.get();
                const auto ctx = h.context (n);
                src.render (buf.getWritePointer (0) + pos, buf.getWritePointer (1) + pos, n, ctx, h.note);
            }

            expect (allFinite (buf));
            const float around = maxStep (buf, swapAt - 8, swapAt + (int) (0.01 * h.sr));
            const float elsewhere = juce::jmax (maxStep (buf, (int) (0.05 * h.sr), swapAt - 64),
                                                maxStep (buf, swapAt + (int) (0.05 * h.sr), total));
            expect (around <= elsewhere * 1.5f + 0.01f,
                    "step at the swap " + juce::String (around) + " vs " + juce::String (elsewhere));
        }
    }

    //==========================================================================
    void testHandoffThreadSafety()
    {
        beginTest ("Published samples are never freed on the audio thread");
        {
            SynthEngine engine;
            engine.prepare (48000.0, 128);
            auto params = engineParams();
            params[(size_t) paramIndex (Param::sourceSelected)] = 3.0f;    // SAMPLE
            params[(size_t) paramIndex (Param::sampleMode)] = 1.0f;        // LOOP

            std::atomic<void*> audioThread { nullptr };
            std::atomic<bool> running { true };
            std::atomic<int> blocksRendered { 0 };
            std::atomic<bool> finite { true };

            std::thread audio ([&]
            {
                audioThread.store (juce::Thread::getCurrentThreadId());
                juce::AudioBuffer<float> block (2, 128);
                TransportInfo transport;
                bool noteSent = false;
                while (running.load())
                {
                    juce::MidiBuffer midi;
                    if (! noteSent) { midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0); noteSent = true; }
                    block.clear();
                    engine.process (block, midi, params, transport);
                    for (int i = 0; i < block.getNumSamples(); ++i)
                        if (! std::isfinite (block.getSample (0, i))) finite.store (false);
                    blocksRendered.fetch_add (1);
                }
            });

            const int liveAtStart = SampleData::liveInstances.load();
            for (int i = 0; i < 8; ++i)
            {
                engine.publishSample (BuiltInSamples::create (i % BuiltInSamples::count()));
                std::this_thread::sleep_for (std::chrono::milliseconds (4));
                engine.messageThreadMaintenance();
            }
            while (blocksRendered.load() < 20) std::this_thread::sleep_for (std::chrono::milliseconds (1));
            running.store (false);
            audio.join();

            engine.messageThreadMaintenance();
            engine.messageThreadMaintenance();

            expect (finite.load(), "engine produced a non-finite sample while the sample was swapped");
            expect (blocksRendered.load() > 0);

            const uint64_t audioId = (uint64_t) (uintptr_t) audioThread.load();
            const uint64_t freedOn = SampleData::lastFreeThread.load();
            expect (freedOn != audioId, "a SampleData was freed on the audio thread");
            expect (freedOn == (uint64_t) (uintptr_t) juce::Thread::getCurrentThreadId(),
                    "SampleData should be freed on the message thread");

            // Everything the engine no longer needs must be gone once the message
            // thread has collected: at most the live one is still around.
            const int liveNow = SampleData::liveInstances.load();
            expect (liveNow <= liveAtStart + 1, "leaked " + juce::String (liveNow - liveAtStart) + " samples");
        }
    }

    //==========================================================================
    void testAnalyzer()
    {
        beginTest ("The analyzer recovers the partials of a synthesised tone");
        {
            const double f0 = 330.0;
            const std::vector<std::array<float, 3>> modes = {
                { 1.00f, 1.00f, 1.20f },
                { 2.00f, 0.60f, 0.90f },
                { 3.17f, 0.36f, 0.60f },
                { 5.40f, 0.20f, 0.40f }
            };
            auto tone = makeModalTone (f0, 48000.0, 2.0, modes);

            const auto table = SampleAnalyzer::analyse (*tone);
            expect (table.isValid());
            expectWithinAbsoluteError ((double) table.fundamentalHz, f0, f0 * 0.01);
            expect (table.count >= (int) modes.size(), "found only " + juce::String (table.count) + " partials");

            for (const auto& m : modes)
            {
                bool found = false;
                for (int i = 0; i < table.count && ! found; ++i)
                    found = std::abs (table.partials[(size_t) i].ratio - m[0]) <= 0.01f * m[0];
                expect (found, "missing partial at ratio " + juce::String (m[0]));
            }

            for (int i = 1; i < table.count; ++i)
                expect (table.partials[(size_t) i].weight <= table.partials[(size_t) (i - 1)].weight + 1.0e-6f,
                        "weights are not ordered at " + juce::String (i));

            expect (table.partials[0].ratio > 0.99f && table.partials[0].ratio < 1.01f, "index 0 must be the fundamental");
            expect (table.partials[0].t60 > 0.6f && table.partials[0].t60 < 2.4f,
                    "fundamental T60 " + juce::String (table.partials[0].t60) + " (expected ~1.2 s)");
            expect (table.harmonicity >= 0.0f && table.harmonicity <= 1.0f);

            // Round trip through JSON.
            const auto restored = PartialTable::fromVar (table.toVar());
            expect (restored.count == table.count);
            expectWithinAbsoluteError (restored.fundamentalHz, table.fundamentalHz, 0.01f);
            for (int i = 0; i < restored.count; ++i)
                expectWithinAbsoluteError (restored.partials[(size_t) i].ratio, table.partials[(size_t) i].ratio, 0.001f);

            // The Shape fit must stay inside the parameter ranges.
            const auto fit = SampleAnalyzer::fitShape (table);
            for (float v : { fit.form, fit.tension, fit.decay, fit.mass, fit.density, fit.distribution })
                expect (v >= 0.0f && v <= 1.0f && std::isfinite (v));

            // Every built-in analyses without exploding.
            for (int i = 0; i < BuiltInSamples::count(); ++i)
            {
                auto s = BuiltInSamples::create (i);
                const auto t = SampleAnalyzer::analyse (*s);
                expect (t.count >= 0 && t.count <= PartialTable::kMaxPartials);
                for (int k = 0; k < t.count; ++k)
                {
                    expect (std::isfinite (t.partials[(size_t) k].ratio) && t.partials[(size_t) k].ratio > 0.0f);
                    expect (t.partials[(size_t) k].weight >= 0.0f && t.partials[(size_t) k].weight <= 1.0f);
                    expect (t.partials[(size_t) k].t60 > 0.0f && t.partials[(size_t) k].t60 <= 30.0f);
                }
            }

            // An empty sample analyses to an invalid (but safe) table.
            SampleData empty;
            expect (! SampleAnalyzer::analyse (empty).isValid());
        }
    }

    //==========================================================================
    void testGestureSafety()
    {
        beginTest ("Every gesture mode is finite and bounded at every extreme");
        {
            const float extremes[] = { 0.0f, 0.5f, 1.0f };
            for (double sr : { 44100.0, 48000.0, 88200.0, 96000.0 })
            {
                for (int mode = 0; mode < (int) GestureSource::Mode::Count; ++mode)
                {
                    for (float pressure : extremes)
                    {
                        for (float speed : extremes)
                        {
                            for (float roughness : extremes)
                            {
                                for (float position : extremes)
                                {
                                    for (float bandwidth : extremes)
                                    {
                                        Harness h;
                                        h.sr = sr;
                                        h.block = 64;
                                        h.set (Param::gestureMode, (float) mode);
                                        h.set (Param::gesturePressure, pressure);
                                        h.set (Param::gestureSpeed, speed);
                                        h.set (Param::gestureRoughness, roughness);
                                        h.set (Param::gesturePosition, position);
                                        h.set (Param::gestureBandwidth, bandwidth);
                                        h.set (Param::gestureMotion, 1.0f);
                                        h.setNote (mode % 2 == 0 ? 24 : 100);   // extreme notes as well
                                        GestureSource src;
                                        auto buf = renderSource (src, h, 0.12);
                                        expect (allFinite (buf), "mode " + juce::String (mode) + " at " + juce::String (sr));
                                        expect (peakOf (buf) <= 1.0f, "mode " + juce::String (mode) + " peaked at "
                                                                          + juce::String (peakOf (buf)));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        beginTest ("Gestures stop shortly after the key is released");
        {
            for (int mode = 0; mode < (int) GestureSource::Mode::Count; ++mode)
            {
                Harness h;
                h.set (Param::gestureMode, (float) mode);
                GestureSource src;
                auto buf = renderSource (src, h, 0.9, 0.3);
                const double sustained = rmsOf (buf, (int) (0.15 * h.sr), (int) (0.29 * h.sr));
                const double after = rmsOf (buf, (int) (0.6 * h.sr));
                expect (sustained > 1.0e-3, "mode " + juce::String (mode) + " produced nothing while gated");
                expect (after < sustained * 0.005,
                        "mode " + juce::String (mode) + " still sounding 300 ms after release: " + juce::String (after));
                expect (! src.isActive());
            }
        }

        beginTest ("Gestures are deterministic for a given note id");
        {
            for (int mode = 0; mode < (int) GestureSource::Mode::Count; ++mode)
            {
                Harness h;
                h.set (Param::gestureMode, (float) mode);
                GestureSource a, b;
                auto first = renderSource (a, h, 0.25);
                auto second = renderSource (b, h, 0.25);
                bool same = true;
                for (int i = 0; i < first.getNumSamples() && same; ++i)
                    same = first.getSample (0, i) == second.getSample (0, i);
                expect (same, "mode " + juce::String (mode) + " is not deterministic");
            }
        }
    }

    //==========================================================================
    void testGesturePressure()
    {
        beginTest ("Gesture level rises with pressure and never explodes");
        {
            for (int mode = 0; mode < (int) GestureSource::Mode::Count; ++mode)
            {
                double previous = -1.0;
                for (float pressure : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
                {
                    Harness h;
                    h.set (Param::gestureMode, (float) mode);
                    h.set (Param::gesturePressure, pressure);
                    h.set (Param::gestureMotion, 0.0f);      // measuring level, not drift
                    GestureSource src;
                    auto buf = renderSource (src, h, 1.0);
                    const double level = rmsOf (buf, (int) (0.1 * h.sr));
                    expect (level > previous * 0.98,
                            "mode " + juce::String (mode) + " level fell from " + juce::String (previous)
                                + " to " + juce::String (level) + " at pressure " + juce::String (pressure));
                    previous = level;
                }
                expect (previous > 0.0, "mode " + juce::String (mode) + " is silent at full pressure");
            }
        }

        beginTest ("Gesture level follows velocity");
        {
            Harness soft, loud;
            soft.note.velocity = 0.1f;
            loud.note.velocity = 1.0f;
            GestureSource a, b;
            const double quiet = rmsOf (renderSource (a, soft, 0.5), (int) (0.1 * soft.sr));
            const double strong = rmsOf (renderSource (b, loud, 0.5), (int) (0.1 * loud.sr));
            expect (strong > quiet * 1.5, "velocity should scale the gesture");
        }
    }

    //==========================================================================
    /**
        BANDWIDTH decides how focused the gesture is, not how loud it is.

        A fixed band normalisation could not hold that: every mode feeds the filter a
        differently coloured signal, so the same gain boosted one and buried another —
        measured 8 to 11 dB across the control on the noisy modes and 7.9 dB the other
        way on ELECTRICAL, which is a level jump in a character control.
    */
    void testGestureBandwidthLevel()
    {
        beginTest ("Gesture BANDWIDTH changes focus, not level");
        {
            for (int mode = 0; mode < (int) GestureSource::Mode::Count; ++mode)
            {
                double quietest = 1.0e9, loudest = 0.0, centroidNarrow = 0.0, centroidWide = 0.0;
                for (float bandwidth : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
                {
                    Harness h;
                    h.set (Param::gestureMode, (float) mode);
                    h.set (Param::gestureBandwidth, bandwidth);
                    h.set (Param::gestureMotion, 0.0f);
                    GestureSource src;
                    auto buf = renderSource (src, h, 1.5);
                    // Skip the first 200 ms: the power followers need a window to settle.
                    const double level = rmsOf (buf, (int) (0.2 * h.sr));
                    expect (level > 1.0e-5, "mode " + juce::String (mode) + " went silent at bandwidth "
                                                + juce::String (bandwidth));
                    quietest = juce::jmin (quietest, level);
                    loudest = juce::jmax (loudest, level);
                    if (bandwidth == 0.0f) centroidNarrow = centroidOf (buf, h.sr);
                    if (bandwidth == 1.0f) centroidWide = centroidOf (buf, h.sr);
                }
                const double spreadDb = juce::Decibels::gainToDecibels (loudest / quietest);
                expect (spreadDb < 4.0, "mode " + juce::String (mode) + " moved the level by "
                                            + juce::String (spreadDb, 1) + " dB across BANDWIDTH");
                expect (centroidWide > centroidNarrow * 1.05,
                        "mode " + juce::String (mode) + " should still open up: centroid "
                            + juce::String (centroidNarrow, 0) + " -> " + juce::String (centroidWide, 0) + " Hz");
            }
        }
    }

    /**
        The source's own soft limiter is a peak safety net, not a tone control. The per-mode
        trims used to drive it by 6.9 dB at LEVEL 1 (ELECTRICAL) and 4.6 dB (SCRAPE),
        flattening exactly the transients those gestures are made of. At the loudest
        velocity it may still round the single worst grain — that is what it is for — but
        it must cost the gesture no measurable loudness.
    */
    void testGestureHeadroom()
    {
        beginTest ("No gesture drives the source soft limiter into compression");
        {
            for (int mode = 0; mode < (int) GestureSource::Mode::Count; ++mode)
            {
                // A quarter of the level cannot reach the 0.9 knee, so four times its peak
                // and RMS is what the mode would do with no limiter in the way.
                auto renderAt = [mode] (float level)
                {
                    Harness h;
                    h.set (Param::gestureMode, (float) mode);
                    h.set (Param::gestureLevel, level);
                    h.set (Param::gestureMotion, 0.0f);
                    h.note.velocity = 1.0f;                 // the worst case the limiter ever sees
                    GestureSource src;
                    auto buf = renderSource (src, h, 1.5);
                    return std::make_pair ((double) peakOf (buf), rmsOf (buf, (int) (0.2 * h.sr)));
                };
                const auto quarter = renderAt (0.25f);
                const auto full = renderAt (1.0f);
                const juce::String tag = "gesture mode " + juce::String (mode) + ": ";
                expect (quarter.first < 0.9, tag + "the reference render must stay under the knee");

                const double peakLoss = -juce::Decibels::gainToDecibels (full.first / juce::jmax (1.0e-9, quarter.first * 4.0));
                const double rmsLoss = -juce::Decibels::gainToDecibels (full.second / juce::jmax (1.0e-12, quarter.second * 4.0));
                expect (peakLoss < 2.5, tag + "loses " + juce::String (peakLoss, 1) + " dB of transient to the soft limiter");
                expect (rmsLoss < 0.25, tag + "the soft limiter is compressing, not catching peaks: "
                                            + juce::String (rmsLoss, 2) + " dB of RMS");
                expect (full.first <= 1.0, tag + "peaked at " + juce::String (full.first));
            }
        }
    }

    /**
        ARCHITECTURE.md: a source renders at about -15 dBFS peak for one note. SAMPLE was
        landing 7.7 dB above WAVE, so selecting it jumped the level.
    */
    void testSampleLevel()
    {
        beginTest ("SAMPLE sits with the other sources, not 8 dB above them");
        {
            auto sample = BuiltInSamples::create (0);
            expect (sample->peak > 0.9f, "the built-in should be normalised near full scale");

            Harness h;
            h.sample = sample.get();
            h.note.velocity = 100.0f / 127.0f;
            SampleSource sampleSource;
            const double samplePeak = (double) peakOf (renderSource (sampleSource, h, 1.0));

            Harness w;
            w.note.velocity = h.note.velocity;
            WaveSource waveSource;
            const double wavePeak = (double) peakOf (renderSource (waveSource, w, 1.0));

            expect (wavePeak > 0.05, "the WAVE reference should be sounding");
            const double diffDb = juce::Decibels::gainToDecibels (samplePeak / wavePeak);
            expect (std::abs (diffDb) < 6.0,
                    "a full scale sample at LEVEL 1 peaks " + juce::String (diffDb, 1) + " dB away from WAVE");
        }
    }

    //==========================================================================
    void testGestureDc()
    {
        beginTest ("Gestures are DC free (below -60 dB)");
        {
            for (int mode = 0; mode < (int) GestureSource::Mode::Count; ++mode)
            {
                for (float position : { 0.0f, 0.3f, 1.0f })
                {
                    Harness h;
                    h.set (Param::gestureMode, (float) mode);
                    h.set (Param::gesturePosition, position);
                    GestureSource src;
                    auto buf = renderSource (src, h, 2.0);
                    const double dc = std::abs (meanOf (buf));
                    const double rms = rmsOf (buf, (int) (0.05 * h.sr));
                    expect (rms > 1.0e-4, "mode " + juce::String (mode) + " produced nothing");
                    // -60 dBFS. Noise modes cannot do better than their own low frequency
                    // content over a finite window (white noise alone means rms/sqrt(N)).
                    expect (dc < 1.0e-3, "mode " + juce::String (mode) + " DC " + juce::String (dc)
                                             + " (" + juce::String (gainToDb ((float) dc), 1) + " dBFS) vs rms "
                                             + juce::String (rms));
                }
            }
        }
    }

    //==========================================================================
    void testEnergyReporting()
    {
        beginTest ("energy() and isActive() are truthful");
        {
            auto sine = makeSine (250.0, 48000.0, 0.3);
            Harness h;
            h.sample = sine.get();
            h.set (Param::sampleKeytrack, 0.0f);
            h.set (Param::sampleMode, 0.0f);      // ONE SHOT

            SampleSource src;
            src.prepare (h.sr, h.block);
            src.reset();
            expectWithinAbsoluteError (src.energy(), 0.0f, 1.0e-6f);
            src.noteOn (h.note, h.params);

            std::array<float, 256> l {}, r {};
            const auto ctx = h.context (256);
            src.render (l.data(), r.data(), 256, ctx, h.note);
            float peak = 0.0f;
            for (int i = 0; i < 256; ++i) peak = juce::jmax (peak, std::abs (l[i]), std::abs (r[i]));
            expectWithinAbsoluteError (src.energy(), peak, 1.0e-6f);
            expect (src.isActive());

            // Run past the end of the sample: a one shot must report itself finished.
            for (int i = 0; i < (int) (0.5 * h.sr / 256) + 4; ++i)
                src.render (l.data(), r.data(), 256, ctx, h.note);
            expect (! src.isActive(), "a finished one shot must not stay active");
            expectWithinAbsoluteError (src.energy(), 0.0f, 1.0e-5f);

            // LOOP stays active forever (the amp envelope ends the voice).
            Harness loop;
            loop.sample = sine.get();
            loop.set (Param::sampleMode, 1.0f);
            SampleSource looper;
            looper.prepare (loop.sr, loop.block);
            looper.reset();
            looper.noteOn (loop.note, loop.params);
            const auto loopCtx = loop.context (256);
            for (int i = 0; i < 200; ++i) looper.render (l.data(), r.data(), 256, loopCtx, loop.note);
            expect (looper.isActive());
            expect (looper.energy() > 1.0e-3f);

            // GESTURE: active while gated, silent and inactive after the release.
            Harness g;
            GestureSource gesture;
            gesture.prepare (g.sr, g.block);
            gesture.reset();
            expect (! gesture.isActive());
            gesture.noteOn (g.note, g.params);
            const auto gctx = g.context (256);
            for (int i = 0; i < 40; ++i) gesture.render (l.data(), r.data(), 256, gctx, g.note);
            expect (gesture.isActive());
            peak = 0.0f;
            for (int i = 0; i < 256; ++i) peak = juce::jmax (peak, std::abs (l[i]), std::abs (r[i]));
            expectWithinAbsoluteError (gesture.energy(), peak, 1.0e-6f);
            gesture.noteOff();
            for (int i = 0; i < 100; ++i) gesture.render (l.data(), r.data(), 256, gctx, g.note);
            expect (! gesture.isActive());
        }
    }

    //==========================================================================
    void testFullEngine()
    {
        beginTest ("SAMPLE and GESTURE stay safe through the whole engine");
        {
            for (int source : { 3, 4 })
            {
                const int numModes = source == 3 ? (int) SampleSource::Mode::Count : (int) GestureSource::Mode::Count;
                for (int mode = 0; mode < numModes; ++mode)
                {
                    for (double sr : { 44100.0, 96000.0 })
                    {
                        SynthEngine engine;
                        engine.prepare (sr, 256);
                        engine.publishSample (BuiltInSamples::create (mode % BuiltInSamples::count()));

                        auto params = engineParams();
                        params[(size_t) paramIndex (Param::sourceSelected)] = (float) source;
                        params[(size_t) paramIndex (source == 3 ? Param::sampleMode : Param::gestureMode)] = (float) mode;
                        params[(size_t) paramIndex (Param::shapeMix)] = 1.0f;     // through Matter
                        params[(size_t) paramIndex (Param::gesturePressure)] = 0.9f;
                        params[(size_t) paramIndex (Param::gestureSpeed)] = 0.8f;

                        const int total = (int) (1.2 * sr);
                        juce::AudioBuffer<float> out (2, 256);
                        TransportInfo transport;
                        float peak = 0.0f;
                        bool finite = true;
                        for (int pos = 0; pos < total; pos += 256)
                        {
                            juce::MidiBuffer midi;
                            if (pos == 0)
                            {
                                midi.addEvent (juce::MidiMessage::noteOn (1, 48, 0.9f), 0);
                                midi.addEvent (juce::MidiMessage::noteOn (1, 55, 0.9f), 8);
                                midi.addEvent (juce::MidiMessage::noteOn (1, 64, 0.9f), 16);
                            }
                            if (pos <= (int) (0.6 * sr) && pos + 256 > (int) (0.6 * sr))
                                for (int n : { 48, 55, 64 }) midi.addEvent (juce::MidiMessage::noteOff (1, n), 0);
                            out.clear();
                            engine.process (out, midi, params, transport);
                            for (int ch = 0; ch < 2; ++ch)
                                for (int i = 0; i < out.getNumSamples(); ++i)
                                {
                                    const float v = out.getSample (ch, i);
                                    if (! std::isfinite (v)) finite = false;
                                    else peak = juce::jmax (peak, std::abs (v));
                                }
                        }

                        const auto safety = engine.diagnostics().safety.snapshot();
                        expect (finite, "source " + juce::String (source) + " mode " + juce::String (mode) + " went non-finite");
                        expect (peak <= 1.0f, "peak " + juce::String (peak));
                        expect (peak > 1.0e-4f, "source " + juce::String (source) + " mode " + juce::String (mode) + " was silent");
                        expect (safety.counts[(int) SafetyEvent::NaN] == 0, "NaN events reported");
                        expect (safety.counts[(int) SafetyEvent::Infinity] == 0, "Infinity events reported");
                    }
                }
            }
        }
    }
};

static SampleGestureTests sampleGestureTests;

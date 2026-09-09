#include <juce_core/juce_core.h>
#include "core/Random.h"
#include "core/LockFreeQueue.h"
#include "core/Smoothing.h"
#include "core/FastMath.h"
#include "core/RealtimeUtils.h"
#include "dsp/Envelope.h"

using namespace am;

class CoreTests : public juce::UnitTest
{
public:
    CoreTests() : juce::UnitTest ("Core utilities", "core") {}

    void runTest() override
    {
        beginTest ("Rng is deterministic and bounded");
        {
            Rng a (42), b (42), c (43);
            bool same = true, different = false;
            for (int i = 0; i < 1000; ++i)
            {
                const float x = a.nextFloat(), y = b.nextFloat(), z = c.nextFloat();
                same = same && (x == y);
                different = different || (x != z);
                expect (x >= 0.0f && x < 1.0f);
            }
            expect (same);
            expect (different);
            Rng g (7);
            double sum = 0.0; int n = 20000;
            for (int i = 0; i < n; ++i) sum += g.nextGaussian();
            expectWithinAbsoluteError (sum / n, 0.0, 0.05);
            for (int i = 0; i < 1000; ++i) expect (g.nextInt (5) >= 0 && g.nextInt (5) < 5);
        }

        beginTest ("SpscQueue push/pop and overflow");
        {
            SpscQueue<int, 8> q;
            for (int i = 0; i < 8; ++i) expect (q.push (i));
            expect (! q.push (99));
            int out = -1;
            for (int i = 0; i < 8; ++i) { expect (q.pop (out)); expectEquals (out, i); }
            expect (! q.pop (out));
        }

        beginTest ("TripleBuffer delivers latest snapshot");
        {
            struct S { int v; };
            TripleBuffer<S> tb;
            S out { -1 };
            expect (! tb.read (out));
            for (int i = 0; i < 10; ++i) { tb.beginWrite().v = i; tb.endWrite(); }
            expect (tb.read (out));
            expectEquals (out.v, 9);
            expect (! tb.read (out));
            expectEquals (tb.latest().v, 9);
        }

        beginTest ("OnePoleSmoother converges");
        {
            OnePoleSmoother s;
            s.prepare (48000.0, 10.0f);
            s.reset (0.0f);
            s.setTarget (1.0f);
            for (int i = 0; i < 4800; ++i) s.next();
            expectWithinAbsoluteError (s.getCurrent(), 1.0f, 0.001f);
            s.reset (0.0f); s.setTarget (1.0f);
            const float skipped = s.skip (480);
            OnePoleSmoother t; t.prepare (48000.0, 10.0f); t.reset (0.0f); t.setTarget (1.0f);
            float stepped = 0.0f; for (int i = 0; i < 480; ++i) stepped = t.next();
            expectWithinAbsoluteError (skipped, stepped, 1.0e-4f);
        }

        beginTest ("fastSin01 and fastTanh accuracy");
        {
            float maxErr = 0.0f;
            for (int i = 0; i < 1000; ++i)
            {
                const float p = (float) i / 1000.0f;
                maxErr = juce::jmax (maxErr, std::abs (fastSin01 (p) - std::sin (p * juce::MathConstants<float>::twoPi)));
            }
            expect (maxErr < 2.0e-3f, "fastSin01 max error " + juce::String (maxErr));
            float maxTanh = 0.0f;
            for (float x = -4.0f; x <= 4.0f; x += 0.01f) maxTanh = juce::jmax (maxTanh, std::abs (fastTanh (x) - std::tanh (x)));
            expect (maxTanh < 2.0e-3f, "fastTanh max error " + juce::String (maxTanh));
        }

        beginTest ("scrubBuffer removes non-finite values");
        {
            float buf[6] = { 0.5f, std::numeric_limits<float>::quiet_NaN(), -0.25f, std::numeric_limits<float>::infinity(), 0.0f, -std::numeric_limits<float>::infinity() };
            expect (containsNonFinite (buf, 6));
            expectEquals (scrubBuffer (buf, 6), 3);
            expect (! containsNonFinite (buf, 6));
            expectEquals (buf[1], 0.0f);
        }

        beginTest ("AudioTapRing returns latest samples");
        {
            AudioTapRing<64> ring;
            float in[100]; for (int i = 0; i < 100; ++i) in[i] = (float) i;
            ring.push (in, in, 100);
            float out[16]; ring.readLatest (out, nullptr, 16);
            expectEquals (out[0], 84.0f);
            expectEquals (out[15], 99.0f);
        }

        beginTest ("ADSR envelope shape and timing");
        {
            ADSREnvelope env;
            const double sr = 48000.0;
            env.prepare (sr);
            env.setParameters (0.01f, 0.1f, 0.5f, 0.2f, 0.5f);
            env.noteOn();
            float peak = 0.0f; int samplesToPeak = 0;
            for (int i = 0; i < 2000; ++i) { const float v = env.next(); if (v > peak) { peak = v; samplesToPeak = i; } }
            expectWithinAbsoluteError (peak, 1.0f, 0.02f);
            expect (samplesToPeak > 300 && samplesToPeak < 700, "attack took " + juce::String (samplesToPeak) + " samples");
            for (int i = 0; i < 48000; ++i) env.next();
            expectWithinAbsoluteError (env.getLevel(), 0.5f, 0.01f);
            env.noteOff();
            int releaseSamples = 0;
            while (env.isActive() && releaseSamples < 96000) { env.next(); ++releaseSamples; }
            expect (! env.isActive());
            expect (releaseSamples > 4000 && releaseSamples < 20000, "release took " + juce::String (releaseSamples));
            env.noteOn(); env.next(); env.kill();
            int killSamples = 0; while (env.isActive() && killSamples < 5000) { env.next(); ++killSamples; }
            expect (killSamples < 1500);
        }
    }
};

static CoreTests coreTests;

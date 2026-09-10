#include <juce_core/juce_core.h>

#include "dsp/SynthEngine.h"
#include "dsp/mod/ModSources.h"
#include "dsp/mod/ModulationEngine.h"
#include "state/ModRouting.h"
#include "state/StateManager.h"

#include <cstdlib>
#include <new>

using namespace am;

//==============================================================================
// Test-only allocation counter.
//
// The whole test binary routes through these so we can prove that the engine
// never allocates on the audio thread once prepare() has run. Counting is off
// unless a test switches it on, and every path is a plain malloc/free pair.

namespace amtest
{
    std::atomic<int>  allocationCount { 0 };
    std::atomic<bool> countingAllocations { false };

    inline void* trackedAllocate (std::size_t size)
    {
        if (countingAllocations.load (std::memory_order_relaxed))
            allocationCount.fetch_add (1, std::memory_order_relaxed);
        void* p = std::malloc (size == 0 ? 1 : size);
        if (p == nullptr) throw std::bad_alloc();
        return p;
    }

    inline void* trackedAllocateAligned (std::size_t size, std::size_t alignment)
    {
        if (countingAllocations.load (std::memory_order_relaxed))
            allocationCount.fetch_add (1, std::memory_order_relaxed);
        if (alignment < sizeof (void*)) alignment = sizeof (void*);
        const std::size_t rounded = ((size == 0 ? 1 : size) + alignment - 1) / alignment * alignment;
        void* p = std::aligned_alloc (alignment, rounded);
        if (p == nullptr) throw std::bad_alloc();
        return p;
    }

    /** RAII guard: counts allocations for the lifetime of the object. */
    struct AllocationScope
    {
        AllocationScope() { allocationCount.store (0, std::memory_order_relaxed); countingAllocations.store (true, std::memory_order_relaxed); }
        ~AllocationScope() { countingAllocations.store (false, std::memory_order_relaxed); }
        int count() const noexcept { return allocationCount.load (std::memory_order_relaxed); }
    };
}

void* operator new (std::size_t size)                       { return amtest::trackedAllocate (size); }
void* operator new[] (std::size_t size)                     { return amtest::trackedAllocate (size); }
void* operator new (std::size_t s, std::align_val_t a)      { return amtest::trackedAllocateAligned (s, (std::size_t) a); }
void* operator new[] (std::size_t s, std::align_val_t a)    { return amtest::trackedAllocateAligned (s, (std::size_t) a); }
void  operator delete (void* p) noexcept                    { std::free (p); }
void  operator delete[] (void* p) noexcept                  { std::free (p); }
void  operator delete (void* p, std::size_t) noexcept       { std::free (p); }
void  operator delete[] (void* p, std::size_t) noexcept     { std::free (p); }
void  operator delete (void* p, std::align_val_t) noexcept  { std::free (p); }
void  operator delete[] (void* p, std::align_val_t) noexcept { std::free (p); }
void  operator delete (void* p, std::size_t, std::align_val_t) noexcept  { std::free (p); }
void  operator delete[] (void* p, std::size_t, std::align_val_t) noexcept { std::free (p); }

//==============================================================================
namespace
{
    ParamValues defaultParams()
    {
        return ParameterRegistry::defaults();
    }

    void setParam (ParamValues& v, Param p, float value)
    {
        v[(size_t) paramIndex (p)] = ParameterRegistry::get (p).clampValue (value);
    }

    /**
        Measures an LFO's frequency by linearly interpolating its rising zero
        crossings, so the result is far more precise than the control rate.
    */
    double measureLfoHz (ModLFO::Settings settings, double sampleRate, int blockSize, double seconds)
    {
        ModLFO lfo;
        lfo.prepare (sampleRate, 1234);
        TransportInfo transport;

        const int totalBlocks = (int) std::floor (seconds * sampleRate / (double) blockSize);
        double previous = 0.0, firstCrossing = -1.0, lastCrossing = -1.0;
        int crossings = 0;

        for (int b = 0; b < totalBlocks; ++b)
        {
            lfo.advance (settings, blockSize, sampleRate, transport, false);
            const double v = lfo.value();
            const double t = (double) b * (double) blockSize / sampleRate;
            if (b > 0 && previous <= 0.0 && v > 0.0)
            {
                const double frac = previous == v ? 0.0 : -previous / (v - previous);
                const double crossing = t - (double) blockSize / sampleRate * (1.0 - frac);
                if (firstCrossing < 0.0) firstCrossing = crossing;
                lastCrossing = crossing;
                ++crossings;
            }
            previous = v;
        }

        if (crossings < 2) return 0.0;
        return (double) (crossings - 1) / (lastCrossing - firstCrossing);
    }

    /** A routing table that hits a wide spread of destinations with every source. */
    ModRoutingTable buildStressTable (float depth)
    {
        static const Param targets[] =
        {
            Param::shapeDensity, Param::shapeForm, Param::shapeMass, Param::shapeTension,
            Param::shapeDecay, Param::shapeSurface, Param::shapeBlend, Param::shapeCoupling,
            Param::shapeDistribution, Param::shapeMix, Param::shapePitch, Param::shapeStereo,
            Param::shapeExcite, Param::shapeStrike, Param::evolveBend, Param::evolveMelt,
            Param::evolveTear, Param::evolveMagnet, Param::evolveGravity, Param::evolveScatter,
            Param::evolveCrush, Param::evolveSpeed, Param::evolveMotion, Param::wavePosition,
            Param::waveScan, Param::waveMorph, Param::waveDetune, Param::waveSpread,
            Param::waveFM, Param::wavePM, Param::waveAM, Param::waveRing,
            Param::dustDensity, Param::dustColor, Param::dustGrain, Param::dustJitter,
            Param::fractureAmount, Param::fractureSpread, Param::fractureFeedback, Param::fractureMix,
            Param::spaceMix, Param::spaceSize, Param::spaceTone, Param::spaceFeedback,
            Param::masterGain, Param::masterFine, Param::waveFine, Param::dustPitch,
            Param::impactHardness, Param::impactBrightness, Param::impactLength, Param::impactCurve,
            Param::gesturePressure, Param::gestureSpeed, Param::gestureRoughness, Param::gestureMotion,
            Param::spaceReverbSize, Param::spaceReverbDecay, Param::spaceDelayFeedback, Param::spaceDistDrive,
            Param::ampAttack, Param::ampDecay, Param::ampSustain, Param::ampRelease
        };
        constexpr int numTargets = (int) (sizeof (targets) / sizeof (targets[0]));

        ModRoutingTable table;
        for (int i = 0; i < numTargets && ! table.isFull(); ++i)
        {
            ModRouting r;
            r.source  = (ModSource) (1 + (i % (kNumModSources - 1)));
            r.target  = targets[i];
            r.depth   = (i % 2 == 0) ? depth : -depth;
            r.curve   = 0.0f;
            r.bipolar = (i % 3) != 0;
            r.enabled = true;
            table.add (r);
        }
        return table;
    }

    /** Renders `seconds` of audio through the engine, returning the peak / non-finite count. */
    struct RenderResult { float peak = 0.0f; int nonFinite = 0; double realtimeRatio = 0.0; };

    RenderResult renderEngine (SynthEngine& engine, const ParamValues& params, double sampleRate, int blockSize,
                               double seconds, const std::vector<int>& notes, float velocity, double releaseAt)
    {
        RenderResult result;
        juce::AudioBuffer<float> buffer (2, blockSize);
        TransportInfo transport;
        transport.isPlaying = true;

        const int totalBlocks = (int) std::floor (seconds * sampleRate / (double) blockSize);
        bool released = false;
        const double start = juce::Time::getMillisecondCounterHiRes();

        for (int b = 0; b < totalBlocks; ++b)
        {
            juce::MidiBuffer midi;
            if (b == 0)
                for (int n : notes) midi.addEvent (juce::MidiMessage::noteOn (1, n, velocity), 0);
            const double t = (double) b * (double) blockSize / sampleRate;
            if (! released && releaseAt > 0.0 && t >= releaseAt)
            {
                for (int n : notes) midi.addEvent (juce::MidiMessage::noteOff (1, n), 0);
                released = true;
            }

            buffer.clear();
            engine.process (buffer, midi, params, transport);
            transport.ppqPosition += (double) blockSize / sampleRate * transport.bpm / 60.0;

            for (int ch = 0; ch < 2; ++ch)
            {
                const float* d = buffer.getReadPointer (ch);
                for (int i = 0; i < blockSize; ++i)
                {
                    if (! std::isfinite (d[i])) { ++result.nonFinite; continue; }
                    result.peak = juce::jmax (result.peak, std::abs (d[i]));
                }
            }
        }
        const double elapsed = juce::Time::getMillisecondCounterHiRes() - start;
        result.realtimeRatio = elapsed / (seconds * 1000.0);
        return result;
    }
}

//==============================================================================
class ModTests : public juce::UnitTest
{
public:
    ModTests() : juce::UnitTest ("Modulation", "mod") {}

    void runTest() override
    {
        testLfoFrequency();
        testLfoTempoSync();
        testLfoPhaseSymmetryFade();
        testLfoShapes();
        testEnvelope();
        testChaos();
        testRoutingTable();
        testRoutingJson();
        testDepthAndPolarity();
        testPerVoiceIndependence();
        testEngineIntegration();
        testNoAudioThreadAllocation();
        testNaNSafety();
        testCpuBudget();
    }

private:
    //==========================================================================
    void testLfoFrequency()
    {
        beginTest ("LFO frequency is accurate at every sample rate and block size");

        const double rates[] = { 0.5, 3.7, 12.0 };
        const double sampleRates[] = { 44100.0, 48000.0, 88200.0, 96000.0 };
        const int blocks[] = { 32, 64, 128, 256, 512, 1024 };

        double worstError = 0.0;
        for (double rate : rates)
        {
            for (double sr : sampleRates)
            {
                for (int block : blocks)
                {
                    ModLFO::Settings s;
                    s.rate = (float) rate;
                    s.shape = LFOShape::Sine;
                    const double measured = measureLfoHz (s, sr, block, 10.0);
                    const double error = std::abs (measured - rate) / rate;
                    worstError = juce::jmax (worstError, error);
                    expect (error < 0.01, juce::String ("rate ") + juce::String (rate) + " @ " + juce::String (sr)
                                          + " / block " + juce::String (block) + " measured " + juce::String (measured));
                }
            }
        }
        logMessage ("LFO frequency: worst error over 10 s = " + juce::String (worstError * 100.0, 4) + " %");
    }

    //==========================================================================
    void testLfoTempoSync()
    {
        beginTest ("Tempo-synced LFOs lock to the transport's PPQ position");

        TransportInfo transport;
        transport.bpm = 120.0;
        transport.isPlaying = true;

        ModLFO::Settings s;
        s.sync = true;
        s.division = 5;          // 1/4 = one beat per cycle
        s.shape = LFOShape::Sine;

        ModLFO lfo;
        lfo.prepare (48000.0, 1);

        // On every beat the phase must be exactly at the start of the cycle.
        for (int beat = 0; beat < 8; ++beat)
        {
            transport.ppqPosition = (double) beat;
            lfo.advance (s, 64, 48000.0, transport, true);
            expectWithinAbsoluteError (lfo.value(), 0.0f, 1.0e-5f, "beat " + juce::String (beat));
            expectWithinAbsoluteError ((float) lfo.phase(), 0.0f, 1.0e-6f);
        }

        // A quarter of the way through the beat the sine is at its peak.
        transport.ppqPosition = 0.25;
        lfo.advance (s, 64, 48000.0, transport, true);
        expectWithinAbsoluteError (lfo.value(), 1.0f, 1.0e-4f);

        // 1/8 division: two cycles per beat.
        s.division = 6;
        transport.ppqPosition = 0.5;
        lfo.advance (s, 64, 48000.0, transport, true);
        expectWithinAbsoluteError ((float) lfo.phase(), 0.0f, 1.0e-6f);

        // Divisions are the musical lengths we promise.
        expectWithinAbsoluteError ((float) lfoDivisionBeats (3), 4.0f, 1.0e-6f);      // 1/1
        expectWithinAbsoluteError ((float) lfoDivisionBeats (5), 1.0f, 1.0e-6f);      // 1/4
        expectWithinAbsoluteError ((float) lfoDivisionBeats (7), 0.25f, 1.0e-6f);     // 1/16
        expectWithinAbsoluteError ((float) lfoDivisionBeats (9), 2.0f / 3.0f, 1.0e-6f);  // 1/4T
        expectWithinAbsoluteError ((float) lfoDivisionBeats (12), 1.5f, 1.0e-6f);     // 1/4D

        // Not playing: the synced LFO free-runs at the tempo-derived rate.
        TransportInfo stopped;
        stopped.bpm = 120.0;
        stopped.isPlaying = false;
        s.division = 5;
        expectWithinAbsoluteError ((float) ModLFO::frequencyFor (s, stopped), 2.0f, 1.0e-5f);
        const double freeHz = measureLfoHz (s, 48000.0, 64, 10.0);
        expect (std::abs (freeHz - 2.0) / 2.0 < 0.01, "free-running synced rate " + juce::String (freeHz));
    }

    //==========================================================================
    void testLfoPhaseSymmetryFade()
    {
        beginTest ("LFO phase offset, symmetry and fade-in behave");

        TransportInfo transport;
        ModLFO lfo;
        lfo.prepare (48000.0, 99);

        // Phase offset: a quarter turn puts the sine at its peak straight away.
        ModLFO::Settings s;
        s.phase = 0.25f;
        lfo.advance (s, 64, 48000.0, transport, false);
        expectWithinAbsoluteError (lfo.value(), 1.0f, 1.0e-4f);

        // Symmetry: a square's duty cycle follows the skew.
        for (float symmetry : { 0.25f, 0.5f, 0.75f })
        {
            ModLFO sq;
            sq.prepare (48000.0, 7);
            ModLFO::Settings ss;
            ss.shape = LFOShape::Square;
            ss.rate = 1.0f;
            ss.symmetry = symmetry;
            int high = 0, total = 0;
            for (int i = 0; i < 4000; ++i)   // 4 s at 1 Hz, 1 ms slices
            {
                sq.advance (ss, 48, 48000.0, transport, false);
                if (sq.value() > 0.0f) ++high;
                ++total;
            }
            const float duty = (float) high / (float) total;
            expectWithinAbsoluteError (duty, symmetry, 0.02f, "duty at symmetry " + juce::String (symmetry));
        }

        // Depth scales the output linearly.
        {
            ModLFO a, b;
            a.prepare (48000.0, 3); b.prepare (48000.0, 3);
            ModLFO::Settings sa; sa.phase = 0.25f; sa.depth = 1.0f;
            ModLFO::Settings sb = sa; sb.depth = 0.35f;
            a.advance (sa, 64, 48000.0, transport, false);
            b.advance (sb, 64, 48000.0, transport, false);
            expectWithinAbsoluteError (b.value(), a.value() * 0.35f, 1.0e-5f);
        }

        // Fade-in: silent at the retrigger, half way after half the fade, full after it.
        {
            ModLFO f;
            f.prepare (48000.0, 5);
            ModLFO::Settings sf;
            sf.rate = 4.0f;
            sf.fade = 1.0f;
            f.retrigger();
            f.advance (sf, 64, 48000.0, transport, false);
            expectWithinAbsoluteError (f.fadeGain(), 0.0f, 1.0e-5f);
            expectWithinAbsoluteError (f.value(), 0.0f, 1.0e-5f);

            const int slices = (int) (0.5 * 48000.0 / 64.0);
            for (int i = 0; i < slices; ++i) f.advance (sf, 64, 48000.0, transport, false);
            expectWithinAbsoluteError (f.fadeGain(), 0.5f, 0.01f);

            for (int i = 0; i < slices * 2; ++i) f.advance (sf, 64, 48000.0, transport, false);
            expectWithinAbsoluteError (f.fadeGain(), 1.0f, 1.0e-5f);

            // Without a fade the LFO is at full depth immediately.
            ModLFO g;
            g.prepare (48000.0, 5);
            ModLFO::Settings sg; sg.rate = 4.0f; sg.phase = 0.25f;
            g.advance (sg, 64, 48000.0, transport, false);
            expectWithinAbsoluteError (g.fadeGain(), 1.0f, 1.0e-6f);
        }

        // Retriggering puts the phase back to the start.
        {
            ModLFO r;
            r.prepare (48000.0, 11);
            ModLFO::Settings sr2; sr2.rate = 3.0f;
            for (int i = 0; i < 100; ++i) r.advance (sr2, 64, 48000.0, transport, false);
            expect (r.phase() > 0.0);
            r.retrigger();
            expectWithinAbsoluteError ((float) r.phase(), 0.0f, 1.0e-9f);
        }
    }

    //==========================================================================
    void testLfoShapes()
    {
        beginTest ("Every LFO shape stays inside -1 … 1 and actually moves");

        TransportInfo transport;
        for (int shape = 0; shape < (int) LFOShape::Count; ++shape)
        {
            ModLFO lfo;
            lfo.prepare (48000.0, (uint32_t) (shape + 1));
            ModLFO::Settings s;
            s.shape = (LFOShape) shape;
            s.rate = 5.0f;
            s.symmetry = shape == (int) LFOShape::Warp ? 0.4f : 0.5f;

            float lo = 1.0e9f, hi = -1.0e9f;
            for (int i = 0; i < 4000; ++i)
            {
                lfo.advance (s, 64, 48000.0, transport, false);
                const float v = lfo.value();
                expect (std::isfinite (v), "shape " + juce::String (shape));
                lo = juce::jmin (lo, v); hi = juce::jmax (hi, v);
            }
            expect (lo >= -1.0001f && hi <= 1.0001f, "shape " + juce::String (shape) + " range " + juce::String (lo) + " … " + juce::String (hi));
            expect (hi - lo > 0.5f, "shape " + juce::String (shape) + " barely moves");
        }

        // Random and smooth random are deterministic for a given seed and different for different seeds.
        auto run = [&transport] (uint32_t seed, LFOShape shape)
        {
            ModLFO lfo;
            lfo.prepare (48000.0, seed);
            ModLFO::Settings s; s.shape = shape; s.rate = 8.0f;
            std::vector<float> out;
            for (int i = 0; i < 500; ++i) { lfo.advance (s, 64, 48000.0, transport, false); out.push_back (lfo.value()); }
            return out;
        };
        const auto a = run (3, LFOShape::Random), b = run (3, LFOShape::Random), c = run (4, LFOShape::Random);
        expect (a == b, "S&H random is deterministic for a seed");
        expect (a != c, "different seeds give different S&H sequences");

        // Smooth random is continuous: no jumps larger than a fraction of the range per slice.
        const auto smooth = run (3, LFOShape::SmoothRandom);
        float worstStep = 0.0f;
        for (size_t i = 1; i < smooth.size(); ++i) worstStep = juce::jmax (worstStep, std::abs (smooth[i] - smooth[i - 1]));
        expect (worstStep < 0.35f, "smooth random step " + juce::String (worstStep));
    }

    //==========================================================================
    void testEnvelope()
    {
        beginTest ("Modulation envelope stage timings, curve and loop");

        constexpr double sr = 48000.0;
        constexpr int block = 64;
        const double blockSeconds = (double) block / sr;

        ModEnvelope::Settings s;
        s.attack = 0.1f; s.decay = 0.2f; s.sustain = 0.5f; s.release = 0.3f; s.curve = 0.5f; s.loop = false;

        ModEnvelope env;
        env.reset();
        env.noteOn();

        double decayAt = -1.0, sustainAt = -1.0, idleAt = -1.0;
        float midAttack = -1.0f;
        const double noteOffAt = 1.0;
        bool released = false;

        for (int i = 0; i < (int) (2.0 * sr / block); ++i)
        {
            const double t = (double) i * blockSeconds;
            if (! released && t >= noteOffAt) { env.noteOff(); released = true; }
            const auto before = env.stage();
            env.advance (s, block, sr);
            if (std::abs (t - 0.05) < blockSeconds * 0.5) midAttack = env.value();
            if (before == ModEnvelope::Stage::Attack && env.stage() == ModEnvelope::Stage::Decay && decayAt < 0.0) decayAt = t;
            if (env.stage() == ModEnvelope::Stage::Sustain && sustainAt < 0.0) sustainAt = t;
            if (released && env.stage() == ModEnvelope::Stage::Idle && idleAt < 0.0) idleAt = t;
        }

        expectWithinAbsoluteError (decayAt, 0.1, blockSeconds * 1.5, "attack ends at " + juce::String (decayAt));
        expectWithinAbsoluteError (sustainAt, 0.3, blockSeconds * 2.0, "decay ends at " + juce::String (sustainAt));
        expectWithinAbsoluteError (idleAt, noteOffAt + 0.3, blockSeconds * 2.0, "release ends at " + juce::String (idleAt));
        expectWithinAbsoluteError (midAttack, 0.5f, 0.05f, "linear curve mid-attack " + juce::String (midAttack));

        // Sustain level is exactly the parameter.
        {
            ModEnvelope e2;
            e2.noteOn();
            for (int i = 0; i < (int) (0.6 * sr / block); ++i) e2.advance (s, block, sr);
            expectEquals ((int) e2.stage(), (int) ModEnvelope::Stage::Sustain);
            expectWithinAbsoluteError (e2.value(), 0.5f, 1.0e-5f);
        }

        // Curve: 0 rises faster than 0.5, which rises faster than 1.
        {
            auto levelAfter = [sr, block] (float curve, double seconds)
            {
                ModEnvelope e;
                ModEnvelope::Settings c; c.attack = 0.2f; c.decay = 1.0f; c.sustain = 1.0f; c.curve = curve;
                e.noteOn();
                for (int i = 0; i < (int) (seconds * sr / block); ++i) e.advance (c, block, sr);
                return e.value();
            };
            const float snappy = levelAfter (0.0f, 0.1), linear = levelAfter (0.5f, 0.1), soft = levelAfter (1.0f, 0.1);
            expect (snappy > linear + 0.1f, "snappy " + juce::String (snappy) + " vs linear " + juce::String (linear));
            expect (linear > soft + 0.1f, "linear " + juce::String (linear) + " vs soft " + juce::String (soft));
        }

        // Loop: never reaches sustain and keeps cycling while the gate is held.
        {
            ModEnvelope::Settings loop = s;
            loop.loop = true;
            loop.attack = 0.05f; loop.decay = 0.05f;
            ModEnvelope e;
            e.noteOn();
            int sustainSeen = 0, attackRestarts = 0;
            auto previousStage = e.stage();
            float lo = 1.0f, hi = 0.0f;
            for (int i = 0; i < (int) (2.0 * sr / block); ++i)
            {
                e.advance (loop, block, sr);
                if (e.stage() == ModEnvelope::Stage::Sustain) ++sustainSeen;
                if (previousStage == ModEnvelope::Stage::Decay && e.stage() == ModEnvelope::Stage::Attack) ++attackRestarts;
                previousStage = e.stage();
                lo = juce::jmin (lo, e.value()); hi = juce::jmax (hi, e.value());
            }
            expectEquals (sustainSeen, 0, "a looping envelope never sustains");
            expect (attackRestarts >= 15, "loop restarts " + juce::String (attackRestarts));
            expect (hi - lo > 0.4f, "loop swing " + juce::String (hi - lo));

            // Note-off still releases out of the loop.
            e.noteOff();
            for (int i = 0; i < (int) (0.6 * sr / block); ++i) e.advance (loop, block, sr);
            expectEquals ((int) e.stage(), (int) ModEnvelope::Stage::Idle);
        }

        // Zero-length stages are safe.
        {
            ModEnvelope::Settings fast;
            fast.attack = 0.0f; fast.decay = 0.0f; fast.sustain = 0.7f; fast.release = 0.0f;
            ModEnvelope e;
            e.noteOn();
            e.advance (fast, block, sr);
            expectWithinAbsoluteError (e.value(), 0.7f, 1.0e-4f);
            e.noteOff();
            e.advance (fast, block, sr);
            expectEquals ((int) e.stage(), (int) ModEnvelope::Stage::Idle);
        }
    }

    //==========================================================================
    void testChaos()
    {
        beginTest ("Chaos generators are bounded, deterministic per seed and different across seeds");

        constexpr double sr = 48000.0;
        constexpr int block = 64;

        for (int t = 0; t < (int) ChaosType::Count; ++t)
        {
            const auto type = (ChaosType) t;
            const juce::String name ("chaos type " + juce::String (t));

            ChaosGenerator a, b, c;
            a.prepare (7); b.prepare (7); c.prepare (8);
            ChaosGenerator::Settings s;
            s.type = type; s.rate = 6.0f; s.depth = 1.0f; s.stability = 0.4f; s.symmetry = 0.5f; s.seed = 7;
            ChaosGenerator::Settings s2 = s; s2.seed = 8;

            float lo = 1.0e9f, hi = -1.0e9f;
            bool identical = true, differs = false;
            for (int i = 0; i < 4000; ++i)
            {
                a.advance (s, block, sr);
                b.advance (s, block, sr);
                c.advance (s2, block, sr);
                const float v = a.value();
                expect (std::isfinite (v), name + " produced a non-finite value");
                lo = juce::jmin (lo, v); hi = juce::jmax (hi, v);
                if (a.value() != b.value()) identical = false;
                if (std::abs (a.value() - c.value()) > 1.0e-4f) differs = true;
            }
            expect (lo >= -1.0001f && hi <= 1.0001f, name + " range " + juce::String (lo) + " … " + juce::String (hi));
            expect (hi - lo > 0.1f, name + " barely moves (" + juce::String (hi - lo) + ")");
            expect (identical, name + " is not deterministic for a fixed seed");
            expect (differs, name + " gives the same output for different seeds");
        }

        // Depth scales, stability tames, symmetry biases — all still bounded.
        for (int t = 0; t < (int) ChaosType::Count; ++t)
        {
            ChaosGenerator quiet, wild;
            quiet.prepare (21); wild.prepare (21);
            ChaosGenerator::Settings q;
            q.type = (ChaosType) t; q.rate = 6.0f; q.depth = 0.25f; q.stability = 0.5f; q.symmetry = 0.5f; q.seed = 21;
            ChaosGenerator::Settings w = q; w.depth = 1.0f;

            float quietMax = 0.0f, wildMax = 0.0f;
            for (int i = 0; i < 3000; ++i)
            {
                quiet.advance (q, block, sr);
                wild.advance (w, block, sr);
                quietMax = juce::jmax (quietMax, std::abs (quiet.value()));
                wildMax = juce::jmax (wildMax, std::abs (wild.value()));
            }
            expect (quietMax <= 0.2501f, "depth 0.25 exceeded its bound: " + juce::String (quietMax));
            expect (wildMax > quietMax, "depth 1.0 should swing further than 0.25");
        }

        // Symmetry pushes the average without leaving the bounds.
        for (int t = 0; t < (int) ChaosType::Count; ++t)
        {
            auto mean = [t] (float symmetry)
            {
                ChaosGenerator g;
                g.prepare (33);
                ChaosGenerator::Settings s;
                s.type = (ChaosType) t; s.rate = 8.0f; s.depth = 1.0f; s.stability = 0.4f; s.symmetry = symmetry; s.seed = 33;
                double sum = 0.0;
                float peak = 0.0f;
                for (int i = 0; i < 6000; ++i)
                {
                    g.advance (s, 64, 48000.0);
                    sum += g.value();
                    peak = juce::jmax (peak, std::abs (g.value()));
                }
                return std::make_pair (sum / 6000.0, peak);
            };
            const auto low = mean (0.15f), high = mean (0.85f);
            expect (low.second <= 1.0001f && high.second <= 1.0001f, "symmetry broke the bounds");
            expect (high.first > low.first, "symmetry did not bias chaos type " + juce::String (t));
        }
    }

    //==========================================================================
    void testRoutingTable()
    {
        beginTest ("Routing table validates, de-duplicates and stays ordered");

        ModRoutingTable table;
        ModRouting r;
        r.source = ModSource::LFO1; r.target = Param::shapeDecay; r.depth = 0.5f;
        expect (table.add (r) >= 0);
        expectEquals (table.size(), 1);

        // Duplicate source + target is rejected.
        expectEquals (table.add (r), -1);
        expectEquals (table.size(), 1);

        // Same source, different target is fine.
        r.target = Param::shapeForm;
        expect (table.add (r) >= 0);
        expectEquals (table.size(), 2);

        // Non-modulatable targets are refused.
        ModRouting bad;
        bad.source = ModSource::LFO2; bad.target = Param::shapeMaterialA; bad.depth = 1.0f;
        expect (! ModRoutingTable::isValid (bad));
        expectEquals (table.add (bad), -1);

        bad.target = Param::evolveFreeze;         // Bool
        expectEquals (table.add (bad), -1);

        bad.source = ModSource::None; bad.target = Param::shapeMass;
        expectEquals (table.add (bad), -1);

        // Depth and curve clamp.
        ModRouting wide;
        wide.source = ModSource::Macro1; wide.target = Param::shapeMass; wide.depth = 9.0f; wide.curve = -7.0f;
        const int index = table.add (wide);
        expect (index >= 0);
        expectWithinAbsoluteError (table[index].depth, 1.0f, 1.0e-6f);
        expectWithinAbsoluteError (table[index].curve, -1.0f, 1.0e-6f);

        // Non-finite values are refused.
        ModRouting nan;
        nan.source = ModSource::LFO3; nan.target = Param::shapeTension;
        nan.depth = std::numeric_limits<float>::quiet_NaN();
        expect (! ModRoutingTable::isValid (nan));

        // Deterministic ordering: source first, then target index.
        for (int i = 0; i + 1 < table.size(); ++i)
        {
            const bool ordered = (int) table[i].source < (int) table[i + 1].source
                              || ((int) table[i].source == (int) table[i + 1].source
                                  && paramIndex (table[i].target) < paramIndex (table[i + 1].target));
            expect (ordered, "table is not ordered at " + juce::String (i));
        }

        // Adding the same set in a different order yields an identical table.
        ModRoutingTable other;
        other.add ({ ModSource::Macro1, Param::shapeMass, 1.0f, -1.0f, true, true });
        other.add ({ ModSource::LFO1, Param::shapeForm, 0.5f, 0.0f, true, true });
        other.add ({ ModSource::LFO1, Param::shapeDecay, 0.5f, 0.0f, true, true });
        expect (other == table, "insertion order changed the table");

        // Editing helpers.
        expect (table.setDepth (0, -0.25f));
        expectWithinAbsoluteError (table[0].depth, -0.25f, 1.0e-6f);
        expect (table.setBipolar (0, false));
        expect (! table[0].bipolar);
        expect (table.setEnabled (0, false));
        expectEquals (table.numEnabled(), table.size() - 1);
        expect (! table.setDepth (99, 0.5f));

        const int before = table.size();
        expect (table.remove (0));
        expectEquals (table.size(), before - 1);
        expect (! table.remove (before));

        // Capacity.
        ModRoutingTable full = buildStressTable (0.5f);
        expectEquals (full.size(), ModRoutingTable::kMaxRoutings);
        expect (full.isFull());
        ModRouting extra { ModSource::Gate, Param::spaceReverbMix, 0.5f, 0.0f, true, true };
        expectEquals (full.add (extra), -1, "a full table must reject further routings");

        // Source metadata is complete and unique.
        juce::StringArray ids;
        for (int i = 1; i < kNumModSources; ++i)
        {
            const auto s = (ModSource) i;
            const juce::String id (modSourceId (s));
            expect (id.isNotEmpty());
            expect (! ids.contains (id), "duplicate source id " + id);
            ids.add (id);
            expectEquals ((int) modSourceFromId (id.toRawUTF8()), i, "round trip of " + id);
            expect (juce::String (modSourceName (s)).isNotEmpty());
        }
        expectEquals ((int) modSourceFromId ("nonsense"), (int) ModSource::None);
    }

    //==========================================================================
    void testRoutingJson()
    {
        beginTest ("A full routing table survives a JSON round trip and rejects nonsense");

        const auto table = buildStressTable (0.75f);
        expectEquals (table.size(), ModRoutingTable::kMaxRoutings);

        const auto json = juce::JSON::toString (table.toVar());
        const auto parsed = juce::JSON::parse (json);
        juce::String warnings;
        const auto back = ModRoutingTable::fromVar (parsed, &warnings);

        expect (warnings.isEmpty(), warnings);
        expectEquals (back.size(), table.size());
        expect (back == table, "the table changed across the round trip");

        for (int i = 0; i < table.size(); ++i)
        {
            expectEquals ((int) back[i].source, (int) table[i].source);
            expectEquals (paramIndex (back[i].target), paramIndex (table[i].target));
            expectWithinAbsoluteError (back[i].depth, table[i].depth, 1.0e-6f);
            expectEquals (back[i].bipolar ? 1 : 0, table[i].bipolar ? 1 : 0);
        }

        // Through the patch state, exactly the way the plugin stores it.
        {
            PatchState state;
            state.mod = table.toVar();
            PatchState reloaded;
            juce::String stateWarnings;
            expect (StateManager::fromJson (StateManager::toJson (state), reloaded, &stateWarnings), stateWarnings);
            const auto fromPatch = ModRoutingTable::fromVar (reloaded.mod);
            expect (fromPatch == table, "PatchState.mod round trip failed");
        }

        // Invalid documents are dropped one routing at a time, with a warning each.
        {
            const juce::String bad = R"({"routings":[
                {"src":"lfo1","dst":"shape.decay","depth":0.5},
                {"src":"nope","dst":"shape.decay","depth":0.5},
                {"src":"lfo2","dst":"not.a.parameter","depth":0.5},
                {"src":"lfo3","dst":"shape.materialA","depth":0.5},
                {"src":"lfo1","dst":"shape.decay","depth":0.9},
                "junk"
            ]})";
            juce::String w;
            const auto parsedBad = ModRoutingTable::fromVar (juce::JSON::parse (bad), &w);
            expectEquals (parsedBad.size(), 1, "only the one valid routing should survive");
            expect (w.contains ("nope"));
            expect (w.contains ("not.a.parameter"));
            expect (w.contains ("shape.materialA"));
            expect (w.contains ("duplicate"));
            expectWithinAbsoluteError (parsedBad[0].depth, 0.5f, 1.0e-6f);
        }

        // An empty / missing section yields an empty table rather than failing.
        expect (ModRoutingTable::fromVar (juce::var()).isEmpty());
        expect (ModRoutingTable::fromVar (juce::JSON::parse ("{}")).isEmpty());
    }

    //==========================================================================
    void testDepthAndPolarity()
    {
        beginTest ("Depth scales to the target's range, clamps, and honours polarity");

        constexpr double sr = 48000.0;

        auto effective = [] (Param target, float base, ModSource source, float depth, bool bipolar,
                             float macroValue, float curve = 0.0f)
        {
            ParamValues params = defaultParams();
            setParam (params, target, base);
            setParam (params, Param::macro1, macroValue);

            ModulationEngine engine;
            engine.prepare (sr, 512);
            auto table = std::make_unique<ModRoutingTable>();
            table->add ({ source, target, depth, curve, bipolar, true });
            engine.publishRoutings (std::move (table));

            ControlGraph graph;
            graph.prepare (sr, 512);
            graph.resetTo (params);
            engine.beginBlock (params);

            TransportInfo transport;
            for (int i = 0; i < 8; ++i)     // let the smoother settle
            {
                engine.process (graph, 64, transport);
                graph.update (params, 64);
            }
            return graph.value (target);
        };

        // shape.decay spans 0…1: unipolar macro at 1.0 with depth 0.5 adds exactly 0.5.
        expectWithinAbsoluteError (effective (Param::shapeDecay, 0.25f, ModSource::Macro1, 0.5f, false, 1.0f), 0.75f, 1.0e-4f);
        expectWithinAbsoluteError (effective (Param::shapeDecay, 0.25f, ModSource::Macro1, 0.5f, false, 0.5f), 0.5f, 1.0e-4f);
        expectWithinAbsoluteError (effective (Param::shapeDecay, 0.25f, ModSource::Macro1, 0.5f, false, 0.0f), 0.25f, 1.0e-4f);

        // Negative depth subtracts.
        expectWithinAbsoluteError (effective (Param::shapeDecay, 0.75f, ModSource::Macro1, -0.5f, false, 1.0f), 0.25f, 1.0e-4f);

        // Bipolar presentation of a unipolar source: 0 -> -1, 0.5 -> 0, 1 -> +1.
        expectWithinAbsoluteError (effective (Param::shapeDecay, 0.5f, ModSource::Macro1, 0.4f, true, 1.0f), 0.9f, 1.0e-4f);
        expectWithinAbsoluteError (effective (Param::shapeDecay, 0.5f, ModSource::Macro1, 0.4f, true, 0.5f), 0.5f, 1.0e-4f);
        expectWithinAbsoluteError (effective (Param::shapeDecay, 0.5f, ModSource::Macro1, 0.4f, true, 0.0f), 0.1f, 1.0e-4f);

        // Clamping: never leaves the descriptor's range.
        expectWithinAbsoluteError (effective (Param::shapeDecay, 0.9f, ModSource::Macro1, 1.0f, false, 1.0f), 1.0f, 1.0e-5f);
        expectWithinAbsoluteError (effective (Param::shapeDecay, 0.1f, ModSource::Macro1, -1.0f, false, 1.0f), 0.0f, 1.0e-5f);

        // A wide, offset range: master.gain spans -60 … +12 dB (72 dB).
        expectWithinAbsoluteError (effective (Param::masterGain, -24.0f, ModSource::Macro1, 0.25f, false, 1.0f), -6.0f, 1.0e-3f);
        expectWithinAbsoluteError (effective (Param::masterGain, 0.0f, ModSource::Macro1, 0.5f, false, 1.0f), 12.0f, 1.0e-3f);

        // A bipolar range: space.shift spans -1 … 1, so depth 0.5 with a full unipolar source adds 1.0.
        expectWithinAbsoluteError (effective (Param::spaceShiftAmount, -0.5f, ModSource::Macro1, 0.5f, false, 1.0f), 0.5f, 1.0e-4f);

        // Curve bends the source: a convex curve at half depth gives less than linear.
        {
            const float linear = effective (Param::shapeDecay, 0.0f, ModSource::Macro1, 1.0f, false, 0.5f, 0.0f);
            const float convex = effective (Param::shapeDecay, 0.0f, ModSource::Macro1, 1.0f, false, 0.5f, 1.0f);
            const float concave = effective (Param::shapeDecay, 0.0f, ModSource::Macro1, 1.0f, false, 0.5f, -1.0f);
            expectWithinAbsoluteError (linear, 0.5f, 1.0e-4f);
            expect (convex < linear - 0.1f, "convex " + juce::String (convex));
            expect (concave > linear + 0.1f, "concave " + juce::String (concave));
        }

        // Several routings on one target sum.
        {
            ParamValues params = defaultParams();
            setParam (params, Param::shapeDecay, 0.2f);
            setParam (params, Param::macro1, 1.0f);
            setParam (params, Param::macro2, 1.0f);

            ModulationEngine engine;
            engine.prepare (sr, 512);
            auto table = std::make_unique<ModRoutingTable>();
            table->add ({ ModSource::Macro1, Param::shapeDecay, 0.2f, 0.0f, false, true });
            table->add ({ ModSource::Macro2, Param::shapeDecay, 0.3f, 0.0f, false, true });
            engine.publishRoutings (std::move (table));

            ControlGraph graph;
            graph.prepare (sr, 512);
            graph.resetTo (params);
            engine.beginBlock (params);
            TransportInfo transport;
            for (int i = 0; i < 8; ++i) { engine.process (graph, 64, transport); graph.update (params, 64); }
            expectWithinAbsoluteError (graph.value (Param::shapeDecay), 0.7f, 1.0e-4f);
        }

        // A disabled routing contributes nothing.
        {
            ParamValues params = defaultParams();
            setParam (params, Param::shapeDecay, 0.2f);
            setParam (params, Param::macro1, 1.0f);

            ModulationEngine engine;
            engine.prepare (sr, 512);
            auto table = std::make_unique<ModRoutingTable>();
            const int i = table->add ({ ModSource::Macro1, Param::shapeDecay, 0.5f, 0.0f, false, true });
            table->setEnabled (i, false);
            engine.publishRoutings (std::move (table));

            ControlGraph graph;
            graph.prepare (sr, 512);
            graph.resetTo (params);
            engine.beginBlock (params);
            TransportInfo transport;
            for (int k = 0; k < 8; ++k) { engine.process (graph, 64, transport); graph.update (params, 64); }
            expectWithinAbsoluteError (graph.value (Param::shapeDecay), 0.2f, 1.0e-4f);
            expect (! engine.isActive(), "a table with no enabled routings must stay idle");
        }
    }

    //==========================================================================
    void testPerVoiceIndependence()
    {
        beginTest ("Per-voice sources give each voice its own effective parameters");

        constexpr double sr = 48000.0;

        // The mechanism, straight through VoiceModulator.
        {
            ParamValues global = defaultParams();
            setParam (global, Param::shapeDecay, 0.2f);

            ModulationEngine engine;
            engine.prepare (sr, 512);
            auto table = std::make_unique<ModRoutingTable>();
            table->add ({ ModSource::Velocity, Param::shapeDecay, 0.8f, 0.0f, false, true });
            engine.publishRoutings (std::move (table));
            engine.beginBlock (global);
            expectEquals (engine.modPlan().numPoly, 1);
            expectEquals (engine.modPlan().numMono, 0);

            TransportInfo transport;
            auto voiceValue = [&] (float velocity)
            {
                NoteState note;
                note.velocity = velocity;
                note.gate = true;
                VoiceModulator mod;
                mod.prepare (sr);
                mod.noteOn (note, 1);
                const auto* p = mod.process (global, &engine.modPlan(), 64, sr, note, transport);
                return paramValue (*p, Param::shapeDecay);
            };

            const float quiet = voiceValue (0.2f), loud = voiceValue (1.0f);
            expectWithinAbsoluteError (quiet, 0.2f + 0.8f * 0.2f, 1.0e-5f);
            expectWithinAbsoluteError (loud, 1.0f, 1.0e-5f);   // 0.2 + 0.8 clamps at 1
            expect (loud > quiet + 0.5f, "voices did not diverge");
        }

        // The same thing through the real engine and two live voices.
        {
            SynthEngine engine;
            engine.prepare (sr, 128);
            ParamValues params = defaultParams();
            setParam (params, Param::shapeDecay, 0.2f);
            setParam (params, Param::ampAttack, 0.5f);
            engine.control().resetTo (params);

            auto table = std::make_unique<ModRoutingTable>();
            table->add ({ ModSource::Velocity, Param::shapeDecay, 0.8f, 0.0f, false, true });
            engine.modulationEngine().publishRoutings (std::move (table));

            juce::AudioBuffer<float> buffer (2, 128);
            TransportInfo transport;
            for (int b = 0; b < 40; ++b)
            {
                juce::MidiBuffer midi;
                if (b == 0)
                {
                    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.2f), 0);
                    midi.addEvent (juce::MidiMessage::noteOn (1, 67, 1.0f), 8);
                }
                buffer.clear();
                engine.process (buffer, midi, params, transport);
            }

            float quiet = -1.0f, loud = -1.0f;
            for (int v = 0; v < engine.voiceManager().getMaxVoices(); ++v)
            {
                const auto& voice = engine.voiceManager().voice (v);
                if (! voice.isActive()) continue;
                const float decay = paramValue (voice.modulator().parameters(), Param::shapeDecay);
                if (voice.midiNote() == 60) quiet = decay;
                if (voice.midiNote() == 67) loud = decay;
            }
            expect (quiet >= 0.0f && loud >= 0.0f, "both voices should still be alive");
            expectWithinAbsoluteError (quiet, 0.36f, 0.01f, "velocity 0.2 -> " + juce::String (quiet));
            expectWithinAbsoluteError (loud, 1.0f, 0.01f, "velocity 1.0 -> " + juce::String (loud));
            logMessage ("per-voice shape.decay: vel 0.2 -> " + juce::String (quiet, 3) + ", vel 1.0 -> " + juce::String (loud, 3));
        }

        // Key tracking is centred on C3 and note random differs per note.
        {
            ParamValues global = defaultParams();
            ModulationEngine engine;
            engine.prepare (sr, 512);
            auto table = std::make_unique<ModRoutingTable>();
            table->add ({ ModSource::KeyTrack, Param::shapeForm, 1.0f, 0.0f, true, true });
            table->add ({ ModSource::NoteRandom, Param::shapeMass, 1.0f, 0.0f, true, true });
            engine.publishRoutings (std::move (table));
            engine.beginBlock (global);

            TransportInfo transport;
            auto forNote = [&] (int midiNote, uint32_t id)
            {
                NoteState note; note.midiNote = midiNote; note.gate = true; note.velocity = 1.0f;
                VoiceModulator mod;
                mod.prepare (sr);
                mod.noteOn (note, id);
                mod.process (global, &engine.modPlan(), 64, sr, note, transport);
                return std::make_pair (mod.value (ModSource::KeyTrack), mod.value (ModSource::NoteRandom));
            };
            expectWithinAbsoluteError (forNote (60, 1).first, 0.0f, 1.0e-6f, "C3 must be the key-track centre");
            expect (forNote (72, 2).first > 0.15f);
            expect (forNote (48, 3).first < -0.15f);
            expectWithinAbsoluteError (forNote (127, 4).first, 1.0f, 1.0e-6f, "key track clamps at the top");

            std::set<float> randoms;
            for (uint32_t i = 0; i < 24; ++i) randoms.insert (forNote (60 + (int) i, i).second);
            expect ((int) randoms.size() > 18, "per-note random is not varied enough");
        }

        // An LFO with retrigger off is mono; with retrigger on it is per voice.
        {
            ParamValues params = defaultParams();
            ModulationEngine engine;
            engine.prepare (sr, 512);
            auto table = std::make_unique<ModRoutingTable>();
            table->add ({ ModSource::LFO1, Param::shapeForm, 0.5f, 0.0f, true, true });
            engine.publishRoutings (std::move (table));

            setParam (params, Param::lfo1Retrig, 0.0f);
            engine.beginBlock (params);
            expectEquals (engine.modPlan().numMono, 1, "free-running LFOs are mono");
            expectEquals (engine.modPlan().numPoly, 0);

            setParam (params, Param::lfo1Retrig, 1.0f);
            engine.beginBlock (params);
            expectEquals (engine.modPlan().numPoly, 1, "retriggered LFOs are per voice");
            expectEquals (engine.modPlan().numMono, 0);
        }
    }

    //==========================================================================
    void testEngineIntegration()
    {
        beginTest ("Routings reach the engine, move the sound and publish a snapshot");

        constexpr double sr = 48000.0;
        constexpr int block = 128;

        SynthEngine engine;
        engine.prepare (sr, block);
        ParamValues params = defaultParams();
        setParam (params, Param::macro1, 1.0f);
        setParam (params, Param::shapeDecay, 0.2f);
        engine.control().resetTo (params);

        auto table = std::make_unique<ModRoutingTable>();
        table->add ({ ModSource::Macro1, Param::shapeDecay, 0.6f, 0.0f, false, true });
        table->add ({ ModSource::Env1, Param::shapeForm, 0.5f, 0.0f, false, true });
        engine.modulationEngine().publishRoutings (std::move (table));

        renderEngine (engine, params, sr, block, 0.5, { 60 }, 0.9f, -1.0);

        expectWithinAbsoluteError (engine.control().value (Param::shapeDecay), 0.8f, 1.0e-3f,
                                   "the mono routing must land in the effective values");

        ModulationSnapshot snapshot;
        expect (engine.diagnostics().modulationSnapshots.read (snapshot), "no modulation snapshot was published");
        expectEquals (snapshot.numRoutings, 2);
        expectEquals (snapshot.numEnabled, 2);
        expect (snapshot.isModulated (Param::shapeDecay));
        expect (snapshot.isModulated (Param::shapeForm));
        expect (! snapshot.isModulated (Param::spaceMix));
        expectWithinAbsoluteError (snapshot.modulation[paramIndex (Param::shapeDecay)], 0.6f, 1.0e-3f);
        expectWithinAbsoluteError (snapshot.valueOf (ModSource::Macro1), 1.0f, 1.0e-5f);
        expect (snapshot.modMax[paramIndex (Param::shapeDecay)] >= snapshot.modMin[paramIndex (Param::shapeDecay)]);

        // An LFO must actually make the modulation move.
        {
            SynthEngine e2;
            e2.prepare (sr, block);
            ParamValues p2 = defaultParams();
            setParam (p2, Param::lfo1Rate, 4.0f);
            setParam (p2, Param::lfo1Retrig, 0.0f);
            setParam (p2, Param::shapeForm, 0.5f);
            e2.control().resetTo (p2);

            auto t2 = std::make_unique<ModRoutingTable>();
            t2->add ({ ModSource::LFO1, Param::shapeForm, 0.4f, 0.0f, true, true });
            e2.modulationEngine().publishRoutings (std::move (t2));

            juce::AudioBuffer<float> buffer (2, block);
            TransportInfo transport;
            float lo = 10.0f, hi = -10.0f;
            for (int b = 0; b < 200; ++b)
            {
                juce::MidiBuffer midi;
                if (b == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
                buffer.clear();
                e2.process (buffer, midi, p2, transport);
                const float v = e2.control().value (Param::shapeForm);
                lo = juce::jmin (lo, v); hi = juce::jmax (hi, v);
            }
            expect (hi - lo > 0.6f, "LFO swing on shape.form was only " + juce::String (hi - lo));
            expect (lo >= 0.0f && hi <= 1.0f, "modulation left the parameter's range");
        }

        // Nothing routed: the engine stays on full-block control updates.
        {
            SynthEngine idle;
            idle.prepare (sr, block);
            expect (! idle.modulationEngine().isActive());
            expectEquals (idle.modulationEngine().controlBlockSize (block), block);
        }
    }

    //==========================================================================
    void testNoAudioThreadAllocation()
    {
        beginTest ("Nothing allocates on the audio thread after prepare");

        constexpr double sr = 48000.0;
        constexpr int block = 256;

        SynthEngine engine;
        engine.prepare (sr, block);
        ParamValues params = defaultParams();
        engine.control().resetTo (params);

        // Message thread: publish a table (this allocates, deliberately).
        auto table = std::make_unique<ModRoutingTable>(buildStressTable (0.4f));
        engine.modulationEngine().publishRoutings (std::move (table));

        // Warm every code path up first: voices, sources, matter, fracture, space.
        renderEngine (engine, params, sr, block, 0.5, { 60, 64, 67 }, 0.9f, 0.3);

        // A second table is queued so the audio thread performs a handoff swap while counted.
        engine.modulationEngine().publishRoutings (std::make_unique<ModRoutingTable> (buildStressTable (0.8f)));

        // Pre-build every MIDI buffer: juce::MidiBuffer itself allocates.
        std::vector<juce::MidiBuffer> midi (60);
        midi[0].addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
        midi[1].addEvent (juce::MidiMessage::noteOn (1, 67, 0.5f), 10);
        midi[30].addEvent (juce::MidiMessage::noteOff (1, 60), 0);
        juce::AudioBuffer<float> buffer (2, block);
        TransportInfo transport;
        transport.isPlaying = true;

        int allocations = 0;
        {
            amtest::AllocationScope scope;
            for (size_t b = 0; b < midi.size(); ++b)
            {
                buffer.clear();
                engine.process (buffer, midi[b], params, transport);
                transport.ppqPosition += (double) block / sr * transport.bpm / 60.0;
            }
            allocations = scope.count();
        }
        expectEquals (allocations, 0, "the audio thread allocated " + juce::String (allocations) + " times");

        // The garbage the handoff parked is freed on the message thread.
        engine.messageThreadMaintenance();
    }

    //==========================================================================
    void testNaNSafety()
    {
        beginTest ("Everything routed everywhere at extreme depths stays finite");

        constexpr double sr = 48000.0;

        for (int blockSize : { 32, 128, 1024 })
        {
            SynthEngine engine;
            engine.prepare (sr, blockSize);

            ParamValues params = defaultParams();
            setParam (params, Param::fractureOn, 1.0f);
            setParam (params, Param::sourceMode, 1.0f);        // LAYER: every source at once
            setParam (params, Param::spaceReverbOn, 1.0f);
            setParam (params, Param::spaceDelayOn, 1.0f);
            setParam (params, Param::spaceDistOn, 1.0f);
            for (int i = 0; i < kNumLFOs; ++i)
            {
                setParam (params, i == 0 ? Param::lfo1Rate : i == 1 ? Param::lfo2Rate : i == 2 ? Param::lfo3Rate : Param::lfo4Rate, 37.0f);
                setParam (params, i == 0 ? Param::lfo1Depth : i == 1 ? Param::lfo2Depth : i == 2 ? Param::lfo3Depth : Param::lfo4Depth, 1.0f);
            }
            for (int i = 0; i < kNumChaos; ++i)
            {
                setParam (params, i == 0 ? Param::chaos1Rate : i == 1 ? Param::chaos2Rate : i == 2 ? Param::chaos3Rate : Param::chaos4Rate, 31.0f);
                setParam (params, i == 0 ? Param::chaos1Depth : i == 1 ? Param::chaos2Depth : i == 2 ? Param::chaos3Depth : Param::chaos4Depth, 1.0f);
                setParam (params, i == 0 ? Param::chaos1Stability : i == 1 ? Param::chaos2Stability : i == 2 ? Param::chaos3Stability : Param::chaos4Stability, 0.0f);
            }
            for (int i = 1; i <= kNumMacros; ++i)
                setParam (params, (Param) ((int) Param::macro1 + i - 1), 1.0f);
            engine.control().resetTo (params);

            engine.modulationEngine().publishRoutings (std::make_unique<ModRoutingTable> (buildStressTable (1.0f)));

            const auto result = renderEngine (engine, params, sr, blockSize, 3.0, { 48, 55, 60, 67, 72 }, 1.0f, 2.0);
            expectEquals (result.nonFinite, 0, "non-finite samples at block " + juce::String (blockSize));
            expect (result.peak <= 1.05f, "peak " + juce::String (result.peak) + " at block " + juce::String (blockSize));

            const auto safety = engine.diagnostics().safety.snapshot();
            expectEquals ((int) safety.counts[(int) SafetyEvent::NaN], 0, "master saw NaN samples");
            expectEquals ((int) safety.counts[(int) SafetyEvent::Infinity], 0, "master saw infinite samples");

            // Every effective value must still be inside its declared range.
            const auto& effective = engine.control().values();
            for (const auto& d : ParameterRegistry::all())
            {
                const float v = effective[(size_t) paramIndex (d.param)];
                expect (std::isfinite (v), juce::String (d.id) + " is not finite");
                expect (v >= d.min - 1.0e-3f && v <= d.max + 1.0e-3f, juce::String (d.id) + " = " + juce::String (v));
            }
            logMessage ("stress @ block " + juce::String (blockSize) + ": peak " + juce::String (result.peak, 4)
                        + ", realtime x" + juce::String (1.0 / juce::jmax (1.0e-6, result.realtimeRatio), 1));
        }
    }

    //==========================================================================
    void testCpuBudget()
    {
        beginTest ("64 voices with 64 routings stay well inside the CPU budget");

        constexpr double sr = 48000.0;
        constexpr int block = 512;      // a realistic host block, so the control slicing is measured

        // depth < 0 means "no routings at all"; depth 0 keeps the whole machinery running
        // (64 routings, control-rate slicing, per-voice sources) while leaving the parameter
        // values — and therefore the DSP workload — identical to the unmodulated run.
        auto measure = [sr] (int voiceChoice, int numNotes, float depth)
        {
            SynthEngine engine;
            engine.prepare (sr, block);
            ParamValues params = defaultParams();
            setParam (params, Param::masterVoices, (float) voiceChoice);
            setParam (params, Param::ampRelease, 8.0f);
            engine.control().resetTo (params);
            if (depth >= 0.0f) engine.modulationEngine().publishRoutings (std::make_unique<ModRoutingTable> (buildStressTable (depth)));

            std::vector<int> notes;
            for (int i = 0; i < numNotes; ++i) notes.push_back (24 + i);

            // Warm up (voice allocation, wavetables, FFT plans) before timing.
            renderEngine (engine, params, sr, block, 0.5, notes, 0.8f, -1.0);
            const auto result = renderEngine (engine, params, sr, block, 4.0, notes, 0.8f, -1.0);
            return std::make_pair (result, engine.activeVoices());
        };

        const auto full = measure (3, 64, 0.5f);
        const auto idle = measure (3, 64, 0.0f);
        const auto plain = measure (3, 64, -1.0f);
        const auto sixteen = measure (1, 16, 0.5f);

        expectEquals (full.first.nonFinite, 0);
        expectEquals (full.second, 64, "all 64 voices should be sounding");

        const double percent = full.first.realtimeRatio * 100.0;
        const double plainPercent = plain.first.realtimeRatio * 100.0;
        logMessage ("CPU @ " + juce::String (sr / 1000.0, 1) + " kHz / " + juce::String (block) + " samples:");
        logMessage ("   64 voices + 64 routings = " + juce::String (percent, 1) + " % of realtime  (x"
                    + juce::String (1.0 / juce::jmax (1.0e-6, full.first.realtimeRatio), 2) + " faster than realtime)");
        logMessage ("   64 voices + 64 routings at depth 0 = " + juce::String (idle.first.realtimeRatio * 100.0, 1)
                    + " % of realtime  -> the machinery itself costs "
                    + juce::String ((idle.first.realtimeRatio - plain.first.realtimeRatio) * 100.0, 1) + " points");
        logMessage ("   64 voices, no routings  = " + juce::String (plainPercent, 1) + " % of realtime  (the rest of the "
                    + juce::String (percent - plainPercent, 1) + " point difference is the DSP the modulation switches on)");
        logMessage ("   16 voices + 64 routings = " + juce::String (sixteen.first.realtimeRatio * 100.0, 1) + " % of realtime");

        expect (full.first.realtimeRatio < 0.75, "64 voices with 64 routings used " + juce::String (percent, 1) + " % of realtime");
        expect (sixteen.first.realtimeRatio < 0.25, "16 voices with 64 routings used "
                                                    + juce::String (sixteen.first.realtimeRatio * 100.0, 1) + " % of realtime");
    }
};

static ModTests modTests;

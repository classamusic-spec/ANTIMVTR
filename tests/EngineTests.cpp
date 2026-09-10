#include <juce_core/juce_core.h>
#include "dsp/SynthEngine.h"

using namespace am;

namespace
{
    struct Rendered
    {
        juce::AudioBuffer<float> audio;
        float peak = 0.0f;
        bool finite = true;
    };

    /** Renders a note (on at 0, off at `hold` seconds) for `seconds` total. */
    Rendered renderNote (SynthEngine& engine, double sr, int block, double seconds, double hold, int note,
                         const ParamValues& params, float velocity = 0.8f, std::function<void (juce::MidiBuffer&, int)> extraMidi = nullptr)
    {
        const int total = (int) (seconds * sr);
        Rendered r;
        r.audio.setSize (2, total);
        r.audio.clear();
        const int offSample = (int) (hold * sr);
        TransportInfo transport;
        for (int pos = 0; pos < total; pos += block)
        {
            const int n = juce::jmin (block, total - pos);
            juce::MidiBuffer midi;
            if (pos == 0) midi.addEvent (juce::MidiMessage::noteOn (1, note, velocity), 0);
            if (offSample >= pos && offSample < pos + n) midi.addEvent (juce::MidiMessage::noteOff (1, note), offSample - pos);
            if (extraMidi) extraMidi (midi, pos);
            juce::AudioBuffer<float> chunk (r.audio.getArrayOfWritePointers(), 2, pos, n);
            engine.process (chunk, midi, params, transport);
        }
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < total; ++i)
            {
                const float v = r.audio.getSample (ch, i);
                if (! std::isfinite (v)) r.finite = false;
                else r.peak = juce::jmax (r.peak, std::abs (v));
            }
        return r;
    }

    float rmsOfRange (const juce::AudioBuffer<float>& b, int start, int end)
    {
        double s = 0.0; int n = 0;
        for (int i = juce::jmax (0, start); i < juce::jmin (end, b.getNumSamples()); ++i) { const float v = b.getSample (0, i); s += (double) v * v; ++n; }
        return n > 0 ? (float) std::sqrt (s / n) : 0.0f;
    }

    int countZeroCrossings (const juce::AudioBuffer<float>& b, int start, int end)
    {
        int c = 0;
        for (int i = juce::jmax (1, start); i < juce::jmin (end, b.getNumSamples()); ++i)
            if ((b.getSample (0, i - 1) < 0.0f) != (b.getSample (0, i) < 0.0f)) ++c;
        return c;
    }
}

class EngineTests : public juce::UnitTest
{
public:
    EngineTests() : juce::UnitTest ("Synth engine", "engine") {}

    void runTest() override
    {
        auto params = ParameterRegistry::defaults();
        params[(size_t) paramIndex (Param::ampAttack)] = 0.001f;
        params[(size_t) paramIndex (Param::ampRelease)] = 0.05f;
        params[(size_t) paramIndex (Param::spaceMix)] = 0.0f;     // these tests measure the voice envelope, not the Space tail
        params[(size_t) paramIndex (Param::shapeMix)] = 0.0f;     // ...nor Matter's ring-out (covered by MatterTests)

        beginTest ("Note produces sound and releases to silence at every sample rate and block size");
        {
            for (double sr : { 44100.0, 48000.0, 88200.0, 96000.0 })
            {
                for (int block : { 32, 64, 128, 256, 512, 1024 })
                {
                    SynthEngine engine;
                    engine.prepare (sr, block);
                    engine.control().resetTo (params);
                    auto r = renderNote (engine, sr, block, 1.0, 0.5, 60, params);
                    const juce::String tag = juce::String (sr) + "/" + juce::String (block);
                    expect (r.finite, "non-finite output at " + tag);
                    const float sustain = rmsOfRange (r.audio, (int) (0.2 * sr), (int) (0.45 * sr));
                    expect (sustain > 0.05f, "too quiet during sustain at " + tag + " rms=" + juce::String (sustain));
                    const float tail = rmsOfRange (r.audio, (int) (0.9 * sr), (int) sr);
                    expect (tail < 1.0e-4f, "did not release at " + tag + " rms=" + juce::String (tail));
                    expect (r.peak <= 1.0f, "peak above ceiling at " + tag);
                    expectEquals ((int) engine.diagnostics().safety.total(), 0, "safety events at " + tag);
                    expectEquals (engine.activeVoices(), 0);
                }
            }
        }

        beginTest ("Oscillator pitch accuracy (A4 = 440 Hz)");
        {
            SynthEngine engine;
            engine.prepare (48000.0, 128);
            engine.control().resetTo (params);
            auto r = renderNote (engine, 48000.0, 128, 1.2, 1.1, 69, params);
            const int crossings = countZeroCrossings (r.audio, 4800, 4800 + 48000);
            expectWithinAbsoluteError ((double) crossings / 2.0, 440.0, 2.0);
        }

        beginTest ("Sample-accurate note timing");
        {
            SynthEngine engine;
            engine.prepare (48000.0, 512);
            engine.control().resetTo (params);
            juce::AudioBuffer<float> buf (2, 512);
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 300);
            TransportInfo t;
            engine.process (buf, midi, params, t);
            expect (rmsOfRange (buf, 0, 290) < 1.0e-6f, "audio before the note-on offset");
            expect (rmsOfRange (buf, 320, 512) > 0.005f, "no audio after the note-on offset");
        }

        beginTest ("Polyphony: 16-note chord stays finite and limited; stealing beyond capacity");
        {
            SynthEngine engine;
            engine.prepare (48000.0, 128);
            engine.control().resetTo (params);
            auto r = renderNote (engine, 48000.0, 128, 0.6, 0.3, 48, params, 0.9f, [] (juce::MidiBuffer& m, int pos)
            {
                if (pos == 0) for (int i = 1; i < 16; ++i) m.addEvent (juce::MidiMessage::noteOn (1, 48 + i * 2, 0.9f), 1);
            });
            expect (r.finite);
            expect (r.peak <= 1.0f);
            expect (rmsOfRange (r.audio, 4800, 9600) > 0.05f);

            // Flood: 200 note-ons into 16 voices must not crash and must steal.
            auto flood = params;
            flood[(size_t) paramIndex (Param::masterVoices)] = 1.0f; // 16
            auto r2 = renderNote (engine, 48000.0, 128, 0.5, 0.4, 36, flood, 0.9f, [] (juce::MidiBuffer& m, int pos)
            {
                if (pos < 128 * 10) for (int i = 0; i < 20; ++i) m.addEvent (juce::MidiMessage::noteOn (1, 40 + (pos / 128) + i, 0.7f), i);
            });
            expect (r2.finite);
            expect (engine.diagnostics().safety.count (SafetyEvent::NaN) == 0);
            expect (engine.voiceManager().getMaxVoices() == 16);
        }

        beginTest ("Mono / legato use a single voice; glide moves pitch");
        {
            auto mono = params;
            mono[(size_t) paramIndex (Param::masterMode)] = 2.0f;   // LEGATO
            mono[(size_t) paramIndex (Param::masterGlide)] = 0.2f;
            SynthEngine engine;
            engine.prepare (48000.0, 128);
            engine.control().resetTo (mono);
            int maxActive = 0;
            auto r = renderNote (engine, 48000.0, 128, 1.0, 0.9, 48, mono, 0.8f, [&] (juce::MidiBuffer& m, int pos)
            {
                if (pos == 128 * 20) m.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
                if (pos == 128 * 200) m.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
                maxActive = juce::jmax (maxActive, engine.activeVoices());
            });
            expect (r.finite);
            expectEquals (maxActive, 1);
            // right after the second note-on the pitch is still near 48 (130 Hz); 0.2 s later it has glided toward 60 (261 Hz)
            const int early = countZeroCrossings (r.audio, 128 * 20, 128 * 20 + 2400);
            const int late  = countZeroCrossings (r.audio, 128 * 90, 128 * 90 + 2400);
            expect (late > early + 8, "glide did not raise pitch: " + juce::String (early) + " -> " + juce::String (late));
            // after releasing 60 the held 48 returns
            const int back = countZeroCrossings (r.audio, 128 * 300, 128 * 300 + 2400);
            expect (back < late - 8, "legato did not return to the held note: " + juce::String (late) + " -> " + juce::String (back));
        }

        beginTest ("Sustain pedal holds released notes");
        {
            SynthEngine engine;
            engine.prepare (48000.0, 128);
            engine.control().resetTo (params);
            auto r = renderNote (engine, 48000.0, 128, 1.0, 0.2, 60, params, 0.8f, [] (juce::MidiBuffer& m, int pos)
            {
                if (pos == 0) m.addEvent (juce::MidiMessage::controllerEvent (1, 64, 127), 0);
                if (pos == 128 * 250) m.addEvent (juce::MidiMessage::controllerEvent (1, 64, 0), 0);
            });
            expect (rmsOfRange (r.audio, (int) (0.4 * 48000), (int) (0.6 * 48000)) > 0.05f, "pedal did not sustain");
            expect (rmsOfRange (r.audio, (int) (0.95 * 48000), 48000) < 1.0e-4f, "pedal release did not stop the note");
        }

        beginTest ("Pitch bend changes frequency");
        {
            SynthEngine engine;
            engine.prepare (48000.0, 128);
            engine.control().resetTo (params);
            auto r = renderNote (engine, 48000.0, 128, 2.0, 1.9, 69, params, 0.8f, [] (juce::MidiBuffer& m, int pos)
            {
                if (pos == 128 * 250) m.addEvent (juce::MidiMessage::pitchWheel (1, 16383), 0); // +2 semitones
            });
            const int before = countZeroCrossings (r.audio, 4800, 4800 + 24000);
            const int after  = countZeroCrossings (r.audio, 128 * 260, 128 * 260 + 24000);
            expectWithinAbsoluteError ((double) after / (double) before, std::pow (2.0, 2.0 / 12.0), 0.01);
        }

        beginTest ("Large host blocks are chunked correctly");
        {
            SynthEngine engine;
            engine.prepare (48000.0, 8192);
            engine.control().resetTo (params);
            auto r = renderNote (engine, 48000.0, 8192, 1.0, 0.5, 60, params);
            expect (r.finite);
            expect (rmsOfRange (r.audio, 9600, 19200) > 0.05f);
        }

        beginTest ("Master gain and dry modes");
        {
            auto quiet = params;
            quiet[(size_t) paramIndex (Param::masterGain)] = -20.0f;
            SynthEngine engine;
            engine.prepare (48000.0, 128);
            engine.control().resetTo (quiet);
            auto r = renderNote (engine, 48000.0, 128, 0.5, 0.4, 60, quiet);
            SynthEngine loud;
            loud.prepare (48000.0, 128);
            loud.control().resetTo (params);
            auto r2 = renderNote (loud, 48000.0, 128, 0.5, 0.4, 60, params);
            const float ratio = rmsOfRange (r.audio, 4800, 9600) / rmsOfRange (r2.audio, 4800, 9600);
            expectWithinAbsoluteError (ratio, 0.1f, 0.01f);

            loud.diagnostics().dev.dryMode.store ((int) DryMode::SourceOnly);
            auto r3 = renderNote (loud, 48000.0, 128, 0.5, 0.4, 60, params);
            expect (r3.finite && rmsOfRange (r3.audio, 4800, 9600) > 0.05f);
        }

        beginTest ("Diagnostics snapshots are published during processing");
        {
            SynthEngine engine;
            engine.prepare (48000.0, 128);
            engine.control().resetTo (params);
            renderNote (engine, 48000.0, 128, 0.3, 0.2, 60, params);
            VisualStateSnapshot vs;
            expect (engine.diagnostics().visualSnapshots.read (vs));
            expect (vs.sampleTime > 0);
            DiagnosticSnapshot ds;
            expect (engine.diagnostics().diagnosticSnapshots.read (ds));
            expectEquals (ds.blockSize, 128);
            expect (ds.perf.blocksMeasured > 0);
            expect (ds.stages[(int) Stage::Master].peak <= 1.0f);
            int events = 0;
            engine.diagnostics().events.drain ([&] (const EngineEvent&) { ++events; });
            expect (events >= 3, "expected prepare + voice start + voice end events");
        }
    }
};

static EngineTests engineTests;

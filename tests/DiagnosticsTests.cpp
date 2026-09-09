#include <juce_core/juce_core.h>
#include "dev/diagnostics/Diagnostics.h"

using namespace am;

class DiagnosticsTests : public juce::UnitTest
{
public:
    DiagnosticsTests() : juce::UnitTest ("Diagnostics", "diagnostics") {}

    void runTest() override
    {
        beginTest ("SafetyMonitor counts and logs the first occurrences");
        {
            Diagnostics d;
            for (int i = 0; i < 20; ++i) d.safety.note (SafetyEvent::NaN, Subsystem::Matter, 3);
            d.safety.note (SafetyEvent::HardClip, Subsystem::Master, -1, 5);
            expectEquals ((int) d.safety.count (SafetyEvent::NaN), 20);
            expectEquals ((int) d.safety.count (SafetyEvent::HardClip), 5);
            expectEquals ((int) d.safety.total(), 25);
            const auto snap = d.safety.snapshot();
            expectEquals ((int) snap.lastVoice[(int) SafetyEvent::NaN], 3);
            expectEquals ((int) snap.lastSubsystem[(int) SafetyEvent::NaN], (int) Subsystem::Matter);
            int logged = 0;
            d.events.drain ([&] (const EngineEvent& e) { if (e.type == EngineEventType::SafetyEvent) ++logged; });
            expect (logged > 0 && logged < 20, "expected rate-limited safety events, got " + juce::String (logged));
            d.safety.reset();
            expectEquals ((int) d.safety.total(), 0);
        }

        beginTest ("EngineEventQueue drops when full and reports it");
        {
            EngineEventQueue q;
            for (int i = 0; i < 3000; ++i) q.push (EngineEventType::Custom, Subsystem::Unknown, -1, (uint32_t) i, 0.0f, 0);
            expect (q.droppedCount() > 0);
            int drained = 0;
            q.drain ([&] (const EngineEvent&) { ++drained; });
            expectEquals ((int) q.pending(), 0);
            expect (drained == 2048);
            EngineEvent e; e.type = EngineEventType::VoiceStolen; e.a = 60; e.voice = 2;
            expect (EngineEventQueue::describe (e).contains ("stolen"));
        }

        beginTest ("PerformanceProfiler produces percentages");
        {
            PerformanceProfiler p;
            p.prepare (48000.0, 128);
            for (int i = 0; i < 50; ++i)
            {
                p.beginBlock (128);
                {
                    PerformanceProfiler::Scoped t (p, Subsystem::Matter);
                    volatile double x = 0.0; for (int k = 0; k < 2000; ++k) x = x + std::sin ((double) k);
                }
                p.endBlock();
            }
            const auto s = p.snapshot();
            expectEquals ((int) s.blocksMeasured, 50);
            expect (s.avgPercent[(int) Subsystem::Matter] > 0.0f);
            expect (s.totalPeakPercent >= s.totalAvgPercent);
            expectEquals (s.blockSize, 128);
        }
    }
};

static DiagnosticsTests diagnosticsTests;

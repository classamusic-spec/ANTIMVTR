#include <juce_core/juce_core.h>
#include "state/ParameterRegistry.h"
#include "state/MutationEngine.h"

using namespace am;

class ParameterTests : public juce::UnitTest
{
public:
    ParameterTests() : juce::UnitTest ("Parameter registry", "state") {}

    void runTest() override
    {
        beginTest ("Registry validates (unique IDs, defaults in range)");
        {
            const auto problems = ParameterRegistry::validate();
            expect (problems.isEmpty(), problems);
            expect (kNumParams > 150, "expected a full parameter set, got " + juce::String (kNumParams));
        }

        beginTest ("Every ID round-trips through fromID");
        {
            for (const auto& d : ParameterRegistry::all())
            {
                const auto p = ParameterRegistry::fromID (d.id);
                expect (p.has_value(), juce::String ("missing ") + d.id);
                if (p.has_value()) expect (*p == d.param);
                expect (juce::String (d.id).containsChar ('.'), juce::String ("ID should be dotted: ") + d.id);
                expect (juce::String (d.id).toLowerCase().substring (0, 1) == juce::String (d.id).substring (0, 1));
            }
            expect (! ParameterRegistry::fromID ("does.not.exist").has_value());
        }

        beginTest ("Normalised conversions round-trip");
        {
            for (const auto& d : ParameterRegistry::all())
            {
                for (float t : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
                {
                    const float v = d.fromNormalised (t);
                    expect (v >= d.min && v <= d.max);
                    const float back = d.toNormalised (v);
                    if (! d.isDiscrete())
                        expectWithinAbsoluteError (back, t, 1.0e-4f, juce::String (d.id));
                }
                expectEquals (d.fromNormalised (0.0f), d.min);
                expectEquals (d.fromNormalised (1.0f), d.max);
            }
        }

        beginTest ("Choices and formatting");
        {
            const auto& mode = ParameterRegistry::get (Param::masterMode);
            expectEquals (mode.numChoices(), 3);
            expectEquals (mode.formatValue (2.0f), juce::String ("LEGATO"));
            expectEquals (mode.max, 2.0f);
            const auto& gain = ParameterRegistry::get (Param::masterGain);
            expect (gain.formatValue (-6.0f).contains ("dB"));
            expectEquals (ParameterRegistry::get (Param::fractureOn).formatValue (1.0f), juce::String ("ON"));
            expectEquals (ParameterRegistry::get (Param::shapeMaterialA).numChoices(), 9);
            expectEquals (ParameterRegistry::get (Param::spaceType).numChoices(), 8);
        }

        beginTest ("Clamping handles NaN and discrete rounding");
        {
            const auto& d = ParameterRegistry::get (Param::waveUnison);
            expectEquals (d.clampValue (std::numeric_limits<float>::quiet_NaN()), d.defaultValue);
            expectEquals (d.clampValue (3.4f), 3.0f);
            expectEquals (d.clampValue (99.0f), d.max);
        }

        beginTest ("Mutation is deterministic, bounded and never touches None-category parameters");
        {
            auto base = ParameterRegistry::defaults();
            auto a = base, b = base;
            MutationEngine::mutate (a, MutationStrength::Evolve, 123);
            MutationEngine::mutate (b, MutationStrength::Evolve, 123);
            expect (a == b);
            auto c = base;
            MutationEngine::mutate (c, MutationStrength::Evolve, 124);
            expect (c != a);
            int changed = 0;
            for (const auto& d : ParameterRegistry::all())
            {
                const float v = a[(size_t) paramIndex (d.param)];
                expect (v >= d.min && v <= d.max, juce::String (d.id));
                if (d.mutation == MutationCategory::None) expectEquals (v, base[(size_t) paramIndex (d.param)]);
                else if (v != base[(size_t) paramIndex (d.param)]) ++changed;
            }
            expect (changed > 20, "mutation changed only " + juce::String (changed) + " parameters");
            expectEquals (a[(size_t) paramIndex (Param::masterGain)], 0.0f);

            for (int seed = 0; seed < 50; ++seed)
            {
                auto r = base;
                MutationEngine::randomize (r, (uint32_t) seed);
                for (const auto& d : ParameterRegistry::all())
                    expect (r[(size_t) paramIndex (d.param)] >= d.min && r[(size_t) paramIndex (d.param)] <= d.max);
                expect (paramValue (r, Param::fractureFeedback) <= 0.85f);
            }
        }
    }
};

static ParameterTests parameterTests;

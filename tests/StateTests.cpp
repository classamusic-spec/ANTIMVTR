#include <juce_core/juce_core.h>
#include "state/StateManager.h"
#include "presets/PresetManager.h"
#include "state/PatchMorph.h"

using namespace am;

class StateTests : public juce::UnitTest
{
public:
    StateTests() : juce::UnitTest ("State serialization", "state") {}

    void runTest() override
    {
        beginTest ("JSON round trip preserves parameters and metadata");
        {
            PatchState s;
            s.meta.name = "Test Patch"; s.meta.author = "QA"; s.meta.category = "PAD"; s.meta.tags = { "a", "b" };
            s.params[(size_t) paramIndex (Param::shapeForm)] = 0.731f;
            s.params[(size_t) paramIndex (Param::masterVoices)] = 3.0f;
            s.params[(size_t) paramIndex (Param::masterGain)] = -12.5f;
            s.params[(size_t) paramIndex (Param::fractureOn)] = 1.0f;
            auto* extra = new juce::DynamicObject(); extra->setProperty ("hello", 42);
            s.matter = juce::var (extra);

            const auto json = StateManager::toJson (s);
            PatchState back;
            juce::String warnings;
            expect (StateManager::fromJson (json, back, &warnings), warnings);
            expectEquals (back.meta.name, juce::String ("Test Patch"));
            expectEquals (back.meta.author, juce::String ("QA"));
            expectEquals (back.meta.tags.size(), 2);
            expectWithinAbsoluteError (paramValue (back.params, Param::shapeForm), 0.731f, 1.0e-5f);
            expectEquals (paramValue (back.params, Param::masterVoices), 3.0f);
            expectWithinAbsoluteError (paramValue (back.params, Param::masterGain), -12.5f, 1.0e-5f);
            expectEquals (paramValue (back.params, Param::fractureOn), 1.0f);
            expectEquals ((int) back.matter.getDynamicObject()->getProperty ("hello"), 42);
            for (const auto& d : ParameterRegistry::all())
                expectWithinAbsoluteError (back.params[(size_t) paramIndex (d.param)], s.params[(size_t) paramIndex (d.param)], 1.0e-5f, juce::String (d.id));
        }

        beginTest ("Missing parameters fall back to defaults, unknown ones warn, out-of-range clamps");
        {
            const juce::String json = R"({"format":"ANTI-MATR","meta":{"schema":1,"name":"Partial"},"params":{"shape.density":0.9,"bogus.param":1,"shape.mass":7.0}})";
            PatchState s;
            juce::String warnings;
            expect (StateManager::fromJson (json, s, &warnings));
            expect (warnings.contains ("bogus.param"));
            expectWithinAbsoluteError (paramValue (s.params, Param::shapeDensity), 0.9f, 1.0e-6f);
            expectEquals (paramValue (s.params, Param::shapeMass), 1.0f);
            expectEquals (paramValue (s.params, Param::shapeForm), ParameterRegistry::get (Param::shapeForm).defaultValue);
        }

        beginTest ("Rejects foreign documents");
        {
            PatchState s;
            expect (! StateManager::fromJson ("{\"format\":\"other\"}", s));
            expect (! StateManager::fromJson ("not json", s));
            expect (! StateManager::fromBinary (nullptr, 0, s));
        }

        beginTest ("Binary round trip");
        {
            PatchState s;
            s.params[(size_t) paramIndex (Param::evolveMelt)] = 0.42f;
            const auto blob = StateManager::toBinary (s);
            PatchState back;
            expect (StateManager::fromBinary (blob.getData(), blob.getSize(), back));
            expectWithinAbsoluteError (paramValue (back.params, Param::evolveMelt), 0.42f, 1.0e-6f);
        }

        beginTest ("Older schema migrates forward");
        {
            const juce::String json = R"({"format":"ANTI-MATR","meta":{"schema":0,"name":"Old"},"params":{"shape.density":0.2}})";
            PatchState s;
            expect (StateManager::fromJson (json, s));
            expectEquals (s.meta.schemaVersion, PatchState::kSchemaVersion);
            expectWithinAbsoluteError (paramValue (s.params, Param::shapeDensity), 0.2f, 1.0e-6f);
        }

        beginTest ("Parameter ValueTree round trip");
        {
            auto values = ParameterRegistry::defaults();
            values[(size_t) paramIndex (Param::spaceMix)] = 0.77f;
            const auto tree = StateManager::toParameterTree (values, "PARAMETERS");
            expectEquals (tree.getNumChildren(), kNumParams);
            ParamValues back {};
            StateManager::fromParameterTree (tree, back);
            expectWithinAbsoluteError (paramValue (back, Param::spaceMix), 0.77f, 1.0e-6f);
        }

        beginTest ("Factory presets build and round-trip");
        {
            PresetManager pm;
            expect (pm.numFactoryPresets() >= 3);
            for (int i = 0; i < pm.numFactoryPresets(); ++i)
            {
                auto s = pm.buildFactory (i);
                expectEquals (s.meta.name, pm.factoryPreset (i).name);
                for (const auto& d : ParameterRegistry::all())
                    expect (s.params[(size_t) paramIndex (d.param)] >= d.min && s.params[(size_t) paramIndex (d.param)] <= d.max, s.meta.name + " " + d.id);
                PatchState back;
                expect (StateManager::fromJson (StateManager::toJson (s), back));
                expect (back.params == s.params, "round trip differs for " + s.meta.name);
            }
            expect (pm.findFactory ("void bloom") >= 0);
            expect (pm.findFactory ("nope") < 0);
        }

        beginTest ("A/B morph: ends are exact, floats move perceptually, choices snap, everything stays in range");
        {
            PatchState a = PresetManager::initPatch(), b = PresetManager::initPatch();
            a.meta.name = "Alpha"; b.meta.name = "Beta";
            auto& ra = a.params; auto& rb = b.params;
            ra[(size_t) paramIndex (Param::shapeDecay)] = 0.2f;   rb[(size_t) paramIndex (Param::shapeDecay)] = 0.8f;
            ra[(size_t) paramIndex (Param::ampRelease)] = 0.05f;  rb[(size_t) paramIndex (Param::ampRelease)] = 5.0f;
            ra[(size_t) paramIndex (Param::shapeMaterialA)] = 0;  rb[(size_t) paramIndex (Param::shapeMaterialA)] = 5;
            ra[(size_t) paramIndex (Param::waveOctave)] = -2;     rb[(size_t) paramIndex (Param::waveOctave)] = 2;
            ra[(size_t) paramIndex (Param::evolveFreeze)] = 0;    rb[(size_t) paramIndex (Param::evolveFreeze)] = 1;

            expect (PatchMorph::interpolate (ra, rb, 0.0f) == ra, "t = 0 must be A exactly");
            expect (PatchMorph::interpolate (ra, rb, 1.0f) == rb, "t = 1 must be B exactly");

            const auto mid = PatchMorph::interpolate (ra, rb, 0.5f);
            expectWithinAbsoluteError (mid[(size_t) paramIndex (Param::shapeDecay)], 0.5f, 1.0e-5f);
            const float rel = mid[(size_t) paramIndex (Param::ampRelease)];
            expect (rel > 0.05f && rel < 5.0f, "release must sit between the ends");
            expect (rel < 2.525f, "release must morph on the perceptual (skewed) curve, not linearly");
            expectEquals ((int) mid[(size_t) paramIndex (Param::shapeMaterialA)], 5, "choices snap to B at 0.5");
            expectEquals ((int) PatchMorph::interpolate (ra, rb, 0.49f)[(size_t) paramIndex (Param::shapeMaterialA)], 0, "choices stay A below 0.5");
            expectEquals ((int) mid[(size_t) paramIndex (Param::waveOctave)], 0, "integers round");
            expectEquals ((int) PatchMorph::interpolate (ra, rb, 0.3f)[(size_t) paramIndex (Param::evolveFreeze)], 0);

            for (float t : { -1.0f, 0.0f, 0.25f, 0.5f, 0.75f, 1.0f, 2.0f, std::numeric_limits<float>::quiet_NaN() })
            {
                const auto m = PatchMorph::interpolate (ra, rb, t);
                for (const auto& d : ParameterRegistry::all())
                {
                    const float v = m[(size_t) paramIndex (d.param)];
                    expect (std::isfinite (v) && v >= d.min && v <= d.max, juce::String (d.id) + " out of range at t = " + juce::String (t));
                }
            }

            const auto patch = PatchMorph::interpolate (a, b, 0.25f);
            expect (patch.meta.name.contains ("Alpha") && patch.meta.name.contains ("Beta") && patch.meta.name.contains ("25%"));
            expectEquals (PatchMorph::interpolate (a, b, 0.0f).meta.name, juce::String ("Alpha"));
            expectEquals (PatchMorph::interpolate (a, b, 1.0f).meta.name, juce::String ("Beta"));
        }
    }
};

static StateTests stateTests;

/*
    Geometry and maths behind the centre object (VISUAL_SPEC §5).

    These run in the console test runner, which links no graphics module, so
    everything under test is deliberately free of juce::Colour / juce::Rectangle:
    the field maths, the volumetric field and PortholeLayout are plain floats
    and Vec3 throughout.

    What is covered:
      * the colour ramp travels cyan → blue → violet → magenta → pink and stays
        in gamut however it is whitened or deepened,
      * the porthole layout is proportional and stays inside its component,
      * a seed reproduces the volumetric field exactly, frame after frame,
      * every output stays finite and bounded for every parameter extreme,
        including NaN and infinity arriving from a broken snapshot,
      * each engine input really moves what it claims to move: a node's
        frequency its radius, its energy its brightness, a note-on a front
        through the shells, a fragment step a shatter, a chord a mass per voice.
*/

#include <juce_core/juce_core.h>

#include "ui/visualizers/FieldMaths.h"
#include "ui/visualizers/NodeField.h"
#include "ui/visualizers/Porthole.h"

using namespace am;
using namespace am::ui;

namespace
{
    bool finite (const Vec3& v) noexcept
    {
        return std::isfinite (v.x) && std::isfinite (v.y) && std::isfinite (v.z);
    }
}

//==============================================================================
class LiquidRampTests : public juce::UnitTest
{
public:
    LiquidRampTests() : juce::UnitTest ("Ribbon colour ramp", "visualizer") {}

    void runTest() override
    {
        beginTest ("Every channel stays inside 0..1 for any input");
        for (float u = -4.0f; u <= 4.0f; u += 0.013f)
        {
            const auto c = liquid::ramp (u);
            expect (c.r >= 0.0f && c.r <= 1.0f, "red out of range at u = " + juce::String (u));
            expect (c.g >= 0.0f && c.g <= 1.0f, "green out of range at u = " + juce::String (u));
            expect (c.b >= 0.0f && c.b <= 1.0f, "blue out of range at u = " + juce::String (u));
        }

        beginTest ("Non-finite input degrades to the start of the ramp");
        for (float bad : { std::numeric_limits<float>::quiet_NaN(),
                           std::numeric_limits<float>::infinity(),
                           -std::numeric_limits<float>::infinity() })
        {
            const auto c = liquid::ramp (bad);
            expect (std::isfinite (c.r) && std::isfinite (c.g) && std::isfinite (c.b), "ramp produced a non-finite colour");
        }

        beginTest ("The ramp wraps without a seam");
        {
            const auto a = liquid::ramp (0.999f), b = liquid::ramp (0.0f);
            expect (std::abs (a.r - b.r) < 0.05f && std::abs (a.g - b.g) < 0.05f && std::abs (a.b - b.b) < 0.05f,
                    "there is a visible step where the ramp wraps");
            const auto c = liquid::ramp (1.37f), d = liquid::ramp (0.37f);
            expectWithinAbsoluteError (c.r, d.r, 1.0e-5f);
            expectWithinAbsoluteError (c.g, d.g, 1.0e-5f);
            expectWithinAbsoluteError (c.b, d.b, 1.0e-5f);
        }

        beginTest ("It really travels cyan into blue into violet into magenta into pink");
        {
            const auto cyan = liquid::ramp (0.00f);
            const auto blue = liquid::ramp (0.24f);
            const auto violet = liquid::ramp (0.48f);
            const auto magenta = liquid::ramp (0.74f);
            const auto pink = liquid::ramp (0.90f);

            expect (cyan.g > cyan.r && cyan.b > cyan.r, "the ramp does not start cyan");
            expect (blue.b > blue.g && blue.g > blue.r, "the second stop is not blue");
            expect (violet.b > violet.r && violet.r > violet.g, "the third stop is not violet");
            expect (magenta.r > magenta.b && magenta.b > magenta.g, "the fourth stop is not magenta");
            expect (pink.r > 0.9f && pink.g > magenta.g, "the fifth stop is not pink");

            // Red rises and green falls monotonically enough that the mass reads as a
            // travelling hue rather than an arbitrary shuffle of colours.
            expect (magenta.r > violet.r && violet.r > blue.r, "red does not rise along the ramp");
            expect (cyan.g > blue.g && blue.g > violet.g, "green does not fall along the ramp");
        }

        beginTest ("Whitening and deepening stay in gamut");
        for (float u = 0.0f; u < 1.0f; u += 0.05f)
        {
            for (float t : { 0.0f, 0.5f, 1.0f, -1.0f, 2.0f })
            {
                const auto w = liquid::whiten (liquid::ramp (u), t);
                const auto d = liquid::deepen (liquid::ramp (u), t);
                expect (w.r >= 0.0f && w.r <= 1.0f && w.g >= 0.0f && w.g <= 1.0f && w.b >= 0.0f && w.b <= 1.0f, "whiten left the gamut");
                expect (d.r >= 0.0f && d.r <= 1.0f && d.g >= 0.0f && d.g <= 1.0f && d.b >= 0.0f && d.b <= 1.0f, "deepen left the gamut");
            }
        }
    }
};

static LiquidRampTests liquidRampTests;




//==============================================================================
class PortholeLayoutTests : public juce::UnitTest
{
public:
    PortholeLayoutTests() : juce::UnitTest ("Porthole layout", "visualizer") {}

    void runTest() override
    {
        // The centre component's share of the reference design (MainView: 592 x 486
        // of 1600 x 810 page units), at the two sizes the interface is reviewed at.
        struct Size { int w, h; };
        const Size sizes[] = { { 592, 486 }, { 407, 334 }, { 296, 243 }, { 900, 700 }, { 1200, 400 } };

        beginTest ("Everything the porthole draws stays inside the component");
        for (const auto& s : sizes)
        {
            const auto l = PortholeLayout::forBounds (0.0f, 0.0f, (float) s.w, (float) s.h);
            const juce::String at = " at " + juce::String (s.w) + "x" + juce::String (s.h);
            expect (l.outerR > 0.0f && std::isfinite (l.outerR), "no bezel" + at);
            expect (l.centreX - l.outerR >= -0.5f, "the bezel runs off the left" + at);
            expect (l.centreX + l.outerR <= (float) s.w + 0.5f, "the bezel runs off the right" + at);
            expect (l.centreY - l.outerR >= -0.5f, "the bezel runs off the top" + at);
            expect (l.centreY + l.outerR <= (float) s.h + 0.5f, "the bezel runs off the bottom" + at);
            expect (l.glassR > 0.0f && l.glassR < l.outerR, "the glass is not inside the bezel" + at);
            expect (l.bezelWidth > 0.0f, "the bezel has no thickness" + at);
            expect (l.unit > 0.0f, "the reference unit collapsed" + at);
        }

        beginTest ("The layout is identical in proportion at 1600x1000 and 1100x690");
        {
            // The centre component scales with the editor, so the same fractions must fall out.
            const auto big = PortholeLayout::forBounds (0.0f, 0.0f, 592.0f, 486.0f);
            const float k = 1100.0f / 1600.0f;
            const auto small = PortholeLayout::forBounds (0.0f, 0.0f, 592.0f * k, 486.0f * k);
            expectWithinAbsoluteError (small.outerR / big.outerR, k, 0.005f);
            expectWithinAbsoluteError (small.glassR / big.glassR, k, 0.005f);
            expectWithinAbsoluteError (small.bezelWidth / big.bezelWidth, k, 0.005f);
            expectWithinAbsoluteError (small.unit / big.unit, k, 0.005f);
        }

        beginTest ("The bezel grows with the component and never inverts");
        {
            float previous = 0.0f;
            for (int w = 60; w <= 1600; w += 20)
            {
                const auto l = PortholeLayout::forBounds (0.0f, 0.0f, (float) w, (float) w * 0.82f);
                expect (l.outerR >= previous - 0.01f, "the bezel shrank as the component grew at width " + juce::String (w));
                expect (l.glassR < l.outerR, "the glass swallowed the bezel at width " + juce::String (w));
                previous = l.outerR;
            }
        }

        beginTest ("An offset component moves the whole porthole with it");
        {
            const auto a = PortholeLayout::forBounds (0.0f, 0.0f, 592.0f, 486.0f);
            const auto b = PortholeLayout::forBounds (140.0f, -30.0f, 592.0f, 486.0f);
            expectWithinAbsoluteError (b.centreX - a.centreX, 140.0f, 0.01f);
            expectWithinAbsoluteError (b.centreY - a.centreY, -30.0f, 0.01f);
            expectWithinAbsoluteError (b.outerR, a.outerR, 0.01f);
        }

        beginTest ("Degenerate and non-finite sizes stay finite");
        {
            const float nan = std::numeric_limits<float>::quiet_NaN();
            const float inf = std::numeric_limits<float>::infinity();
            const float sizes2[][2] = { { 0.0f, 0.0f }, { 1.0f, 1.0f }, { -40.0f, 90.0f }, { 40.0f, -90.0f },
                                        { nan, 100.0f }, { 100.0f, nan }, { inf, inf }, { 1.0e9f, 1.0e9f } };
            for (const auto& s : sizes2)
            {
                const auto l = PortholeLayout::forBounds (0.0f, 0.0f, s[0], s[1]);
                expect (std::isfinite (l.outerR) && std::isfinite (l.glassR) && std::isfinite (l.centreX)
                        && std::isfinite (l.centreY) && std::isfinite (l.glassR)
                        && std::isfinite (l.unit) && std::isfinite (l.bezelWidth),
                        "a degenerate size produced a non-finite layout");
                expect (l.outerR > 0.0f, "a degenerate size collapsed the bezel");
            }
            expect (PortholeLayout::forBounds (0.0f, 0.0f, 30.0f, 24.0f).isTiny(), "a tiny component was not reported as tiny");
            expect (! PortholeLayout::forBounds (0.0f, 0.0f, 592.0f, 486.0f).isTiny(), "a full-size porthole was reported as tiny");
        }
    }
};

static PortholeLayoutTests portholeLayoutTests;

//==============================================================================
/**
    THE VOLUMETRIC FIELD (NodeField).

    The field is the object, so what is tested here is what the object promises:
    that a seed reproduces it exactly, that nothing it is handed — including a
    snapshot full of NaNs — can put a point outside a bounded box, and that every
    engine input it claims to read actually moves something.
*/
class NodeFieldTests : public juce::UnitTest
{
public:
    NodeFieldTests() : juce::UnitTest ("ANTI-MATTER field", "Visualizers") {}

    /** A snapshot of a note ringing on a material with a full set of resonators. */
    static VisualStateSnapshot ringing()
    {
        VisualStateSnapshot s;
        s.density = 0.5f; s.form = 0.3f; s.mass = 0.4f; s.tension = 0.5f; s.decay = 0.5f; s.surface = 0.2f;
        s.pitchHz = 261.6f;
        s.numVisualNodes = VisualStateSnapshot::kVisualNodes;
        s.clusterCount = 4;
        for (int i = 0; i < s.numVisualNodes; ++i)
        {
            s.nodeFrequency[i] = 261.6f * (float) (i + 1);
            s.nodeEnergy[i] = 0.06f / (float) (i + 1);
            s.nodePan[i] = (i % 2 == 0 ? 0.4f : -0.4f);
            s.nodeCluster[i] = (uint8_t) (i % 4);
        }
        s.noteId = 7; s.noteVelocity = 0.8f; s.noteMidi = 60; s.noteHeld = true;
        s.numVisualVoices = 1;
        s.voicePitchHz[0] = 261.6f; s.voiceEnergy[0] = 0.8f; s.voiceVelocity[0] = 0.8f;
        s.activeVoices = 1;
        return s;
    }

    static NodeField::Anim playing()
    {
        NodeField::Anim a;
        a.dt = 1.0f / 30.0f;
        a.time = 4.0f; a.flowTime = 2.5f;
        a.level = 0.4f; a.energy = 0.7f; a.life = 1.0f;
        a.coreR = 0.3f;
        return a;
    }

    /** Runs `frames` frames and returns the field, so state that accumulates is exercised. */
    static void run (NodeField& f, const VisualStateSnapshot& s, NodeField::Anim a,
                     const ValueNoise& n, int count, int frames)
    {
        for (int i = 0; i < frames; ++i)
        {
            a.time += a.dt;
            a.flowTime += a.dt * 0.5f;
            f.update (s, a, n, count);
        }
    }

    static float meanRadius (const NodeField& f, int count)
    {
        if (count <= 0) return 0.0f;
        float sum = 0.0f;
        for (int i = 0; i < count; ++i) sum += f.point (i).p.length();
        return sum / (float) count;
    }

    static float radiusSpread (const NodeField& f, int count)
    {
        if (count <= 0) return 0.0f;
        const float mean = meanRadius (f, count);
        float sum = 0.0f;
        for (int i = 0; i < count; ++i)
        {
            const float d = f.point (i).p.length() - mean;
            sum += d * d;
        }
        return std::sqrt (sum / (float) count);
    }

    static float meanShiftOf (const NodeField& a, const NodeField& b, int count)
    {
        if (count <= 0) return 0.0f;
        float sum = 0.0f;
        for (int i = 0; i < count; ++i) sum += (a.point (i).p - b.point (i).p).length();
        return sum / (float) count;
    }

    static float totalLight (const NodeField& f, int count)
    {
        float sum = 0.0f;
        for (int i = 0; i < count; ++i) sum += f.point (i).bright;
        return sum;
    }

    void runTest() override
    {
        const ValueNoise noise (0x5EEDA11u);
        const int count = 512;

        beginTest ("A seed reproduces the field exactly");
        {
            NodeField a (0xB0DE5u), b (0xB0DE5u);
            run (a, ringing(), playing(), noise, count, 20);
            run (b, ringing(), playing(), noise, count, 20);
            for (int i = 0; i < count; ++i)
            {
                expect (a.point (i).p.x == b.point (i).p.x
                        && a.point (i).p.y == b.point (i).p.y
                        && a.point (i).p.z == b.point (i).p.z, "the same seed gave a different position");
                expect (a.point (i).bright == b.point (i).bright, "the same seed gave a different brightness");
            }
            NodeField c (0xB0DE6u);
            run (c, ringing(), playing(), noise, count, 20);
            expect (meanShiftOf (a, c, count) > 0.01f, "a different seed produced the same field");
        }

        beginTest ("Every output is finite and bounded, whatever arrives in the snapshot");
        {
            const float nan = std::numeric_limits<float>::quiet_NaN();
            const float inf = std::numeric_limits<float>::infinity();
            const float extremes[] = { 0.0f, 1.0f, -1.0f, 0.5f, nan, inf, -inf, 1.0e12f, -1.0e12f };

            for (float v : extremes)
            {
                VisualStateSnapshot s = ringing();
                s.density = s.form = s.mass = s.tension = s.decay = s.surface = v;
                s.bend = s.melt = s.tear = s.magnet = s.gravity = s.scatter = s.crush = v;
                s.pitchHz = v;
                for (int i = 0; i < s.numVisualNodes; ++i)
                {
                    s.nodeFrequency[i] = v;
                    s.nodeEnergy[i] = v;
                    s.nodePan[i] = v;
                }
                for (int k = 0; k < VisualStateSnapshot::kVisualVoices; ++k)
                {
                    s.voicePitchHz[k] = v;
                    s.voiceEnergy[k] = v;
                    s.voiceVelocity[k] = v;
                }
                s.numVisualVoices = VisualStateSnapshot::kVisualVoices;
                s.noteVelocity = v;

                NodeField::Anim a = playing();
                a.breathe = v; a.level = v; a.energy = v; a.life = v; a.fracture = v;
                a.freezeMix = v; a.hueDrift = v; a.coreR = v;

                NodeField f;
                for (int frame = 0; frame < 12; ++frame)
                {
                    a.time += 1.0f / 30.0f;
                    a.flowTime += 0.02f;
                    s.noteId += 1;             // a new note every frame: the shock list must not run away
                    s.fractureHits += 2;
                    f.update (s, a, noise, count);
                }
                for (int i = 0; i < count; ++i)
                {
                    const auto& q = f.point (i);
                    expect (finite (q.p), "a point left the finite numbers");
                    expect (std::abs (q.p.x) <= 3.0f && std::abs (q.p.y) <= 3.0f && std::abs (q.p.z) <= 3.0f,
                            "a point escaped its bounding box");
                    expect (std::isfinite (q.bright) && q.bright >= 0.0f && q.bright <= 3.0f, "brightness left its range");
                    expect (std::isfinite (q.size) && q.size >= 0.0f && q.size < 0.2f, "size left its range");
                    expect (std::isfinite (q.hue), "hue left the finite numbers");
                    expect (std::isfinite (q.white) && q.white >= 0.0f && q.white <= 1.0f, "whiteness left its range");
                    expect (finite (q.vel), "a velocity left the finite numbers");
                }
            }
        }

        beginTest ("The count is a whole number of swarms and respects its cap");
        {
            VisualStateSnapshot s = ringing();
            for (float d : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
            {
                s.density = d;
                for (int cap : { 128, 900, 2400, NodeField::kMaxPoints })
                {
                    const int n = NodeField::pointCount (s, playing(), cap);
                    expect (n <= cap && n <= NodeField::kMaxPoints, "the point count ignored its cap");
                    expect (n % NodeField::kSlots == 0, "the swarms were left uneven");
                    expect (n > 0, "the field emptied itself");
                }
            }
            s.density = 0.0f;
            const int low = NodeField::pointCount (s, playing(), NodeField::kMaxPoints);
            s.density = 1.0f;
            const int high = NodeField::pointCount (s, playing(), NodeField::kMaxPoints);
            expect (high > low + 500, "Density did not carry the population");
        }

        beginTest ("A node's frequency sets its radius and its energy sets its brightness");
        {
            // Two snapshots differing only in which end of the spectrum carries the energy.
            auto withEnergyAt = [] (bool low)
            {
                VisualStateSnapshot s = ringing();
                for (int i = 0; i < s.numVisualNodes; ++i)
                    s.nodeEnergy[i] = low ? (i < 4 ? 0.3f : 0.0005f) : (i >= 28 ? 0.3f : 0.0005f);
                return s;
            };
            NodeField deep, high;
            run (deep, withEnergyAt (true), playing(), noise, count, 30);
            run (high, withEnergyAt (false), playing(), noise, count, 30);

            // Brightness-weighted mean radius: energy low in the spectrum lights the
            // inside of the volume, energy high in it lights the shell.
            auto litRadius = [] (const NodeField& f, int n)
            {
                float w = 0.0f, sum = 0.0f;
                for (int i = 0; i < n; ++i)
                {
                    const float b = f.point (i).bright;
                    sum += b * f.point (i).p.length();
                    w += b;
                }
                return w > 0.0f ? sum / w : 0.0f;
            };
            expect (litRadius (high, count) > litRadius (deep, count) + 0.03f,
                    "energy high in the spectrum did not light the outside of the volume");
        }

        beginTest ("A node that falls silent takes its swarm's light with it");
        {
            VisualStateSnapshot loud = ringing(), quiet = ringing();
            for (int i = 0; i < quiet.numVisualNodes; ++i) quiet.nodeEnergy[i] = 0.0f;
            quiet.voiceEnergy[0] = 0.0f;
            NodeField a, b;
            run (a, loud, playing(), noise, count, 40);
            run (b, quiet, playing(), noise, count, 40);
            expect (totalLight (a, count) > totalLight (b, count) * 1.6f,
                    "a silent set of resonators looked the same as a ringing one");
            expect (totalLight (b, count) > 0.0f, "a silent field went completely dark");
        }

        beginTest ("Nothing playing still leaves a field that breathes");
        {
            VisualStateSnapshot s;          // default: no nodes, no voices, nothing sounding
            NodeField::Anim a = playing();
            a.life = 0.0f; a.level = 0.0f; a.energy = 0.0f;
            NodeField f;
            run (f, s, a, noise, count, 30);
            expect (totalLight (f, count) > 0.15f * (float) count, "the idle field is too dark to read");
            float far = 0.0f;
            for (int i = 0; i < count; ++i) far = std::max (far, f.point (i).p.length());
            expect (far > 0.5f, "the idle field collapsed into the middle");
        }

        beginTest ("Tension draws the shells onto one skin");
        {
            VisualStateSnapshot loose = ringing(), tight = ringing();
            loose.tension = 0.0f; tight.tension = 1.0f;
            NodeField a, b;
            run (a, loose, playing(), noise, count, 30);
            run (b, tight, playing(), noise, count, 30);
            expect (radiusSpread (b, count) < radiusSpread (a, count) * 0.7f,
                    "Tension did not tighten the orbits");
        }

        beginTest ("Mass opens a cavity the field keeps out of");
        {
            for (float mass : { 0.0f, 1.0f })
            {
                VisualStateSnapshot s = ringing();
                s.mass = mass;
                NodeField::Anim a = playing();
                a.coreR = 0.145f + 0.27f * mass;
                NodeField f;
                run (f, s, a, noise, count, 30);
                float nearest = 10.0f;
                for (int i = 0; i < count; ++i) nearest = std::min (nearest, f.point (i).p.length());
                expect (nearest > a.coreR * 0.75f, "a point was left inside the mass");
            }
        }

        beginTest ("Every Evolve operator moves the volume");
        {
            NodeField rest;
            run (rest, ringing(), playing(), noise, count, 30);

            struct Op { const char* name; float VisualStateSnapshot::* field; float value; float least; };
            const Op ops[] = {
                { "bend",    &VisualStateSnapshot::bend,    1.0f, 0.08f },
                { "melt",    &VisualStateSnapshot::melt,    1.0f, 0.08f },
                { "tear",    &VisualStateSnapshot::tear,    1.0f, 0.10f },
                { "magnet",  &VisualStateSnapshot::magnet,  1.0f, 0.05f },
                { "gravity", &VisualStateSnapshot::gravity, 0.0f, 0.08f },
                { "scatter", &VisualStateSnapshot::scatter, 1.0f, 0.08f },
                { "crush",   &VisualStateSnapshot::crush,   1.0f, 0.02f },
            };
            for (const auto& op : ops)
            {
                VisualStateSnapshot s = ringing();
                s.*(op.field) = op.value;
                NodeField f;
                run (f, s, playing(), noise, count, 30);
                const float shift = meanShiftOf (rest, f, count);
                expect (shift > op.least,
                        juce::String (op.name) + " barely moved the volume (" + juce::String (shift, 4) + ")");
            }
        }

        beginTest ("Freeze holds the flow: the same clock gives the same volume");
        {
            VisualStateSnapshot s = ringing();
            NodeField::Anim a = playing();
            a.freezeMix = 1.0f;
            NodeField f;
            // Long enough for the voice cells to settle, advancing the same `a` the
            // freeze loop below will use.
            for (int k = 0; k < 90; ++k)
            {
                a.time += a.dt;
                a.flowTime += a.dt * 0.5f;
                f.update (s, a, noise, count);
            }
            // Now advance the wall clock but not the flow clock, exactly as Freeze does.
            std::array<Vec3, 512> before {};
            for (int i = 0; i < count; ++i) before[(size_t) i] = f.point (i).p;
            for (int k = 0; k < 10; ++k) { a.time += a.dt; f.update (s, a, noise, count); }
            float moved = 0.0f;
            for (int i = 0; i < count; ++i) moved = std::max (moved, (f.point (i).p - before[(size_t) i]).length());
            expect (moved < 0.02f, "the volume kept drifting while frozen");
        }

        beginTest ("A new note throws a front out through the shells");
        {
            VisualStateSnapshot s = ringing();
            NodeField::Anim a = playing();
            NodeField f;
            run (f, s, a, noise, count, 20);
            const float quiet = totalLight (f, count);

            s.noteId = 8;                   // the event: a change of id, not of envelope
            s.noteVelocity = 1.0f;
            a.time += a.dt; f.update (s, a, noise, count);
            expect (f.shockAmplitude() > 0.3f, "a new note did not start a front");

            float peak = 0.0f;
            for (int k = 0; k < 12; ++k)
            {
                a.time += a.dt; a.flowTime += a.dt * 0.5f;
                f.update (s, a, noise, count);
                peak = std::max (peak, totalLight (f, count));
            }
            expect (peak > quiet * 1.15f, "the front did not light the volume as it passed");

            // A soft strike must throw a visibly weaker front than a hard one. Both
            // fields start with nothing sounding so no earlier front is in flight.
            VisualStateSnapshot ss = ringing(), hs = ringing();
            ss.noteId = 0; hs.noteId = 0;
            NodeField soft, hard;
            NodeField::Anim aa = playing();
            run (soft, ss, aa, noise, count, 5);
            run (hard, hs, aa, noise, count, 5);
            expect (soft.shockAmplitude() < 0.001f, "a front fired without a note");
            ss.noteId = 9; ss.noteVelocity = 0.15f;
            hs.noteId = 9; hs.noteVelocity = 1.0f;
            soft.update (ss, aa, noise, count);
            hard.update (hs, aa, noise, count);
            expect (hard.shockAmplitude() > soft.shockAmplitude() * 1.4f,
                    "a hard strike threw the same front as a soft one");
        }

        beginTest ("A fragment step is answered one for one");
        {
            VisualStateSnapshot s = ringing();
            NodeField::Anim a = playing();
            NodeField f;
            run (f, s, a, noise, count, 10);
            expect (f.shatterAmount() < 0.01f, "the field was already shattered");
            s.fractureHits += 1;
            a.time += a.dt;
            f.update (s, a, noise, count);
            expect (f.shatterAmount() > 0.02f, "a fragment step did not scramble the volume");
            for (int k = 0; k < 40; ++k) { a.time += a.dt; f.update (s, a, noise, count); }
            expect (f.shatterAmount() < 0.01f, "the shatter never settled");
        }

        beginTest ("A chord fans the volume into one mass per voice");
        {
            VisualStateSnapshot one = ringing(), three = ringing();
            three.numVisualVoices = 3;
            three.activeVoices = 3;
            const float hz[3] = { 261.6f, 329.6f, 392.0f };
            for (int v = 0; v < 3; ++v)
            {
                three.voicePitchHz[v] = hz[v];
                three.voiceEnergy[v] = 0.8f;
                three.voiceVelocity[v] = 0.8f;
            }
            NodeField a, b;
            run (a, one, playing(), noise, count, 60);
            run (b, three, playing(), noise, count, 60);
            expect (meanShiftOf (a, b, count) > 0.02f, "a chord looked exactly like one note");
            expect (radiusSpread (b, count) > radiusSpread (a, count) * 1.02f,
                    "the voices of a chord did not take their own radii");
            expect (b.voiceCells() == 3, "the field did not see all three voices");
        }
    }
};

static NodeFieldTests nodeFieldTests;

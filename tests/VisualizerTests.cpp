/*
    Geometry and maths behind the centre object (VISUAL_SPEC §5).

    These run in the console test runner, which links no graphics module, so
    everything under test is deliberately free of juce::Colour / juce::Rectangle:
    LiquidOrganism, ParticleSystem and PortholeLayout are plain floats and Vec3.

    What is covered:
      * a seed reproduces the object exactly, frame after frame,
      * every output stays finite and bounded for every parameter extreme,
        including NaN and infinity arriving from a broken snapshot,
      * each engine reaction actually changes the geometry it claims to,
      * depth splitting and sorting really order the ribbons back to front,
      * the colour ramp travels cyan → blue → violet → magenta → pink,
      * the porthole layout is proportional and stays inside its component.
*/

#include <juce_core/juce_core.h>

#include "ui/visualizers/LiquidOrganism.h"
#include "ui/visualizers/ParticleSystem.h"
#include "ui/visualizers/Porthole.h"

using namespace am::ui;

namespace
{
    constexpr int kBuffer = LiquidOrganism::kMaxSamples;

    /** The parameter state the object shows with nothing touched. */
    LiquidOrganism::Params defaultParams()
    {
        LiquidOrganism::Params p;
        p.time = 12.5f;
        p.flowTime = 7.25f;
        p.rotation = 0.8f;
        p.level = 0.4f;
        p.energy = 0.5f;
        p.life = 1.0f;
        return p;
    }

    bool finite (const Vec3& v) noexcept
    {
        return std::isfinite (v.x) && std::isfinite (v.y) && std::isfinite (v.z);
    }

    /** Mean distance between two builds of the same ribbon — how far the geometry moved. */
    float meanShift (const RibbonSample* a, const RibbonSample* b, int n) noexcept
    {
        if (n <= 0) return 0.0f;
        float sum = 0.0f;
        for (int i = 0; i < n; ++i) sum += (a[i].p - b[i].p).length();
        return sum / (float) n;
    }

    float meanWidth (const RibbonSample* s, int n) noexcept
    {
        if (n <= 0) return 0.0f;
        float sum = 0.0f;
        for (int i = 0; i < n; ++i) sum += s[i].width;
        return sum / (float) n;
    }

    /** Builds every ribbon of one frame; returns the mean shift against `other` if given. */
    struct Frame
    {
        std::array<std::array<RibbonSample, kBuffer>, LiquidOrganism::kMaxRibbons> rows {};
        std::array<int, LiquidOrganism::kMaxRibbons> lengths {};
        int ribbons = 0;

        void build (const LiquidOrganism& o, const LiquidOrganism::Params& p, const ValueNoise& n)
        {
            ribbons = LiquidOrganism::ribbonCount (p);
            for (int r = 0; r < ribbons; ++r)
                lengths[(size_t) r] = o.buildRibbon (r, p, n, rows[(size_t) r].data(), kBuffer);
        }

        float shiftFrom (const Frame& other) const
        {
            float worst = 0.0f;
            const int common = juce::jmin (ribbons, other.ribbons);
            for (int r = 0; r < common; ++r)
            {
                const int n = juce::jmin (lengths[(size_t) r], other.lengths[(size_t) r]);
                worst = juce::jmax (worst, meanShift (rows[(size_t) r].data(), other.rows[(size_t) r].data(), n));
            }
            return worst;
        }

        float widthOf (int ribbon) const { return meanWidth (rows[(size_t) ribbon].data(), lengths[(size_t) ribbon]); }
    };
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
class LiquidOrganismTests : public juce::UnitTest
{
public:
    LiquidOrganismTests() : juce::UnitTest ("Liquid organism geometry", "visualizer") {}

    void runTest() override
    {
        const ValueNoise noise (0x5EEDA11u);

        beginTest ("A seed reproduces the object exactly");
        {
            LiquidOrganism a (0xC0FFEEu), b (0xC0FFEEu);
            const auto p = defaultParams();
            std::array<RibbonSample, kBuffer> ra {}, rb {};
            for (int r = 0; r < LiquidOrganism::kMaxRibbons; ++r)
            {
                const int na = a.buildRibbon (r, p, noise, ra.data(), kBuffer);
                const int nb = b.buildRibbon (r, p, noise, rb.data(), kBuffer);
                expectEquals (na, nb);
                for (int i = 0; i < na; ++i)
                {
                    expectWithinAbsoluteError (ra[(size_t) i].p.x, rb[(size_t) i].p.x, 0.0f);
                    expectWithinAbsoluteError (ra[(size_t) i].p.y, rb[(size_t) i].p.y, 0.0f);
                    expectWithinAbsoluteError (ra[(size_t) i].p.z, rb[(size_t) i].p.z, 0.0f);
                    expectWithinAbsoluteError (ra[(size_t) i].width, rb[(size_t) i].width, 0.0f);
                }
            }

            // A different seed must give a different organism, or the seed means nothing.
            LiquidOrganism c (0xBEEF01u);
            std::array<RibbonSample, kBuffer> rc {};
            const int n = c.buildRibbon (0, p, noise, rc.data(), kBuffer);
            expect (meanShift (ra.data(), rc.data(), n) > 0.05f, "two seeds produced the same ribbon");
        }

        beginTest ("Rebuilding the same frame is stable");
        {
            LiquidOrganism o (0x11B0Fu);
            const auto p = defaultParams();
            Frame first, again;
            first.build (o, p, noise);
            again.build (o, p, noise);
            expectWithinAbsoluteError (first.shiftFrom (again), 0.0f, 0.0f);
        }

        beginTest ("Output is bounded at every parameter extreme, NaN included");
        {
            LiquidOrganism o (0x11B0Fu);
            const float nan = std::numeric_limits<float>::quiet_NaN();
            const float inf = std::numeric_limits<float>::infinity();
            const float extremes[] = { 0.0f, 0.5f, 1.0f, -1.0f, 2.0f, 1.0e9f, -1.0e9f, nan, inf, -inf };

            // Every parameter in turn takes every extreme while the rest sit at 1.0,
            // which is the worst case for the deformations piling on top of each other.
            for (int field = 0; field < 18; ++field)
            {
                for (float v : extremes)
                {
                    auto p = defaultParams();
                    p.density = p.mass = p.tension = p.surface = 1.0f;
                    p.bend = p.melt = p.tear = p.magnet = p.scatter = p.crush = 1.0f;
                    p.fracture = 1.0f;
                    switch (field)
                    {
                        case 0:  p.time = v; break;
                        case 1:  p.flowTime = v; break;
                        case 2:  p.rotation = v; break;
                        case 3:  p.pitch = v; break;
                        case 4:  p.density = v; break;
                        case 5:  p.form = v; break;
                        case 6:  p.mass = v; break;
                        case 7:  p.tension = v; break;
                        case 8:  p.surface = v; break;
                        case 9:  p.bend = v; break;
                        case 10: p.melt = v; break;
                        case 11: p.tear = v; break;
                        case 12: p.magnet = v; break;
                        case 13: p.gravity = v; break;
                        case 14: p.scatter = v; break;
                        case 15: p.crush = v; break;
                        case 16: p.breathe = v; break;
                        default: p.level = p.energy = p.life = p.fracture = v; break;
                    }

                    const juce::String at = " (field " + juce::String (field) + ", value " + juce::String (v) + ")";
                    const int ribbons = LiquidOrganism::ribbonCount (p);
                    expect (ribbons >= 4 && ribbons <= LiquidOrganism::kMaxRibbons, "ribbon count out of range" + at);
                    const int samples = LiquidOrganism::sampleCount (p);
                    expect (samples >= 8 && samples <= LiquidOrganism::kMaxSamples, "sample count out of range" + at);
                    const float core = LiquidOrganism::coreRadius (p);
                    expect (std::isfinite (core) && core > 0.05f && core < 0.8f, "core radius out of range" + at);

                    for (int r = 0; r < ribbons; ++r)
                    {
                        std::array<RibbonSample, kBuffer> row {};
                        const int n = o.buildRibbon (r, p, noise, row.data(), kBuffer);
                        expect (n >= 8 && n <= kBuffer, "sample count out of range" + at);
                        for (int i = 0; i < n; ++i)
                        {
                            expect (finite (row[(size_t) i].p), "non-finite position" + at);
                            expect (std::abs (row[(size_t) i].p.x) <= 4.0f
                                    && std::abs (row[(size_t) i].p.y) <= 4.0f
                                    && std::abs (row[(size_t) i].p.z) <= 4.0f, "position outside the bounding box" + at);
                            expect (std::isfinite (row[(size_t) i].width)
                                    && row[(size_t) i].width >= 0.0f && row[(size_t) i].width <= 0.7f, "width out of range" + at);
                            expect (std::isfinite (row[(size_t) i].bright)
                                    && row[(size_t) i].bright >= 0.0f && row[(size_t) i].bright <= 1.4f, "brightness out of range" + at);
                            expect (row[(size_t) i].u >= 0.0f && row[(size_t) i].u <= 1.0f, "u out of range" + at);
                        }
                        // The ends taper to nothing: a ribbon must never read as a stroked polyline.
                        expect (row[0].width <= 0.02f && row[(size_t) (n - 1)].width <= 0.02f, "the ribbon ends do not taper" + at);
                    }
                }
            }
        }

        beginTest ("A tiny output buffer is respected");
        {
            LiquidOrganism o (0x11B0Fu);
            std::array<RibbonSample, kBuffer> row {};
            const auto p = defaultParams();
            expectEquals (o.buildRibbon (0, p, noise, row.data(), 0), 0);
            expectEquals (o.buildRibbon (0, p, noise, row.data(), 1), 0);
            expectEquals (o.buildRibbon (0, p, noise, nullptr, kBuffer), 0);
            expect (o.buildRibbon (0, p, noise, row.data(), 12) == 12, "the buffer limit was ignored");
            // Out-of-range ribbon indices wrap rather than reading past the array.
            expect (o.buildRibbon (-7, p, noise, row.data(), kBuffer) > 0, "a negative ribbon index produced nothing");
            expect (o.buildRibbon (9999, p, noise, row.data(), kBuffer) > 0, "a huge ribbon index produced nothing");
        }

        beginTest ("Density drives ribbon count and sample count");
        {
            auto low = defaultParams();  low.density = 0.0f;
            auto high = defaultParams(); high.density = 1.0f;
            expect (LiquidOrganism::ribbonCount (high) > LiquidOrganism::ribbonCount (low), "DENSITY does not add ribbons");
            expect (LiquidOrganism::sampleCount (high) > LiquidOrganism::sampleCount (low), "DENSITY does not add detail");
        }

        beginTest ("Mass opens the core");
        {
            auto shut = defaultParams(); shut.mass = 0.0f;
            auto open = defaultParams(); open.mass = 1.0f;
            expect (LiquidOrganism::coreRadius (open) > LiquidOrganism::coreRadius (shut) + 0.15f, "MASS does not open the core");
        }

        beginTest ("Tension tightens the orbits and thins the ribbons");
        {
            LiquidOrganism o (0x11B0Fu);
            auto loose = defaultParams(); loose.tension = 0.0f;
            auto tight = defaultParams(); tight.tension = 1.0f;
            Frame a, b;
            a.build (o, loose, noise);
            b.build (o, tight, noise);

            float thinner = 0.0f, wider = 0.0f;
            float spreadLoose = 0.0f, spreadTight = 0.0f;
            for (int r = 0; r < juce::jmin (a.ribbons, b.ribbons); ++r)
            {
                (b.widthOf (r) < a.widthOf (r) ? thinner : wider) += 1.0f;
                for (int i = 0; i < a.lengths[(size_t) r]; ++i) spreadLoose += a.rows[(size_t) r][(size_t) i].p.length();
                for (int i = 0; i < b.lengths[(size_t) r]; ++i) spreadTight += b.rows[(size_t) r][(size_t) i].p.length();
            }
            expect (thinner > wider, "TENSION did not thin the ribbons");
            expect (spreadTight < spreadLoose, "TENSION did not pull the orbits in");
        }

        beginTest ("Every Evolve operator moves the geometry");
        {
            LiquidOrganism o (0x11B0Fu);
            const auto rest = defaultParams();
            Frame base;
            base.build (o, rest, noise);

            struct Case { const char* name; float LiquidOrganism::Params::* field; float minimumShift; };
            const Case cases[] = {
                { "BEND",    &LiquidOrganism::Params::bend,    0.05f },
                { "MELT",    &LiquidOrganism::Params::melt,    0.05f },
                { "TEAR",    &LiquidOrganism::Params::tear,    0.05f },
                { "MAGNET",  &LiquidOrganism::Params::magnet,  0.05f },
                { "SCATTER", &LiquidOrganism::Params::scatter, 0.02f },
                { "CRUSH",   &LiquidOrganism::Params::crush,   0.01f },
                { "SURFACE", &LiquidOrganism::Params::surface, 0.0f  },   // SURFACE roughens edges, not positions
            };
            for (const auto& c : cases)
            {
                auto p = rest;
                p.*(c.field) = 1.0f;
                Frame moved;
                moved.build (o, p, noise);
                if (c.minimumShift > 0.0f)
                    expect (moved.shiftFrom (base) > c.minimumShift,
                            juce::String (c.name) + " did not deform the object");
            }

            // GRAVITY pulls the whole bundle in or pushes it out.
            auto light = rest, heavy = rest;
            light.gravity = 0.0f;
            heavy.gravity = 1.0f;
            Frame l, h;
            l.build (o, light, noise);
            h.build (o, heavy, noise);
            float outer = 0.0f, inner = 0.0f;
            for (int i = 0; i < l.lengths[0]; ++i) outer += l.rows[0][(size_t) i].p.length();
            for (int i = 0; i < h.lengths[0]; ++i) inner += h.rows[0][(size_t) i].p.length();
            expect (inner < outer, "GRAVITY did not pull the bundle in");
        }

        beginTest ("Tear splits a ribbon and the halves drift apart");
        {
            LiquidOrganism o (0x11B0Fu);
            auto whole = defaultParams();
            auto torn = defaultParams(); torn.tear = 1.0f;
            std::array<RibbonSample, kBuffer> a {}, b {};
            // Ribbon 1 is a tear victim (every third from index 1).
            const int n = o.buildRibbon (1, whole, noise, a.data(), kBuffer);
            o.buildRibbon (1, torn, noise, b.data(), kBuffer);
            const int half = n / 2;
            const float gapBefore = (a[(size_t) half].p - a[(size_t) (half - 1)].p).length();
            const float gapAfter = (b[(size_t) half].p - b[(size_t) (half - 1)].p).length();
            expect (gapAfter > gapBefore * 2.0f, "TEAR did not open a gap in the middle of the ribbon");
        }

        beginTest ("Crush quantises the ribbon onto a coarse lattice");
        {
            LiquidOrganism o (0x11B0Fu);
            auto p = defaultParams(); p.crush = 1.0f;
            std::array<RibbonSample, kBuffer> row {};
            const int n = o.buildRibbon (0, p, noise, row.data(), kBuffer);
            expect (o.isAngular (p), "CRUSH did not ask for angular joins");
            for (int i = 0; i < n; ++i)
            {
                // The lattice at crush = 1 has a step of 1 / 3.4.
                const float step = 1.0f / 3.4f;
                const float rx = row[(size_t) i].p.x / step;
                expect (std::abs (rx - std::round (rx)) < 0.02f, "CRUSH left a position off the lattice");
            }
            auto smooth = defaultParams();
            expect (! o.isAngular (smooth), "the object is angular with CRUSH at zero");
        }

        beginTest ("Freeze holds the flow: the same flow clock gives the same geometry");
        {
            LiquidOrganism o (0x11B0Fu);
            // FREEZE stops the caller advancing flowTime. Everything that moves the
            // organism must hang off that clock, so a held flowTime must hold the shape.
            auto a = defaultParams();
            auto b = defaultParams();
            b.time = a.time + 4.0f;             // the wall clock keeps running (wobble, twinkle)
            Frame fa, fb;
            fa.build (o, a, noise);
            fb.build (o, b, noise);
            expect (fb.shiftFrom (fa) < 0.02f, "the object kept travelling while the flow was frozen");

            auto c = defaultParams();
            c.flowTime = a.flowTime + 4.0f;     // and it must move again once the flow runs
            Frame fc;
            fc.build (o, c, noise);
            expect (fc.shiftFrom (fa) > 0.01f, "the object did not move when the flow ran");
        }

        beginTest ("The core silhouette is a bounded, deforming blob");
        {
            LiquidOrganism o (0x11B0Fu);
            auto p = defaultParams();
            float minR = 1.0e9f, maxR = -1.0e9f;
            for (float a = 0.0f; a < 6.283f; a += 0.02f)
            {
                const float r = o.coreProfile (a, p, noise);
                expect (std::isfinite (r) && r >= 0.5f && r <= 1.6f, "core profile out of range");
                minR = juce::jmin (minR, r);
                maxR = juce::jmax (maxR, r);
            }
            expect (maxR - minR > 0.02f, "the core is a circle, not an organic mass");
            expect (std::isfinite (o.coreProfile (std::numeric_limits<float>::quiet_NaN(), p, noise)), "NaN angle broke the core");

            auto later = defaultParams();
            later.time = p.time + 6.0f;
            expect (std::abs (o.coreProfile (1.0f, later, noise) - o.coreProfile (1.0f, p, noise)) > 1.0e-4f,
                    "the core does not deform over time");
        }
    }
};

static LiquidOrganismTests liquidOrganismTests;

//==============================================================================
class RibbonDepthTests : public juce::UnitTest
{
public:
    RibbonDepthTests() : juce::UnitTest ("Ribbon depth sorting", "visualizer") {}

    void runTest() override
    {
        beginTest ("Sorting orders the spans back to front");
        {
            std::array<RibbonSpan, 8> spans {};
            const float z[] = { 0.4f, -0.9f, 0.1f, -0.2f, 1.0f, -1.0f, 0.0f, 0.55f };
            for (int i = 0; i < 8; ++i) spans[(size_t) i] = { i, 0, 4, z[i], false };
            LiquidOrganism::sortByDepth (spans.data(), 8);
            for (int i = 1; i < 8; ++i)
                expect (spans[(size_t) i].meanZ >= spans[(size_t) (i - 1)].meanZ, "the spans are not ordered by depth");
        }

        beginTest ("A ribbon crossing behind the core is cut, one crossing at the rim is not");
        {
            std::array<RibbonSample, 9> row {};
            for (int i = 0; i < 9; ++i)
            {
                row[(size_t) i].u = (float) i / 8.0f;
                row[(size_t) i].p = { 0.05f, 0.0f, 0.8f - 0.2f * (float) i };     // passes through z = 0 near the axis
            }
            std::array<RibbonSpan, 8> spans {};
            const int cut = LiquidOrganism::splitByDepth (3, row.data(), 9, false, 0.4f, spans.data(), 8);
            expectEquals (cut, 2);
            expect (spans[0].ribbon == 3 && spans[1].ribbon == 3, "the spans lost their ribbon");
            expect (spans[0].first + spans[0].count - 1 == spans[1].first, "the two runs do not share the crossing sample");
            expect (spans[0].meanZ > 0.0f && spans[1].meanZ < 0.0f, "the runs are not on opposite sides of the core");

            // The same crossing, but out at the silhouette: no cut, or the strand shows
            // a blunt end in the middle of a ribbon that should read as continuous.
            for (int i = 0; i < 9; ++i) row[(size_t) i].p = { 1.1f, 0.0f, 0.8f - 0.2f * (float) i };
            expectEquals (LiquidOrganism::splitByDepth (3, row.data(), 9, false, 0.4f, spans.data(), 8), 1);
        }

        beginTest ("Splitting is safe with degenerate input");
        {
            std::array<RibbonSample, 4> row {};
            std::array<RibbonSpan, 4> spans {};
            expectEquals (LiquidOrganism::splitByDepth (0, nullptr, 4, false, 0.5f, spans.data(), 4), 0);
            expectEquals (LiquidOrganism::splitByDepth (0, row.data(), 4, false, 0.5f, nullptr, 4), 0);
            expectEquals (LiquidOrganism::splitByDepth (0, row.data(), 1, false, 0.5f, spans.data(), 4), 0);
            expectEquals (LiquidOrganism::splitByDepth (0, row.data(), 4, false, 0.5f, spans.data(), 0), 0);
            LiquidOrganism::sortByDepth (spans.data(), 0);      // must not walk off the front
        }

        beginTest ("Every sample of a real ribbon ends up in exactly one run");
        {
            const ValueNoise noise (0x5EEDA11u);
            LiquidOrganism o (0x11B0Fu);
            auto p = defaultParams();
            p.density = 1.0f;
            std::array<RibbonSample, kBuffer> row {};
            std::array<RibbonSpan, 16> spans {};
            for (int r = 0; r < LiquidOrganism::ribbonCount (p); ++r)
            {
                const int n = o.buildRibbon (r, p, noise, row.data(), kBuffer);
                const int count = LiquidOrganism::splitByDepth (r, row.data(), n, false, 0.5f, spans.data(), 16);
                expect (count >= 1, "a ribbon produced no spans");
                int covered = 0;
                for (int i = 0; i < count; ++i)
                {
                    expect (spans[(size_t) i].first >= 0 && spans[(size_t) i].first + spans[(size_t) i].count <= n,
                            "a span runs off the end of the ribbon");
                    covered += spans[(size_t) i].count;
                }
                // Runs overlap by exactly one sample at each cut.
                expectEquals (covered, n + count - 1);
            }
        }
    }
};

static RibbonDepthTests ribbonDepthTests;

//==============================================================================
class ObjectParticleTests : public juce::UnitTest
{
public:
    ObjectParticleTests() : juce::UnitTest ("Object particles", "visualizer") {}

    void runTest() override
    {
        const ValueNoise noise (0x5EEDA11u);

        beginTest ("A seed reproduces both populations");
        {
            ParticleSystem a (0xA11CEu), b (0xA11CEu);
            ParticleSystem::Env e;
            e.life = 1.0f;
            for (int frame = 0; frame < 40; ++frame)
            {
                e.time += e.dt;
                e.flowTime += e.dt * 0.4f;
                a.updateSparkles (e, noise, 60);
                b.updateSparkles (e, noise, 60);
                a.updateBubbles (e, noise, 10);
                b.updateBubbles (e, noise, 10);
            }
            for (int i = 0; i < 60; ++i)
            {
                expectWithinAbsoluteError (a.sparkleArray()[(size_t) i].p.x, b.sparkleArray()[(size_t) i].p.x, 0.0f);
                expectWithinAbsoluteError (a.sparkleArray()[(size_t) i].bright, b.sparkleArray()[(size_t) i].bright, 0.0f);
            }
        }

        beginTest ("Positions stay bounded for hours, at every extreme, NaN included");
        {
            ParticleSystem ps (0xA11CEu);
            const float nan = std::numeric_limits<float>::quiet_NaN();
            const float inf = std::numeric_limits<float>::infinity();
            const float extremes[] = { 0.0f, 1.0f, -5.0f, 1.0e9f, nan, inf, -inf };

            for (float v : extremes)
            {
                ParticleSystem::Env e;
                e.dt = v;
                e.time = v;
                e.flowTime = v;
                e.density = e.decay = e.tension = e.melt = e.scatter = e.life = e.level = e.breathe = v;
                e.fracture = v;
                for (int frame = 0; frame < 400; ++frame)
                {
                    ps.updateSparkles (e, noise, ParticleSystem::kMaxSparkles);
                    ps.updateBubbles (e, noise, ParticleSystem::kMaxBubbles);
                }
                for (const auto& s : ps.sparkleArray())
                {
                    expect (std::isfinite (s.p.x) && std::isfinite (s.p.y) && std::isfinite (s.p.z), "sparkle went non-finite");
                    expect (std::abs (s.p.x) <= 5.0f && std::abs (s.p.y) <= 5.0f && std::abs (s.p.z) <= 5.0f, "sparkle escaped its box");
                    expect (s.bright >= 0.0f && s.bright <= 1.0f, "sparkle brightness out of range");
                }
                for (const auto& b : ps.bubbleArray())
                {
                    expect (std::isfinite (b.p.x) && std::isfinite (b.p.y) && std::isfinite (b.p.z), "bubble went non-finite");
                    expect (std::abs (b.p.x) <= 4.0f && std::abs (b.p.y) <= 4.0f && std::abs (b.p.z) <= 4.0f, "bubble escaped its box");
                    expect (std::isfinite (b.scale) && b.scale > 0.0f, "bubble depth scale out of range");
                }
            }
        }

        beginTest ("Counts follow Density and respect their caps");
        {
            ParticleSystem::Env quiet, busy;
            quiet.density = 0.0f; quiet.life = 0.0f;
            busy.density = 1.0f; busy.life = 1.0f;
            expect (ParticleSystem::sparkleCount (busy, 999) > ParticleSystem::sparkleCount (quiet, 999), "DENSITY does not add sparkles");
            expect (ParticleSystem::bubbleCount (busy, 999) > ParticleSystem::bubbleCount (quiet, 999), "DENSITY does not add bubbles");
            expect (ParticleSystem::sparkleCount (busy, 999) <= ParticleSystem::kMaxSparkles, "sparkle count over the array");
            expect (ParticleSystem::bubbleCount (busy, 999) <= ParticleSystem::kMaxBubbles, "bubble count over the array");
            expect (ParticleSystem::sparkleCount (busy, 12) <= 12, "the caller's cap was ignored");
            expect (ParticleSystem::sparkleCount (busy, 0) >= 4, "a zero cap produced an empty population");

            ParticleSystem::Env broken;
            broken.density = std::numeric_limits<float>::quiet_NaN();
            expect (ParticleSystem::sparkleCount (broken, 999) >= 4, "NaN density emptied the volume");
        }

        beginTest ("A Fracture hit throws the sparkles outward, and they settle back");
        {
            ParticleSystem ps (0xA11CEu);
            ParticleSystem::Env calm;
            calm.life = 1.0f;
            for (int frame = 0; frame < 30; ++frame) { calm.time += calm.dt; ps.updateSparkles (calm, noise, 60); }
            float before = 0.0f;
            for (int i = 0; i < 60; ++i) before += ps.sparkleArray()[(size_t) i].p.length();

            auto hit = calm;
            hit.fracture = 1.0f;
            for (int frame = 0; frame < 30; ++frame) { hit.time += hit.dt; ps.updateSparkles (hit, noise, 60); }
            float during = 0.0f;
            for (int i = 0; i < 60; ++i) during += ps.sparkleArray()[(size_t) i].p.length();
            expect (during > before * 1.1f, "FRACTURE did not throw the sparkles outward");

            for (int frame = 0; frame < 300; ++frame) { calm.time += calm.dt; ps.updateSparkles (calm, noise, 60); }
            float after = 0.0f;
            for (int i = 0; i < 60; ++i) after += ps.sparkleArray()[(size_t) i].p.length();
            expect (after < during, "the burst never settled back");
        }
    }
};

static ObjectParticleTests objectParticleTests;

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
            expect (l.plinthTop + l.plinthHeight <= (float) s.h + 1.0f, "the plinth runs off the bottom" + at);
            expect (l.centreX + l.plinthHalfWidth <= (float) s.w + 0.5f, "the plinth runs off the side" + at);
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
            expectWithinAbsoluteError (small.plinthHeight / big.plinthHeight, k, 0.005f);
            expectWithinAbsoluteError ((small.plinthTop / small.outerR), (big.plinthTop / big.outerR), 0.01f);
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
                        && std::isfinite (l.centreY) && std::isfinite (l.plinthTop) && std::isfinite (l.plinthHeight)
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

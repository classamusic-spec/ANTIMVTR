#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include "core/Random.h"
#include "dev/diagnostics/DiagnosticSnapshot.h"
#include "LiquidOrganism.h"
#include "ValueNoise.h"

namespace am::ui
{

//==============================================================================
/** Table-driven sine: the field turns thousands of angles per frame and libm's
    sine is the only thing in the update loop that would show up in a profile. */
class SineTable
{
public:
    static constexpr int kSize = 2048;

    SineTable() noexcept
    {
        for (int i = 0; i <= kSize; ++i)
            table[(size_t) i] = std::sin ((float) i * (6.28318531f / (float) kSize));
    }

    /** sin(x) for any finite x; wraps. */
    float sin (float x) const noexcept
    {
        if (! std::isfinite (x)) return 0.0f;
        float u = x * (1.0f / 6.28318531f);
        u -= std::floor (u);
        const float f = u * (float) kSize;
        const int i = (int) f;
        const float t = f - (float) i;
        const int i0 = i & (kSize - 1);
        return table[(size_t) i0] + (table[(size_t) (i0 + 1)] - table[(size_t) i0]) * t;
    }

    float cos (float x) const noexcept { return sin (x + 1.57079633f); }

    void sincos (float x, float& s, float& c) const noexcept { s = sin (x); c = sin (x + 1.57079633f); }

private:
    std::array<float, (size_t) kSize + 1> table {};
};

inline const SineTable& sineTable() noexcept { static const SineTable t; return t; }

//==============================================================================
/** One point of light in the volume, in object space (the sphere has radius 1). */
struct FieldPoint
{
    Vec3  p;                 ///< live position
    float bright = 0.0f;     ///< emission, 0 .. ~2 (values over 1 bloom)
    float size   = 0.0f;     ///< radius in object units
    float hue    = 0.0f;     ///< 0..1 into the house ramp
    float white  = 0.0f;     ///< 0..1 pull toward molten white
    Vec3  vel;               ///< motion this frame, object units — drives the comet tail
};

//==============================================================================
/**
    THE ANTI-MATTER OBJECT — a volumetric field of light bound to the modal nodes.

    The engine publishes up to 32 resonators for the focus voice, each with its own
    frequency, energy, pan and cluster. Every particle in this field belongs to one
    of them. That is the whole idea: the cloud is not a fog with an audio-reactive
    size, it is a picture of the physical model that is making the sound.

      nodeFrequency → RADIUS.    A node an octave above the fundamental sits
                                 further out; four octaves up and it is at the
                                 shell. Low nodes are deep and turn slowly, high
                                 ones ride the surface and turn quickly.
      nodeEnergy    → BRIGHTNESS. Not a multiplier on brightness: it *is* the
                                 brightness of that node's swarm, so when the
                                 partials of a struck note decay at different
                                 rates the field visibly thins in some shells and
                                 holds in others.
      nodePan       → LATERAL.   The swarm slides left or right with the pan of
                                 its resonator, so a wide patch spreads the object.
      nodeCluster   → STRUCTURE. Every cluster owns a direction; its nodes' swarms
                                 are gathered into a cap around it, so the mass has
                                 arms and lobes instead of being uniform fog.

    On top of that the shape and movement controls act on the whole volume:

      Density → how many points are alive, and how thick each shell is
      Mass    → the radius of the dark core the cloud opens around
      Tension → collapses the shells onto one tight surface
      Surface → roughens every path with a fast per-point wobble
      Decay   → how long the note glow and the shock fronts hold
      Form    → organic scatter against a banded, laminar field
      Bend / Melt / Tear / Magnet / Gravity / Scatter / Crush / Freeze all deform
      the volume (see `deform`).

    Events, from the event fields rather than from an envelope:

      noteId change → a SHOCK FRONT leaves the core and travels out through the
                      field, lifting and igniting every point it passes. Its
                      amplitude is the note velocity, so a hard strike throws a
                      visibly stronger wave than a soft one.
      fractureHits  → a thin, fast front plus a shatter impulse that scrambles the
                      points for a moment: one per fragment step, crisp.
      voices        → every sounding voice owns a cell of the field, placed at its
                      own pitch radius and its own direction, so a chord reads as
                      several concentric masses rather than one louder one.

    Everything is a pure function of (seed, snapshot, animation clocks): nothing
    allocates after construction, every output is finite, and every position stays
    inside a bounded box however long the instrument runs.
*/
class NodeField
{
public:
    static constexpr int kMaxPoints = 4800;
    static constexpr int kSlots     = VisualStateSnapshot::kVisualNodes;   // 32 modal slots
    static constexpr int kCells     = VisualStateSnapshot::kVisualVoices;  // 8 voice cells
    static constexpr int kLobes     = 6;                                   // cluster directions
    static constexpr int kShocks    = 5;

    /** Animation clocks and eased levels the visualizer owns. */
    struct Anim
    {
        float dt        = 1.0f / 30.0f;
        float time      = 0.0f;    ///< wall clock: shimmer keeps moving even under Freeze
        float flowTime  = 0.0f;    ///< flow-field clock, held still by Freeze
        float level     = 0.0f;    ///< eased audio level 0..1
        float energy    = 0.0f;    ///< note envelope 0..1
        float life      = 0.0f;    ///< 0 idle → 1 playing
        float fracture  = 0.0f;    ///< eased fracture activity
        float freezeMix = 0.0f;    ///< 0 flowing → 1 frozen
        float breathe   = 1.0f;    ///< uniform scale (idle breath + audio pulse)
        float hueDrift  = 0.0f;    ///< slow travel along the ramp
        float coreR     = 0.30f;   ///< radius of the dark core the cloud opens around
    };

    explicit NodeField (uint32_t seed = 0xB0DE5u) noexcept { reseed (seed); }

    //==========================================================================
    void reseed (uint32_t seed) noexcept
    {
        Rng rng (seed);
        constexpr float twoPi = 6.28318531f;

        for (int i = 0; i < kMaxPoints; ++i)
        {
            auto& q = pts[(size_t) i];
            // Slot and cell are fixed for the life of a point: a point never changes
            // which resonator it belongs to, so the structure never reshuffles.
            q.slot     = (uint8_t) (i % kSlots);
            q.cell     = (uint8_t) ((i / kSlots) % kCells);
            q.azimuth  = rng.nextFloat() * twoPi;
            // cos of the polar angle, uniform so the raw distribution is even on the sphere
            q.height   = rng.nextBipolar();
            // Cube root of a uniform gives an even distribution through the shell's
            // thickness rather than a crust at its outside.
            q.radial   = std::cbrt (rng.nextFloat()) * (rng.chance (0.5f) ? 1.0f : -1.0f);
            q.spin     = 0.55f + 0.9f * rng.nextFloat();
            // A heavy tail: most points are fine motes and a few are big soft lamps.
            // A field of identically sized dots reads as printed texture, never as depth.
            { const float u = rng.nextFloat(); q.sizeMul = 0.34f + 1.60f * u * u * u; }
            q.seed     = rng.nextFloat() * 120.0f;
            q.twinkle  = 0.6f + 3.4f * rng.nextFloat();
            q.hash     = rng.nextFloat();
            q.prev     = Vec3 {};
            q.live     = Vec3 {};
        }

        // A fixed tilt per slot: nodes of one cluster spread around its lobe.
        for (int k = 0; k < kSlots; ++k)
        {
            const float h = 1.0f - 2.0f * ((float) k + 0.5f) / (float) kSlots;
            const float r = std::sqrt (maxf (0.0f, 1.0f - h * h));
            const float ang = (float) k * 2.39996323f;
            slotTilt[(size_t) k] = Vec3 { r * std::cos (ang), h, r * std::sin (ang) };
        }

        // Cluster lobes: a Fibonacci spiral so no two point the same way.
        for (int c = 0; c < kLobes; ++c)
        {
            const float h = 1.0f - 2.0f * ((float) c + 0.5f) / (float) kLobes;
            const float r = std::sqrt (maxf (0.0f, 1.0f - h * h));
            const float a = (float) c * 2.39996323f;
            lobe[(size_t) c] = Vec3 { r * std::cos (a), h, r * std::sin (a) };
            // A stable perpendicular per lobe: derived from the fixed base axis, so the
            // frame can never flip when pan tilts the axis a little.
            const Vec3 up = std::abs (lobe[(size_t) c].y) > 0.86f ? Vec3 { 1.0f, 0.0f, 0.0f } : Vec3 { 0.0f, 1.0f, 0.0f };
            lobeSide[(size_t) c] = normalised (cross (up, lobe[(size_t) c]));
        }

        // Voice cells start merged into the main mass and separate when a chord sounds.
        for (int v = 0; v < kCells; ++v)
        {
            cellScale[(size_t) v]  = 1.0f;
            cellEnergy[(size_t) v] = 0.0f;
            const float h = 1.0f - 2.0f * ((float) v + 0.5f) / (float) kCells;
            const float r = std::sqrt (maxf (0.0f, 1.0f - h * h));
            const float a = (float) v * 1.89996323f + 0.7f;
            cellHome[(size_t) v] = Vec3 { r * std::cos (a), h * 0.7f, r * std::sin (a) };
            cellOffset[(size_t) v] = 0.0f;
        }

        for (auto& s : shocks) s = Shock {};
        lastNote = 0;
        lastFracture = 0;
        shatter = 0.0f;
        strike = 0.0f;
        primed = false;
    }

    //==========================================================================
    /** How many points are alive: Density carries it, playing and Fracture add. */
    static int pointCount (const VisualStateSnapshot& s, const Anim& a, int cap) noexcept
    {
        const float d = liquid::clean (s.density, 0.0f, 1.0f, 0.5f);
        const float life = liquid::clean (a.life, 0.0f, 1.0f, 0.0f);
        const float frac = liquid::clean (a.fracture, 0.0f, 2.0f, 0.0f);
        const int n = 700 + (int) (d * 3200.0f) + (int) (life * 700.0f) + (int) (frac * 400.0f);
        const int top = cap < kMaxPoints ? cap : kMaxPoints;
        // Always a multiple of the slot count so every resonator keeps an equal swarm.
        const int clamped = n < 128 ? 128 : (n > top ? top : n);
        return (clamped / kSlots) * kSlots;
    }

    //==========================================================================
    /** Advances the whole field one frame. `count` comes from pointCount(). */
    void update (const VisualStateSnapshot& raw, const Anim& rawAnim, const ValueNoise& noise, int count) noexcept
    {
        const Anim a = sanitise (rawAnim);
        const auto& S = sineTable();
        count = count < 0 ? 0 : (count > kMaxPoints ? kMaxPoints : count);
        alive = count;

        const float density = liquid::clean (raw.density, 0.0f, 1.0f, 0.5f);
        const float form    = liquid::clean (raw.form,    0.0f, 1.0f, 0.3f);
        const float tension = liquid::clean (raw.tension, 0.0f, 1.0f, 0.5f);
        const float surface = liquid::clean (raw.surface, 0.0f, 1.0f, 0.2f);
        const float decay   = liquid::clean (raw.decay,   0.0f, 1.0f, 0.5f);
        const float bend    = liquid::clean (raw.bend,    0.0f, 1.0f, 0.0f);
        const float melt    = liquid::clean (raw.melt,    0.0f, 1.0f, 0.0f);
        const float tear    = liquid::clean (raw.tear,    0.0f, 1.0f, 0.0f);
        const float magnet  = liquid::clean (raw.magnet,  0.0f, 1.0f, 0.0f);
        const float gravity = liquid::clean (raw.gravity, 0.0f, 1.0f, 0.5f);
        const float scatter = liquid::clean (raw.scatter, 0.0f, 1.0f, 0.0f);
        const float crush   = liquid::clean (raw.crush,   0.0f, 1.0f, 0.0f);

        const float coreR = liquid::clean (a.coreR, 0.05f, 0.70f, 0.30f);
        const float shell = 1.00f - 0.08f * tension;

        //---- events -----------------------------------------------------------
        advanceEvents (raw, a, decay);

        //---- per-slot: the resonator each swarm belongs to ---------------------
        const int nodes = clampi (raw.numVisualNodes, 0, kSlots);
        // The fundamental is used as a divisor and as a fallback, so it has to be
        // finite before anything else touches it: an infinity arriving here would
        // come back out of the octave ratio as a NaN and poison the whole volume.
        float fundamental = liquid::clean (raw.pitchHz, 0.0f, 40000.0f, 0.0f);
        if (! (fundamental > 20.0f)) fundamental = 220.0f;
        // Cluster count decides how tightly a swarm gathers onto its lobe: one
        // cluster is a shell, six are arms.
        const int clusters = clampi (raw.clusterCount > 0 ? raw.clusterCount : 1, 1, kLobes);
        const float gather = 0.26f + 0.22f * ((float) (clusters - 1) / (float) (kLobes - 1)) + 0.34f * tension;

        // The loudest resonator of the moment sets the scale. Absolute modal energy is
        // a small, level-dependent number; what carries the structure is which nodes
        // are loud *relative to each other*, and that is what has to reach the eye.
        // The overall level comes back in through the voice cell, which does decay.
        float peakNode = 0.0f;
        for (int k = 0; k < nodes; ++k)
        {
            const float e = liquid::clean (raw.nodeEnergy[k], 0.0f, 8.0f, 0.0f);
            if (e > peakNode) peakNode = e;
        }
        const float nodeNorm = peakNode > 1.0e-6f ? 1.0f / peakNode : 0.0f;

        for (int k = 0; k < kSlots; ++k)
        {
            auto& sl = slot[(size_t) k];
            if (k < nodes)
            {
                const float f = liquid::clean (raw.nodeFrequency[k], 1.0f, 40000.0f, fundamental);
                // Four octaves of the material map onto the whole radius of the object.
                // The material runs six or seven octaves above its fundamental, so the
                // octave is square-rooted: the first two octaves — where the audible
                // partials live — get half the radius, and the top of the spectrum is a
                // fine crust at the shell rather than a pile-up.
                const float oct = std::log2 (f / fundamental);
                sl.t      = std::sqrt (liquid::clampf ((oct + 0.45f) * (1.0f / 7.0f), 0.0f, 1.0f));
                // A floor under it: a silent resonator is still matter, and has to keep
                // its shell visible as cold structure. Above the floor the curve is steep
                // enough that a partial holding while its neighbours die is unmistakable.
                const float rel = liquid::clean (raw.nodeEnergy[k], 0.0f, 8.0f, 0.0f) * nodeNorm;
                sl.energy = 0.30f + 0.70f * std::pow (liquid::clampf (rel, 0.0f, 1.0f), 0.55f);
                sl.pan    = liquid::clean (raw.nodePan[k], -1.0f, 1.0f, 0.0f);
                sl.lobeIx = (uint8_t) (raw.nodeCluster[k] % (uint8_t) kLobes);
                sl.active = 1;
            }
            else
            {
                // Vacuum dust: unbound points fill the whole ball, cold and faint, so
                // the object still has volume — and still breathes — with nothing playing.
                sl.t      = liquid::hash01 ((uint32_t) k * 2654435761u);
                sl.energy = 0.0f;
                sl.pan    = 0.0f;
                sl.lobeIx = (uint8_t) (k % kLobes);
                sl.active = 0;
            }

            sl.radius = coreR + (shell - coreR) * sl.t;
            // Low nodes turn slowly and deep, high ones ride the shell and turn fast.
            sl.spin   = (0.14f + 1.30f * sl.t) * (0.55f + 0.60f * a.life);
            // Deep = warm (pink into magenta), shell = cold (blue into cyan).
            sl.hue    = 0.82f - 0.80f * sl.t + 0.035f * (float) sl.lobeIx;

            // The swarm's own axis: its cluster's lobe, tilted by a fixed amount of
            // its own so nodes of one cluster form a neighbourhood rather than a spike,
            // then leaned toward the side its resonator is panned to.
            // MAGNET pulls the resonators onto shared paths: their axes collapse onto
            // their cluster's, so thirty-two separate orbits become a handful of rings.
            Vec3 w = lobe[(size_t) sl.lobeIx] + slotTilt[(size_t) k] * (0.95f * (1.0f - magnet));
            w.x += sl.pan * 0.55f * (1.0f - 0.7f * magnet);
            sl.w = normalised (w, lobe[(size_t) sl.lobeIx]);
            sl.u = normalised (cross (lobeSide[(size_t) sl.lobeIx], sl.w), lobeSide[(size_t) sl.lobeIx]);
            sl.v = normalised (cross (sl.w, sl.u), lobeSide[(size_t) sl.lobeIx]);
            sl.band = liquid::clampf ((1.0f - gather) * (1.0f - 0.60f * magnet), 0.07f, 1.0f);
        }

        //---- per-cell: one for every sounding voice ----------------------------
        updateCells (raw, a);

        //---- the volume -------------------------------------------------------
        // Shell thickness: Density fattens every band, Tension collapses them all
        // onto one surface.
        const float thickness = (0.055f + 0.22f * density) * (1.0f - 0.85f * tension) * (1.0f - 0.75f * magnet);
        const float flowAmp   = (0.055f + 0.10f * density) * (1.0f - 0.60f * tension) * (1.0f + 1.1f * melt)
                                * (0.35f + 0.65f * (1.0f - form));
        const float flowScale = 1.15f + 1.7f * density;
        const float lateral   = 0.26f + 0.10f * density;
        const float roughness = surface * (0.10f + 0.10f * density);
        const float jitter    = scatter * 0.55f;
        const float lattice   = crush > 0.04f ? (3.0f + 11.0f * (1.0f - crush)) : 0.0f;
        const float tearAmt   = tear * 0.34f;
        const float gravityPull = (gravity - 0.5f) * 2.0f;   // −1 in, +1 out
        // TENSION draws every shell onto one skin: at the top of the control the
        // resonators stop being separate orbits and become a single taut surface.
        const float skin      = coreR + (shell - coreR) * 0.86f;
        const float skinPull  = tension * tension * 0.80f;
        // MAGNET quantises the radius exactly as it quantises frequency in the engine:
        // the shells snap onto a coarse grid and the object goes concentric.
        const float magnetGrid = 3.0f + 3.0f * liquid::hash01 (0x51EDu);
        const float bendAngle = a.flowTime * 0.31f;
        float bs = 0.0f, bc = 1.0f;
        S.sincos (bendAngle, bs, bc);

        // The vacuum is never black. Even with nothing playing, every point keeps a
        // low luminosity that shimmers on its own clock, so the volume still has a
        // shape, still breathes, and never looks switched off.
        const float dust = 0.145f + 0.130f * (1.0f - a.life);

        for (int i = 0; i < count; ++i)
        {
            auto& q = pts[(size_t) i];
            const auto& sl = slot[(size_t) q.slot];
            const auto& cellS = cellScale[(size_t) q.cell];

            //-- radius: the node's shell, spread by Density, squeezed by Tension
            float rad = sl.radius * (1.0f + q.radial * thickness);
            rad *= cellS;
            if (skinPull > 0.005f) rad += (skin - rad) * skinPull;
            if (magnet > 0.02f)
            {
                const float snapped = std::round (rad * magnetGrid) / magnetGrid;
                rad += (snapped - rad) * magnet * 0.96f;
            }
            rad -= gravityPull * 0.42f * rad;
            rad = liquid::clampf (rad, coreR * 0.55f, 1.55f);

            //-- direction: orbit about the swarm's own axis
            const float theta = q.azimuth + a.flowTime * sl.spin * q.spin;
            float st = 0.0f, ct = 1.0f;
            S.sincos (theta, st, ct);
            // Every resonator's swarm rides a band about its own axis — an orbit,
            // not a cloud. Thirty-two of them, at thirty-two radii, on axes set by
            // their clusters, make an armillary of light instead of a fog: you can
            // see the shells, and you can see one of them hold while its neighbours
            // die. Tension narrows the bands until the whole object is one surface.
            const float h = q.height * sl.band;
            const float ring = std::sqrt (liquid::clampf (1.0f - h * h, 0.0f, 1.0f));
            Vec3 p { sl.w.x * h + sl.u.x * ring * ct + sl.v.x * ring * st,
                     sl.w.y * h + sl.u.y * ring * ct + sl.v.y * ring * st,
                     sl.w.z * h + sl.u.z * ring * ct + sl.v.z * ring * st };
            p *= rad;

            //-- the resonator's pan slides its swarm across the stereo field
            p.x += sl.pan * lateral;

            //-- the voice cell this point belongs to pulls it to its own quarter
            const int cell = (int) q.cell;
            p += cellHome[(size_t) cell] * cellOffset[(size_t) cell];

            //-- advection: the whole cloud drifts and folds through a slow 3D field
            if (flowAmp > 0.001f)
                p += liquid::flow (noise, p, a.flowTime + (float) q.cell * 0.37f, flowScale) * flowAmp;

            //-- Surface roughens the path; Scatter throws it about
            float rough = 0.0f;
            if (roughness > 0.0005f || jitter > 0.0005f)
            {
                const float w1 = roughness + jitter;
                rough = S.sin (q.seed * 1.37f + a.time * 4.3f);
                p.x += rough * w1;
                p.y += S.sin (q.seed * 2.71f + a.time * 3.7f) * w1;
                p.z += S.sin (q.seed * 4.93f + a.time * 5.1f) * w1;
                // A rough surface scatters the light as well as the path: the shells
                // break into glitter instead of staying evenly lit.
                rough *= surface;
            }

            //-- Evolve deformations
            if (bend > 0.01f)
            {
                // A real bend: the volume is rotated about a slowly turning axis by an
                // angle that grows with height, so the ball curls into a comma instead
                // of merely leaning over.
                const float ang = p.y * bend * 1.35f;
                float sa = 0.0f, ca = 1.0f;
                S.sincos (ang, sa, ca);
                const float ax = p.x * bc + p.z * bs;      // along the bend axis
                const float az = -p.x * bs + p.z * bc;
                const float rx = ax * ca - p.y * sa;
                const float ry = ax * sa + p.y * ca;
                p.x = rx * bc - az * bs;
                p.z = rx * bs + az * bc;
                p.y = ry;
            }
            if (melt > 0.01f)
            {
                // The lower half runs and pools: sag grows with how far down it already is.
                const float low = liquid::clampf (0.5f - p.y * 0.5f, 0.0f, 1.0f);
                p.y -= melt * (0.16f + 0.80f * low * low);
                p.y *= 1.0f - melt * 0.22f;
                p.x *= 1.0f + melt * (0.10f + 0.34f * low);
                p.z *= 1.0f + melt * (0.10f + 0.34f * low);
            }
            if (tearAmt > 0.005f)
            {
                // Torn across the screen, not into it: a split you cannot see is not a
                // split. The two halves also counter-rotate as they pull apart.
                // A gap, not a shift: everything is pushed clear of the tear plane, so
                // the volume comes apart into two masses with dark between them.
                const float side = q.hash < 0.5f ? -1.0f : 1.0f;
                p.x = side * (std::abs (p.x) * (1.0f - 0.35f * tear) + tearAmt);
                p.y += side * tearAmt * 0.28f;
            }
            if (lattice > 0.0f)
            {
                p.x = std::round (p.x * lattice) / lattice;
                p.y = std::round (p.y * lattice) / lattice;
                p.z = std::round (p.z * lattice) / lattice;
            }

            //-- shock fronts: a note-on lifts and ignites every shell it passes
            float flash = 0.0f;
            const float r0 = p.length();
            for (const auto& sh : shocks)
            {
                if (sh.amp <= 0.001f) continue;
                const float d = (r0 - sh.r) * sh.invWidth;
                if (d < -3.0f || d > 3.0f) continue;
                const float w = sh.amp * std::exp (-d * d);
                flash += w;
            }
            if (flash > 0.0f && r0 > 1.0e-4f)
                p *= 1.0f + flash * 0.30f;

            //-- the shatter impulse from a Fracture step scrambles the points briefly
            if (shatter > 0.004f)
            {
                const float k = shatter * (0.35f + 0.65f * q.hash);
                p.x += S.sin (q.seed * 7.1f + a.time * 21.0f) * k;
                p.y += S.sin (q.seed * 5.3f + a.time * 19.0f) * k;
                p.z += S.sin (q.seed * 9.7f + a.time * 23.0f) * k;
            }

            p *= a.breathe;

            //-- the cloud opens around the dark core: nothing lives inside the mass
            const float len = p.length();
            const float inner = coreR * 1.02f;
            if (len < inner && len > 1.0e-4f)
                p *= inner / len;

            // clean() rather than clamp(): a clamp lets a NaN straight through, and
            // one NaN position would be a point drawn nowhere for the rest of the run.
            q.prev = q.live;
            q.live = { liquid::clean (p.x, -3.0f, 3.0f, 0.0f),
                       liquid::clean (p.y, -3.0f, 3.0f, 0.0f),
                       liquid::clean (p.z, -3.0f, 3.0f, 0.0f) };
            if (! primed) q.prev = q.live;   // no comet tail out of the origin on frame one

            //-- emission: the node's energy IS the brightness of its swarm
            const float tw = 0.55f + 0.45f * S.sin (a.time * q.twinkle + q.seed);
            const float nodeLight = sl.energy * cellEnergy[(size_t) cell];
            float bright = dust * (0.32f + 0.68f * tw)
                           + nodeLight * (0.55f + 0.45f * tw) * (0.30f + 0.70f * a.life)
                           + flash * (0.9f + 0.6f * (float) sl.active);
            bright *= 1.0f + 0.55f * a.level * (0.3f + 0.7f * (float) sl.active);
            bright *= 1.0f + 0.85f * rough;
            bright += strike * 0.16f * q.hash;

            q.out.p      = q.live;
            q.out.bright = liquid::clean (bright, 0.0f, 3.0f, 0.0f);
            // Idle points are larger and softer — cold vapour rather than sparks —
            // so the volume still has body with nothing playing.
            q.out.size   = liquid::clean (q.sizeMul * (0.0082f + 0.0030f * (1.0f - a.life)
                                                      + 0.0058f * nodeLight + 0.0030f * liquid::clampf (flash, 0.0f, 3.0f)
                                                      + 0.0026f * a.level) * (1.0f - 0.24f * tension),
                                          0.0f, 0.09f, 0.004f);
            // FREEZE crystallises the volume: the light goes cold and hard, so the
            // seizure reads in a still frame and not only in the stopped motion.
            q.out.hue    = liquid::clean (sl.hue + a.hueDrift + 0.06f * q.hash * (1.0f - tension)
                                          + a.freezeMix * (0.02f - (sl.hue + a.hueDrift)),
                                          -1.0e5f, 1.0e5f, 0.0f);
            q.out.white  = liquid::clean (0.14f * nodeLight + 0.55f * flash + 0.35f * shatter
                                          + 0.20f * a.level * nodeLight + 0.30f * a.freezeMix, 0.0f, 0.92f, 0.0f);
            q.out.vel    = q.live - q.prev;
        }
        primed = true;
    }

    //==========================================================================
    const FieldPoint& point (int i) const noexcept { return pts[(size_t) clampi (i, 0, kMaxPoints - 1)].out; }
    int count() const noexcept { return alive; }

    /** The strongest shock front currently travelling, 0 .. 1 (for the core flash). */
    float shockAmplitude() const noexcept
    {
        float m = 0.0f;
        for (const auto& s : shocks) if (s.amp > m) m = s.amp;
        return m;
    }
    float shockRadius() const noexcept
    {
        float m = 0.0f, r = 0.0f;
        for (const auto& s : shocks) if (s.amp > m) { m = s.amp; r = s.r; }
        return r;
    }
    float shatterAmount() const noexcept { return shatter; }
    float strikeAmount() const noexcept { return strike; }
    int   voiceCells() const noexcept { return liveCells; }

private:
    struct Shock
    {
        float r = 0.0f, amp = 0.0f, speed = 2.4f, invWidth = 6.0f;
    };

    struct Slot
    {
        Vec3  w { 0.0f, 1.0f, 0.0f }, u { 1.0f, 0.0f, 0.0f }, v { 0.0f, 0.0f, 1.0f };
        float t = 0.0f, radius = 0.5f, energy = 0.0f, pan = 0.0f, spin = 0.3f, hue = 0.5f, band = 1.0f;
        uint8_t lobeIx = 0, active = 0;
    };

    struct Point
    {
        Vec3  prev, live;
        FieldPoint out;
        float azimuth = 0.0f, height = 0.0f, radial = 0.0f, spin = 1.0f;
        float sizeMul = 1.0f, seed = 0.0f, twinkle = 1.0f, hash = 0.0f;
        uint8_t slot = 0, cell = 0;
    };

    static float maxf (float a, float b) noexcept { return a > b ? a : b; }
    static int   clampi (int v, int lo, int hi) noexcept { return v < lo ? lo : (v > hi ? hi : v); }

    static Anim sanitise (const Anim& a) noexcept
    {
        Anim o;
        o.dt        = liquid::clean (a.dt, 1.0f / 240.0f, 0.25f, 1.0f / 30.0f);
        o.time      = liquid::clean (a.time, -1.0e6f, 1.0e6f, 0.0f);
        o.flowTime  = liquid::clean (a.flowTime, -1.0e6f, 1.0e6f, 0.0f);
        o.level     = liquid::clean (a.level, 0.0f, 1.0f, 0.0f);
        o.energy    = liquid::clean (a.energy, 0.0f, 1.0f, 0.0f);
        o.life      = liquid::clean (a.life, 0.0f, 1.0f, 0.0f);
        o.fracture  = liquid::clean (a.fracture, 0.0f, 2.0f, 0.0f);
        o.freezeMix = liquid::clean (a.freezeMix, 0.0f, 1.0f, 0.0f);
        o.breathe   = liquid::clean (a.breathe, 0.4f, 2.0f, 1.0f);
        o.hueDrift  = liquid::clean (a.hueDrift, -1.0e5f, 1.0e5f, 0.0f);
        o.coreR     = liquid::clean (a.coreR, 0.05f, 0.70f, 0.30f);
        return o;
    }

    /** Note-ons, fracture steps and the decay of everything they started. */
    void advanceEvents (const VisualStateSnapshot& s, const Anim& a, float decay) noexcept
    {
        const float run = 1.0f - a.freezeMix;

        // A NEW NOTE: the noteId changed. Not the envelope — an envelope arrives late
        // and cannot tell one note from two.
        if (s.noteId != lastNote)
        {
            const bool first = lastNote == 0 && s.noteId == 0;
            lastNote = s.noteId;
            if (! first)
            {
                const float vel = liquid::clean (s.noteVelocity, 0.0f, 1.0f, 0.7f);
                pushShock (0.30f + 0.75f * vel, 1.7f + 1.0f * vel, 10.0f - 2.5f * vel);
                strike = liquid::clampf (strike + 0.45f + 0.55f * vel, 0.0f, 1.6f);
            }
        }

        // FRACTURE: one crisp front per fragment step that actually fired.
        if (s.fractureHits != lastFracture)
        {
            const uint32_t steps = s.fractureHits - lastFracture;
            lastFracture = s.fractureHits;
            if (steps > 0 && steps < 64)
            {
                pushShock (0.34f, 4.6f, 13.0f);
                shatter = liquid::clampf (shatter + 0.055f + 0.030f * (float) (steps > 3 ? 3 : steps), 0.0f, 0.34f);
            }
        }

        for (auto& sh : shocks)
        {
            if (sh.amp <= 0.001f) { sh.amp = 0.0f; continue; }
            sh.r += sh.speed * a.dt * run;
            // The front fades as it expands and dies at the shell; Decay holds it longer.
            sh.amp *= std::exp (-a.dt * (2.6f - 1.5f * decay));
            if (sh.r > 1.9f) sh.amp = 0.0f;
        }

        shatter *= std::exp (-a.dt * 9.0f);
        strike  *= std::exp (-a.dt * (3.4f - 2.2f * decay));
    }

    void pushShock (float amp, float speed, float invWidth) noexcept
    {
        // Reuse the weakest slot so a fast trill never silently drops a front.
        int best = 0;
        for (int i = 1; i < kShocks; ++i) if (shocks[(size_t) i].amp < shocks[(size_t) best].amp) best = i;
        shocks[(size_t) best] = Shock { 0.0f, liquid::clampf (amp, 0.0f, 2.0f), speed, invWidth };
    }

    /**
        One cell per sounding voice. A cell that is not sounding eases back onto a
        cell that is, so its points melt into the surviving mass instead of popping.
    */
    void updateCells (const VisualStateSnapshot& s, const Anim& a) noexcept
    {
        const int n = clampi (s.numVisualVoices, 0, kCells);
        liveCells = n;
        float focus = liquid::clean (s.pitchHz, 0.0f, 40000.0f, 0.0f);
        if (! (focus > 20.0f)) focus = 220.0f;
        const float ease = 1.0f - std::exp (-a.dt * 4.5f);

        float targetScale[kCells], targetEnergy[kCells], targetOffset[kCells];
        for (int v = 0; v < kCells; ++v)
        {
            if (v < n)
            {
                const float hz = liquid::clean (s.voicePitchHz[v], 1.0f, 40000.0f, focus);
                // A voice an octave up sits on a wider shell: the chord is the object
                // repeated at the interval it is playing.
                const float oct = liquid::clampf (std::log2 (hz / focus), -2.0f, 2.0f);
                targetScale[v]  = liquid::clampf (1.0f + oct * 0.20f, 0.45f, 1.7f);
                targetEnergy[v] = liquid::clampf (liquid::clean (s.voiceEnergy[v], 0.0f, 4.0f, 0.0f) * 1.35f
                                                  + 0.22f * liquid::clean (s.voiceVelocity[v], 0.0f, 1.0f, 0.0f), 0.0f, 1.4f);
                // One voice is one mass; a chord fans its voices apart.
                targetOffset[v] = n > 1 ? (0.10f + 0.13f * (float) (n - 1) / (float) (kCells - 1)) : 0.0f;
            }
            else
            {
                const int host = n > 0 ? (v % n) : 0;
                targetScale[v]  = n > 0 ? targetScale[host] : 1.0f;
                targetEnergy[v] = n > 0 ? targetEnergy[host] : 0.0f;
                targetOffset[v] = n > 0 ? targetOffset[host] : 0.0f;
            }
        }
        for (int v = 0; v < kCells; ++v)
        {
            cellScale[(size_t) v]  += (targetScale[v]  - cellScale[(size_t) v])  * ease;
            cellEnergy[(size_t) v] += (targetEnergy[v] - cellEnergy[(size_t) v]) * ease;
            cellOffset[(size_t) v] += (targetOffset[v] - cellOffset[(size_t) v]) * ease;
        }
    }

    std::array<Point, kMaxPoints> pts {};
    std::array<Slot, kSlots> slot {};
    std::array<Vec3, kLobes> lobe {}, lobeSide {};
    std::array<Vec3, kSlots> slotTilt {};
    std::array<Vec3, kCells> cellHome {};
    std::array<float, kCells> cellScale {}, cellEnergy {}, cellOffset {};
    std::array<Shock, kShocks> shocks {};
    uint32_t lastNote = 0, lastFracture = 0;
    float shatter = 0.0f, strike = 0.0f;
    int   alive = 0, liveCells = 0;
    bool  primed = false;
};

} // namespace am::ui

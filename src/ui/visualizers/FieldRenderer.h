#pragma once

#include <juce_graphics/juce_graphics.h>
#include <vector>
#include "LiquidOrganism.h"

namespace am::ui
{

/**
    Additive renderer for the volumetric field.

    JUCE's software renderer has no additive blend mode, and a few thousand
    `fillEllipse` calls would cost more than the whole frame budget. So the field
    is accumulated into a plain float RGB buffer — every point of light simply adds
    energy where it lands — and the buffer is tone mapped once into a premultiplied
    ARGB image that is blitted in a single call.

    That is what makes the mass read as a volume: where points overlap, the light
    genuinely sums and blooms toward white instead of the nearest one flat-shading
    over the others, and a faint far point can sit behind a bright near one without
    punching a hole in it.

    The buffer covers the square around the glass. Its alpha is the largest of the
    three channels, so the image composites as light added to whatever is behind it:
    over a dark well it reads additive, over a pale one it reads as saturated
    contents suspended in the glass. Nothing about it assumes a dark chassis.
*/
class FieldRenderer
{
public:
    static constexpr int kLut = 257;         ///< falloff tables, indexed by d² × 256
    static constexpr int kTone = 2048;       ///< tone table, indexed by value × 256

    FieldRenderer()
    {
        for (int i = 0; i < kLut; ++i)
        {
            const float d2 = (float) i / (float) (kLut - 1);
            const float v = 1.0f - d2;
            core[(size_t) i] = v * v * (0.35f + 0.65f * v);   // tight, bright centre
            soft[(size_t) i] = v * v * v * v;                 // broad, gentle halo
            hard[(size_t) i] = d2 < 0.72f ? 1.0f : (1.0f - (d2 - 0.72f) * (1.0f / 0.28f));
        }
        for (int i = 0; i < kTone; ++i)
        {
            // Reinhard, then a gentle lift so the faint far dust is not crushed away.
            const float v = (float) i * (1.0f / 256.0f);
            const float t = v / (1.0f + v);
            tone[(size_t) i] = (uint8_t) juce::jlimit (0, 255, (int) (255.0f * std::pow (t, 0.80f) + 0.5f));
        }
    }

    /** Sizes the buffers. `w`/`h` are buffer texels; the caller scales to the glass. */
    void prepare (int w, int h)
    {
        w = juce::jlimit (8, 4096, w);
        h = juce::jlimit (8, 4096, h);
        if (w == bw && h == bh) return;
        bw = w; bh = h;
        accum.assign ((size_t) bw * (size_t) bh * 3, 0.0f);
        img = juce::Image (juce::Image::ARGB, bw, bh, false);

        // The glow plane. A cloud only reads as a volume when the light between the
        // points joins up, and a halo wide enough to do that costs more per point
        // than the point does. So the wide light is accumulated at a quarter of the
        // resolution — where a 28-pixel wash is a 7-texel splat — and sampled back
        // bilinearly while the fine plane is being tone mapped.
        gw = bw / kGlowStep + 3;
        gh = bh / kGlowStep + 3;
        glow.assign ((size_t) gw * (size_t) gh * 3, 0.0f);
        gxi.resize ((size_t) bw);
        gxw.resize ((size_t) bw);
        for (int x = 0; x < bw; ++x)
        {
            const float gc = ((float) x + 0.5f) * (1.0f / (float) kGlowStep) + (float) kGlowPad - 0.5f;
            const int i = juce::jlimit (0, gw - 2, (int) gc);
            gxi[(size_t) x] = i;
            gxw[(size_t) x] = juce::jlimit (0.0f, 1.0f, gc - (float) i);
        }
        // Where each glow texel's run of output pixels ends, so the resolve never
        // has to scan for it.
        gxe.assign ((size_t) gw, bw);
        for (int x = 0; x < bw; ++x) gxe[(size_t) gxi[(size_t) x]] = x + 1;
    }

    int width() const noexcept { return bw; }
    int height() const noexcept { return bh; }
    const juce::Image& image() const noexcept { return img; }

    /** Clears the accumulator and resets the per-frame texel budget. */
    void begin (int texelBudget) noexcept
    {
        if (! accum.empty()) std::memset (accum.data(), 0, accum.size() * sizeof (float));
        if (! glow.empty()) std::memset (glow.data(), 0, glow.size() * sizeof (float));
        budget = texelBudget;
        touched = 0;
    }

    /** True while there is still room in this frame's splat budget. */
    bool hasBudget() const noexcept { return budget > 0; }

    /** Texels touched since begin() — for profiling. */
    long long texels() const noexcept { return touched; }

    const float* coreKernel() const noexcept { return core.data(); }
    const float* softKernel() const noexcept { return soft.data(); }
    const float* hardKernel() const noexcept { return hard.data(); }

    /** Adds one disc of light. `lut` picks the falloff; colour is premultiplied by intensity. */
    void splat (float fx, float fy, float rad, float r, float g, float b, const float* lut) noexcept
    {
        splatInto (accum.data(), bw, bh, fx, fy, rad, r, g, b, lut, 0.40f);
    }

    /** Adds one wide, soft wash of light to the glow plane. Coordinates are the
        fine plane's; the quarter-resolution mapping is handled here. */
    void splatGlow (float fx, float fy, float rad, float r, float g, float b) noexcept
    {
        splatInto (glow.data(), gw, gh,
                   fx * (1.0f / (float) kGlowStep) + (float) kGlowPad,
                   fy * (1.0f / (float) kGlowStep) + (float) kGlowPad,
                   rad * (1.0f / (float) kGlowStep), r, g, b, soft.data(), 0.55f);
    }

    /**
        Tone maps the accumulator into the image, masked to the circle of radius
        `maskR` about (`cx`, `cy`) in buffer texels. Everything outside is cleared,
        so the object can never reach past the glass however hard it is driven.
    */
    void resolve (float cx, float cy, float maskR) noexcept
    {
        if (accum.empty() || ! img.isValid()) return;
        const juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);
        const float r2 = maskR * maskR;

        for (int y = 0; y < bh; ++y)
        {
            auto* out = (juce::PixelARGB*) data.getLinePointer (y);
            const float dy = (float) y + 0.5f - cy;
            const float half = r2 - dy * dy;

            int x0 = bw, x1 = bw;
            if (half > 0.0f)
            {
                const float hx = std::sqrt (half);
                x0 = juce::jlimit (0, bw, (int) (cx - hx));
                x1 = juce::jlimit (0, bw, (int) (cx + hx) + 1);
            }

            // The two glow rows this line of pixels sits between.
            const float gcy = ((float) y + 0.5f) * (1.0f / (float) kGlowStep) + (float) kGlowPad - 0.5f;
            const int gyi = juce::jlimit (0, gh - 2, (int) gcy);
            const float wy = juce::jlimit (0.0f, 1.0f, gcy - (float) gyi);
            const float iwy = 1.0f - wy;
            const float* const gr0 = glow.data() + (size_t) gyi * (size_t) gw * 3;
            const float* const gr1 = gr0 + (size_t) gw * 3;

            for (int x = 0; x < x0; ++x) out[x].setARGB (0, 0, 0, 0);
            const float* p = accum.data() + ((size_t) y * (size_t) bw + (size_t) x0) * 3;

            // Four output pixels share one pair of glow texels, so the texel reads and
            // the vertical blend are hoisted out of the run and only the horizontal
            // weight changes inside it.
            int x = x0;
            while (x < x1)
            {
                const int gi = gxi[(size_t) x];
                int xe = gxe[(size_t) gi];
                if (xe > x1) xe = x1;
                if (xe <= x) xe = x + 1;
                const float* const u = gr0 + (size_t) gi * 3;
                const float* const v = gr1 + (size_t) gi * 3;
                const float a0 = u[0] * iwy + v[0] * wy, a1 = u[1] * iwy + v[1] * wy, a2 = u[2] * iwy + v[2] * wy;
                const float b0 = u[3] * iwy + v[3] * wy, b1 = u[4] * iwy + v[4] * wy, b2 = u[5] * iwy + v[5] * wy;

                // Most of the outer volume has no glow in it at all; those runs skip
                // the interpolation entirely and fall back to the fine plane.
                const bool lit = (a0 + a1 + a2 + b0 + b1 + b2) > 0.0008f;
                for (; x < xe; ++x, p += 3)
                {
                    float vr = p[0], vg = p[1], vb = p[2];
                    if (lit)
                    {
                        const float wx = gxw[(size_t) x], iwx = 1.0f - wx;
                        vr += a0 * iwx + b0 * wx;
                        vg += a1 * iwx + b1 * wx;
                        vb += a2 * iwx + b2 * wx;
                    }
                    if (vr + vg + vb < 0.0045f) { out[x].setARGB (0, 0, 0, 0); continue; }
                    const int ir = (int) (vr * 256.0f), ig = (int) (vg * 256.0f), ib = (int) (vb * 256.0f);
                    const uint8_t r = tone[(size_t) (ir < 0 ? 0 : (ir > kTone - 1 ? kTone - 1 : ir))];
                    const uint8_t g = tone[(size_t) (ig < 0 ? 0 : (ig > kTone - 1 ? kTone - 1 : ig))];
                    const uint8_t b = tone[(size_t) (ib < 0 ? 0 : (ib > kTone - 1 ? kTone - 1 : ib))];
                    const uint8_t a = r > g ? (r > b ? r : b) : (g > b ? g : b);
                    out[x].setARGB (a, r, g, b);
                }
            }
            for (int xx = x1; xx < bw; ++xx) out[xx].setARGB (0, 0, 0, 0);
        }
    }

private:
    static constexpr int kGlowStep = 4;   ///< glow plane resolution divisor
    static constexpr int kGlowPad  = 1;   ///< texels of margin so bilinear never runs off

    void splatInto (float* base, int w, int h, float fx, float fy, float rad,
                    float r, float g, float b, const float* lut, float minRad) noexcept
    {
        if (base == nullptr || ! (std::isfinite (fx) && std::isfinite (fy) && std::isfinite (rad))) return;
        if (rad < minRad) rad = minRad;
        if (rad > 160.0f) rad = 160.0f;

        int x0 = (int) (fx - rad); if (x0 < 0) x0 = 0;
        int x1 = (int) (fx + rad) + 1; if (x1 > w) x1 = w;
        int y0 = (int) (fy - rad); if (y0 < 0) y0 = 0;
        int y1 = (int) (fy + rad) + 1; if (y1 > h) y1 = h;
        if (x0 >= x1 || y0 >= y1) return;

        budget -= (x1 - x0) * (y1 - y0);
        touched += (long long) (x1 - x0) * (long long) (y1 - y0);
        const float inv = 1.0f / (rad * rad);

        for (int y = y0; y < y1; ++y)
        {
            const float dy = (float) y + 0.5f - fy;
            const float dy2 = dy * dy * inv;
            if (dy2 >= 1.0f) continue;
            float* row = base + ((size_t) y * (size_t) w + (size_t) x0) * 3;
            for (int x = x0; x < x1; ++x, row += 3)
            {
                const float dx = (float) x + 0.5f - fx;
                const float d2 = dx * dx * inv + dy2;
                if (d2 < 1.0f)
                {
                    const float k = lut[(int) (d2 * 256.0f)];
                    row[0] += r * k;
                    row[1] += g * k;
                    row[2] += b * k;
                }
            }
        }
    }

    std::vector<float> accum, glow;
    std::vector<int> gxi, gxe;
    std::vector<float> gxw;
    int gw = 0, gh = 0;
    juce::Image img;
    int bw = 0, bh = 0;
    int budget = 0;
    long long touched = 0;
    std::array<float, (size_t) kLut> core {}, soft {}, hard {};
    std::array<uint8_t, (size_t) kTone> tone {};
};

} // namespace am::ui

#include "MasterSection.h"
#include "dev/diagnostics/SafetyMonitor.h"

namespace am
{

void MasterSection::prepare (double sampleRate, int)
{
    sr = sampleRate;
    gainSmoother.prepare (sampleRate, 20.0f);
    gainSmoother.reset (1.0f);
    dcL.prepare (sampleRate, 5.0f);
    dcR.prepare (sampleRate, 5.0f);
    releaseCoeff = (float) std::exp (-1.0 / (0.080 * sr));
    reset();
}

void MasterSection::reset()
{
    dcL.reset(); dcR.reset();
    envelope = 0.0f;
    currentReduction = 1.0f;
    dcEstimate = 0.0f;
}

void MasterSection::process (float* l, float* r, int n, const RenderContext& ctx, SafetyMonitor* safety)
{
    // 1. Non-finite scrub. A NaN anywhere would otherwise propagate forever.
    const int badL = scrubBuffer (l, n);
    const int badR = scrubBuffer (r, n);
    if ((badL + badR) > 0 && safety != nullptr)
        safety->note (SafetyEvent::NaN, Subsystem::Master, -1, badL + badR);

    // 2. Gain (dB → linear, smoothed).
    gainSmoother.setTarget (dbToGain (ctx.param (Param::masterGain)));

    int clipped = 0;
    int limited = 0;
    float dcAccum = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float g = gainSmoother.next();
        float a = dcL.process (l[i]) * g;
        float b = dcR.process (r[i]) * g;

        // 3. Hard ceiling first: anything absurd is clamped before the limiter sees it.
        if (a > kHardCeiling || a < -kHardCeiling || b > kHardCeiling || b < -kHardCeiling)
        {
            a = juce::jlimit (-kHardCeiling, kHardCeiling, a);
            b = juce::jlimit (-kHardCeiling, kHardCeiling, b);
            ++clipped;
        }

        // 4. Fast peak limiter.
        // Instant attack: a strike transient is shorter than any detector time constant, so the gain is
        // computed from the current sample (no lookahead latency) and released over 80 ms.
        const float peak = std::max (std::abs (a), std::abs (b));
        envelope = peak > envelope ? peak : peak + releaseCoeff * (envelope - peak);
        float reduction = 1.0f;
        if (envelope > kCeiling)
        {
            reduction = kCeiling / envelope;
            ++limited;
        }
        a *= reduction;
        b *= reduction;
        currentReduction = reduction;

        // 5. Final safety clip (cannot trigger after the instant-attack limiter, kept as a hard guarantee).
        if (a > 1.0f || a < -1.0f || b > 1.0f || b < -1.0f) ++limited;
        l[i] = juce::jlimit (-1.0f, 1.0f, a);
        r[i] = juce::jlimit (-1.0f, 1.0f, b);
        dcAccum += l[i] + r[i];
    }

    if (safety != nullptr)
    {
        if (clipped > 0) safety->note (SafetyEvent::HardClip, Subsystem::Master, -1, clipped);
        if (limited > 0) safety->note (SafetyEvent::LimiterEngaged, Subsystem::Master, -1, limited);
        // Slow running mean: a block of a low sine is not DC, a sustained offset is.
        const float blockMean = dcAccum / (float) (2 * std::max (1, n));
        const float alpha = juce::jlimit (0.0f, 1.0f, (float) n / (float) (0.3 * sr));
        dcEstimate += (blockMean - dcEstimate) * alpha;
        if (std::abs (dcEstimate) > 0.2f) safety->note (SafetyEvent::DCOffset, Subsystem::Master);
    }
}

} // namespace am

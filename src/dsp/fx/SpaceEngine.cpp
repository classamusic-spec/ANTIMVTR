#include "SpaceEngine.h"
#include "dev/diagnostics/Diagnostics.h"

namespace am
{

namespace
{
    constexpr int kIdleBlocksBeforeRelease = 96;   ///< ~0.25 s at 48 k / 128
}

void SpaceEngine::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    maxBlock = std::max (16, std::min (maxBlockSize, kMaxBlockSize));

    rack.prepare (sampleRate, maxBlock);
    dryL.assign ((size_t) maxBlock, 0.0f);
    dryR.assign ((size_t) maxBlock, 0.0f);

    // ~6 ms to fade the wet path across a Space change.
    changeStep = 1.0f / std::max (1.0f, (float) (sampleRate * 0.006));

    reset();
}

void SpaceEngine::reset()
{
    rack.reset();
    dryRamp.reset (1.0f);
    wetRamp.reset (0.0f);
    routing = SpacePresets::Routing {};
    pendingRouting = routing;
    routingChanging = false;
    routingInitialised = false;
    changeGain = changeTarget = 1.0f;
    idleBlocks = 0;
    firstMixBlock = true;
    blockDryEnergy = blockWetEnergy = 0.0;
    lastActivity = 0.0f;
}

void SpaceEngine::process (float* l, float* r, int n, const RenderContext& ctx)
{
    if (l == nullptr || r == nullptr || n <= 0) return;

    juce::ScopedNoDenormals noDenormals;
    blockDryEnergy = blockWetEnergy = 0.0;

    for (int pos = 0; pos < n; pos += maxBlock)
        processChunk (l + pos, r + pos, std::min (maxBlock, n - pos), ctx);

    const float raw = (float) (blockWetEnergy / (blockWetEnergy + blockDryEnergy + 1.0e-12));
    lastActivity += (juce::jlimit (0.0f, 1.0f, raw) - lastActivity) * 0.35f;
}

void SpaceEngine::processChunk (float* l, float* r, int n, const RenderContext& ctx)
{
    // ---- Space type → module order (and the shimmer configuration)
    const auto wanted = SpacePresets::routing (ctx.choice (Param::spaceType));
    if (! routingInitialised)
    {
        routing = wanted;
        pendingRouting = wanted;
        routingInitialised = true;
    }
    else if (wanted != (routingChanging ? pendingRouting : routing))
    {
        pendingRouting = wanted;
        routingChanging = true;
        changeTarget = 0.0f;
    }

    // ---- macros
    float dryGain = 1.0f, wetGain = 0.0f;
    fx::equalPower (ctx.param (Param::spaceMix), dryGain, wetGain);

    // ---- fully dry: bypass the rack completely (and release its tails)
    if (wetGain <= 0.0f && wetRamp.getCurrent() <= 0.0f && ! routingChanging)
    {
        dryRamp.reset (1.0f);
        firstMixBlock = false;        // a dry block still counts as "something was rendered"
        for (int i = 0; i < n; ++i) blockDryEnergy += (double) l[i] * l[i] + (double) r[i] * r[i];
        if (idleBlocks < kIdleBlocksBeforeRelease && ++idleBlocks == kIdleBlocksBeforeRelease)
            rack.reset();
        return;
    }
    idleBlocks = 0;

    std::copy (l, l + n, dryL.begin());
    std::copy (r, r + n, dryR.begin());

    rack.process (l, r, n, ctx, routing);

    // ---- a poisoned wet path must never reach the master
    const int bad = scrubBuffer (l, n) + scrubBuffer (r, n);
    if (bad > 0)
    {
        rack.reset();
        if (ctx.diagnostics != nullptr)
            ctx.diagnostics->safety.note (SafetyEvent::NaN, Subsystem::Space, -1, bad);
    }

    if (rack.feedbackClamped() && ctx.diagnostics != nullptr)
        ctx.diagnostics->safety.note (SafetyEvent::FeedbackClamp, Subsystem::Space);

    // ---- equal-power dry/wet, with the Space-change fade folded into the wet
    if (firstMixBlock)
    {
        dryRamp.reset (dryGain);      // very first block after reset: nothing to crossfade from
        wetRamp.reset (wetGain);
        firstMixBlock = false;
    }
    dryRamp.setTarget (dryGain, n);
    wetRamp.setTarget (wetGain, n);

    for (int i = 0; i < n; ++i)
    {
        if (changeGain < changeTarget)      changeGain = std::min (changeTarget, changeGain + changeStep);
        else if (changeGain > changeTarget) changeGain = std::max (changeTarget, changeGain - changeStep);

        const float d = dryRamp.next();
        const float w = wetRamp.next() * changeGain;
        const float dl = dryL[(size_t) i], dr = dryR[(size_t) i];
        const float wl = l[i] * w, wr = r[i] * w;

        l[i] = dl * d + wl;
        r[i] = dr * d + wr;

        blockDryEnergy += (double) (dl * d) * (dl * d) + (double) (dr * d) * (dr * d);
        blockWetEnergy += (double) wl * wl + (double) wr * wr;
    }

    // ---- rack output limiter (post-mix: it protects whatever follows SPACE)
    rack.processOutput (l, r, n);

    if (routingChanging && changeGain <= 0.0f)
    {
        routing = pendingRouting;
        routingChanging = false;
        changeTarget = 1.0f;
    }
}

} // namespace am

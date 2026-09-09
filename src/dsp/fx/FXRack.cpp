#include "FXRack.h"

namespace am
{

using Slot = SpacePresets::Slot;

void FXRack::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    maxBlock = std::max (16, std::min (maxBlockSize, kMaxBlockSize));

    distortion.prepare (sampleRate, maxBlock);
    chorus.prepare (sampleRate, maxBlock);
    shifter.prepare (sampleRate, maxBlock);
    delay.prepare (sampleRate, maxBlock);
    granular.prepare (sampleRate, maxBlock);
    diffusion.prepare (sampleRate, maxBlock);
    reverb.prepare (sampleRate, maxBlock);
    eq.prepare (sampleRate, maxBlock);
    compressor.prepare (sampleRate, maxBlock);
    limiter.prepare (sampleRate, maxBlock);
    tilt.prepare (sampleRate);

    for (auto& g : gates) g.prepare (sampleRate, 8.0f);

    tmpL.assign ((size_t) maxBlock, 0.0f);
    tmpR.assign ((size_t) maxBlock, 0.0f);

    reset();
}

void FXRack::reset()
{
    distortion.reset();
    chorus.reset();
    shifter.reset();
    delay.reset();
    granular.reset();
    diffusion.reset();
    reverb.reset();
    eq.reset();
    compressor.reset();
    limiter.reset();
    tilt.reset();

    for (auto& g : gates) g.snapTo (false);
    idleBlocks.fill (kIdleBlocksBeforeReset);
    clamped = false;
    delayNeedsSnap = true;
    currentTilt = 0.0f;
    tiltActive = false;
}

//==============================================================================
void FXRack::updateParameters (const RenderContext& ctx, const SpacePresets::Routing& routing)
{
    const float size = ctx.param (Param::spaceSize);
    const float tone = ctx.param (Param::spaceTone);
    const float feedback = fx::clampf (ctx.param (Param::spaceFeedback), 0.0f, 1.0f);
    const float scale = sizeScale (size);
    const float tilt01 = toneTilt (tone);

    currentTilt = tilt01;
    tiltActive = std::abs (tilt01) > 0.01f;

    // ---- gates (module enables)
    gates[(int) Slot::Distortion].setEnabled (ctx.flag (Param::spaceDistOn));
    gates[(int) Slot::Chorus]    .setEnabled (ctx.flag (Param::spaceChorusOn));
    gates[(int) Slot::Shifter]   .setEnabled (ctx.flag (Param::spaceShiftOn));
    gates[(int) Slot::Delay]     .setEnabled (ctx.flag (Param::spaceDelayOn));
    gates[(int) Slot::Granular]  .setEnabled (ctx.flag (Param::spaceGrainOn));
    gates[(int) Slot::Diffusion] .setEnabled (ctx.flag (Param::spaceDiffuseOn));
    gates[(int) Slot::Reverb]    .setEnabled (ctx.flag (Param::spaceReverbOn));
    gates[(int) Slot::Compressor].setEnabled (ctx.flag (Param::spaceCompOn));
    gates[(int) Slot::Limiter]   .setEnabled (ctx.flag (Param::spaceLimiterOn));

    // ---- distortion
    distortion.setParams (ctx.choice (Param::spaceDistMode),
                          ctx.param (Param::spaceDistDrive),
                          ctx.param (Param::spaceDistMix));

    // ---- chorus: bigger spaces breathe deeper and a little slower
    chorus.setParams (ctx.param (Param::spaceChorusRate) / std::sqrt (scale),
                      ctx.param (Param::spaceChorusDepth),
                      ctx.param (Param::spaceChorusMix),
                      scale);

    // ---- delay: FEEDBACK scales regeneration, TONE darkens/brightens the loop
    const float delayFeedback = ctx.param (Param::spaceDelayFeedback) * (0.45f + 1.15f * feedback);
    const float delayTone = fx::clampf (ctx.param (Param::spaceDelayTone) + 0.4f * tilt01, 0.0f, 1.0f);
    delay.setParams (ctx.param (Param::spaceDelayTime),
                     ctx.flag (Param::spaceDelaySync),
                     ctx.transport.bpm,
                     delayFeedback,
                     delayTone,
                     ctx.param (Param::spaceDelayMix),
                     scale,
                     delayNeedsSnap);
    delayNeedsSnap = false;

    // ---- granular: FEEDBACK becomes cloud regeneration
    granular.setParams (ctx.param (Param::spaceGrainSize),
                        ctx.param (Param::spaceGrainDensity),
                        ctx.param (Param::spaceGrainPitch),
                        ctx.param (Param::spaceGrainMix),
                        feedback * 0.95f,
                        scale);

    shifter.setParams (ctx.param (Param::spaceShiftAmount), ctx.param (Param::spaceShiftMix));
    diffusion.setParams (ctx.param (Param::spaceDiffuseAmount), scale);

    reverb.setParams (ctx.param (Param::spaceReverbSize),
                      ctx.param (Param::spaceReverbDecay),
                      ctx.param (Param::spaceReverbDamp),
                      ctx.param (Param::spaceReverbPredelay),
                      ctx.param (Param::spaceReverbMod),
                      ctx.param (Param::spaceReverbMix),
                      scale,
                      tilt01,
                      feedback,
                      routing.shimmerAmount,
                      routing.shimmerSemitones);

    eq.setParams (ctx.param (Param::spaceEqLow), ctx.param (Param::spaceEqMid), ctx.param (Param::spaceEqHigh));
    gates[(int) Slot::EQ].setEnabled (! eq.isNeutral());

    compressor.setParams (ctx.param (Param::spaceCompAmount));

    // ---- feedback safety: only report paths that are actually running
    clamped = (gates[(int) Slot::Delay].active()    && delay.feedbackClamped())
           || (gates[(int) Slot::Granular].active() && granular.feedbackClamped())
           || (gates[(int) Slot::Reverb].active()   && reverb.feedbackClamped());
}

//==============================================================================
void FXRack::process (float* l, float* r, int n, const RenderContext& ctx, const SpacePresets::Routing& routing)
{
    if (n <= 0) return;
    updateParameters (ctx, routing);

    bool tiltDone = false;
    for (int i = 0; i < kNumSlots; ++i)
    {
        const auto slot = (Slot) juce::jlimit (0, kNumSlots - 1, (int) routing.order[(size_t) i]);

        if (! tiltDone && (slot == Slot::EQ || slot == Slot::Compressor || slot == Slot::Limiter))
        {
            applyTilt (l, r, n);
            tiltDone = true;
        }
        if (slot != Slot::Limiter) runSlot (slot, l, r, n);   // the limiter runs after the mix
    }

    if (! tiltDone) applyTilt (l, r, n);
}

void FXRack::processOutput (float* l, float* r, int n)
{
    if (n > 0) runSlot (Slot::Limiter, l, r, n);
}

void FXRack::applyTilt (float* l, float* r, int n)
{
    if (! tiltActive) return;
    tilt.setTilt (currentTilt);
    for (int i = 0; i < n; ++i)
    {
        l[i] = tilt.process (0, l[i]);
        r[i] = tilt.process (1, r[i]);
    }
}

void FXRack::runSlot (Slot slot, float* l, float* r, int n)
{
    auto& gate = gates[(int) slot];

    if (gate.closed())
    {
        // Fully bypassed: release the module's tail once, then leave it alone.
        auto& idle = idleBlocks[(int) slot];
        if (idle < kIdleBlocksBeforeReset && ++idle == kIdleBlocksBeforeReset)
            resetSlot (slot);
        return;
    }

    idleBlocks[(int) slot] = 0;

    if (gate.value() >= 1.0f && gate.active())
    {
        processSlot (slot, l, r, n);      // fully open: no crossfade needed
        return;
    }

    std::copy (l, l + n, tmpL.begin());
    std::copy (r, r + n, tmpR.begin());
    processSlot (slot, l, r, n);

    for (int i = 0; i < n; ++i)
    {
        const float g = gate.next();
        l[i] = tmpL[(size_t) i] + g * (l[i] - tmpL[(size_t) i]);
        r[i] = tmpR[(size_t) i] + g * (r[i] - tmpR[(size_t) i]);
    }
}

void FXRack::processSlot (Slot slot, float* l, float* r, int n)
{
    switch (slot)
    {
        case Slot::Distortion: distortion.process (l, r, n); break;
        case Slot::Chorus:     chorus.process (l, r, n); break;
        case Slot::Shifter:    shifter.process (l, r, n); break;
        case Slot::Delay:      delay.process (l, r, n); break;
        case Slot::Granular:   granular.process (l, r, n); break;
        case Slot::Diffusion:  diffusion.process (l, r, n); break;
        case Slot::Reverb:     reverb.process (l, r, n); break;
        case Slot::EQ:         eq.process (l, r, n); break;
        case Slot::Compressor: compressor.process (l, r, n); break;
        case Slot::Limiter:    limiter.process (l, r, n); break;
        default: break;
    }
}

void FXRack::resetSlot (Slot slot)
{
    switch (slot)
    {
        case Slot::Distortion: distortion.reset(); break;
        case Slot::Chorus:     chorus.reset(); break;
        case Slot::Shifter:    shifter.reset(); break;
        case Slot::Delay:      delay.reset(); delayNeedsSnap = true; break;
        case Slot::Granular:   granular.reset(); break;
        case Slot::Diffusion:  diffusion.reset(); break;
        case Slot::Reverb:     reverb.reset(); break;
        case Slot::EQ:         eq.reset(); break;
        case Slot::Compressor: compressor.reset(); break;
        case Slot::Limiter:    limiter.reset(); break;
        default: break;
    }
}

} // namespace am

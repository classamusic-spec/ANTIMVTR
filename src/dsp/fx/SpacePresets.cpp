#include "SpacePresets.h"

namespace am::SpacePresets
{

namespace
{
    /** One curated rack configuration, in natural parameter units. */
    struct RackConfig
    {
        const char* displayName;
        const char* text;

        bool  distOn;    int distMode; float distDrive, distMix;
        bool  chorusOn;  float chorusRate, chorusDepth, chorusMix;
        bool  delayOn;   float delayTime; bool delaySync; float delayFeedback, delayTone, delayMix;
        bool  grainOn;   float grainSize, grainDensity, grainPitch, grainMix;
        bool  shiftOn;   float shiftAmount, shiftMix;
        bool  diffuseOn; float diffuseAmount;
        bool  reverbOn;  float reverbSize, reverbDecay, reverbDamp, reverbPredelay, reverbMod, reverbMix;
        float eqLow, eqMid, eqHigh;
        bool  compOn;    float compAmount;
        bool  limiterOn;
    };

    // Delay divisions: index / 12 selects a musical value (see fx::Delay).
    constexpr float kEighth  = 5.0f / 12.0f;    // 1/8
    constexpr float kDotted8 = 7.0f / 12.0f;    // 1/8 dotted
    constexpr float kFree28ms = 0.203f;         // free-running comb length

    constexpr RackConfig kConfigs[NumTypes]
    {
        //  NEBULA — the house sound: soft, wide, always moving.
        {
            "NEBULA", "Wide soft cloud — slow chorus and diffusion feeding an evolving hall",
            false, 0, 0.25f, 1.0f,
            true,  0.22f, 0.55f, 0.45f,
            false, kEighth, true, 0.35f, 0.5f, 0.25f,
            false, 0.4f, 0.5f, 0.0f, 0.3f,
            false, 0.0f, 0.3f,
            true,  0.45f,
            true,  0.72f, 0.62f, 0.42f, 18.0f, 0.55f, 0.85f,
            -1.0f, -1.5f, 1.5f,
            false, 0.3f,
            true
        },
        //  VOID — no walls, no light.
        {
            "VOID", "Enormous dark cavern — heavy damping, long decay, glued together",
            false, 0, 0.25f, 1.0f,
            false, 0.15f, 0.3f, 0.3f,
            false, kEighth, true, 0.3f, 0.4f, 0.25f,
            false, 0.4f, 0.5f, 0.0f, 0.3f,
            false, 0.0f, 0.3f,
            true,  0.70f,
            true,  1.0f, 0.88f, 0.78f, 45.0f, 0.30f, 0.95f,
            2.5f, -2.0f, -6.0f,
            true,  0.35f,
            true
        },
        //  CHAMBER — a real room, believable and short.
        {
            "CHAMBER", "Small bright room — early reflections, short natural tail",
            false, 0, 0.25f, 1.0f,
            false, 0.3f, 0.3f, 0.3f,
            false, kEighth, true, 0.25f, 0.5f, 0.2f,
            false, 0.4f, 0.5f, 0.0f, 0.3f,
            false, 0.0f, 0.3f,
            false, 0.25f,
            true,  0.22f, 0.30f, 0.45f, 6.0f, 0.15f, 0.70f,
            -1.5f, 0.5f, 1.0f,
            false, 0.25f,
            true
        },
        //  ORBIT — rhythm from regeneration.
        {
            "ORBIT", "Tempo-synced ping-pong delays regenerating into a medium room",
            false, 0, 0.25f, 1.0f,
            false, 0.3f, 0.3f, 0.3f,
            true,  kDotted8, true, 0.62f, 0.45f, 0.50f,
            false, 0.4f, 0.5f, 0.0f, 0.3f,
            false, 0.0f, 0.3f,
            false, 0.25f,
            true,  0.45f, 0.40f, 0.50f, 12.0f, 0.25f, 0.32f,
            -1.0f, 0.0f, 1.0f,
            true,  0.30f,
            true
        },
        //  DREAM — soft, pitched, slightly unreal.
        {
            "DREAM", "Soft octave-tinted tails — grains and chorus drifting through a long hall",
            false, 0, 0.25f, 1.0f,
            true,  0.35f, 0.45f, 0.35f,
            false, kEighth, true, 0.3f, 0.5f, 0.25f,
            true,  0.55f, 0.50f, 12.0f, 0.30f,
            false, 0.0f, 0.3f,
            true,  0.35f,
            true,  0.80f, 0.70f, 0.30f, 25.0f, 0.70f, 0.80f,
            -2.0f, -1.0f, 2.5f,
            false, 0.25f,
            true
        },
        //  MACHINE — saturation and metal.
        {
            "MACHINE", "Driven tube saturation into short metallic combs and a tight plate",
            true,  1, 0.55f, 0.90f,
            false, 0.3f, 0.3f, 0.3f,
            true,  kFree28ms, false, 0.68f, 0.62f, 0.45f,
            false, 0.4f, 0.5f, 0.0f, 0.3f,
            true,  0.12f, 0.35f,
            false, 0.2f,
            true,  0.30f, 0.28f, 0.60f, 3.0f, 0.10f, 0.25f,
            -1.0f, 2.0f, -1.0f,
            true,  0.50f,
            true
        },
        //  SHIMMER — the tail climbs forever.
        {
            "SHIMMER", "Octave-up regeneration inside the reverb — endless rising tails",
            false, 0, 0.25f, 1.0f,
            true,  0.18f, 0.32f, 0.25f,
            false, kEighth, true, 0.3f, 0.5f, 0.25f,
            false, 0.4f, 0.5f, 12.0f, 0.3f,
            false, 0.0f, 0.3f,
            true,  0.30f,
            true,  0.85f, 0.75f, 0.25f, 20.0f, 0.50f, 0.90f,
            -3.0f, -1.0f, 3.0f,
            false, 0.25f,
            true
        },
        //  DUST — matter falling apart into particles.
        {
            "DUST", "Granular clouds smeared through diffusion into a mid-size space",
            false, 0, 0.25f, 1.0f,
            false, 0.3f, 0.3f, 0.3f,
            false, kEighth, true, 0.3f, 0.5f, 0.25f,
            true,  0.28f, 0.72f, 0.0f, 0.55f,
            false, 0.0f, 0.3f,
            true,  0.80f,
            true,  0.60f, 0.55f, 0.50f, 8.0f, 0.40f, 0.50f,
            -2.0f, 0.0f, 1.0f,
            true,  0.25f,
            true
        }
    };

    inline int safeType (int type) noexcept
    {
        return type < 0 ? 0 : (type >= NumTypes ? NumTypes - 1 : type);
    }

    inline void set (ParamValues& values, Param p, float value) noexcept
    {
        values[(size_t) paramIndex (p)] = ParameterRegistry::get (p).clampValue (value);
    }

    inline void setFlag (ParamValues& values, Param p, bool on) noexcept
    {
        set (values, p, on ? 1.0f : 0.0f);
    }
}

int count() noexcept { return NumTypes; }

const char* name (int type) noexcept { return kConfigs[safeType (type)].displayName; }

const char* description (int type) noexcept { return kConfigs[safeType (type)].text; }

Routing routing (int type) noexcept
{
    Routing r;
    switch (safeType (type))
    {
        case Machine:
            // Distortion straight into the comb, then the shifter smears the metal.
            r.order = { { (uint8_t) Slot::Distortion, (uint8_t) Slot::Delay, (uint8_t) Slot::Shifter,
                          (uint8_t) Slot::Chorus, (uint8_t) Slot::Granular, (uint8_t) Slot::Diffusion,
                          (uint8_t) Slot::Reverb, (uint8_t) Slot::EQ, (uint8_t) Slot::Compressor,
                          (uint8_t) Slot::Limiter } };
            break;

        case Dust:
            // Granulate first, then smear the grains before they reach the room.
            r.order = { { (uint8_t) Slot::Granular, (uint8_t) Slot::Diffusion, (uint8_t) Slot::Distortion,
                          (uint8_t) Slot::Chorus, (uint8_t) Slot::Shifter, (uint8_t) Slot::Delay,
                          (uint8_t) Slot::Reverb, (uint8_t) Slot::EQ, (uint8_t) Slot::Compressor,
                          (uint8_t) Slot::Limiter } };
            break;

        case Shimmer:
            r.shimmerAmount = 0.85f;
            r.shimmerSemitones = 12.0f;
            break;

        case Dream:
            r.shimmerAmount = 0.28f;
            r.shimmerSemitones = 12.0f;
            break;

        default:
            break;
    }
    return r;
}

void apply (int type, ParamValues& values)
{
    const auto& c = kConfigs[safeType (type)];

    setFlag (values, Param::spaceDistOn, c.distOn);
    set (values, Param::spaceDistMode, (float) c.distMode);
    set (values, Param::spaceDistDrive, c.distDrive);
    set (values, Param::spaceDistMix, c.distMix);

    setFlag (values, Param::spaceChorusOn, c.chorusOn);
    set (values, Param::spaceChorusRate, c.chorusRate);
    set (values, Param::spaceChorusDepth, c.chorusDepth);
    set (values, Param::spaceChorusMix, c.chorusMix);

    setFlag (values, Param::spaceDelayOn, c.delayOn);
    set (values, Param::spaceDelayTime, c.delayTime);
    setFlag (values, Param::spaceDelaySync, c.delaySync);
    set (values, Param::spaceDelayFeedback, c.delayFeedback);
    set (values, Param::spaceDelayTone, c.delayTone);
    set (values, Param::spaceDelayMix, c.delayMix);

    setFlag (values, Param::spaceGrainOn, c.grainOn);
    set (values, Param::spaceGrainSize, c.grainSize);
    set (values, Param::spaceGrainDensity, c.grainDensity);
    set (values, Param::spaceGrainPitch, c.grainPitch);
    set (values, Param::spaceGrainMix, c.grainMix);

    setFlag (values, Param::spaceShiftOn, c.shiftOn);
    set (values, Param::spaceShiftAmount, c.shiftAmount);
    set (values, Param::spaceShiftMix, c.shiftMix);

    setFlag (values, Param::spaceDiffuseOn, c.diffuseOn);
    set (values, Param::spaceDiffuseAmount, c.diffuseAmount);

    setFlag (values, Param::spaceReverbOn, c.reverbOn);
    set (values, Param::spaceReverbSize, c.reverbSize);
    set (values, Param::spaceReverbDecay, c.reverbDecay);
    set (values, Param::spaceReverbDamp, c.reverbDamp);
    set (values, Param::spaceReverbPredelay, c.reverbPredelay);
    set (values, Param::spaceReverbMod, c.reverbMod);
    set (values, Param::spaceReverbMix, c.reverbMix);

    set (values, Param::spaceEqLow, c.eqLow);
    set (values, Param::spaceEqMid, c.eqMid);
    set (values, Param::spaceEqHigh, c.eqHigh);

    setFlag (values, Param::spaceCompOn, c.compOn);
    set (values, Param::spaceCompAmount, c.compAmount);

    setFlag (values, Param::spaceLimiterOn, c.limiterOn);
}

} // namespace am::SpacePresets

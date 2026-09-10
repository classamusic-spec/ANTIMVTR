#include "Fragment.h"

namespace am
{

namespace
{
    inline float clampf (float v, float lo, float hi) noexcept
    {
        return ! std::isfinite (v) ? lo : (v < lo ? lo : (v > hi ? hi : v));
    }

    inline float readFloat (const juce::var& obj, const char* key, float fallback, float lo, float hi)
    {
        if (auto* o = obj.getDynamicObject())
            if (o->hasProperty (key))
                return clampf ((float) (double) o->getProperty (key), lo, hi);
        return clampf (fallback, lo, hi);
    }

    inline int readInt (const juce::var& obj, const char* key, int fallback, int lo, int hi)
    {
        if (auto* o = obj.getDynamicObject())
            if (o->hasProperty (key))
                return juce::jlimit (lo, hi, (int) o->getProperty (key));
        return juce::jlimit (lo, hi, fallback);
    }
}

//==============================================================================
FractureTable FractureTable::makeDefault()
{
    FractureTable t;
    t.numFragments = 16;
    t.numSteps = 8;

    // Fragments: rising delays across the spectrum, octave / fifth pitch terraces,
    // alternating pan, gentle feedback on the upper bands.
    static constexpr float kPitchCycle[8] = { 0.0f, 0.0f, 12.0f, 0.0f, 7.0f, 12.0f, 0.0f, 19.0f };

    for (int f = 0; f < kMaxFractureFragments; ++f)
    {
        const float u = (float) f / (float) (kMaxFractureFragments - 1); // 0 … 1 low → high
        auto& fr = t.fragments[(size_t) f];
        fr.pitch       = kPitchCycle[f & 7];
        fr.delay       = 0.10f + 0.50f * u * u;             // low bands stay tight, highs smear
        fr.pan         = ((f & 1) == 0 ? -1.0f : 1.0f) * (0.15f + 0.6f * u);
        fr.decay       = 0.50f + 0.35f * u;
        fr.probability = 1.0f;
        fr.feedback    = 0.40f + 0.35f * u;                 // every band can ring
        fr.spread      = 0.25f + 0.75f * u;
        fr.gain        = 1.0f;
    }

    // Steps: a driving 8-step pattern that alternates low/high halves of the spectrum.
    for (int s = 0; s < kMaxSequencerSteps; ++s)
    {
        auto& st = t.steps[(size_t) s];
        const bool downbeat = (s % 4) == 0;
        st.mask        = downbeat ? 0xFFFFFFFFu : ((s & 1) ? 0xFFFF0000u : 0x0000FFFFu);
        st.gate        = downbeat ? 1.0f : ((s & 1) ? 0.75f : 0.9f);
        st.pitch       = downbeat ? 0.0f : ((s % 8) == 6 ? 12.0f : 0.0f);
        st.pan         = ((s & 2) == 0 ? -0.25f : 0.25f);
        st.gain        = 1.0f;
        st.probability = downbeat ? 1.0f : 0.85f;
        st.evolve      = (float) (s % 4) * 0.25f;
        st.shape       = (float) (s % 8) / 7.0f;
    }

    return t;
}

//==============================================================================
FractureTable FractureTable::fromVar (const juce::var& v)
{
    auto t = makeDefault();

    if (auto* root = v.getDynamicObject())
    {
        t.numFragments = readInt (v, "numFragments", t.numFragments, 1, kMaxFractureFragments);
        t.numSteps     = readInt (v, "numSteps",     t.numSteps,     1, kMaxSequencerSteps);

        if (auto* frags = root->getProperty ("fragments").getArray())
        {
            const int n = juce::jmin ((int) frags->size(), kMaxFractureFragments);
            for (int i = 0; i < n; ++i)
            {
                const auto& e = frags->getReference (i);
                if (e.getDynamicObject() == nullptr) continue;
                auto& fr = t.fragments[(size_t) i];
                fr.pitch       = readFloat (e, "pitch",       fr.pitch,       -48.0f, 48.0f);
                fr.delay       = readFloat (e, "delay",       fr.delay,         0.0f,  1.0f);
                fr.pan         = readFloat (e, "pan",         fr.pan,          -1.0f,  1.0f);
                fr.decay       = readFloat (e, "decay",       fr.decay,         0.0f,  1.0f);
                fr.probability = readFloat (e, "probability", fr.probability,   0.0f,  1.0f);
                fr.feedback    = readFloat (e, "feedback",    fr.feedback,      0.0f,  1.0f);
                fr.spread      = readFloat (e, "spread",      fr.spread,        0.0f,  1.0f);
                fr.gain        = readFloat (e, "gain",        fr.gain,          0.0f,  2.0f);
            }
        }

        if (auto* stepArray = root->getProperty ("steps").getArray())
        {
            const int n = juce::jmin ((int) stepArray->size(), kMaxSequencerSteps);
            for (int i = 0; i < n; ++i)
            {
                const auto& e = stepArray->getReference (i);
                if (e.getDynamicObject() == nullptr) continue;
                auto& st = t.steps[(size_t) i];
                if (auto* o = e.getDynamicObject())
                    if (o->hasProperty ("mask"))
                        st.mask = (uint32_t) (juce::int64) o->getProperty ("mask");
                st.gate        = readFloat (e, "gate",        st.gate,        0.0f, 1.0f);
                st.pitch       = readFloat (e, "pitch",       st.pitch,     -48.0f, 48.0f);
                st.pan         = readFloat (e, "pan",         st.pan,        -1.0f, 1.0f);
                st.gain        = readFloat (e, "gain",        st.gain,        0.0f, 2.0f);
                st.probability = readFloat (e, "probability", st.probability, 0.0f, 1.0f);
                st.evolve      = readFloat (e, "evolve",      st.evolve,      0.0f, 1.0f);
                st.shape       = readFloat (e, "shape",       st.shape,       0.0f, 1.0f);
            }
        }
    }

    return t;
}

juce::var FractureTable::toVar() const
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("numFragments", numFragments);
    root->setProperty ("numSteps", numSteps);

    juce::Array<juce::var> frags;
    for (int i = 0; i < kMaxFractureFragments; ++i)
    {
        const auto& fr = fragments[(size_t) i];
        auto* o = new juce::DynamicObject();
        o->setProperty ("pitch", fr.pitch);
        o->setProperty ("delay", fr.delay);
        o->setProperty ("pan", fr.pan);
        o->setProperty ("decay", fr.decay);
        o->setProperty ("probability", fr.probability);
        o->setProperty ("feedback", fr.feedback);
        o->setProperty ("spread", fr.spread);
        o->setProperty ("gain", fr.gain);
        frags.add (juce::var (o));
    }
    root->setProperty ("fragments", frags);

    juce::Array<juce::var> stepArray;
    for (int i = 0; i < kMaxSequencerSteps; ++i)
    {
        const auto& st = steps[(size_t) i];
        auto* o = new juce::DynamicObject();
        o->setProperty ("mask", (juce::int64) st.mask);
        o->setProperty ("gate", st.gate);
        o->setProperty ("pitch", st.pitch);
        o->setProperty ("pan", st.pan);
        o->setProperty ("gain", st.gain);
        o->setProperty ("probability", st.probability);
        o->setProperty ("evolve", st.evolve);
        o->setProperty ("shape", st.shape);
        stepArray.add (juce::var (o));
    }
    root->setProperty ("steps", stepArray);

    return juce::var (root);
}

} // namespace am

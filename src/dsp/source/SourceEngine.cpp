#include "SourceEngine.h"

namespace am
{

SourceEngine::SourceEngine()
{
    sources[(size_t) SourceType::Wave] = std::make_unique<WaveSource>();
    // Dust, Impact, Sample and Gesture are registered by their modules in later phases.
}

Param SourceEngine::levelParam (SourceType t) noexcept
{
    switch (t)
    {
        case SourceType::Wave:    return Param::waveLevel;
        case SourceType::Dust:    return Param::dustLevel;
        case SourceType::Impact:  return Param::impactLevel;
        case SourceType::Sample:  return Param::sampleLevel;
        case SourceType::Gesture: return Param::gestureLevel;
        default:                  return Param::waveLevel;
    }
}

void SourceEngine::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    maxBlock = maxBlockSize;
    for (auto& s : sources)
        if (s != nullptr) s->prepare (sampleRate, maxBlockSize);
}

void SourceEngine::reset()
{
    gate = false;
    for (auto& s : sources)
        if (s != nullptr) s->reset();
}

void SourceEngine::noteOn (const NoteState& note, const ParamValues& params)
{
    gate = true;
    selected = paramChoice (params, Param::sourceSelected);
    layer = paramChoice (params, Param::sourceMode) == 1;

    for (size_t i = 0; i < sources.size(); ++i)
        if (sources[i] != nullptr && (layer || (int) i == selected))
            sources[i]->noteOn (note, params);
}

void SourceEngine::noteOff()
{
    gate = false;
    for (auto& s : sources)
        if (s != nullptr) s->noteOff();
}

void SourceEngine::render (float* l, float* r, int n, const RenderContext& ctx, const NoteState& note)
{
    // Follow the selector live so the Main page reacts while a note is held.
    selected = ctx.choice (Param::sourceSelected);
    layer    = ctx.choice (Param::sourceMode) == 1;

    juce::FloatVectorOperations::clear (l, n);
    juce::FloatVectorOperations::clear (r, n);

    bool first = true;
    for (size_t i = 0; i < sources.size(); ++i)
    {
        auto* s = sources[i].get();
        if (s == nullptr) continue;
        if (! layer && (int) i != selected) continue;

        if (first)
        {
            s->render (l, r, n, ctx, note);
            first = false;
        }
        else
        {
            s->render (scratchL.data(), scratchR.data(), n, ctx, note);
            juce::FloatVectorOperations::add (l, scratchL.data(), n);
            juce::FloatVectorOperations::add (r, scratchR.data(), n);
        }
    }
}

bool SourceEngine::isActive() const noexcept
{
    if (gate) return true;
    for (size_t i = 0; i < sources.size(); ++i)
        if (sources[i] != nullptr && (layer || (int) i == selected) && sources[i]->isActive())
            return true;
    return false;
}

float SourceEngine::energy() const noexcept
{
    float e = 0.0f;
    for (size_t i = 0; i < sources.size(); ++i)
        if (sources[i] != nullptr && (layer || (int) i == selected))
            e = std::max (e, sources[i]->energy());
    return e;
}

} // namespace am

#include "SpaceEngine.h"

namespace am
{

void SpaceEngine::prepare (double sampleRate, int) { sr = sampleRate; reset(); }
void SpaceEngine::reset() { lastActivity = 0.0f; }

void SpaceEngine::process (float*, float*, int, const RenderContext& ctx)
{
    lastActivity = ctx.param (Param::spaceMix);
}

} // namespace am

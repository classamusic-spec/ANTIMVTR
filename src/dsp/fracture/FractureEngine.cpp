#include "FractureEngine.h"

namespace am
{

void FractureEngine::prepare (double sampleRate, int) { sr = sampleRate; reset(); }
void FractureEngine::reset() { lastActivity = 0.0f; }

void FractureEngine::process (float*, float*, int, const RenderContext& ctx)
{
    lastActivity = ctx.flag (Param::fractureOn) ? ctx.param (Param::fractureAmount) : 0.0f;
}

} // namespace am

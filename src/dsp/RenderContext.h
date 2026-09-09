#pragma once

#include "core/Types.h"
#include "state/ParameterRegistry.h"

namespace am
{

struct Diagnostics;

/**
    Everything a DSP module needs to render one block. Passed by const
    reference down the chain; never stored beyond the block.
*/
struct RenderContext
{
    double            sampleRate = 48000.0;
    int               numSamples = 0;
    const ParamValues* params    = nullptr;   ///< effective (smoothed, modulated) parameter values
    TransportInfo     transport;
    DryMode           dryMode    = DryMode::FullSynth;
    Quality           quality    = Quality::Normal;
    Diagnostics*      diagnostics = nullptr;  ///< may be null in tests

    inline float param (Param p) const noexcept  { return (*params)[(size_t) paramIndex (p)]; }
    inline int   choice (Param p) const noexcept { return (int) std::lround (param (p)); }
    inline bool  flag (Param p) const noexcept   { return param (p) >= 0.5f; }
};

} // namespace am

#pragma once

#include "core/Types.h"

namespace am
{

/**
    One resonating node of the Matter graph — the control-rate description
    that the material, the Shape parameters and (between blocks) the Evolve
    operators manipulate. The audio-rate resonator state itself lives in the
    structure-of-arrays ModalBank; this record is what the rest of the
    instrument sees.

    CONTRACT (per block, in this order):
      1. MatterEngine::process() reads `frequency`, `weight`, `damping`,
         `pan`, `nonlinearity` and `excitation` and renders the block with
         them (frequencies glide toward `frequency`, gains are ramped).
      2. At the end of the block Matter recomputes the material baseline for
         the next block from the smoothed Shape parameters and writes every
         field again: `targetFrequency` = what the material asks for, and
         `frequency` = `targetFrequency`.
      3. Between blocks, Evolve may overwrite `frequency`, `weight`,
         `damping`, `pan`, `nonlinearity` and `excitation`; the change is
         honoured for exactly the next block, so operators are re-applied
         every block and never accumulate. `targetFrequency`, `ratio`,
         `cluster`, `couplingCount`, `energy` and `active` are informational.
*/
struct MatterNode
{
    float frequency       = 0.0f;   ///< Hz — frequency the node renders next block (Evolve may overwrite)
    float targetFrequency = 0.0f;   ///< Hz — frequency the material asks for, before Evolve
    float ratio           = 1.0f;   ///< targetFrequency / fundamental
    float weight          = 0.0f;   ///< output weight 0..1 (0 = inactive)
    float damping         = 0.0f;   ///< per-sample amplitude loss = 1 - pole radius; T60 ≈ 6.91 / (damping · sampleRate)
    float pan             = 0.0f;   ///< -1..1
    float nonlinearity    = 0.0f;   ///< 0..1 amplitude-dependent detune / damping strength
    float excitation      = 1.0f;   ///< how strongly the source and the strike drive this node (0..1-ish)
    float energy          = 0.0f;   ///< output-referred running amplitude estimate (0..1-ish), updated every block
    uint8_t cluster       = 0;      ///< cluster id
    uint8_t couplingCount = 0;      ///< number of coupling edges touching this node
    bool  active          = false;  ///< weight > 0, frequency inside the safe range
};

} // namespace am

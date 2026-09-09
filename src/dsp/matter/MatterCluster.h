#pragma once

#include "core/Types.h"

namespace am
{

/** A group of nodes that share coupling and are treated as one sub-structure (Tear, visualiser). */
struct MatterCluster
{
    uint8_t id    = 0;
    uint8_t first = 0;   ///< first node index
    uint8_t count = 0;   ///< number of nodes (contiguous indices)
};

} // namespace am

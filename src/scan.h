#pragma once

#include "gfx/common.h"
#include "pipeline.h"

namespace vfs {

// GPU prefix-sum (scan)
// TODO: Double check the parallel scan algorithm. This is just a port
// perhps it would be nice to improve it based on more modern implementations.
class GPUScan {
public:
    void Init(const gfx::CoreCtx& ctx);
    void Clear(const gfx::CoreCtx& ctx);

    void Run(gfx::Buffer elements);

private:
    struct PushConstants {
        VkDeviceAddress elements;
        VkDeviceAddress group_sums;
        unsigned int item_count;
    };

    ComputePipeline pipeline;
};
}  // namespace vfs
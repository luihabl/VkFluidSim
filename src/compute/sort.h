#pragma once

#include <unordered_map>

#include "compute_pipeline.h"
#include "gfx/common.h"

namespace vfs {

// GPU prefix-sum (scan)
// TODO: Double check the parallel scan algorithm. This is just a port
// perhps it would be nice to improve it based on more modern implementations.
class GPUScan {
public:
    void Init(const gfx::CoreCtx& ctx);
    void Clear(const gfx::CoreCtx& ctx);

    void Run(VkCommandBuffer cmd, const gfx::CoreCtx& ctx, const gfx::Buffer& elements);

private:
    struct PushConstants {
        VkDeviceAddress elements;
        VkDeviceAddress group_sums;
        u32 item_count;
    };

    ComputePipeline scan_pipeline;
    std::unordered_map<int, gfx::Buffer> free_buffers;
};

class GPUCountSort {
public:
    void Init(const gfx::CoreCtx& ctx);
    void Clear(const gfx::CoreCtx& ctx);

    void Run(VkCommandBuffer cmd,
             const gfx::CoreCtx& ctx,
             const gfx::Buffer& items,
             const gfx::Buffer& keys,
             u32 max_value);

private:
    struct PushConstants {
        VkDeviceAddress input_items;
        VkDeviceAddress input_keys;
        VkDeviceAddress sorted_items;
        VkDeviceAddress sorted_keys;
        VkDeviceAddress counts;
        u32 item_count;
    };

    ComputePipeline sort_pipeline;

    GPUScan gpu_scan;

    gfx::Buffer sorted_items_buffer;
    gfx::Buffer sorted_values_buffer;
    gfx::Buffer counts_buffer;
};

class SpatialOffset {
public:
    void Init(const gfx::CoreCtx& ctx);
    void Clear(const gfx::CoreCtx& ctx);
    void Run(VkCommandBuffer cmd,
             const gfx::CoreCtx& ctx,
             bool init,
             const gfx::Buffer& sorted_keys,
             const gfx::Buffer& offsets);

private:
    struct PushConstants {
        VkDeviceAddress sorted_keys;
        VkDeviceAddress offsets;
        u32 num_inputs;
    };

    ComputePipeline offset_pipeline;
};

}  // namespace vfs

#pragma once

#include <unordered_map>

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

    void Run(VkCommandBuffer cmd, const gfx::CoreCtx& ctx, gfx::Buffer elements);

private:
    struct PushConstants {
        VkDeviceAddress elements;
        VkDeviceAddress group_sums;
        u32 item_count;
    };

    ComputePipeline scan_pipeline;
    ComputePipeline combine_pipeline;
    std::unordered_map<int, gfx::Buffer> free_buffers;
};

class GPUCountSort {
public:
    void Init(const gfx::CoreCtx& ctx);
    void Clear(const gfx::CoreCtx& ctx);

    void Run(VkCommandBuffer cmd,
             const gfx::CoreCtx& ctx,
             gfx::Buffer items,
             gfx::Buffer keys,
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

    ComputePipeline clear_counts_pipeline;
    ComputePipeline count_pipeline;
    ComputePipeline scatter_pipeline;
    ComputePipeline copy_back_pipeline;

    GPUScan gpu_scan;

    gfx::Buffer sorted_items_buffer;
    gfx::Buffer sorted_values_buffer;
    gfx::Buffer counts_buffer;
};

}  // namespace vfs
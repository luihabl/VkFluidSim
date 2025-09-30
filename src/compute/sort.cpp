#include "sort.h"

#include <glm/fwd.hpp>

#include "compute_pipeline.h"
#include "gfx/common.h"

namespace vfs {

namespace {

enum OffsetKernels : u32 {
    KernelInitOffsets = 0,
    KernelCalculateOffsets,
};

enum ScanKernels : u32 {
    KernelScanBlock = 0,
    KernelScanCombine,
};

enum SortKernels : u32 {
    KernelClearCounts = 0,
    KernelCalcCounts,
    KernelScatter,
    KernelCopyBack,
};

bool CreateBufferIfNeeded(const gfx::CoreCtx& ctx, gfx::Buffer& buf, u32 size) {
    bool create_buf = buf.buffer == nullptr || buf.size < size;
    if (create_buf) {
        buf.Destroy();
        buf = gfx::Buffer::Create(
            ctx, size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        return true;
    }

    return false;
}

}  // namespace

void GPUScan::Init(const gfx::CoreCtx& ctx) {
    scan_pipeline.Init(ctx, {.shader_path = "shaders/compiled/scan.slang.spv",
                             .push_const_size = sizeof(PushConstants),
                             .kernels = {"ScanBlock", "ScanCombine"}});
}

void GPUScan::Clear(const gfx::CoreCtx& ctx) {
    scan_pipeline.Clear(ctx);
    for (auto& [k, v] : free_buffers) {
        v.Destroy();
    }
}

void GPUScan::Run(VkCommandBuffer cmd, const gfx::CoreCtx& ctx, const gfx::Buffer& elements) {
    const u32 threads_per_group = 256;
    const u32 count = elements.size / sizeof(u32);
    int num_groups = (int)std::ceil((float)count / 2.0f / (float)threads_per_group);

    gfx::Buffer group_sum_buffer;
    if (free_buffers.contains(num_groups)) {
        group_sum_buffer = free_buffers[num_groups];
    } else {
        group_sum_buffer = gfx::Buffer::Create(
            ctx, sizeof(u32) * num_groups,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        free_buffers[num_groups] = group_sum_buffer;
    }

    auto push_constants = PushConstants{
        .elements = elements.device_addr,
        .group_sums = group_sum_buffer.device_addr,
        .item_count = count,
    };

    scan_pipeline.Compute(cmd, KernelScanBlock, {num_groups, 1, 1}, &push_constants);
    ComputeToComputePipelineBarrier(cmd);

    if (num_groups > 1) {
        // Recursively scan group_sums
        Run(cmd, ctx, group_sum_buffer);
        scan_pipeline.Compute(cmd, KernelScanCombine, {num_groups, 1, 1}, &push_constants);
    }
}

void GPUCountSort::Init(const gfx::CoreCtx& ctx) {
    sort_pipeline.Init(
        ctx, {.shader_path = "shaders/compiled/sort.slang.spv",
              .push_const_size = sizeof(PushConstants),
              .kernels = {"ClearCounts", "CalculateCounts", "ScatterOutputs", "CopyBack"}});

    gpu_scan.Init(ctx);
}

void GPUCountSort::Clear(const gfx::CoreCtx& ctx) {
    gpu_scan.Clear(ctx);

    sort_pipeline.Clear(ctx);

    sorted_items_buffer.Destroy();
    sorted_values_buffer.Destroy();
    counts_buffer.Destroy();
}

void GPUCountSort::Run(VkCommandBuffer cmd,
                       const gfx::CoreCtx& ctx,
                       const gfx::Buffer& items,
                       const gfx::Buffer& keys,
                       u32 max_value) {
    u32 item_count = items.size / (u32)sizeof(u32);

    CreateBufferIfNeeded(ctx, sorted_items_buffer, items.size);
    CreateBufferIfNeeded(ctx, sorted_values_buffer, items.size);
    CreateBufferIfNeeded(ctx, counts_buffer, (max_value + 1) * sizeof(u32));

    auto push_consts = PushConstants{
        .input_items = items.device_addr,
        .input_keys = keys.device_addr,
        .sorted_items = sorted_items_buffer.device_addr,
        .sorted_keys = sorted_values_buffer.device_addr,
        .counts = counts_buffer.device_addr,
        .item_count = item_count,
    };

    u32 n_groups = item_count / 256 + 1;

    sort_pipeline.Compute(cmd, KernelClearCounts, {n_groups, 1, 1}, &push_consts);
    ComputeToComputePipelineBarrier(cmd);

    sort_pipeline.Compute(cmd, KernelCalcCounts, {n_groups, 1, 1}, &push_consts);
    ComputeToComputePipelineBarrier(cmd);

    gpu_scan.Run(cmd, ctx, counts_buffer);
    ComputeToComputePipelineBarrier(cmd);

    sort_pipeline.Compute(cmd, KernelScatter, {n_groups, 1, 1}, &push_consts);
    ComputeToComputePipelineBarrier(cmd);

    sort_pipeline.Compute(cmd, KernelCopyBack, {n_groups, 1, 1}, &push_consts);
    ComputeToComputePipelineBarrier(cmd);
}

void SpatialOffset::Init(const gfx::CoreCtx& ctx) {
    offset_pipeline.Init(ctx, {.shader_path = "shaders/compiled/spatial_offsets.slang.spv",
                               .push_const_size = sizeof(PushConstants),
                               .kernels = {"InitOffsets", "CalculateOffsets"}});
}

void SpatialOffset::Clear(const gfx::CoreCtx& ctx) {
    offset_pipeline.Clear(ctx);
}

void SpatialOffset::Run(VkCommandBuffer cmd,
                        const gfx::CoreCtx& ctx,
                        bool init,
                        const gfx::Buffer& sorted_keys,
                        const gfx::Buffer& offsets) {
    u32 num_inputs = sorted_keys.size / (u32)sizeof(u32);

    auto push_consts = PushConstants{
        .sorted_keys = sorted_keys.device_addr,
        .offsets = offsets.device_addr,
        .num_inputs = num_inputs,
    };

    u32 n_groups = num_inputs / 256 + 1;

    if (init) {
        offset_pipeline.Compute(cmd, KernelInitOffsets, {n_groups, 1, 1}, &push_consts);
        ComputeToComputePipelineBarrier(cmd);
    }

    offset_pipeline.Compute(cmd, KernelCalculateOffsets, {n_groups, 1, 1}, &push_consts);
}

struct ReorderPushConstants {
    VkDeviceAddress sorted_indices;
    VkDeviceAddress buffer;
    VkDeviceAddress sort_target;
    u32 n;
};

void BufferReorder::Init(const gfx::CoreCtx& ctx, Config&& cfg) {
    config = std::move(cfg);

    for (u32 i = 0; i < config.buffers.size(); i++) {
        sort_targets.push_back(gfx::CreateDataBuffer<glm::vec3>(ctx, config.n));
    }

    reorder_pipeline.Init(ctx, {
                                   .shader_path = "shaders/compiled/reorder.slang.spv",
                                   .kernels = {"Reorder"},
                                   .push_const_size = sizeof(ReorderPushConstants),
                               });
}

void BufferReorder::Clear(const gfx::CoreCtx& ctx) {
    for (auto& buf : sort_targets) {
        buf.Destroy();
    }

    reorder_pipeline.Clear(ctx);
}
void BufferReorder::Reorder(VkCommandBuffer cmd, const gfx::CoreCtx& ctx) {
    auto reorder_push_constants = ReorderPushConstants{
        .sorted_indices = config.sort_indices,
        .n = config.n,
    };

    glm::vec3 ngroups{config.n / 256 + 1, 1, 1};

    for (u32 i = 0; i < config.buffers.size(); i++) {
        reorder_push_constants.buffer = config.buffers[i].addr;
        reorder_push_constants.sort_target = sort_targets[i].device_addr;
        reorder_pipeline.Compute(cmd, 0, ngroups, &reorder_push_constants);
    }
}

void BufferReorder::Copyback(VkCommandBuffer cmd) {
    auto cpy_info = VkBufferCopy{.dstOffset = 0, .srcOffset = 0, .size = sort_targets.back().size};
    for (u32 i = 0; i < config.buffers.size(); i++) {
        vkCmdCopyBuffer(cmd, sort_targets[i].buffer, config.buffers[i].buf, 1, &cpy_info);
    }
}

}  // namespace vfs

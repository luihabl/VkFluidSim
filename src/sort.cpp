#include "sort.h"

#include "gfx/common.h"
#include "gfx/vk_util.h"
#include "pipeline.h"

namespace vfs {

namespace {

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
    scan_pipeline.Init(ctx, {.shader_path = "shaders/compiled/scan_block.comp.spv",
                             .push_const_size = sizeof(PushConstants)});
    combine_pipeline.Init(ctx, {.shader_path = "shaders/compiled/scan_combine.comp.spv",
                                .push_const_size = sizeof(PushConstants)});
}

void GPUScan::Clear(const gfx::CoreCtx& ctx) {
    scan_pipeline.Clear(ctx);
    combine_pipeline.Clear(ctx);
    for (auto& [k, v] : free_buffers) {
        v.Destroy();
    }
}

void GPUScan::Run(VkCommandBuffer cmd, const gfx::CoreCtx& ctx, gfx::Buffer elements) {
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
        .elements = vk::util::GetBufferAddress(ctx.device, elements),
        .group_sums = vk::util::GetBufferAddress(ctx.device, group_sum_buffer),
        .item_count = count,
    };

    scan_pipeline.Compute(cmd, {num_groups, 1, 1}, &push_constants);
    ComputeToComputePipelineBarrier(cmd);

    if (num_groups > 1) {
        // Recursively scan group_sums
        Run(cmd, ctx, group_sum_buffer);
        combine_pipeline.Compute(cmd, {num_groups, 1, 1}, &push_constants);
    }
}

void GPUCountSort::Init(const gfx::CoreCtx& ctx) {
    clear_counts_pipeline.Init(ctx, {.shader_path = "shaders/compiled/sort_clear_counts.comp.spv",
                                     .push_const_size = sizeof(PushConstants)});
    count_pipeline.Init(ctx, {.shader_path = "shaders/compiled/sort_calc_counts.comp.spv",
                              .push_const_size = sizeof(PushConstants)});
    scatter_pipeline.Init(ctx, {.shader_path = "shaders/compiled/sort_scatter.comp.spv",
                                .push_const_size = sizeof(PushConstants)});
    copy_back_pipeline.Init(ctx, {.shader_path = "shaders/compiled/sort_copy_back.comp.spv",
                                  .push_const_size = sizeof(PushConstants)});

    gpu_scan.Init(ctx);
}

void GPUCountSort::Clear(const gfx::CoreCtx& ctx) {
    gpu_scan.Clear(ctx);

    clear_counts_pipeline.Clear(ctx);
    count_pipeline.Clear(ctx);
    scatter_pipeline.Clear(ctx);
    copy_back_pipeline.Clear(ctx);

    sorted_items_buffer.Destroy();
    sorted_values_buffer.Destroy();
    counts_buffer.Destroy();
}

void GPUCountSort::Run(VkCommandBuffer cmd,
                       const gfx::CoreCtx& ctx,
                       gfx::Buffer items,
                       gfx::Buffer keys,
                       u32 max_value) {
    u32 item_count = items.size / (u32)sizeof(u32);

    CreateBufferIfNeeded(ctx, sorted_items_buffer, items.size);
    CreateBufferIfNeeded(ctx, sorted_values_buffer, items.size);
    CreateBufferIfNeeded(ctx, counts_buffer, (max_value + 1) * sizeof(u32));

    auto push_consts = PushConstants{
        .input_items = vk::util::GetBufferAddress(ctx.device, items),
        .input_keys = vk::util::GetBufferAddress(ctx.device, keys),
        .sorted_items = vk::util::GetBufferAddress(ctx.device, sorted_items_buffer),
        .sorted_keys = vk::util::GetBufferAddress(ctx.device, sorted_values_buffer),
        .counts = vk::util::GetBufferAddress(ctx.device, counts_buffer),
        .item_count = item_count,
    };

    u32 n_groups = item_count / 256 + 1;

    clear_counts_pipeline.Compute(cmd, {n_groups, 1, 1}, &push_consts);
    count_pipeline.Compute(cmd, {n_groups, 1, 1}, &push_consts);

    gpu_scan.Run(cmd, ctx, counts_buffer);

    scatter_pipeline.Compute(cmd, {n_groups, 1, 1}, &push_consts);
    copy_back_pipeline.Compute(cmd, {n_groups, 1, 1}, &push_consts);
}

}  // namespace vfs
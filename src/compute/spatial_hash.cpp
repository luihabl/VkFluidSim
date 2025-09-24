#include "spatial_hash.h"

namespace vfs {

namespace {
struct PushConstants {
    VkDeviceAddress positions;
    u32 n_particles;
};

struct UniformData {
    float cell_size;
    VkDeviceAddress spatial_keys;
};

}  // namespace

void SpatialHash::Init(const gfx::CoreCtx& ctx, u32 n, float cell_size) {
    this->n = n;

    spatial_keys = CreateDataBuffer<u32>(ctx, n);
    spatial_indices = CreateDataBuffer<u32>(ctx, n);
    spatial_offsets = CreateDataBuffer<u32>(ctx, n);

    sort.Init(ctx);
    offset.Init(ctx);

    spatial_hash_desc.Init(ctx, sizeof(UniformData));
    auto uniforms = UniformData{
        .cell_size = cell_size,
        .spatial_keys = spatial_keys.device_addr,
    };
    spatial_hash_desc.SetUniformData(&uniforms);

    spatial_hash_pipeline.Init(ctx, {.shader_path = "shaders/compiled/spatial_hash.slang.spv",
                                     .kernels = {"update_spatial_hash"},
                                     .push_const_size = sizeof(PushConstants),
                                     .desc_manager = &spatial_hash_desc});
}

void SpatialHash::Run(const gfx::CoreCtx& ctx, VkCommandBuffer cmd, VkDeviceAddress positions) {
    auto pc = PushConstants{
        .positions = positions,
        .n_particles = n,
    };

    spatial_hash_pipeline.Compute(cmd, 0, {n / 256 + 1, 1, 1}, &pc);
    ComputeToComputePipelineBarrier(cmd);
    sort.Run(cmd, ctx, spatial_indices, spatial_keys, n - 1);
    ComputeToComputePipelineBarrier(cmd);
    offset.Run(cmd, ctx, true, spatial_keys, spatial_offsets);
}

void SpatialHash::Clear(const gfx::CoreCtx& ctx) {
    spatial_keys.Destroy();
    spatial_indices.Destroy();
    spatial_offsets.Destroy();
    sort.Clear(ctx);
    offset.Clear(ctx);
    spatial_hash_pipeline.Clear(ctx);
    spatial_hash_desc.Clear(ctx);
}

void SpatialHash::SetCellSize(float size) {
    auto uniforms = UniformData{
        .cell_size = size,
        .spatial_keys = spatial_keys.device_addr,
    };
    spatial_hash_desc.SetUniformData(&uniforms);
}

}  // namespace vfs

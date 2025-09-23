#include "spatial_hash.h"

namespace vfs {

void SpatialHash::Init(const gfx::CoreCtx& ctx, u32 n) {
    this->n = n;

    spatial_keys = CreateDataBuffer<u32>(ctx, n);
    spatial_indices = CreateDataBuffer<u32>(ctx, n);
    spatial_offsets = CreateDataBuffer<u32>(ctx, n);

    sort.Init(ctx);
    offset.Init(ctx);
}

void SpatialHash::Run(const gfx::CoreCtx& ctx, VkCommandBuffer cmd) {
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
}

}  // namespace vfs

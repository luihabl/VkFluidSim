#pragma once

#include "gfx/common.h"
#include "sort.h"

namespace vfs {
class SpatialHash {
public:
    void Init(const gfx::CoreCtx& ctx, u32 n);
    void Run(const gfx::CoreCtx& ctx, VkCommandBuffer cmd);
    void Clear(const gfx::CoreCtx& ctx);

    VkDeviceAddress SpatialKeysAddr() const { return spatial_keys.device_addr; }
    VkDeviceAddress SpatialIndicesAddr() const { return spatial_indices.device_addr; }
    VkDeviceAddress SpatialOffsetsAddr() const { return spatial_offsets.device_addr; }

private:
    u32 n{0};

    GPUCountSort sort;
    SpatialOffset offset;

    gfx::Buffer spatial_keys;
    gfx::Buffer spatial_indices;
    gfx::Buffer spatial_offsets;
};
}  // namespace vfs

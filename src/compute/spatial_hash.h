#pragma once

#include "compute_pipeline.h"
#include "gfx/common.h"
#include "gfx/descriptor.h"
#include "sort.h"

namespace vfs {
class SpatialHash {
public:
    void Init(const gfx::CoreCtx& ctx, u32 n, float cell_size);
    void Run(const gfx::CoreCtx& ctx, VkCommandBuffer cmd, VkDeviceAddress positions);
    void Clear(const gfx::CoreCtx& ctx);

    VkDeviceAddress SpatialKeysAddr() const { return spatial_keys.device_addr; }
    VkDeviceAddress SpatialIndicesAddr() const { return spatial_indices.device_addr; }
    VkDeviceAddress SpatialOffsetsAddr() const { return spatial_offsets.device_addr; }

    void SetCellSize(float size);

private:
    u32 n{0};

    std::vector<gfx::DescriptorManager::DescriptorInfo> desc_info;
    gfx::DescriptorManager spatial_hash_desc;
    ComputePipeline spatial_hash_pipeline;

    GPUCountSort sort;
    SpatialOffset offset;

    gfx::Buffer spatial_keys;
    gfx::Buffer spatial_indices;
    gfx::Buffer spatial_offsets;
};
}  // namespace vfs

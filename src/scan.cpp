#include "scan.h"

namespace vfs {

void GPUScan::Init(const gfx::CoreCtx& ctx) {
    // pipeline.Init(ctx, const DescriptorManager& desc_manager, const char* shader_path)
}

void GPUScan::Clear(const gfx::CoreCtx& ctx) {}

void GPUScan::Run(gfx::Buffer elements) {
    auto push_constants = PushConstants{
        .item_count = 0,
    };

    // pipeline.Compute(VkCommandBuffer cmd, gfx::Device &gfx, const ComputePushConstants
    // &push_constants)
}

}  // namespace vfs
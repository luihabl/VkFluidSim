#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "gfx/common.h"
#include "gfx/descriptor.h"

namespace vfs {

class ComputePipeline {
public:
    struct Config {
        uint32_t push_const_size{0};
        gfx::DescriptorManager* desc_manager{nullptr};
        std::string shader_path{""};
        std::vector<std::string> kernels{{"main"}};
    };

    void Init(const gfx::CoreCtx& ctx, const Config& config);
    void Compute(VkCommandBuffer cmd,
                 u32 kernel_id,
                 glm::ivec3 group_count,
                 void* push_constants = nullptr);
    void Clear(const gfx::CoreCtx& ctx);
    u32 FindKernelId(const std::string& entry_point);

private:
    Config config;
    std::unordered_map<std::string, u32> ids;
    std::vector<VkPipeline> pipelines;
    VkPipelineLayout layout;
};

void ComputeToComputePipelineBarrier(VkCommandBuffer cmd);
void ComputeToGraphicsPipelineBarrier(VkCommandBuffer cmd);

}  // namespace vfs

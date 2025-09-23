#include "compute_pipeline.h"

#include <cstddef>

#include "gfx/common.h"
#include "gfx/descriptor.h"
#include "gfx/vk_util.h"
#include "platform.h"

namespace vfs {

void ComputePipeline::Init(const gfx::CoreCtx& ctx, const Config& input_config) {
    config = input_config;

    auto shader = vk::util::LoadShaderModule(
        ctx, Platform::Info::ResourcePath(config.shader_path.c_str()).c_str());

    auto push_constant_range = VkPushConstantRange{
        .offset = 0,
        .size = config.push_const_size,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    };

    if (config.desc_manager) {
        layout = vk::util::CreatePipelineLayout(ctx, {{config.desc_manager->desc_layout}},
                                                {{push_constant_range}});
    } else {
        layout = vk::util::CreatePipelineLayout(ctx, {}, {{push_constant_range}});
    }

    i32 id = 0;
    for (const auto& kernel : config.kernels) {
        ids[kernel] = id++;
        pipelines.push_back(
            vk::util::BuildComputePipeline(ctx.device, layout, shader, kernel.c_str()));
    }

    vkDestroyShaderModule(ctx.device, shader, nullptr);
}

u32 ComputePipeline::FindKernelId(const std::string& entry_point) {
    return ids.at(entry_point);
}

void ComputePipeline::Compute(VkCommandBuffer cmd,
                              u32 kernel_id,
                              glm::ivec3 group_count,
                              void* push_constants) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines[kernel_id]);

    if (config.desc_manager) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1,
                                &config.desc_manager->desc_set, 0, 0);
    }

    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, config.push_const_size,
                       push_constants);

    // gfx::u32 sz = push_constants.n_particles / 64 + 1;
    vkCmdDispatch(cmd, group_count.x, group_count.y, group_count.z);
}

void ComputePipeline::Clear(const gfx::CoreCtx& ctx) {
    for (auto& pipeline : pipelines) {
        vkDestroyPipeline(ctx.device, pipeline, nullptr);
    }
}

void ComputeToComputePipelineBarrier(VkCommandBuffer cmd) {
    auto mem_barrier = VkMemoryBarrier2{
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        // .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        // .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
    };

    auto dep_info = VkDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pMemoryBarriers = &mem_barrier,
        .memoryBarrierCount = 1,
    };

    vkCmdPipelineBarrier2(cmd, &dep_info);
}
void ComputeToGraphicsPipelineBarrier(VkCommandBuffer cmd) {
    auto mem_barrier = VkMemoryBarrier2{
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
    };

    auto dep_info = VkDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pMemoryBarriers = &mem_barrier,
        .memoryBarrierCount = 1,
    };

    vkCmdPipelineBarrier2(cmd, &dep_info);
}

}  // namespace vfs

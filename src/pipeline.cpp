#include "pipeline.h"

#include <cstddef>

#include "gfx/common.h"
#include "gfx/descriptor.h"
#include "gfx/mesh.h"
#include "gfx/vk_util.h"
#include "platform.h"

namespace vfs {

void SpriteDrawPipeline::Init(const gfx::CoreCtx& ctx, VkFormat draw_img_format) {
    auto gfx_shader = vk::util::LoadShaderModule(
        ctx, Platform::Info::ResourcePath("shaders/compiled/gfx.slang.spv").c_str());

    // TODO: FILL THIS!!! Add the descriptor layouts hrere

    auto push_constant_range = VkPushConstantRange{
        .offset = 0,
        .size = sizeof(DrawPushConstants),
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    layout = vk::util::CreatePipelineLayout(ctx, {}, {{push_constant_range}});

    pipeline =
        vk::util::GraphicsPipelineBuilder(layout)
            .AddShaderStage(gfx_shader, VK_SHADER_STAGE_VERTEX_BIT, "vertex_main")
            .AddShaderStage(gfx_shader, VK_SHADER_STAGE_FRAGMENT_BIT, "frag_main")
            .AddVertexBinding(0, sizeof(gfx::Vertex))
            .AddVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gfx::Vertex, pos))
            .AddVertexAttribute(0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(gfx::Vertex, color))
            .SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .SetPolygonMode(VK_POLYGON_MODE_FILL)
            .SetCullDisabled()
            .SetMultisamplingDisabled()
            .SetDepthTestDisabled()
            .SetBlendingAlphaBlend()  // Check if this is OK
            .SetColorAttachmentFormat(draw_img_format)
            .Build(ctx.device);

    vkDestroyShaderModule(ctx.device, gfx_shader, nullptr);
}

void SpriteDrawPipeline::Draw(VkCommandBuffer cmd,
                              gfx::Device& gfx,
                              const gfx::Image& draw_img,
                              const gfx::GPUMesh& mesh,
                              const DrawPushConstants& push_constants,
                              uint32_t instances) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    const auto viewport = VkViewport{
        .x = 0,
        .y = 0,
        .width = (float)draw_img.extent.width,
        .height = (float)draw_img.extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vkCmdSetViewport(cmd, 0, 1, &viewport);

    const auto scissor = VkRect2D{
        .offset = {0, 0},
        .extent = {.width = draw_img.extent.width, .height = draw_img.extent.height},
    };

    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DrawPushConstants),
                       &push_constants);

    VkDeviceSize offsets[1]{0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertices.buffer, offsets);
    vkCmdBindIndexBuffer(cmd, mesh.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, mesh.index_count, instances, 0, 0, 0);
}

void SpriteDrawPipeline::Clear(const gfx::CoreCtx& ctx) {
    vkDestroyPipeline(ctx.device, pipeline, nullptr);
}

void DescriptorManager::Init(const gfx::CoreCtx& ctx, u32 ubo_size) {
    size = ubo_size;
    uniform_constant_data.resize(size);

    desc_pool.Init(ctx,
                   {{
                       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                   }},
                   256, 256);

    desc_layout = gfx::DescriptorLayoutBuilder{}
                      .Add(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                      .Build(ctx, VK_SHADER_STAGE_ALL);

    global_constants_ubo = gfx::Buffer::Create(ctx, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    desc_set = desc_pool.Alloc(ctx, desc_layout);
    std::vector<VkWriteDescriptorSet> write_desc_sets = {
        vk::util::WriteDescriptorSet(desc_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                     &global_constants_ubo.desc_info),
    };

    vkUpdateDescriptorSets(ctx.device, write_desc_sets.size(), write_desc_sets.data(), 0, nullptr);
}

void DescriptorManager::Clear(const gfx::CoreCtx& ctx) {
    global_constants_ubo.Destroy();
    vkDestroyDescriptorSetLayout(ctx.device, desc_layout, nullptr);
    desc_pool.Clear(ctx);
}

void DescriptorManager::SetUniformData(void* data) {
    memcpy(uniform_constant_data.data(), data, size);
    memcpy(global_constants_ubo.Map(), data, size);
}

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

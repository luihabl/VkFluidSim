#include "pipeline.h"

#include <cstddef>

#include "gfx/common.h"
#include "gfx/descriptor.h"
#include "gfx/mesh.h"
#include "gfx/vk_util.h"
#include "platform.h"

namespace vfs {

void SpriteDrawPipeline::Init(const gfx::CoreCtx& ctx, VkFormat draw_img_format) {
    auto frag = vk::util::LoadShaderModule(
        ctx, Platform::Info::ResourcePath("shaders/compiled/sprite.frag.spv").c_str());
    auto vert = vk::util::LoadShaderModule(
        ctx, Platform::Info::ResourcePath("shaders/compiled/sprite.vert.spv").c_str());

    // TODO: FILL THIS!!! Add the descriptor layouts hrere

    auto push_constant_range = VkPushConstantRange{
        .offset = 0,
        .size = sizeof(DrawPushConstants),
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    layout = vk::util::CreatePipelineLayout(ctx, {}, {{push_constant_range}});

    pipeline =
        vk::util::GraphicsPipelineBuilder(layout)
            .SetShaders(vert, frag)
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

    vkDestroyShaderModule(ctx.device, frag, nullptr);
    vkDestroyShaderModule(ctx.device, vert, nullptr);
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

void DescriptorManager::Init(const gfx::CoreCtx& ctx) {
    desc_pool.Init(ctx,
                   {{
                       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                   }},
                   256, 256);

    desc_layout = gfx::DescriptorLayoutBuilder{}
                      .Add(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                      .Build(ctx, VK_SHADER_STAGE_ALL);

    global_constants_ubo =
        gfx::Buffer::Create(ctx, sizeof(GlobalUniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

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

void DescriptorManager::SetUniformData(const GlobalUniformData& data) {
    uniform_constant_data = data;
    memcpy(global_constants_ubo.Map(), &uniform_constant_data, sizeof(GlobalUniformData));
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

    pipeline = vk::util::BuildComputePipeline(ctx.device, layout, shader);

    vkDestroyShaderModule(ctx.device, shader, nullptr);
}

void ComputePipeline::Compute(VkCommandBuffer cmd,
                              gfx::Device& gfx,
                              glm::ivec3 group_count,
                              void* push_constants) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

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
    vkDestroyPipeline(ctx.device, pipeline, nullptr);
}

}  // namespace vfs
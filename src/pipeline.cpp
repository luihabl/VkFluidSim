#include "pipeline.h"

#include <array>
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

void ComputePipeline::Init(const gfx::CoreCtx& ctx) {
    desc_pool.Init(ctx,
                   {{
                       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                   }},
                   256, 256);

    desc_layout = gfx::DescriptorLayoutBuilder{}
                      .Add(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                      .Add(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                      .Add(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                      .Build(ctx, VK_SHADER_STAGE_ALL);

    for (int i = 0; i < gfx::FRAME_COUNT; i++) {
        auto& frame = frame_data[i];
        frame.global_constants_ubo =
            gfx::Buffer::Create(ctx, sizeof(GlobalUniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        frame.buffers_ubo =
            gfx::Buffer::Create(ctx, sizeof(BufferUniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }

    for (int i = 0; i < gfx::FRAME_COUNT; i++) {
        auto& frame = frame_data[i];
        frame.desc_set = desc_pool.Alloc(ctx, desc_layout);
        std::vector<VkWriteDescriptorSet> write_desc_sets = {
            vk::util::WriteDescriptorSet(frame.desc_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                         &frame.global_constants_ubo.desc_info),
            vk::util::WriteDescriptorSet(frame.desc_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                                         &frame.buffers_ubo.desc_info),
            vk::util::WriteDescriptorSet(
                frame.desc_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2,
                &frame_data[(i + 1) % gfx::FRAME_COUNT].buffers_ubo.desc_info),
        };

        vkUpdateDescriptorSets(ctx.device, write_desc_sets.size(), write_desc_sets.data(), 0,
                               nullptr);
    }

    auto shader = vk::util::LoadShaderModule(
        ctx, Platform::Info::ResourcePath("shaders/compiled/update_pos.comp.spv").c_str());

    auto push_constant_range = VkPushConstantRange{
        .offset = 0,
        .size = sizeof(ComputePushConstants),
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    };

    layout = vk::util::CreatePipelineLayout(ctx, {{desc_layout}}, {{push_constant_range}});
    pipeline = vk::util::BuildComputePipeline(ctx.device, layout, shader);

    vkDestroyShaderModule(ctx.device, shader, nullptr);
}

void ComputePipeline::SetUniformData(const GlobalUniformData& data) {
    for (int i = 0; i < gfx::FRAME_COUNT; i++) {
        frame_data[i].uniform_constant_data = data;
        frame_data[i].should_update = true;
    }
}

void ComputePipeline::SetBuffers(const std::vector<BufferUniformData>& data) {
    for (int i = 0; i < gfx::FRAME_COUNT; i++) {
        frame_data[i].buffer_uniform_data = data[i];
        frame_data[i].should_update = true;
    }
}

void ComputePipeline::UpdateUniformBuffers(uint32_t frame) {
    memcpy(frame_data[frame].global_constants_ubo.Map(), &frame_data[frame].uniform_constant_data,
           sizeof(GlobalUniformData));
    memcpy(frame_data[frame].buffers_ubo.Map(), &frame_data[frame].buffer_uniform_data,
           sizeof(BufferUniformData));
}

void ComputePipeline::Compute(VkCommandBuffer cmd,
                              gfx::Device& gfx,
                              const ComputePushConstants& push_constants) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    if (frame_data[current_frame].should_update) {
        UpdateUniformBuffers(current_frame);
        frame_data[current_frame].should_update = false;
    }

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1,
                            &frame_data[current_frame].desc_set, 0, 0);

    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants),
                       &push_constants);

    gfx::u32 sz = push_constants.n_particles / 64 + 1;
    vkCmdDispatch(cmd, sz, 1, 1);

    current_frame = (current_frame + 1) % gfx::FRAME_COUNT;
}

void ComputePipeline::Clear(const gfx::CoreCtx& ctx) {
    for (auto& frame : frame_data) {
        frame.global_constants_ubo.Destroy();
        frame.buffers_ubo.Destroy();
    }

    vkDestroyDescriptorSetLayout(ctx.device, desc_layout, nullptr);
    desc_pool.Clear(ctx);
    vkDestroyPipeline(ctx.device, pipeline, nullptr);
}

}  // namespace vfs
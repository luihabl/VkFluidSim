#include "draw_pipelines.h"

#include "gfx/mesh.h"
#include "gfx/vk_util.h"
#include "platform.h"

namespace vfs {

void BoxDrawPipeline::Init(const gfx::CoreCtx& ctx,
                           VkFormat draw_img_format,
                           VkFormat depth_img_format,
                           bool draw_3d) {
    auto box_shader = vk::util::LoadShaderModule(
        ctx, Platform::Info::ResourcePath("shaders/compiled/box.slang.spv").c_str());

    auto push_constant_range = VkPushConstantRange{
        .offset = 0,
        .size = sizeof(PushConstants),
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    layout = vk::util::CreatePipelineLayout(ctx, {}, {{push_constant_range}});

    n_draw_vertices = draw_3d ? 24 : 8;

    pipeline = vk::util::GraphicsPipelineBuilder(layout)
                   .AddShaderStage(box_shader, VK_SHADER_STAGE_VERTEX_BIT, "vertex_main")
                   .AddShaderStage(box_shader, VK_SHADER_STAGE_FRAGMENT_BIT, "frag_main")
                   .SetInputTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
                   .SetPolygonMode(VK_POLYGON_MODE_LINE)
                   .SetCullDisabled()
                   .SetMultisamplingDisabled()
                   .SetDepthFormat(depth_img_format)
                   .SetDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL)
                   .SetBlendingAlphaBlend()  // Check if this is OK
                   .SetColorAttachmentFormat(draw_img_format)
                   .Build(ctx.device);

    vkDestroyShaderModule(ctx.device, box_shader, nullptr);
}
void BoxDrawPipeline::Clear(const gfx::CoreCtx& ctx) {
    vkDestroyPipeline(ctx.device, pipeline, nullptr);
}

void BoxDrawPipeline::Draw(VkCommandBuffer cmd,
                           gfx::Device& gfx,
                           const gfx::Image& draw_img,
                           const PushConstants& push_constants) {
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

    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants),
                       &push_constants);

    vkCmdDraw(cmd, n_draw_vertices, 1, 0, 0);
}

void ParticleDrawPipeline::Init(const gfx::CoreCtx& ctx, VkFormat draw_img_format) {
    auto gfx_shader = vk::util::LoadShaderModule(
        ctx, Platform::Info::ResourcePath("shaders/compiled/particles.slang.spv").c_str());

    auto push_constant_range = VkPushConstantRange{
        .offset = 0,
        .size = sizeof(PushConstants),
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    layout = vk::util::CreatePipelineLayout(ctx, {}, {{push_constant_range}});

    pipeline = vk::util::GraphicsPipelineBuilder(layout)
                   .AddShaderStage(gfx_shader, VK_SHADER_STAGE_VERTEX_BIT, "vertex_main")
                   .AddShaderStage(gfx_shader, VK_SHADER_STAGE_FRAGMENT_BIT, "frag_main")
                   .AddVertexBinding(0, sizeof(gfx::Vertex))
                   .AddVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gfx::Vertex, pos))
                   .AddVertexAttribute(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(gfx::Vertex, uv))
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

void ParticleDrawPipeline::Draw(VkCommandBuffer cmd,
                                gfx::Device& gfx,
                                const gfx::Image& draw_img,
                                const gfx::GPUMesh& mesh,
                                const PushConstants& push_constants,
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

    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants),
                       &push_constants);

    VkDeviceSize offsets[1]{0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertices.buffer, offsets);
    vkCmdBindIndexBuffer(cmd, mesh.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, mesh.index_count, instances, 0, 0, 0);
}

void ParticleDrawPipeline::Clear(const gfx::CoreCtx& ctx) {
    vkDestroyPipeline(ctx.device, pipeline, nullptr);
}

void Particle3DDrawPipeline::Init(const gfx::CoreCtx& ctx,
                                  VkFormat draw_img_format,
                                  VkFormat depth_img_format) {
    auto gfx_shader = vk::util::LoadShaderModule(
        ctx, Platform::Info::ResourcePath("shaders/compiled/particles_3d.slang.spv").c_str());

    auto push_constant_range = VkPushConstantRange{
        .offset = 0,
        .size = sizeof(PushConstants),
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    layout = vk::util::CreatePipelineLayout(ctx, {}, {{push_constant_range}});

    pipeline = vk::util::GraphicsPipelineBuilder(layout)
                   .AddShaderStage(gfx_shader, VK_SHADER_STAGE_VERTEX_BIT, "vertex_main")
                   .AddShaderStage(gfx_shader, VK_SHADER_STAGE_FRAGMENT_BIT, "frag_main")
                   .AddVertexBinding(0, sizeof(gfx::Vertex))
                   .AddVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gfx::Vertex, pos))
                   .AddVertexAttribute(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(gfx::Vertex, uv))
                   .SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                   .SetPolygonMode(VK_POLYGON_MODE_FILL)
                   .SetCullDisabled()
                   .SetMultisamplingDisabled()
                   .SetDepthFormat(depth_img_format)
                   .SetDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL)
                   .SetBlendingAlphaBlend()  // Check if this is OK
                   .SetColorAttachmentFormat(draw_img_format)
                   .Build(ctx.device);

    vkDestroyShaderModule(ctx.device, gfx_shader, nullptr);
}

void Particle3DDrawPipeline::Clear(const gfx::CoreCtx& ctx) {
    vkDestroyPipeline(ctx.device, pipeline, nullptr);
}

void Particle3DDrawPipeline::Draw(VkCommandBuffer cmd,
                                  gfx::Device& gfx,
                                  const gfx::Image& draw_img,
                                  const gfx::GPUMesh& mesh,
                                  const PushConstants& push_constants,
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

    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(PushConstants), &push_constants);

    VkDeviceSize offsets[1]{0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertices.buffer, offsets);
    vkCmdBindIndexBuffer(cmd, mesh.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, mesh.index_count, instances, 0, 0, 0);
}

}  // namespace vfs

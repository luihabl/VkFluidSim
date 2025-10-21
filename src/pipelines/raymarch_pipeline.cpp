#include "raymarch_pipeline.h"

#include "gfx/vk_util.h"
#include "platform.h"

namespace vfs {
void DensityRaymarchDrawPipeline::Init(const gfx::CoreCtx& ctx,
                                       VkFormat draw_img_format,
                                       VkFormat depth_img_format) {
    auto gfx_shader = vk::util::LoadShaderModule(
        ctx, Platform::Info::ResourcePath("shaders/compiled/raymarch.slang.spv").c_str());

    auto push_constant_range = VkPushConstantRange{
        .offset = 0,
        .size = sizeof(PushConstants),
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    layout = vk::util::CreatePipelineLayout(ctx, {}, {{push_constant_range}});

    pipeline =
        vk::util::GraphicsPipelineBuilder(layout)
            .AddShaderStage(gfx_shader, VK_SHADER_STAGE_VERTEX_BIT, "vertex_main")
            .AddShaderStage(gfx_shader, VK_SHADER_STAGE_FRAGMENT_BIT, "frag_main")
            .AddVertexBinding(0, sizeof(gfx::Vertex))
            .AddVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gfx::Vertex, pos))
            .AddVertexAttribute(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(gfx::Vertex, uv))
            .AddVertexAttribute(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gfx::Vertex, normal))
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

void DensityRaymarchDrawPipeline::Clear(const gfx::CoreCtx& ctx) {
    vkDestroyPipeline(ctx.device, pipeline, nullptr);
}

void DensityRaymarchDrawPipeline::Draw(VkCommandBuffer cmd,
                                       gfx::Device& gfx,
                                       const gfx::Buffer& field,
                                       const glm::ivec3& field_size,
                                       const gfx::Image& draw_img,
                                       const gfx::MeshDrawObj& mesh,
                                       const gfx::Camera& camera) {
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

    auto pc = PushConstants{
        .transform = mesh.transform.Matrix(),
        .view_proj = camera.GetViewProj(),
        .camera_pos = camera.GetPosition(),
        .field = field.device_addr,
        .field_size = field_size,
    };

    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(PushConstants), &pc);

    VkDeviceSize offsets[1]{0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.mesh.vertices.buffer, offsets);
    vkCmdBindIndexBuffer(cmd, mesh.mesh.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, mesh.mesh.index_count, 1, 0, 0, 0);
}

}  // namespace vfs

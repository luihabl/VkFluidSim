#include "pipeline.h"

#include <array>

#include "gfx/common.h"
#include "gfx/vk_util.h"
#include "platform.h"

namespace vfs {
void SpriteDrawPipeline::Init(const gfx::CoreCtx& ctx, VkFormat draw_img_format) {
    auto frag = vk::util::LoadShaderModule(
        ctx, Platform::Info::ResourcePath("shaders/sprite.frag.spv").c_str());
    auto vert = vk::util::LoadShaderModule(
        ctx, Platform::Info::ResourcePath("shaders/sprite.vert.spv").c_str());

    // TODO: FILL THIS!!! Add the descriptor layouts hrere

    auto push_constant_range = VkPushConstantRange{
        .offset = 0,
        .size = sizeof(PushConstants),
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    layout = vk::util::CreatePipelineLayout(ctx, {}, {{push_constant_range}});

    pipeline = vk::util::PipelineBuilder(layout)
                   .SetShaders(vert, frag)
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
                              const gfx::GPUMesh& mesh) {
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

    PushConstants push_constants;
    push_constants.matrix = glm::mat4{1.f};
    push_constants.vertex_buffer = mesh.vertex_addr;
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants),
                       &push_constants);

    vkCmdBindIndexBuffer(cmd, mesh.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, mesh.index_count, 1, 0, 0, 0);
}

void SpriteDrawPipeline::Clean(const gfx::CoreCtx& ctx) {
    vkDestroyPipeline(ctx.device, pipeline, nullptr);
}

}  // namespace vfs
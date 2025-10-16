#include "box_pipeline.h"

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

}  // namespace vfs

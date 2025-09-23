#include "renderer_2d.h"

#include <glm/ext.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "draw_pipelines.h"
#include "gfx/common.h"
#include "gfx/gfx.h"
#include "gfx/vk_util.h"
#include "simulation.h"

namespace vfs {

void SimulationRenderer2D::Init(const gfx::Device& gfx,
                                const Simulation2D& simulation,
                                int w,
                                int h) {
    draw_img = CreateDrawImage(gfx, w, h);
    depth_img = CreateDepthImage(gfx, w, h);

    // TODO: Create other important images:
    //      1. Post fx image
    //      ...

    clear_color = {0.0f, 0.0f, 0.0f, 1.0f};

    sprite_pipeline.Init(gfx.GetCoreCtx(), draw_img.format);

    gfx::CPUMesh mesh;
    DrawQuad(mesh, glm::vec3(0.0f), 0.2f, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    particle_mesh = gfx::UploadMesh(gfx, mesh);

    box_pipeline.Init(gfx.GetCoreCtx(), draw_img.format, depth_img.format);
}

void SimulationRenderer2D::Draw(gfx::Device& gfx,
                                VkCommandBuffer cmd,
                                const Simulation2D& simulation,
                                u32 current_frame,
                                const glm::mat4 view_proj) {
    auto bbox = simulation.GetBoundingBox();
    box_transform.SetScale(glm::vec3(bbox.size, 1.0f));
    box_transform.SetPosition(glm::vec3(bbox.pos, 0.0f));

    const auto& buffers = simulation.GetFrameData(current_frame);
    auto pos_buffer = buffers.position_buffer.device_addr;
    auto vel_buffer = buffers.velocity_buffer.device_addr;

    auto color_attachment = vk::util::RenderingAttachmentInfo(
        draw_img.view, NULL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.clearValue.color = {
        clear_color.r,
        clear_color.g,
        clear_color.b,
        clear_color.a,
    };

    // auto depth_attachment = vk::util::DepthRenderingAttachmentInfo(
    //     depth_img.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    auto render_info =
        vk::util::RenderingInfo(gfx.GetSwapchainExtent(), &color_attachment, nullptr);

    vkCmdBeginRendering(cmd, &render_info);

    const auto pc = ParticleDrawPipeline::PushConstants{
        .matrix = view_proj * transform.Matrix(),
        .positions = pos_buffer,
        .velocities = vel_buffer,
    };

    sprite_pipeline.Draw(cmd, gfx, draw_img, particle_mesh, pc,
                         simulation.GetParameters().n_particles);

    const auto pc_box = BoxDrawPipeline::PushConstants{
        .color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
        .matrix = view_proj * transform.Matrix() * box_transform.Matrix(),
    };
    box_pipeline.Draw(cmd, gfx, draw_img, pc_box);

    vkCmdEndRendering(cmd);
}

void SimulationRenderer2D::Clear(const gfx::Device& gfx) {
    gfx::DestroyMesh(gfx, particle_mesh);
    gfx.DestroyImage(draw_img);
    gfx.DestroyImage(depth_img);
    sprite_pipeline.Clear(gfx.GetCoreCtx());
    box_pipeline.Clear(gfx.GetCoreCtx());
}

}  // namespace vfs

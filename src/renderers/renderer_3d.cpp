#include "renderer_3d.h"

#include "gfx/vk_util.h"

namespace vfs {

void SimulationRenderer3D::Init(const gfx::Device& gfx, const SPHModel* simulation, int w, int h) {
    draw_img = CreateDrawImage(gfx, w, h);
    depth_img = CreateDepthImage(gfx, w, h);

    clear_color = {0.0f, 0.0f, 0.0f, 1.0f};

    render_buffers = simulation->CreateDataBuffers(gfx.GetCoreCtx());

    box_pipeline.Init(gfx.GetCoreCtx(), draw_img.format, depth_img.format, true);
    particles_pipeline.Init(gfx.GetCoreCtx(), draw_img.format, depth_img.format);

    gfx::CPUMesh mesh;
    DrawQuad(mesh, glm::vec3(0.0f), 0.2f, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    // DrawCircleFill(mesh, glm::vec3(0.0f), 0.1f, 3);
    particle_mesh = gfx::UploadMesh(gfx, mesh);
}

void SimulationRenderer3D::Draw(gfx::Device& gfx,
                                VkCommandBuffer cmd,
                                const SPHModel* simulation,
                                const Camera& camera) {
    ComputeToComputePipelineBarrier(cmd);
    simulation->CopyDataBuffers(cmd, render_buffers);
    ComputeToGraphicsPipelineBarrier(cmd);

    auto color_attachment = vk::util::RenderingAttachmentInfo(
        draw_img.view, NULL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.clearValue.color = {
        clear_color.r,
        clear_color.g,
        clear_color.b,
        clear_color.a,
    };

    vk::util::TransitionImage(cmd, depth_img.image, VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    auto depth_attachment = vk::util::DepthRenderingAttachmentInfo(
        depth_img.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    auto render_info =
        vk::util::RenderingInfo(gfx.GetSwapchainExtent(), &color_attachment, &depth_attachment);
    vkCmdBeginRendering(cmd, &render_info);

    auto view_proj = camera.GetViewProj();

    glm::vec3 box_size = simulation->GetBoundingBox().value().size;
    box_transform.SetScale(box_size);
    box_transform.SetPosition(-box_size / 2.0f);
    sim_transform.SetPosition(-box_size / 2.0f);

    auto pc = BoxDrawPipeline::PushConstants{
        .color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
        .matrix = view_proj * transform.Matrix() * box_transform.Matrix(),
    };
    box_pipeline.Draw(cmd, gfx, draw_img, pc);

    auto pos_buffer = render_buffers.position_buffer.device_addr;
    auto vel_buffer = render_buffers.velocity_buffer.device_addr;

    auto pc_particles = Particle3DDrawPipeline::PushConstants{
        .model_view = camera.GetView() * transform.Matrix() * sim_transform.Matrix(),
        .proj = camera.GetProj(),
        .positions = pos_buffer,
        .velocities = vel_buffer,
    };

    particles_pipeline.Draw(cmd, gfx, draw_img, particle_mesh, pc_particles,
                            simulation->GetParameters().n_particles);

    vkCmdEndRendering(cmd);
}

void SimulationRenderer3D::Clear(const gfx::Device& gfx) {
    gfx::DestroyMesh(gfx, particle_mesh);
    gfx.DestroyImage(draw_img);
    gfx.DestroyImage(depth_img);
    particles_pipeline.Clear(gfx.GetCoreCtx());
    box_pipeline.Clear(gfx.GetCoreCtx());
    render_buffers.position_buffer.Destroy();
    render_buffers.velocity_buffer.Destroy();
    render_buffers.density_buffer.Destroy();
}

}  // namespace vfs

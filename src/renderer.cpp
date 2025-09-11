#include "renderer.h"

#include "gfx/gfx.h"
#include "gfx/vk_util.h"
#include "pipeline.h"

namespace vfs {

namespace {

void DrawCircleFill(gfx::CPUMesh& mesh,
                    const glm::vec3& center,
                    float radius,
                    const glm::vec4& color,
                    int steps) {
    float cx = center.x;
    float cy = center.y;

    for (int i = 0; i < steps; i++) {
        float angle0 = (float)i * 2 * M_PI / (float)steps;
        float angle1 = (float)(i + 1) * 2 * M_PI / (float)steps;

        auto p0 = glm::vec3(cx, cy, 0.0f);
        auto p1 = glm::vec3(cx + radius * sinf(angle0), cy + radius * cosf(angle0), 0.0f);
        auto p2 = glm::vec3(cx + radius * sinf(angle1), cy + radius * cosf(angle1), 0.0f);

        const unsigned int n = (unsigned int)mesh.vertices.size();
        mesh.indices.insert(mesh.indices.end(), {n + 0, n + 2, n + 1});
        mesh.vertices.insert(mesh.vertices.end(),
                             {{.pos = p0, .uv = {}}, {.pos = p1, .uv = {}}, {.pos = p2, .uv = {}}});
    }
}

void DrawSquare(gfx::CPUMesh& mesh, const glm::vec3& center, float side, const glm::vec4& color) {
    const unsigned int n = (unsigned int)mesh.vertices.size();

    float hs = 0.5 * side;

    auto ll = glm::vec3(center.x - hs, center.y - hs, 0.0f);
    auto lr = glm::vec3(center.x + hs, center.y - hs, 0.0f);
    auto ur = glm::vec3(center.x + hs, center.y + hs, 0.0f);
    auto ul = glm::vec3(center.x - hs, center.y + hs, 0.0f);

    mesh.indices.insert(mesh.indices.end(), {n + 0, n + 2, n + 1, n + 0, n + 3, n + 2});
    mesh.vertices.push_back({.pos = ll, .uv = {0.0f, 0.0f}});
    mesh.vertices.push_back({.pos = lr, .uv = {1.0f, 0.0f}});
    mesh.vertices.push_back({.pos = ur, .uv = {1.0f, 1.0f}});
    mesh.vertices.push_back({.pos = ul, .uv = {0.0f, 1.0f}});
}

}  // namespace

void SimulationRenderer2D::Init(const gfx::Device& gfx, int w, int h) {
    VkExtent3D extent = {.width = (uint32_t)w, .height = (uint32_t)h, .depth = 1};

    // draw image
    {
        VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;

        VkImageUsageFlags usage{0};
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        usage |= VK_IMAGE_USAGE_STORAGE_BIT;

        bool use_mipmaps = false;

        draw_img = gfx.CreateImage(extent, format, usage, use_mipmaps);
    }

    // depth image
    {
        VkFormat format = VK_FORMAT_D32_SFLOAT;

        VkImageUsageFlags usage{0};
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        depth_img = gfx.CreateImage(extent, format, usage, false);
    }

    // TODO: Create other important images:
    //      1. Post fx image
    //      ...

    clear_color = {0.0f, 0.0f, 0.0f, 1.0f};

    sprite_pipeline.Init(gfx.GetCoreCtx(), draw_img.format);

    gfx::CPUMesh mesh;
    DrawSquare(mesh, glm::vec3(0.0f), 0.2f, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    particle_mesh = gfx::UploadMesh(gfx, mesh);
}

void SimulationRenderer2D::Draw(gfx::Device& gfx,
                                VkCommandBuffer cmd,
                                const ParticleDrawPipeline::PushConstants& push_constants,
                                u32 instances) {
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

    sprite_pipeline.Draw(cmd, gfx, draw_img, particle_mesh, push_constants, instances);

    vkCmdEndRendering(cmd);
}

void SimulationRenderer2D::Clear(const gfx::Device& gfx) {
    gfx::DestroyMesh(gfx, particle_mesh);
    gfx.DestroyImage(draw_img);
    gfx.DestroyImage(depth_img);
    sprite_pipeline.Clear(gfx.GetCoreCtx());
}

}  // namespace vfs

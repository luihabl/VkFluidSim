#include "renderer.h"

#include "gfx/gfx.h"
#include "gfx/vk_util.h"

namespace vfs {

void Renderer::Init(const gfx::Device& gfx, int w, int h) {
    VkExtent3D extent = {.width = (uint32_t)w, .height = (uint32_t)h, .depth = 1};

    // draw image
    {
        VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;

        VkImageUsageFlags usage;
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

        VkImageUsageFlags usage;
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        depth_img = gfx.CreateImage(extent, format, usage, false);
    }

    // TODO: Create other important images:
    //      1. Post fx image
    //      ...

    clear_color = {0.0f, 0.0f, 0.0f, 1.0f};

    sprite_pipeline.Init(gfx.GetCoreCtx(), draw_img.format);
}

void Renderer::Draw(gfx::Device& gfx,
                    VkCommandBuffer cmd,
                    const gfx::GPUMesh& mesh,
                    const gfx::Buffer& buf,
                    const glm::mat4& transform) {
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

    sprite_pipeline.Draw(cmd, gfx, draw_img, mesh, buf, transform);

    vkCmdEndRendering(cmd);
}

void Renderer::Clean(const gfx::Device& gfx) {
    gfx.DestroyImage(draw_img);
    gfx.DestroyImage(depth_img);
    sprite_pipeline.Clean(gfx.GetCoreCtx());
}

}  // namespace vfs
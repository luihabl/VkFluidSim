#include "swapchain.h"

#include <cstddef>

#include "gfx/common.h"
#include "gfx/vk_util.h"

namespace gfx {

constexpr u64 ONE_SEC_NS = 1000000000;

void Swapchain::Create(const CoreCtx& ctx, u32 w, u32 h) {
    auto sc_builder = vkb::SwapchainBuilder(ctx.chosen_gpu, ctx.device, ctx.surface);

    image_format = VK_FORMAT_B8G8R8A8_UNORM;

    auto vkb_swapchain =
        sc_builder
            .set_desired_format(VkSurfaceFormatKHR{.format = image_format,
                                                   .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_min_image_count(gfx::FRAME_COUNT)
            .set_desired_extent(w, h)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();

    swapchain = vkb_swapchain.swapchain;
    extent = vkb_swapchain.extent;
    images = vkb_swapchain.get_images().value();
    image_views = vkb_swapchain.get_image_views().value();

    VK_CHECK(vkGetSwapchainImagesKHR(ctx.device, swapchain, &image_count, nullptr));
}

void Swapchain::Destroy(const CoreCtx& ctx) {
    for (auto& frame : frames) {
        vkDestroyFence(ctx.device, frame.render_fence, NULL);
        vkDestroySemaphore(ctx.device, frame.swapchain_semaphore, NULL);
    }

    for (auto& rs : render_semaphores) {
        vkDestroySemaphore(ctx.device, rs, NULL);
    }

    vkDestroySwapchainKHR(ctx.device, swapchain, NULL);
    for (int i = 0; i < image_views.size(); i++) {
        vkDestroyImageView(ctx.device, image_views[i], NULL);
    }

    *this = {};
}

void Swapchain::InitSyncStructs(const CoreCtx& ctx) {
    auto fence_create_info = vk::util::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    auto semaphore_create_info = vk::util::SemaphoreCreateInfo();

    for (int i = 0; i < gfx::FRAME_COUNT; i++) {
        VK_CHECK(vkCreateFence(ctx.device, &fence_create_info, NULL, &frames[i].render_fence));
        VK_CHECK(vkCreateSemaphore(ctx.device, &semaphore_create_info, NULL,
                                   &frames[i].swapchain_semaphore));
    }

    render_semaphores.resize(image_count);
    for (int i = 0; i < image_count; i++) {
        VkSemaphoreCreateInfo semaphore_create_info = vk::util::SemaphoreCreateInfo();
        VK_CHECK(
            vkCreateSemaphore(ctx.device, &semaphore_create_info, nullptr, &render_semaphores[i]));
    }
}

void Swapchain::BeginFrame(const CoreCtx& ctx, u32 frame_index) {
    VK_CHECK(vkWaitForFences(ctx.device, 1, &frames[frame_index].render_fence, true, ONE_SEC_NS));
}

VkImage Swapchain::AcquireNextImage(const CoreCtx& ctx, u32 frame_index, u32* out_index) {
    auto& frame = frames[frame_index];

    u32 swapchain_img_idx;
    VkResult e = vkAcquireNextImageKHR(ctx.device, swapchain, ONE_SEC_NS, frame.swapchain_semaphore,
                                       NULL, &swapchain_img_idx);

    if (out_index)
        *out_index = swapchain_img_idx;

    return images[swapchain_img_idx];
}

}  // namespace gfx
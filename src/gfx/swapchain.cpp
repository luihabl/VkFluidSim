#include "swapchain.h"

#include <vulkan/vk_enum_string_helper.h>

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

void Swapchain::ResetFences(const CoreCtx& ctx, u32 frame_index) {
    VK_CHECK(vkResetFences(ctx.device, 1, &frames[frame_index].render_fence));
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

VkExtent2D Swapchain::GetExtent() {
    return extent;
}

bool Swapchain::SubmitAndPresent(VkCommandBuffer cmd,
                                 VkQueue queue,
                                 u32 frame_index,
                                 u32 swapchain_idx) {
    auto& frame = frames[frame_index];
    auto cmd_submit_info = vk::util::CommandBufferSubmitInfo(cmd);
    auto wait_info = vk::util::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                   frame.swapchain_semaphore);
    auto signal_info = vk::util::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                                                     render_semaphores[swapchain_idx]);
    auto submit = vk::util::SubmitInfo2(&cmd_submit_info, &signal_info, &wait_info);

    VK_CHECK(vkQueueSubmit2(queue, 1, &submit, frame.render_fence));

    auto present_info = vk::util::PresentInfoKHR();
    present_info.pSwapchains = &swapchain;
    present_info.swapchainCount = 1;
    present_info.pWaitSemaphores = &render_semaphores[swapchain_idx];
    present_info.waitSemaphoreCount = 1;
    present_info.pImageIndices = &swapchain_idx;

    auto e = vkQueuePresentKHR(queue, &present_info);
    if (e != VK_SUCCESS) {
        // set as resize requested
        if (e != VK_SUBOPTIMAL_KHR) {
            fmt::println("Swapchain failed to present: {}", string_VkResult(e));
        }
        return false;
    }

    return true;
}

}  // namespace gfx
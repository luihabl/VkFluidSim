#pragma once

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

#include "gfx/common.h"

namespace gfx {
struct Swapchain {
    void InitSyncStructs(const CoreCtx& ctx);
    void Create(const CoreCtx& ctx, u32 w, u32 h);
    void Destroy(const CoreCtx& ctx);

    void BeginFrame(const CoreCtx& ctx, u32 frame_index);
    VkImage AcquireNextImage(const CoreCtx& ctx,
                             u32 frame_index,
                             u32* out_swapchain_index = nullptr);
    void ResetFences(const CoreCtx& ctx, u32 frame_index);
    VkExtent2D GetExtent();
    bool SubmitAndPresent(VkCommandBuffer cmd, VkQueue queue, u32 frame_index, u32 swapchain_idx);

private:
    struct FrameData {
        VkSemaphore swapchain_semaphore;
        VkFence render_fence;
    };

    VkSwapchainKHR swapchain;
    VkFormat image_format;
    std::vector<VkImage> images;
    std::vector<VkImageView> image_views;
    std::array<FrameData, gfx::FRAME_COUNT> frames;
    std::vector<VkSemaphore> render_semaphores;  // NOTE: This is not inside FrameData because the
                                                 // swapchain may contain more images than expected.
                                                 // Thus, we need more semaphores.
    u32 image_count = 0;
    VkExtent2D extent;
};
}  // namespace gfx
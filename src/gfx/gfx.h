#pragma once

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

#include <array>
#include <glm/glm.hpp>
#include <span>

#include "gfx/common.h"
#include "gfx/immediate.h"
#include "gfx/swapchain.h"
#include "vk_mem_alloc.h"

namespace gfx {

struct Device {
    struct Config {
        const char* name;
        bool validation_layers = false;
        SDL_Window* window;
    };

    struct FrameData {
        VkCommandPool cmd_pool;
        VkCommandBuffer cmd_buffer;
        // descriptor::DynamicAllocator descriptors;
    };

    void Init(const Config& config);
    void Clear();

    VkCommandBuffer BeginFrame();
    void EndFrame(VkCommandBuffer cmd, const Image& draw_img);

    Image CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mip) const;
    void DestroyImage(Image& img) const;

    const CoreCtx& GetCoreCtx() const { return core; }
    VkQueue GetQueue() const { return graphics_queue; }
    u32 GetQueueFamily() const { return graphics_queue_family; }
    const Swapchain& GetSwapchain() const { return swapchain; }

    VkExtent2D GetSwapchainExtent();

    void ImmediateSubmit(std::function<void(VkCommandBuffer)>&& function) const;

    void SetTopTimestamp(VkCommandBuffer cmd, u32 id);
    void SetBottomTimestamp(VkCommandBuffer cmd, u32 id);

    std::span<u64> GetTimestamps();

private:
    void InitCommandBuffers();
    FrameData& CurrentFrame();
    u32 CurrentFrameIndex();

    CoreCtx core;
    VkQueue graphics_queue;
    u32 graphics_queue_family;

    Swapchain swapchain;
    glm::vec4 swapchain_img_clear_color;

    u32 used_timestamps{0};
    std::vector<u64> time_stamps;
    VkQueryPool query_pool_timestamps;

    ImmediateRunner immediate_runner;

    std::array<FrameData, gfx::FRAME_COUNT> frames;
    uint32_t frame_number = 0;
};

}  // namespace gfx

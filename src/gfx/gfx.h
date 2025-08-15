#pragma once

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

#include <array>

#include "gfx/common.h"
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
    AllocatedImage CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mip);
    void DestroyImage(AllocatedImage& img);
    void Clean();

private:
    CoreCtx core;
    VkQueue graphics_queue;
    uint32_t graphics_queue_family;
    VmaAllocator allocator;

    AllocatedImage draw_img;
    Swapchain swapchain;

    std::array<FrameData, gfx::FRAME_COUNT> frames;
    uint32_t frame_number = 0;
};

}  // namespace gfx
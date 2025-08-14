#pragma once

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

namespace vfs {
struct GfxConfig {
    const char* name;
    bool validation_layers = false;
    SDL_Window* window;
};

struct GfxDevice {
    VkDevice device;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice chosen_gpu;
    VkSurfaceKHR surface;
    VkQueue graphics_queue;
    uint32_t graphics_queue_family;
    VmaAllocator allocator;

    void Init(const GfxConfig& config);
    void Clean();
};

}  // namespace vfs
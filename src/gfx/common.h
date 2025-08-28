#pragma once

#include <fmt/core.h>
#include <fmt/std.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

using f32 = float;
using f64 = double;
using i32 = int32_t;
using i64 = int64_t;
using u32 = uint32_t;
using u64 = uint64_t;

#define VK_CHECK(x)                                                                       \
    do {                                                                                  \
        VkResult err = x;                                                                 \
        if (err) {                                                                        \
            fmt::println("VK_CHECK error[{}:{}]: {}", __FILE_NAME__, __LINE__, (int)err); \
            fmt::println("On call: {}", #x);                                              \
            fmt::println("Aborting...");                                                  \
            abort();                                                                      \
        }                                                                                 \
    } while (0)

namespace gfx {

constexpr u64 ONE_SEC_NS = 1000000000;
constexpr u32 FRAME_COUNT = 2;

struct CoreCtx {
    VkDevice device;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice chosen_gpu;
    VkSurfaceKHR surface;
    VmaAllocator allocator;
};

struct Image {
    VkImage image;
    VkImageView view;
    VkExtent3D extent;
    VkFormat format;
    VmaAllocation allocation;
};

struct Buffer {
    VkBuffer buffer{nullptr};
    VmaAllocation alloc{nullptr};
    void* mapped{nullptr};
    VmaAllocator allocator{nullptr};
    VkDescriptorBufferInfo desc_info{0};
    u32 size{0};

    void* GetMapped();
    void* Map();
    void Unmap();
    void SetDescriptorInfo(VkDeviceSize size, VkDeviceSize offset);

    static Buffer Create(const CoreCtx& ctx,
                         size_t size,
                         VkBufferUsageFlags usage,
                         VmaMemoryUsage mem_usage = VMA_MEMORY_USAGE_AUTO);
    void Destroy();
};

}  // namespace gfx
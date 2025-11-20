#pragma once

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

#include <algorithm>
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
    };

    void Init(const Config& config);
    void Clear();

    VkCommandBuffer BeginFrame();
    void EndFrame(VkCommandBuffer cmd, const Image& draw_img);

    const CoreCtx& GetCoreCtx() const { return core; }
    VkQueue GetQueue() const { return graphics_queue; }
    u32 GetQueueFamily() const { return graphics_queue_family; }
    const Swapchain& GetSwapchain() const { return swapchain; }

    VkExtent2D GetSwapchainExtent();

    void ImmediateSubmit(std::function<void(VkCommandBuffer)>&& function) const;

    template <typename T>
    void SetDataVal(const gfx::Buffer& buf, const T& value, u32 offset = 0, i64 count = -1) const {
        u32 size = buf.size;
        if (size == 0 || count == 0)
            return;

        offset = std::min(offset, static_cast<u32>(size / sizeof(T) - 1));

        if (count >= 0) {
            size = std::min(size, static_cast<u32>(count * sizeof(T)));
        }

        auto staging = gfx::Buffer::Create(core, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                           VMA_MEMORY_USAGE_CPU_ONLY);
        void* data = staging.Map();

        size_t n = size / sizeof(T);
        auto* dp = (T*)data;

        for (int i = 0; i < n; i++) {
            dp[i] = value;
        }

        ImmediateSubmit([&](VkCommandBuffer cmd) {
            auto cpy_info = VkBufferCopy{
                .dstOffset = offset * sizeof(T),
                .srcOffset = 0,
                .size = size,
            };

            if (staging.buffer && buf.buffer)
                vkCmdCopyBuffer(cmd, staging.buffer, buf.buffer, 1, &cpy_info);
        });

        staging.Destroy();
    }

    template <typename T>
    void SetDataVec(const gfx::Buffer& buf,
                    const std::vector<T>& value,
                    u32 offset = 0,
                    i64 count = -1) const {
        u32 size = buf.size;
        if (size == 0 || count == 0)
            return;

        offset = std::min(offset, static_cast<u32>(size / sizeof(T) - 1));

        if (count >= 0) {
            size = std::min(size, static_cast<u32>(count * sizeof(T)));
        }

        auto staging = gfx::Buffer::Create(core, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                           VMA_MEMORY_USAGE_CPU_ONLY);
        void* data = staging.Map();

        size_t n = size / sizeof(T);
        auto* dp = (T*)data;

        for (int i = 0; i < n; i++) {
            dp[i] = value[i];
        }

        ImmediateSubmit([&](VkCommandBuffer cmd) {
            auto cpy_info = VkBufferCopy{
                .dstOffset = offset * sizeof(T),
                .srcOffset = 0,
                .size = size,
            };

            vkCmdCopyBuffer(cmd, staging.buffer, buf.buffer, 1, &cpy_info);
        });

        staging.Destroy();
    }

    void SetImageData(gfx::Image& img, void* data, u32 texel_size, bool mip = false) const;

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

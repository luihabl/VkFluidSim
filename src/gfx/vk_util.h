#pragma once

#include <vulkan/vulkan.h>

namespace vk::util {

inline auto SemaphoreCreateInfo(VkFenceCreateFlags flags = 0) {
    return VkSemaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = flags,
    };
}

inline auto FenceCreateInfo(VkFenceCreateFlags flags = 0) {
    return VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = flags,
    };
}

inline auto CommandPoolCreateInfo(uint32_t queue_family_index, VkCommandPoolCreateFlags flags = 0) {
    return VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .queueFamilyIndex = queue_family_index,
        .flags = flags,
    };
}

inline auto CommandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1) {
    return VkCommandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = pool,
        .commandBufferCount = count,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    };
}

inline auto CommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0) {
    return VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .pInheritanceInfo = NULL,
        .flags = flags,
    };
}

inline auto ImageSubresourceRange(VkImageAspectFlags aspect_mask) {
    return VkImageSubresourceRange{
        .aspectMask = aspect_mask,
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };
}

inline auto SemaphoreSubmitInfo(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore) {
    return VkSemaphoreSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = NULL,
        .semaphore = semaphore,
        .stageMask = stage_mask,
        .deviceIndex = 0,
        .value = 1,
    };
}

inline auto CommandBufferSubmitInfo(VkCommandBuffer cmd) {
    return VkCommandBufferSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = NULL,
        .commandBuffer = cmd,
        .deviceMask = 0,
    };
}

inline auto SubmitInfo2(VkCommandBufferSubmitInfo* cmd,
                        VkSemaphoreSubmitInfo* signal_semaphore_info,
                        VkSemaphoreSubmitInfo* wait_semaphore_info) {
    return VkSubmitInfo2{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = NULL,

        .waitSemaphoreInfoCount = wait_semaphore_info ? 1u : 0u,
        .pWaitSemaphoreInfos = wait_semaphore_info,

        .signalSemaphoreInfoCount = signal_semaphore_info ? 1u : 0u,
        .pSignalSemaphoreInfos = signal_semaphore_info,

        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = cmd,
    };
}

inline auto PresentInfoKHR() {
    return VkPresentInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
    };
}

inline auto ImageCreateInfo(VkFormat format, VkImageUsageFlags flags, VkExtent3D extent) {
    return VkImageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,

        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = extent,

        .mipLevels = 1,
        .arrayLayers = 1,

        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = flags,

    };
}

inline auto ImageViewCreateInfo(VkFormat fmt, VkImage img, VkImageAspectFlags aspect_flags) {
    return VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,

        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .image = img,
        .format = fmt,

        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
        .subresourceRange.aspectMask = aspect_flags,
    };
}

inline auto PipelineShaderStageCreateInfo(VkShaderModule shader_module,
                                          VkShaderStageFlagBits stage,
                                          const char* entry_point = "main") {
    return VkPipelineShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,
        .stage = stage,
        .module = shader_module,
        .pName = entry_point,
    };
}

inline auto PipelineLayoutCreateInfo() {
    return VkPipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };
}

inline auto RenderingAttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout) {
    auto att = VkRenderingAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = NULL,
        .imageView = view,
        .imageLayout = layout,
        .loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };

    if (clear)
        att.clearValue = *clear;

    return att;
}

inline auto DepthRenderingAttachmentInfo(VkImageView view, VkImageLayout layout) {
    return VkRenderingAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = NULL,
        .imageView = view,
        .imageLayout = layout,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue.depthStencil.depth = 0.f,
    };
}

inline auto RenderingInfo(VkExtent2D extent,
                          VkRenderingAttachmentInfo* color_attachment,
                          VkRenderingAttachmentInfo* depth_attachment) {
    return VkRenderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = NULL,
        .renderArea = VkRect2D{.offset = {.x = 0, .y = 0}, .extent = extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = color_attachment,
        .pDepthAttachment = depth_attachment,
        .pStencilAttachment = NULL,
    };
}

};  // namespace vk::util
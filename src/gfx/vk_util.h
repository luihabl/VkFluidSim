#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <span>

#include "gfx/common.h"

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

inline auto ImageCreateInfo(VkFormat format,
                            VkImageUsageFlags flags,
                            VkExtent3D extent,
                            u8 dimensions = 2) {
    VkImageType type = VK_IMAGE_TYPE_2D;
    if (dimensions == 3)
        type = VK_IMAGE_TYPE_3D;
    if (dimensions == 1)
        type = VK_IMAGE_TYPE_1D;

    return VkImageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,

        .imageType = type,
        .format = format,
        .extent = extent,

        .mipLevels = 1,
        .arrayLayers = 1,

        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = flags,

    };
}

inline auto ImageViewCreateInfo(VkFormat fmt,
                                VkImage img,
                                VkImageAspectFlags aspect_flags,
                                u8 dimensions = 2) {
    VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D;
    if (dimensions == 3)
        type = VK_IMAGE_VIEW_TYPE_3D;
    if (dimensions == 1)
        type = VK_IMAGE_VIEW_TYPE_1D;

    return VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,

        .viewType = type,
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
        .clearValue.depthStencil.depth = 1.f,
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

inline auto WriteDescriptorSet(VkDescriptorSet dst_set,
                               VkDescriptorType type,
                               uint32_t binding,
                               VkDescriptorBufferInfo* buffer_info,
                               uint32_t descriptorCount = 1) {
    return VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = dst_set,
        .descriptorType = type,
        .dstBinding = binding,
        .pBufferInfo = buffer_info,
        .descriptorCount = descriptorCount,
    };
}

inline auto WriteDescriptorSet(VkDescriptorSet dst_set,
                               VkDescriptorType type,
                               uint32_t binding,
                               VkDescriptorImageInfo* img_info,
                               uint32_t descriptorCount = 1) {
    return VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = dst_set,
        .descriptorType = type,
        .dstBinding = binding,
        .pImageInfo = img_info,
        .descriptorCount = descriptorCount,
    };
}

void TransitionImage(VkCommandBuffer cmd,
                     VkImage image,
                     VkImageLayout curr_layout,
                     VkImageLayout new_layout);

void CopyImage(VkCommandBuffer cmd,
               VkImage src,
               VkImage dst,
               VkExtent2D src_size,
               VkExtent2D dst_size,
               bool linear_filter = true);

VkShaderModule LoadShaderModule(const gfx::CoreCtx& ctx, const char* path);

VkPipelineLayout CreatePipelineLayout(const gfx::CoreCtx& ctx,
                                      std::span<const VkDescriptorSetLayout> desc_set_layouts,
                                      std::span<const VkPushConstantRange> push_const_ranges);

class GraphicsPipelineBuilder {
public:
    GraphicsPipelineBuilder(VkPipelineLayout layout);
    GraphicsPipelineBuilder();
    VkPipeline Build(VkDevice device);
    void Clear();

    GraphicsPipelineBuilder& SetShaders(VkShaderModule vertex_shader, VkShaderModule frag_shader);
    GraphicsPipelineBuilder& AddShaderStage(VkShaderModule shader_mod,
                                            VkShaderStageFlagBits stage,
                                            const char* entry_point = "main");

    GraphicsPipelineBuilder& AddVertexBinding(uint32_t binding,
                                              uint32_t stride,
                                              VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX);
    GraphicsPipelineBuilder& AddVertexAttribute(uint32_t binding,
                                                uint32_t location,
                                                VkFormat format,
                                                uint32_t offset);
    GraphicsPipelineBuilder& SetInputTopology(VkPrimitiveTopology topo);
    GraphicsPipelineBuilder& SetPolygonMode(VkPolygonMode mode);
    GraphicsPipelineBuilder& SetCullMode(VkCullModeFlags cull_mode, VkFrontFace front_face);
    GraphicsPipelineBuilder& SetCullDisabled();
    GraphicsPipelineBuilder& SetMultisamplingDisabled();
    GraphicsPipelineBuilder& SetBlendingDisabled();
    GraphicsPipelineBuilder& SetBlendingAlphaBlend();
    GraphicsPipelineBuilder& SetBlendingAdditive();
    GraphicsPipelineBuilder& SetColorAttachmentFormat(VkFormat format);
    GraphicsPipelineBuilder& SetDepthFormat(VkFormat format);
    GraphicsPipelineBuilder& SetDepthTest(bool enable_depth_write, VkCompareOp op);
    GraphicsPipelineBuilder& SetDepthTestDisabled();

private:
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    std::vector<VkVertexInputBindingDescription> vert_input_bindings;
    std::vector<VkVertexInputAttributeDescription> vert_attributes;
    VkPipelineInputAssemblyStateCreateInfo input_assembly;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineColorBlendAttachmentState color_blend_attachment;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineLayout pipeline_layout;
    VkPipelineDepthStencilStateCreateInfo depth_stencil;
    VkPipelineRenderingCreateInfo render_info;
    VkFormat color_attachment_format;
};

VkPipeline BuildComputePipeline(VkDevice device,
                                VkPipelineLayout layout,
                                VkShaderModule shader,
                                const char* entry_point = "main");

VkDeviceAddress GetBufferAddress(VkDevice device, const gfx::Buffer& buffer);

};  // namespace vk::util

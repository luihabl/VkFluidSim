#include "vk_util.h"

#include "gfx/common.h"

namespace vk::util {

void TransitionImage(VkCommandBuffer cmd,
                     VkImage image,
                     VkImageLayout curr_layout,
                     VkImageLayout new_layout) {
    VkImageAspectFlags aspect_mask = (new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                                         ? VK_IMAGE_ASPECT_DEPTH_BIT
                                         : VK_IMAGE_ASPECT_COLOR_BIT;

    auto image_barrier = VkImageMemoryBarrier2{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = NULL,

        .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,

        .oldLayout = curr_layout,
        .newLayout = new_layout,

        .subresourceRange = ImageSubresourceRange(aspect_mask),
        .image = image,
    };

    auto dep_info = VkDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = NULL,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &image_barrier,
    };

    vkCmdPipelineBarrier2(cmd, &dep_info);
}

void CopyImage(VkCommandBuffer cmd,
               VkImage src,
               VkImage dst,
               VkExtent2D src_size,
               VkExtent2D dst_size,
               bool linear_filter) {
    auto blit_region = VkImageBlit2{
        .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
        .pNext = NULL,

        .srcOffsets[0] = {.x = 0, .y = 0, .z = 0},
        .srcOffsets[1] = {.x = (int32_t)src_size.width, .y = (int32_t)src_size.height, .z = 1},

        .srcSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .baseArrayLayer = 0,
                           .layerCount = 1,
                           .mipLevel = 0},

        .dstOffsets[0] = {.x = 0, .y = 0, .z = 0},
        .dstOffsets[1] = {.x = (int32_t)dst_size.width, .y = (int32_t)dst_size.height, .z = 1},

        .dstSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .baseArrayLayer = 0,
                           .layerCount = 1,
                           .mipLevel = 0},
    };

    auto blit_info = VkBlitImageInfo2{
        .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
        .pNext = NULL,
        .dstImage = dst,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcImage = src,
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .filter = linear_filter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
        .regionCount = 1,
        .pRegions = &blit_region,
    };

    vkCmdBlitImage2(cmd, &blit_info);
}

VkShaderModule LoadShaderModule(const gfx::CoreCtx& ctx, const char* path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return VK_NULL_HANDLE;
    }

    auto size = (size_t)file.tellg();
    if (size == 0) {
        return VK_NULL_HANDLE;
    }

    auto buffer = std::vector<uint32_t>(size / sizeof(uint32_t));

    file.seekg(0);
    file.read((char*)buffer.data(), size);
    file.close();

    auto create_info = VkShaderModuleCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .codeSize = buffer.size() * sizeof(uint32_t),
        .pCode = buffer.data(),
    };

    VkShaderModule shader;
    if (vkCreateShaderModule(ctx.device, &create_info, NULL, &shader) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    return shader;
}

VkPipelineLayout CreatePipelineLayout(const gfx::CoreCtx& ctx,
                                      std::span<const VkDescriptorSetLayout> desc_set_layouts,
                                      std::span<const VkPushConstantRange> push_const_ranges) {
    auto layout_info = vk::util::PipelineLayoutCreateInfo();
    layout_info.pSetLayouts = desc_set_layouts.data();
    layout_info.setLayoutCount = desc_set_layouts.size();
    layout_info.pPushConstantRanges = push_const_ranges.data();
    layout_info.pushConstantRangeCount = push_const_ranges.size();

    VkPipelineLayout layout;
    VK_CHECK(vkCreatePipelineLayout(ctx.device, &layout_info, nullptr, &layout));

    return layout;
}

GraphicsPipelineBuilder::GraphicsPipelineBuilder(VkPipelineLayout layout) {
    Clear();
    pipeline_layout = layout;
}

GraphicsPipelineBuilder::GraphicsPipelineBuilder() {
    Clear();
}

void GraphicsPipelineBuilder::Clear() {
    input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = NULL,
    };

    rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = NULL,
    };

    color_blend_attachment = {};

    multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = NULL,
    };

    pipeline_layout = {};

    depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = NULL,
    };

    render_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = NULL,
    };

    shader_stages.resize(0);
}

VkPipeline GraphicsPipelineBuilder::Build(VkDevice device) {
    auto view_port_state = VkPipelineViewportStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = NULL,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    auto color_blending = VkPipelineColorBlendStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = NULL,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
    };

    auto vertex_input_info = VkPipelineVertexInputStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = NULL,
    };

    VkDynamicState state[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    auto dynamic_info = VkPipelineDynamicStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = NULL,
        .pDynamicStates = state,
        .dynamicStateCount = 2,
    };

    auto pipeline_info = VkGraphicsPipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &render_info,

        .stageCount = (uint32_t)shader_stages.size(),
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &view_port_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &color_blending,
        .pDepthStencilState = &depth_stencil,
        .layout = pipeline_layout,
        .pDynamicState = &dynamic_info,
    };

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline) !=
        VK_SUCCESS) {
        fmt::println("Failed to create pipeline");
        return VK_NULL_HANDLE;
    } else {
        return pipeline;
    }
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetShaders(VkShaderModule vertex_shader,
                                                             VkShaderModule frag_shader) {
    shader_stages.push_back(
        PipelineShaderStageCreateInfo(vertex_shader, VK_SHADER_STAGE_VERTEX_BIT));
    shader_stages.push_back(
        PipelineShaderStageCreateInfo(frag_shader, VK_SHADER_STAGE_FRAGMENT_BIT));

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetInputTopology(VkPrimitiveTopology topo) {
    input_assembly.topology = topo;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetPolygonMode(VkPolygonMode mode) {
    rasterizer.polygonMode = mode;
    rasterizer.lineWidth = 1.0f;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetCullMode(VkCullModeFlags cull_mode,
                                                              VkFrontFace front_face) {
    rasterizer.cullMode = cull_mode;
    rasterizer.frontFace = front_face;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetCullDisabled() {
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetMultisamplingDisabled() {
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = NULL;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetBlendingDisabled() {
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetBlendingAdditive() {
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetBlendingAlphaBlend() {
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetColorAttachmentFormat(VkFormat format) {
    color_attachment_format = format;
    render_info.colorAttachmentCount = 1;
    render_info.pColorAttachmentFormats = &color_attachment_format;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDepthFormat(VkFormat format) {
    render_info.depthAttachmentFormat = format;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDepthTestDisabled() {
    depth_stencil.depthTestEnable = VK_FALSE;
    depth_stencil.depthWriteEnable = VK_FALSE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;
    depth_stencil.front = {};
    depth_stencil.back = {};
    depth_stencil.minDepthBounds = 0.0f;
    depth_stencil.maxDepthBounds = 1.0f;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDepthTest(bool enable_depth_write,
                                                               VkCompareOp op) {
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = enable_depth_write;
    depth_stencil.depthCompareOp = op;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;
    depth_stencil.front = {};
    depth_stencil.back = {};
    depth_stencil.minDepthBounds = 0.0f;
    depth_stencil.maxDepthBounds = 1.0f;

    return *this;
}

VkPipeline BuildComputePipeline(VkDevice device,
                                VkPipelineLayout layout,
                                VkShaderModule shader_mod) {
    auto shader = PipelineShaderStageCreateInfo(shader_mod, VK_SHADER_STAGE_COMPUTE_BIT);

    auto pipeline_info = VkComputePipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .layout = layout,
        .stage = shader,
    };

    VkPipeline pipeline;
    VK_CHECK(
        vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline));

    return pipeline;
}

VkDeviceAddress GetBufferAddress(VkDevice device, const gfx::Buffer& buffer) {
    auto device_addr_info = VkBufferDeviceAddressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = NULL,
        .buffer = buffer.buffer,
    };

    return vkGetBufferDeviceAddress(device, &device_addr_info);
}

}  // namespace vk::util
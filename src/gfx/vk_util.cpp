#include "vk_util.h"

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

}  // namespace vk::util